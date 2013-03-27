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

#ifndef _REPLAY_FRAME_DATA_H
#define _REPLAY_FRAME_DATA_H

/*
 * Data representing a compressed M-JPEG frame in a buffer.
 */

#include <stdlib.h>
#include <stddef.h>
#include "replay_data.h"

class ReplayFrameData {
    public:
        ReplayFrameData( );
        ~ReplayFrameData( );
      
        ReplayBuffer *source;
        timecode_t pos;

        uint8_t *video_data;
        size_t video_size;

        uint8_t *thumbnail_data;
        size_t thumbnail_size;

        IOAudioPacket *audio;

        void free_data_on_destroy( );
    protected:
        bool should_free_data;
};

#endif
