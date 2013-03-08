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

#ifndef _REPLAY_PLAYOUT_BARS_SOURCE_H
#define _REPLAY_PLAYOUT_BARS_SOURCE_H

#include "replay_playout_source.h"
#include "avspipe_allocators.h"

class ReplayPlayoutBarsSource : public ReplayPlayoutSource {
    public:
        ReplayPlayoutBarsSource( );
        ~ReplayPlayoutBarsSource( );

        void read_frame(ReplayPlayoutFrame &frame_data, Rational speed);
        timecode_t duration( );
        timecode_t position( );

    protected:
        RawFrame *bars;
        AvspipeNTSCSyncAudioAllocator audio_allocator;

        void oscillate(AudioPacket *pkt, float frequency);
        /* phase accumulator for oscillator */
        double phase;
};

#endif
