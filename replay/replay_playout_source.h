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

#ifndef _REPLAY_PLAYOUT_SOURCE_H
#define _REPLAY_PLAYOUT_SOURCE_H

#include "replay_data.h"
#include "rational.h"

class ReplayPlayoutSource {
    public:
        virtual void read_frame(ReplayPlayoutFrame &frame_data, Rational speed) = 0;
        void set_output_dominance(RawFrame::FieldDominance dom) { 
            output_dominance = dom; 
        }
        virtual ~ReplayPlayoutSource( ) { }

        virtual timecode_t position( ) = 0;
        virtual timecode_t duration( ) = 0;

    protected:
        RawFrame::FieldDominance output_dominance;
};

#endif
