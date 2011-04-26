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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PI 3.14

int main( ) {
    RawFrame *frame;
    OutputAdapter *oadp;
    AudioPacket *apkt;
    uint8_t x;

    oadp = create_decklink_output_adapter_with_audio(0, 0, 
            RawFrame::CbYCrY8422);

    for (;;) {
        for (x = 16; x < 224; x++) {
            frame = new RawFrame(1920, 1080, RawFrame::CbYCrY8422);
            apkt = new AudioPacket(48000, 2, 2, 1601);
            if (apkt->read_from_fd(STDIN_FILENO) <= 0) {
                exit(0);
            }

            memset(frame->data( ), x, frame->size( ));

            if (oadp->input_pipe( ).put(frame) < 0) {
                fprintf(stderr, "could not write frame to output");
                exit(1);
            }

            if (oadp->audio_input_pipe( )->put(apkt) < 0) {
                fprintf(stderr, "could not write audio data to output");
                exit(1);
            }
        }
    }
}
