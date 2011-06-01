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

ReplayIngest::ReplayIngest(InputAdapter *iadp_, Buffer *buf_) {
    iadp = iadp_;
    buf = buf_;

    start_thread( );
}

void ReplayIngest::run_thread( ) {
    RawFrame *input;
    RawFrame *preview;

    for (;;) {
        if (iadp->output_pipe( ).get(input) == 0) {
            fprintf(stderr, "ReplayIngest: input adapter died\n");
            break;
        }

        enc.encode(input);
        preview = input->convert->BGRAn8_1_4( );
        preview_port.put(preview);

        /* 
         * FIXME: optimization opportunity: get Mjpeg422Encoder to write data
         * directly to the memory-mapped buffer
         */
        void *dest = buf->block_ptr(buf->write_offset( ));
        memcpy(dest, enc.get_data( ), enc.get_data_size( ));
        buf->write_done( );

    }
}
