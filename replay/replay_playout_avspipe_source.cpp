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

#include "replay_playout_avspipe_source.h"

ReplayPlayoutAvspipeSource::ReplayPlayoutAvspipeSource(const char *cmd) {
    iadp = new AvspipeInputAdapter(cmd);
    n_frames = 0;
}

ReplayPlayoutAvspipeSource::~ReplayPlayoutAvspipeSource( ) {
    delete iadp;
}

void ReplayPlayoutAvspipeSource::read_frame(
        ReplayPlayoutFrame &frame, 
        Rational speed
) {
    (void) speed; /* we can't really do much about the speed here */

    try {
        frame.video_data = iadp->output_pipe( ).get( );
        frame.audio_data = iadp->audio_output_pipe( )->get( );
        frame.source_name = "AVSPIPE Rollout";
        frame.tc = n_frames;
        frame.fractional_tc = 0;
        n_frames++;
    } catch (const BrokenPipe &) {
        frame.video_data = NULL;
    }
}

timecode_t ReplayPlayoutAvspipeSource::position( ) {
    return n_frames;
}

timecode_t ReplayPlayoutAvspipeSource::duration( ) {
    return -1;
}

