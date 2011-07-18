/*
 * Copyright 2011 Exavideo LLC.
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

#include "replay_playout.h"
#include "raw_frame.h"
#include "mjpeg_codec.h"

ReplayPlayout::ReplayPlayout(OutputAdapter *oadp_) {
    oadp = oadp_;
    current_source = NULL;
    running = false;
    start_thread( );
}

ReplayPlayout::~ReplayPlayout( ) {
    
}

void ReplayPlayout::roll_shot(ReplayShot *shot) {
    MutexLock l(m);
    current_source = shot->source;
    current_tc = shot->start;
    running = true;
}

void ReplayPlayout::run_thread( ) {
    ReplayBuffer *source;
    timecode_t tc;
    Mjpeg422Decoder dec(1920, 1080);
    ReplayFrameData rfd;
    RawFrame *frame;
    ReplayRawFrame *monitor_frame;

    for (;;) {
        get_and_advance_current_frame(source, tc);
        if (source == NULL) {
            /* show something useless */
        } else {
            fprintf(stderr, "decode tc=%d\n", (int) tc);
            /* get the frame index we should be showing now */

            /* get frame from buffer and decode it */
            source->get_readable_frame(tc, rfd);
            frame = dec.decode(rfd.data_ptr, rfd.data_size);

            /* scale down to BGRAn8 and send to monitor port */
            monitor_frame = new ReplayRawFrame(
                frame->convert->BGRAn8_scale_1_2( )
            );
            monitor.put(monitor_frame);

            /* send the full CbYCrY frame to output */
            oadp->input_pipe( ).put(frame);
        }
    }
}

void ReplayPlayout::get_and_advance_current_frame(ReplayBuffer *&src, 
        timecode_t &tc) {
    MutexLock l(m);
    if (current_source != NULL) {
        src = current_source;
        tc = current_tc;
        if (running) {
            current_tc++;
        }
    } else {
        src = NULL;
        tc = 0;
    }
}

void ReplayPlayout::stop( ) {
    MutexLock l(m);
    running = false;
}
