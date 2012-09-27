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
#include "mjpeg_codec.h"
#include "posix_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#include <getopt.h>

char *parse_command(const char *cmd, int vpfd, int apfd) {
    size_t len = strlen(cmd);
    size_t out_len = 2*len;
    char *out = (char *)xmalloc(out_len + 1, 
            "avpinput_decklink", "command buffer");
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

    char *cmd_to_exec = parse_command(cmd, vpipe[0], apipe[0]);

    child = fork( );

    if (child == -1) {
        /* fork( ) failed */
        perror("fork");
        return -1;
    } else if (child == 0) {
        /* child */
        close(vpipe[1]);
        close(apipe[1]);
        execl("/bin/sh", "/bin/sh", "-c", cmd_to_exec, NULL); 

        perror("execl");
        exit(1);
    } else {
        /* parent */
        close(vpipe[0]);
        close(apipe[0]);
        vpfd = vpipe[1];
        apfd = apipe[1];
        free(cmd_to_exec);

        return child;
    } 
}

class CompressorThread : public Thread {
    public:
        CompressorThread(Pipe<RawFrame *> *fpipe, int out_fd) 
                : enc(1920, 1080) {
            assert(fpipe != NULL);
            assert(out_fd >= 0);

            _fpipe = fpipe;
            _out_fd = out_fd;
            start_thread( );
        }

    protected:
        void run_thread(void) {
            RawFrame *f;

            for (;;) {
                f = _fpipe->get( );
                enc.encode(f);
                delete f;
                write_all(_out_fd, enc.get_data( ), enc.get_data_size( ));
            }
        }

        Mjpeg422Encoder enc;
        Pipe<RawFrame *> *_fpipe;
        int _out_fd;
};

template <class SendableThing>
class SenderThread : public Thread {
    public:
        SenderThread(Pipe<SendableThing *> *fpipe, int out_fd) {
            assert(fpipe != NULL);
            assert(out_fd >= 0);

            _fpipe = fpipe;
            _out_fd = out_fd;
            start_thread( );
        }
    protected:
        void run_thread(void) {
            SendableThing *thing;
            for (;;) {
                thing = _fpipe->get( );

                if (thing->write_to_fd(_out_fd) <= 0) {
                    fprintf(stderr, "write failed or pipe broken\n");
                    break;
                }

                delete thing;
            }
        }

        Pipe<SendableThing *> *_fpipe;
        int _out_fd;
};

void usage(const char *argv0) {
    fprintf(stderr, "usage: %s [-c n] [-f] 'command'\n", argv0);
    fprintf(stderr, "-c n: use card 'n'");
    fprintf(stderr, "-f: enable ffmpeg streaming hack");
    fprintf(stderr, "in 'command':\n");
    fprintf(stderr, "%%a = audio pipe fd\n");
    fprintf(stderr, "%%v = video pipe fd\n");
}

int main(int argc, char * const *argv) {
    InputAdapter *iadp;
    int vpfd, apfd, opt;
    pid_t child;

    Pipe<AudioPacket *> *apipe;

    static struct option options[] = {
        { "card", 1, 0, 'c' },
        { 0, 0, 0, 0 }
    };

    int card = 0;
    int ffmpeg_hack = 0;

    /* argument processing */
    while ((opt = getopt_long(argc, argv, "fc:", options, NULL)) != -1) {
        switch (opt) {
            case 'f':
                ffmpeg_hack = 1;
                break;

            case 'c':
                card = atoi(optarg);
                break;

            default:
                usage(argv[0]);
                exit(1);
        }
    }

    if (argc - optind != 1) {
        usage(argv[0]);
        exit(1);
    }

    child = start_subprocess(argv[optind], vpfd, apfd);

    if (child == -1) {
        return 1;
    }

    /* bogus ffmpeg workaround (i.e. writing filler to the pipes) goes here */
    /* (It may be unnecessary with an audio stream?) */
    if (ffmpeg_hack) {
        RawFrame *empty = new RawFrame(1920, 1080, RawFrame::CbYCrY8422);
        AudioPacket *dummy_audio = new AudioPacket(48000, 2, 2, 1601);

        for (int i = 0; i < 2; i++) {
            empty->write_to_fd(vpfd);
            dummy_audio->write_to_fd(apfd);
        }
    }

    iadp = create_decklink_input_adapter_with_audio(card, 0, 0, 
            RawFrame::CbYCrY8422);
    apipe = iadp->audio_output_pipe( );

    /* start video and audio sender threads */
    CompressorThread vsthread(&(iadp->output_pipe( )), vpfd);
    SenderThread<AudioPacket> asthread(apipe, apfd);

    /* wait on child process */
    while (waitpid(child, NULL, 0) == EINTR) { /* busy wait */ } 

    /* done! */
}
