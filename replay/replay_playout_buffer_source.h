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

#ifndef _REPLAY_PLAYOUT_BUFFER_SOURCE_H
#define _REPLAY_PLAYOUT_BUFFER_SOURCE_H

#include "replay_data.h"
#include "replay_buffer.h"
#include "replay_frame_cache.h"
#include "replay_playout_source.h"
#include "replay_audio_buffer_playout.h"
#include "avspipe_allocators.h"

class ReplayPlayoutBufferSource : public ReplayPlayoutSource {
    public:
        ReplayPlayoutBufferSource(const ReplayShot &shot);
        ~ReplayPlayoutBufferSource( );
        void read_frame(ReplayPlayoutFrame &frame_data, Rational speed);
        void map_channel(unsigned int ch, ReplayBuffer *buf);
        timecode_t position( );
        timecode_t duration( );

    protected:
        ReplayFrameCache cache;
        ReplayBuffer *source;
        Rational pos;
        AvspipeNTSCSyncAudioAllocator audio_allocator;

        coord_t first_scanline(RawFrame::FieldDominance dom, int field);
        void weave_field(
            RawFrame *dst, int dst_field, 
            RawFrame *src, int src_field
        );

        ReplayAudioBufferPlayout audio_playout;
};

#endif
