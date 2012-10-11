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
#include "thread.h"
#include "condition.h"

#include <stdexcept>

class ReplayFrameNotFoundException : public virtual std::exception {
    const char *what() const throw() { return "Frame off ends of buffer"; }
};

class ReplayBuffer;

class ReplayBufferLocker : public Thread {
    public:
        ReplayBufferLocker( );
        ~ReplayBufferLocker( );

        void set_position(ReplayBuffer *buf, timecode_t tc);

    protected:
        timecode_t start, end;
        ReplayBuffer *buf;        

        Mutex m;
        Condition c;

        void run_thread( );

        void lock_all_frames(ReplayBuffer *buf, timecode_t start, 
                timecode_t end);
        
        void unlock_all_frames(ReplayBuffer *buf, timecode_t start, 
                timecode_t end);

        void move_range(ReplayBuffer *buf, timecode_t s1, timecode_t e1,
                timecode_t s2, timecode_t e2);
};

class ReplayBuffer {
    public:
        enum whence_t { ZERO, START, END };

        ReplayBuffer(const char *path, size_t buffer_size, size_t frame_size,
                const char *name="(unnamed)");
        ~ReplayBuffer( );

        ReplayShot *make_shot(timecode_t offset, whence_t whence = END);

        void get_writable_frame(ReplayFrameData &frame_data);
        void finish_frame_write(ReplayFrameData &frame_data);

        void get_readable_frame(timecode_t tc, ReplayFrameData &frame_data);
        void finish_frame_read(ReplayFrameData &frame_data);

        RawFrame::FieldDominance field_dominance( ) { return _field_dominance; }
        void set_field_dominance(RawFrame::FieldDominance dom) { _field_dominance = dom; }

        const char *get_name( );

        void lock_frame(timecode_t frame);
        void unlock_frame(timecode_t frame);

    private:
        RawFrame::FieldDominance _field_dominance;

        char *name;
        int fd;

        size_t buffer_size;
        size_t frame_size;
        timecode_t n_frames;

        uint8_t *data;

        volatile timecode_t tc_current;

        int *locks;

        ReplayBufferLocker write_lock;

        Mutex m;

        void try_readahead(timecode_t tc);
        void try_readahead(timecode_t tc, unsigned int n);
};

#endif
