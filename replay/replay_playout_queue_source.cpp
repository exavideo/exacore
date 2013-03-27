/*
 * Copyright 2011, 2012, 2013 Exavideo LLC.
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

#include "replay_playout_queue_source.h"

ReplayPlayoutQueueSource::ReplayPlayoutQueueSource(
    SourceQueue &sources
) {
    timecode_t source_duration;
    duration_known = true;
    total_duration = 0;
    frames_rolled = 0;

    for (
        SourceQueue::iterator i = sources.begin( ); 
        i != sources.end( ); 
        i++
    ) {
        source_duration = (*i)->duration( );
        if (source_duration > 0) {
            total_duration += source_duration;
        } else {
            duration_known = false;
        }

        this->sources.push_back(*i);
    }
}

void ReplayPlayoutQueueSource::read_frame(
    ReplayPlayoutFrame &frame_data, 
    Rational speed
) {
    frame_data.video_data = NULL;
    ReplayPlayoutSource *csource;

    while (frame_data.video_data == NULL && !sources.empty( )) {
        csource = sources.front( );    
        csource->read_frame(frame_data, speed);

        if (frame_data.video_data == NULL) {
            /* this source can give us no more video, so move to next */
            delete csource;
            sources.pop_front( );
        }
    }

    frames_rolled++;
}

ReplayPlayoutQueueSource::~ReplayPlayoutQueueSource( ) {
    while (!sources.empty( )) {
        delete sources.front( );
        sources.pop_front( );
    }
}

timecode_t ReplayPlayoutQueueSource::position( ) {
    return frames_rolled;
}

timecode_t ReplayPlayoutQueueSource::duration( ) {
    if (duration_known) {
        return total_duration;
    } else {
        return -1;
    }
}
