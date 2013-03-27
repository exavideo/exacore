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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mjpeg_codec.h"
#include "raw_frame.h"
#include "posix_util.h"
#include "cpu_dispatch.h"

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "-n") == 0) {
        cpu_force_no_simd( );
    }

    /* Read 1080p UYVY422 frames on stdin. Dump M-JPEG stream on stdout. */
    RawFrame frame(1920, 1080, RawFrame::CbYCrY8422);
    Mjpeg422Encoder enc(1920, 1080);
    ssize_t ret;


    for (;;) {
        ret = frame.read_from_fd(STDIN_FILENO);

        if (ret < 0) {
            perror("frame.read_from_fd");
            exit(1);
        } else if (ret == 0) {
            break;
        } else {
            enc.set_comment("Hello World!");
            enc.encode(&frame);
            ret = write_all(STDOUT_FILENO, 
                    enc.get_data( ), enc.get_data_size( ));

            if (ret < 0) {
                perror("write_all");
                exit(2);
            }
        }            
    }
}
