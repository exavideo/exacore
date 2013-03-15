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
#include "block_set.h"

#include <stdexcept>

class ReplayBuffer {
    public:
        enum whence_t { ZERO, START, END };
        enum LoadFlags { LOAD_VIDEO = 0x01, LOAD_THUMBNAIL = 0x02, LOAD_AUDIO = 0x04 };

        ReplayBuffer(const char *path, const char *name="(unnamed)");
        ~ReplayBuffer( );

        ReplayShot *make_shot(timecode_t offset, whence_t whence = END);
        
        /* these are now convenience wrappers around {read,write}_blockset */
        ReplayFrameData *read_frame(timecode_t frame, int flags);
        timecode_t write_frame(const ReplayFrameData &frame);

        void read_blockset(timecode_t frame, BlockSet &blkset);
        timecode_t write_blockset(const BlockSet &blkset);

        RawFrame::FieldDominance field_dominance( ) { return _field_dominance; }
        void set_field_dominance(RawFrame::FieldDominance dom) { _field_dominance = dom; }

        const char *get_name( );

        struct FrameHeader {
            int version;
            size_t video_size;
            size_t thumbnail_size;
            size_t audio_size;
        };

    private:
        ReplayBufferIndex *index;
        RawFrame::FieldDominance _field_dominance;

        char *name;
        char *path;
        int fd;
};

#endif
