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

ReplayIngest::ReplayIngest(InputAdapter *iadp_, ReplayBuffer *buf_,
        ReplayGameData *gds) {
    iadp = iadp_;
    buf = buf_;
    gd = gds;
    encode_suspended = false;
    start_thread( );
}

ReplayIngest::~ReplayIngest( ) {

}

void ReplayIngest::run_thread( ) {
    RawFrame *input, *thumb;
    ReplayRawFrame *monitor_frame;
    ReplayFrameData dest;
    std::string com;
    Mjpeg422Encoder enc(1920, 1080); /* FIXME: hard coded frame size */

    for (;;) {

        /* obtain writable frame from buffer */
        buf->get_writable_frame(dest);

        /* obtain frame from input adapter */
        input = iadp->output_pipe( ).get( );

        bool suspended;

        { MutexLock l(m);
            suspended = encode_suspended;
        }

        if (!suspended) {
            /* set field dominance if necessary */
            if (buf->field_dominance( ) == RawFrame::UNKNOWN) {
                buf->set_field_dominance(input->field_dominance( ));
            }

            if (gd != NULL) {
                gd->as_jpeg_comment(com);
                enc.set_comment(com);
            }

            /* encode to M-JPEG */
            enc.encode_to(input, dest.main_jpeg( ), dest.main_jpeg_size( ));

            /* scale input and make JPEG thumbnail */
            thumb = input->convert->CbYCrY8422_scaled(480, 270);
            //enc.encode_to(thumb, dest.thumb_jpeg( ), dest.thumb_jpeg_size( ));
            delete thumb;

            buf->finish_frame_write( );

            /* scale down frame to send to monitor */
            monitor_frame = new ReplayRawFrame(
                input->convert->BGRAn8_scale_1_4( )
            );
            
            /* fill in monitor status info */
            monitor_frame->source_name = buf->get_name( );
            monitor_frame->tc = dest.pos;

            monitor.put(monitor_frame);
        }
        
        delete input;
    }
}

void ReplayIngest::suspend_encode( ) {
    MutexLock l(m);
    encode_suspended = true;
}

void ReplayIngest::resume_encode( ) {
    MutexLock l(m);
    encode_suspended = false;
}
