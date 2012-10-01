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

#include "mjpeg_codec.h"
#include "raw_frame.h"
#include "posix_util.h"
#include "cpu_dispatch.h"
#include "hex_dump.h"

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "-n") == 0) {
        cpu_force_no_simd( );
    }
    
    /* allocate 4MB buffer */
    const size_t bufsize = 4*1024*1024;

    void *buf = malloc(bufsize);
    ssize_t size = read_all(STDIN_FILENO, buf, bufsize);

    for (int i = 0; i < 1000; i++) {
        fprintf(stderr, "buf=%p size=%zu\n", buf, size);
        RawFrame *frame = RawFrame::from_png_data(buf, size);
        delete frame;
    }
}
