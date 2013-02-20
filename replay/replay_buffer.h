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
#include "replay_buffer_index.h"
#include "mutex.h"
#include "thread.h"
#include "condition.h"

#include <stdexcept>

class ReplayBufferReader {
    public:
        ReplayBufferReader(ReplayBuffer *buf_,
            ReplayBufferIndex *index_, int fd_);
        ~ReplayBufferReader( );

        ReplayFrameData *read_frame(timecode_t tc);
        ReplayFrameData *read_next_frame( );
        ReplayBuffer *source( ) { return buf; }

    protected:
        ReplayBuffer *buf;
        ReplayBufferIndex *index;
        int fd;
        timecode_t timecode;

        void seek_to(off_t where, int whence = SEEK_SET);
};

class ReplayBufferWriter {
    public:
        ReplayBufferWriter(ReplayBuffer *buf_, 
            ReplayBufferIndex *index_, int fd_);
        ~ReplayBufferWriter( );

        timecode_t write_frame(const ReplayFrameData &data);
    protected:
        ReplayBuffer *buf;
        ReplayBufferIndex *index;
        int fd;
};

class ReplayBuffer {
    public:
        enum whence_t { ZERO, START, END };

        ReplayBuffer(const char *path, const char *name="(unnamed)");
        ~ReplayBuffer( );

        ReplayShot *make_shot(timecode_t offset, whence_t whence = END);

        ReplayBufferWriter *make_writer( );
        ReplayBufferReader *make_reader( );

        RawFrame::FieldDominance field_dominance( ) { return _field_dominance; }
        void set_field_dominance(RawFrame::FieldDominance dom) { _field_dominance = dom; }

        const char *get_name( );

        struct FrameHeader {
            int version;
            size_t video_size;
            size_t thumbnail_size;
            size_t audio_size;
        };

        /* do not call directly, called automatically when writer deleted */
        void release_writer(ReplayBufferWriter *writer);
    private:
        ReplayBufferIndex *index;
        RawFrame::FieldDominance _field_dominance;

        char *name;
        char *path;
        int fd;

        ReplayBufferWriter *writer; /* so we know if one already exists */
};

#endif
