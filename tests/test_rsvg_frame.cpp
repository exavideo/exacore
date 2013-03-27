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

#include "rsvg_frame.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_SVG 65536

int main( ) {
    static char svg_buf[MAX_SVG];
    ssize_t read_ret;
    size_t read_pos = 0;

    /* slurp input */
    while (read_pos < MAX_SVG) {
        read_ret = read(STDIN_FILENO, svg_buf + read_pos, MAX_SVG - read_pos);
        if (read_ret < 0) {
            perror("read");
            exit(1);
        } else if (read_ret == 0) {
            break;
        } else {
            read_pos += read_ret;
        }
    }

    /* create SVG frame */
    RawFrame *svg = RsvgFrame::render_svg(svg_buf, read_pos);

    /* dump raw BGRAn8 video to stdout */
    fprintf(stderr, "w=%d h=%d\n", svg->w( ), svg->h( ));
    svg->write_to_fd(STDOUT_FILENO);
}
