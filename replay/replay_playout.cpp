/*
 * Copyright 2013 Exavideo LLC.
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
#include "replay_playout_bars_source.h"

ReplayPlayout::ReplayPlayout(OutputAdapter *oadp_) : next_source_pipe(2) {
    oadp = oadp_;
    idle_source = new ReplayPlayoutBarsSource;
    start_thread( );
}

ReplayPlayout::~ReplayPlayout( ) {
    delete idle_source;
}

void ReplayPlayout::run_thread( ) {
    ReplayPlayoutSource *active_source = idle_source; 
    ReplayPlayoutSource *next_source;
    ReplayPlayoutFrame frame_data;
    ReplayRawFrame *monitor_frame;

    for (;;) {
        /* is there a next source available? if so, we take it */
        if (next_source_pipe.data_ready( )) {
            next_source = next_source_pipe.get( );
            if (active_source != idle_source) {
                delete active_source;
            }

            active_source = next_source;
        }


        /* read data from currently active source */
        active_source->read_frame(frame_data, speed);
        
        /* add any keys that are enabled */

        /* create monitor frame */
        monitor_frame = new ReplayRawFrame(
            frame_data.video_data->convert->BGRAn8_scale_1_2( )
        );
        monitor_frame->source_name = "Program";
        monitor_frame->source_name2 = frame_data.source_name;
        monitor_frame->tc = frame_data.tc;
        monitor_frame->fractional_tc = frame_data.fractional_tc;
        monitor.put(monitor_frame);

        /* write data to output */
        oadp->input_pipe( ).put(frame_data.video_data);
        if (oadp->audio_input_pipe( )) {
            oadp->audio_input_pipe( )->put(frame_data.audio_data);
        } else {
            delete frame_data.audio_data;
        }
    }

}
