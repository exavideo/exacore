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

#include "raw_frame.h"
#include "posix_util.h"
#include "cpu_dispatch.h"

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "-n") == 0) {
        cpu_force_no_simd( );
    }

    /* Read 540p frames on stdin; dump 1080p frames on stdout... */
    RawFrame frame(960, 540, RawFrame::CbYCrY8422);
    ssize_t ret;

    for (;;) {
        ret = frame.read_from_fd(STDIN_FILENO);

        if (ret < 0) {
            perror("frame.read_from_fd");
            exit(1);
        } else if (ret == 0) {
            break;
        } else {
            RawFrame *out = frame.convert->CbYCrY8422_scan_double( );

            if (out->write_to_fd(STDOUT_FILENO) < 0) {
                perror("write_to_fd");
                break;
            }
            delete out;
        }            
    }
}
