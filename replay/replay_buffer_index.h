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

#ifndef _REPLAY_BUFFER_INDEX_H
#define _REPLAY_BUFFER_INDEX_H

#include "replay_data.h"
#include <stddef.h>
#include <sys/types.h>
#include <exception>
#include <atomic>

class ReplayFrameNotFoundException : public virtual std::exception {
    const char *what() const throw() { return "Frame off ends of buffer"; }
};

class ReplayBufferIndexFullException : public virtual std::exception {
    const char *what() const throw() { return "Replay buffer index full"; }
};

class ReplayBufferIndex {
    public:
        ReplayBufferIndex(size_t n_frames = 8192*1024);
        ~ReplayBufferIndex( );
        off_t get_frame_location(timecode_t frame);
        uint64_t get_frame_timestamp(timecode_t frame);
        timecode_t mark_frame(size_t length);
        timecode_t find_timecode(uint64_t timestamp);
        timecode_t get_length( );

    protected:
        struct IndexEntry {
            off_t start;
            uint64_t timestamp;
        } *data;

        std::atomic<timecode_t> n_frames_written; 
        size_t n_bytes_written;         /* only writer cares or uses */
        timecode_t n_frames_total;          /* never changes */

        /* find the frame with largest timestamp where timestamp < stamp */
        timecode_t timestamp_search(
            uint64_t stamp, 
            timecode_t start, 
            timecode_t end
        );
};

#endif
