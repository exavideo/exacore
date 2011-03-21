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
#include "decklink.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX_SVG 65536

int main( ) {
    static char svg_buf[MAX_SVG];
    ssize_t read_ret;
    size_t read_pos = 0;

    InputAdapter *iadp;
    OutputAdapter *oadp;
    RawFrame *key, *frame;
    uint8_t galpha = 0;

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

    iadp = create_decklink_input_adapter(0, 0, 0, RawFrame::CbYCrY8422);
    oadp = create_decklink_output_adapter(1, 0, RawFrame::CbYCrY8422);
    
    for (;;) {
        /* get frame */
        if (iadp->output_pipe( ).get(frame) == 0) {
            break;
        }

        /* render SVG */
        key = RsvgFrame::render_svg(svg_buf, read_pos);
    
        /* crude dissolve in */
        if (galpha < 255) {
            galpha++;
        }

        /* draw as key */
        frame->draw->alpha_key(20, 20, key, galpha);
        delete key;

        /* send to output */
        if (oadp->input_pipe( ).put(frame) == 0) {
            break;
        }
    }
}
