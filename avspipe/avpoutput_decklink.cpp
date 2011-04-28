/*
 * Copyright 2011 Andrew H. Armenia.
 * 
 * This file is part of openreplay.
 * 
 * openreplay is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * openreplay is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with openreplay.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "decklink.h"
#include "raw_frame.h"
#include "audio_packet.h"
#include "pipe.h"
#include "thread.h"
#include "xmalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

char *parse_command(const char *cmd, int vpfd, int apfd) {
    size_t len = strlen(cmd);
    size_t out_len = 2*len;
    char *out = (char *)xmalloc(out_len + 1, 
            "avpoutput_decklink", "command buffer");
    char *outp = out;
    char number[80];
    int number_size;
    char ch;

    memset(out, 0, out_len + 1);

    while (*cmd) {
        /*
         * Consume one character of input.
         * This leaves *cmd at the character after the one 
         * currently being acted upon.
         */
        ch = *cmd;
        cmd++;

        if (ch == '%') {
            /* 
             * A % sign introduces an escape sequence. 
             * Look at the next character to decide what it means.
             */
            if (*cmd == '%') { 
                /* %%: literal % sign */
                if (out_len > 0) {
                    *outp = '%';
                    outp++;
                    out_len--;
                }
                cmd++;
            } else if (*cmd == '\0') {
                /* 
                 * a lonely '%' sign at the end of 
                 * the string cannot be parsed 
                 */
                if (out_len > 0) {
                    *outp = '%';
                    outp++;
                    out_len--;
                }
            } else if (*cmd == 'a') {
                /* %a: substitute audio fd */
                number_size = snprintf(number, sizeof(number), "%d", apfd);
                if (number_size >= 0 && out_len >= (size_t) number_size) {
                    memcpy(outp, number, number_size);
                    outp += number_size;
                    out_len -= number_size;
                }
                cmd++;
            } else if (*cmd == 'v') {
                /* %v: substitute video fd */
                number_size = snprintf(number, sizeof(number), "%d", vpfd);
                if (number_size >= 0 && out_len >= (size_t) number_size) {
                    memcpy(outp, number, number_size);
                    outp += number_size;
                    out_len -= number_size;
                }
                cmd++;
            } else {
                /* it doesn't match... so just use it as a literal */
                if (out_len > 0) {
                    *outp = '%';
                    outp++;
                    out_len--;
                }
            }
        } else {
            /* literal character... we just pass it through */
            if (out_len > 0) {
                *outp = ch;
                outp++;
                out_len--;
            }
        }
    }

    return out;
}

pid_t start_subprocess(const char *cmd, int &vpfd, int &apfd) {
    pid_t child;

    int vpipe[2], apipe[2];

    if (pipe(vpipe) != 0) {
        perror("pipe(vpipe)");
        return -1;
    }

    if (pipe(apipe) != 0) {
        /* 
         * FIXME a pair of FDs leak here if there is an error. 
         * Does not matter now.
         */
        perror("pipe(apipe)");
        return -1;
    }

    char *cmd_to_exec = parse_command(cmd, vpipe[1], apipe[1]);

    child = fork( );

    if (child == -1) {
        /* fork( ) failed */
        perror("fork");
        return -1;
    } else if (child == 0) {
        /* child */
        close(vpipe[0]);
        close(apipe[0]);
        execl("/bin/sh", "/bin/sh", "-c", cmd_to_exec, NULL); 

        perror("execl");
        exit(1);
    } else {
        /* parent */
        close(vpipe[1]);
        close(apipe[1]);
        vpfd = vpipe[0];
        apfd = apipe[0];
        free(cmd_to_exec);

        return child;
    } 
}

class RawFrame1080Allocator {
    public:
        RawFrame1080Allocator( ) { }
        virtual RawFrame *allocate( ) {
            return new RawFrame(1920, 1080, RawFrame::CbYCrY8422);
        }
};

class NTSCSyncAudioAllocator {
    public:
        NTSCSyncAudioAllocator( ) { 
            frame = 0;
        }

        virtual AudioPacket *allocate( ) {
            AudioPacket *ret;
            
            /*
             * the goal is to generate a sequence like this:
             *  0       1       2       3       4
             *  1602    1601    1602    1601    1602
             * 
             * that makes a total of 8,008 audio sample frames
             * for every 5 NTSC frames.
             * 5 * 1,001 / 30,000 * 48,000 = 8,008 exactly,
             * so this rate is locked to the video rate.
             */

            if (frame == 1 || frame == 3) {
                ret = new AudioPacket(48000, 2, 2, 1601);
            } else {
                ret = new AudioPacket(48000, 2, 2, 1602);
            }

            frame++;
            if (frame >= 5) {
                frame = 0;
            }

            return ret;
        }

    protected:
        int frame;
};

template <class SendableThing, class _Allocator>
class SenderThread : public Thread {
    public:
        SenderThread(Pipe<SendableThing *> *fpipe, int in_fd) {
            assert(fpipe != NULL);
            assert(in_fd >= 0);

            _fpipe = fpipe;
            _in_fd = in_fd;

            start_thread( );
        }
    protected:
        void run_thread(void) {
            SendableThing *thing;
            for (;;) {
                thing = _allocator.allocate( );

                if (thing->read_from_fd(_in_fd) == 0) {
                    break;
                }

                if (_fpipe->put(thing) == 0) {
                    break;
                }
            }
        }

        Pipe<SendableThing *> *_fpipe;
        int _in_fd;
        _Allocator _allocator;
};


int main(int argc, const char **argv) {
    OutputAdapter *oadp;
    int vpfd, apfd;
    pid_t child;

    Pipe<AudioPacket *> *apipe;

    if (argc != 2) {
        fprintf(stderr, "usage: %s 'command'\n", argv[0]);
        fprintf(stderr, "in 'command':\n");
        fprintf(stderr, "%%a = audio pipe fd\n");
        fprintf(stderr, "%%v = video pipe fd\n");
    }

    child = start_subprocess(argv[1], vpfd, apfd);

    if (child == -1) {
        return 1;
    }

    oadp = create_decklink_output_adapter_with_audio(0, 0, 
            RawFrame::CbYCrY8422);
    apipe = oadp->audio_input_pipe( );

    /* start video and audio sender threads */
    Thread *video_th = new SenderThread<RawFrame, RawFrame1080Allocator>
            (&(oadp->input_pipe( )), vpfd);
    Thread *audio_th = new SenderThread<AudioPacket, NTSCSyncAudioAllocator>
            (apipe, apfd);

    /* wait on child process */
    while (waitpid(child, NULL, 0) == EINTR) { /* busy wait */ } 

    /* done! */
    delete video_th;
    delete audio_th;
}
