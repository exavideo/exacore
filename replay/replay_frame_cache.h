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

#ifndef _REPLAY_FRAME_CACHE_H
#define _REPLAY_FRAME_CACHE_H

#include "raw_frame.h"
#include "replay_buffer.h"
#include "mjpeg_codec.h"

/* Cache decoded frames for faster access. */
class ReplayFrameCache {
    public:
        ReplayFrameCache( );
        ~ReplayFrameCache( );
        RawFrame *get_frame(ReplayBuffer *source, timecode_t tc);
    protected:
        ReplayBufferReader *reader;
        ReplayFrameData *cached_compressed_frame;
        RawFrame *cached_raw_frame;
        Mjpeg422Decoder decoder;

        void load_frame(timecode_t tc);
};

#endif
