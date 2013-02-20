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

#include "replay_frame_data.h"

ReplayFrameData::ReplayFrameData( ) {
    source = NULL;
    pos = 0;

    video_data = NULL;
    video_size = 0;

    thumbnail_data = NULL;
    thumbnail_size = 0;

    audio_data = NULL;
    audio_size = 0;

    should_free_data = false;
}

ReplayFrameData::~ReplayFrameData( ) {
    if (should_free_data) {
        if (video_data) { 
            free(video_data); 
        }
        if (audio_data) { 
            free(audio_data); 
        }
        if (thumbnail_data) { 
            free(thumbnail_data); 
        }
    }
}

void ReplayFrameData::free_data_on_destroy( ) {
    should_free_data = true;
}
