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
#include "pipe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main( ) {
    RawFrame *frame = NULL;
    InputAdapter *iadp;
    OutputAdapter *oadp;
    uint8_t galpha = 0;

    RawFrame key(168, 70, RawFrame::RGBAn8);
    //RawFrame key(640, 200, RawFrame::RGBAn8);
    //RawFrame key(1000, 500, RawFrame::RGBAn8);

    key.read_from_fd(STDIN_FILENO);

    iadp = create_decklink_input_adapter(0, 0, 0, RawFrame::CbYCrY8422);
    oadp = create_decklink_output_adapter(1, 0, RawFrame::CbYCrY8422);
    //iadp = create_decklink_input_adapter(0, 0, 0, RawFrame::BGRAn8);
    //oadp = create_decklink_output_adapter(1, 0, RawFrame::BGRAn8);

    for (;;) {
        if (iadp->output_pipe( ).get(frame) == 0) {
            break;
        }

        if (galpha < 255) {
            galpha++;
        }
        
        frame->draw->alpha_key(96, 830, &key, galpha);
        
        if (oadp->input_pipe( ).put(frame) == 0) {
            break;
        }
    }
}

