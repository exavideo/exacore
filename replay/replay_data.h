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

#ifndef _REPLAY_DATA_H
#define _REPLAY_DATA_H

#include <stddef.h>
#include <stdint.h>

#include "raw_frame.h"

class ReplayBuffer;

typedef int_fast32_t timecode_t;

/*
 * A shot, or a sequence of frames residing in one of the replay buffers.
 */
struct ReplayShot {
    ReplayBuffer *source;
    timecode_t start;
    timecode_t length;
};

/*
 * Data representing a compressed M-JPEG frame in a buffer.
 */
struct ReplayFrameData {
    ReplayBuffer *source;
    timecode_t pos;
    void *data_ptr;
    size_t data_size;
};

/*
 * Wrap a reference to a RawFrame object used in the monitor ports.
 * ReplayRawFrames own their corresponding RawFrames and will delete
 * them when they go out of scope.
 */
struct ReplayRawFrame {
    ReplayRawFrame(RawFrame *f) { frame_data = f; }
    ~ReplayRawFrame( ) { delete frame_data; }

    RawFrame *frame_data;
    /* other stuff goes here */
    const char *source_name;
    timecode_t tc;
};

#endif
