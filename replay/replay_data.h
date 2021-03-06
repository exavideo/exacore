/*
 * Copyright 2011, 2013 Exavideo LLC.
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
#include "packed_audio_packet.h"
#include "rational.h"

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

#include "replay_frame_data.h"

/*
 * Wrap a reference to a RawFrame object used in the monitor ports.
 * ReplayRawFrames own their corresponding RawFrames and will delete
 * them when they go out of scope.
 */
struct ReplayRawFrame {
    ReplayRawFrame(RawFrame *f) : fractional_tc(0) { 
        frame_data = f; 
        source_name = NULL;
        source_name2 = NULL;
        bgra_data = NULL;
        tc = 0;
    }
    ~ReplayRawFrame( ) { 
        delete frame_data;
    }

    RawFrame *frame_data;
    RawFrame *bgra_data;
    /* other stuff goes here */
    const char *source_name;
    const char *source_name2;
    timecode_t tc;
    Rational fractional_tc;
};

/* These are passed between playout and playout sources */
struct ReplayPlayoutFrame {
    RawFrame *video_data;
    IOAudioPacket *audio_data;
    const char *source_name;
    timecode_t tc;
    Rational fractional_tc;
};

#endif
