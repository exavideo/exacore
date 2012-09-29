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

#include "avspipe_input_adapter.h"

#include "xmalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>


AvspipeInputAdapter::AvspipeInputAdapter(const char *cmd, 
        bool use_builtin_audio) : vpipe(64) {

    pid_t child;
    signal(SIGCHLD, SIG_IGN);
    apipe = NULL;

    vpfd = apfd = -1;

    fprintf(stderr, "Starting subprocess: %s\n", cmd);

    child = start_subprocess(cmd, vpfd, apfd);

    if (child == -1) {
        throw std::runtime_error("Failed to start avspipe subprocess");
    }

    vread = new AvspipeReaderThread<RawFrame, AvspipeRawFrame1080Allocator>
        (&vpipe, vpfd);

    if (use_builtin_audio) {
        start_aplay(apfd);
        aread = NULL;
        apfd = -1;
    } else {
        apipe = new Pipe<AudioPacket *>(64);
        aread = new AvspipeReaderThread<AudioPacket, 
                AvspipeNTSCSyncAudioAllocator>(apipe, apfd);
    }
}

AvspipeInputAdapter::~AvspipeInputAdapter( ) {
    vpipe.done_reading( );
    if (apipe) {
        apipe->done_reading( );
    }

    if (aread) {
        aread->join( );
        delete aread;
    }

    if (vread) {
        vread->join( );
        delete vread;
    }
}

char *AvspipeInputAdapter::parse_command(const char *cmd, int vpfd, int apfd) {
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

pid_t AvspipeInputAdapter::start_subprocess(const char *cmd, int &vpfd, int &apfd) {
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

        size_t frames_delay = 30;
        size_t samples_per_frame = 1601;
        size_t bytes_per_sample = 4;

        size_t bogus_audio_size = frames_delay * samples_per_frame 
                * bytes_per_sample;
        /* artificial audio delay */
        void *bogus_audio = malloc(bogus_audio_size);
        memset(bogus_audio, 0, bogus_audio_size);
        write_all(apipe[1], bogus_audio, bogus_audio_size);

        close(STDIN_FILENO);
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

pid_t AvspipeInputAdapter::start_aplay(int apfd) {
    pid_t child;

    child = fork( );

    if (child == -1) {
        perror("fork");
        return -1;
    } else if (child == 0) {
        dup2(apfd, STDIN_FILENO);
        execl("/bin/sh", "/bin/sh", "-c", 
            "aplay -c 2 -f s16_le -r 48000 -", NULL);
        perror("execl");
        exit(1);
    } else {
        close(apfd);
        return child;
    }
}


