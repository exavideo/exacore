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

#ifndef _REPLAY_PLAYOUT_QUEUE_SOURCE_H
#define _REPLAY_PLAYOUT_QUEUE_SOURCE_H

#include "replay_playout_source.h"
#include <list>

class ReplayPlayoutQueueSource : public ReplayPlayoutSource {
    public:
        typedef std::list<ReplayPlayoutSource *> SourceQueue;
        ReplayPlayoutQueueSource(SourceQueue &src);
        virtual void read_frame(ReplayPlayoutFrame &frame_data, Rational speed);
        virtual ~ReplayPlayoutQueueSource( );

        virtual timecode_t position( );
        virtual timecode_t duration( );

    protected:
        SourceQueue sources;
        timecode_t frames_rolled;
        timecode_t total_duration;
        bool duration_known;
};

#endif
