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

#include "replay_ingest.h"
#include "mjpeg_codec.h"
#include <assert.h>

ReplayIngest::ReplayIngest(InputAdapter *iadp_, ReplayBuffer *buf_) {
    iadp = iadp_;
    buf = buf_;
    start_thread( );
}

ReplayIngest::~ReplayIngest( ) {

}

void ReplayIngest::run_thread( ) {
    RawFrame *input;
    ReplayRawFrame *monitor_frame;
    ReplayFrameData dest;
    Mjpeg422Encoder enc(1920, 1080); /* FIXME: hard coded frame size */

    for (;;) {
        /* obtain writable frame from buffer */
        buf->get_writable_frame(dest);

        /* obtain frame from input adapter */
        input = iadp->output_pipe( ).get( );

        /* encode to M-JPEG */
        enc.encode_to(input, dest.data_ptr, dest.data_size);
        buf->finish_frame_write( );

        /* scale down frame to send to monitor */
        monitor_frame = new ReplayRawFrame(
            input->convert->BGRAn8_scale_1_4( )
        );
        monitor.put(monitor_frame);
        
        delete input;
    }
}

