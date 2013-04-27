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

#include "replay_buffer_index.h"
#include "clocks.h"

ReplayBufferIndex::ReplayBufferIndex(size_t n_frames) {
    n_frames_written = 0;
    n_bytes_written = 0;
    data = new IndexEntry[n_frames];

    for (size_t i = 0; i < n_frames; i++) {
        data[i].start = 0;
    }

    n_frames_total = n_frames;
}

ReplayBufferIndex::~ReplayBufferIndex( ) {
    delete [] data;
}

off_t ReplayBufferIndex::get_frame_location(timecode_t frame) {
    if (frame < n_frames_written && frame >= 0) {
        return data[frame].start;
    } else {
        throw ReplayFrameNotFoundException( );
    }
}

uint64_t ReplayBufferIndex::get_frame_timestamp(timecode_t frame) {
    if (frame < n_frames_written && frame >= 0) {
        return data[frame].timestamp;
    } else {
        throw ReplayFrameNotFoundException( );
    }
}

timecode_t ReplayBufferIndex::find_timecode(uint64_t timestamp) {
    timecode_t end = n_frames_written-1;

    if (timestamp <= data[0].timestamp) {
        return data[0].timestamp;
    } else if (timestamp >= data[end].timestamp) {
        return data[end].timestamp;
    } else {
        return timestamp_search(timestamp, 0, end);
    }
}

timecode_t ReplayBufferIndex::timestamp_search(
    uint64_t stamp,
    timecode_t start,
    timecode_t end
) {
    timecode_t mid = (start+end)/2;
    if (end <= start) {
        return start;
    } else if (data[end].timestamp <= stamp) {
        return end;
    } else if (data[mid].timestamp <= stamp) {
        /* data[end].timestamp > stamp so exclude it */
        return timestamp_search(stamp, mid, end-1);
    } else {
        /* data[mid].timestamp > stamp so we exclude it */
        return timestamp_search(stamp, start, mid-1);
    }
}

timecode_t ReplayBufferIndex::mark_frame(size_t length) {
    if (n_frames_written < n_frames_total) {
        data[n_frames_written].start = n_bytes_written;
        data[n_frames_written].timestamp = clock_monotonic_msec( );
        n_bytes_written += length;
        /* 
         * since n_frames_written is atomic, all the above writes must 
         * complete before a reader will see the new frame.
         * We return the value before the increment as the timecode of the
         * newly written frame.
         */
        return n_frames_written++;
    } else {
        throw ReplayBufferIndexFullException( );
    }
}

timecode_t ReplayBufferIndex::get_length( ) {
    return n_frames_written;
}
