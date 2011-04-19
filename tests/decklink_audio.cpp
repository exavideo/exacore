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
#include <unistd.h>

int main( ) {
    RawFrame *frame = NULL;
    AudioPacket *apkt = NULL;

    InputAdapter *iadp;
    Pipe<AudioPacket *> *apipe;

    iadp = create_decklink_input_adapter_with_audio(0, 0, 0, RawFrame::CbYCrY8422);
    apipe = iadp->audio_output_pipe( );

    for (;;) {
        if (iadp->output_pipe( ).data_ready( )) {
            if (iadp->output_pipe( ).get(frame) == 0) {
                break;
            }
            fprintf(stderr, "got video frame\n");
            delete frame;
            frame = NULL;
        } else {
            if (apipe->get(apkt) == 0) {
                break;
            }
            fprintf(stderr, "got audio packet\n");
            apkt->write_to_fd(STDOUT_FILENO);
            delete apkt;
            apkt = NULL;
        }
    }
}

