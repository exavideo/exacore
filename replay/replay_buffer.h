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

#ifndef _REPLAY_BUFFER_H
#define _REPLAY_BUFFER_H

#include "replay_data.h"
#include "mutex.h"

#include <stdexcept>

class ReplayFrameNotFoundException : public virtual std::exception {
    const char *what() const throw() { return "Frame off ends of buffer"; }
};

class ReplayBuffer {
    public:
        enum whence_t { ZERO, START, END };

        ReplayBuffer(const char *path, size_t buffer_size, size_t frame_size);
        ~ReplayBuffer( );

        ReplayShot *make_shot(timecode_t offset, whence_t whence = END);

        void get_writable_frame(ReplayFrameData &frame_data);
        void finish_frame_write( );

        void get_readable_frame(timecode_t tc, ReplayFrameData &frame_data);

    private:
        int fd;

        size_t buffer_size;
        size_t frame_size;
        unsigned int n_frames;

        uint8_t *data;

        timecode_t tc_current;

        Mutex m;
};

#endif
