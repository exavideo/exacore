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

#include "replay_buffer.h"
#include "posix_util.h"

#include "pipe.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdexcept>

ReplayBuffer::ReplayBuffer(const char *path, const char *name) {
    struct stat stat;
    
    _field_dominance = RawFrame::UNKNOWN;

    /* open and allocate (if necessary) buffer file */
    fd = open(path, O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        throw POSIXError("open");
    }

    if (fstat(fd, &stat) != 0) {
        throw POSIXError("fstat");
    }

    fprintf(stderr, "st_mode=%x\n", stat.st_mode);
    if (S_ISREG(stat.st_mode) == 0) {
        throw std::runtime_error("not a regular file, cannot use as buffer");
    } 

    this->name = strdup(name);
    if (this->name == NULL) {
        throw std::runtime_error("allocation failure");
    }

    this->path = strdup(path);
    if (this->path == NULL) {
        throw std::runtime_error("allocation failure");
    }

    index = new ReplayBufferIndex;
}

ReplayBuffer::~ReplayBuffer( ) {
    if (close(fd) != 0) {
        throw POSIXError("close");
    }

    /* 
     * we deliberately do not delete writer here, that is the responsibility
     * of the user of the writer. The pointer is only kept around in 
     * ReplayBuffer for bookkeeping (so we only have one writer per buffer)
     */

    free(name);
    free(path);
    delete index;
}

ReplayShot *ReplayBuffer::make_shot(timecode_t offset, whence_t whence) {
    ReplayShot *shot = new ReplayShot;

    shot->source = this;
    switch (whence) {
        case ZERO:
        case START:
            shot->start = offset;
            break;
        case END:
            shot->start = index->get_length( ) - 1 + offset;
            break;
    }

    shot->length = 0;
    return shot;
}

ReplayShot *ReplayBuffer::align_shot(ReplayShot *other) {
    ReplayShot *shot = new ReplayShot;
    uint64_t timestamp;
    timecode_t frame;

    shot->source = this;
    timestamp = other->source->get_frame_timestamp(other->start);
    frame = index->find_timecode(timestamp);

    shot->start = frame;
    shot->length = 0;

    return shot;
}

uint64_t ReplayBuffer::get_frame_timestamp(timecode_t frame) {
    return index->get_frame_timestamp(frame);
}

const char *ReplayBuffer::get_name( ) {
    return name;
}

void ReplayBuffer::read_blockset(timecode_t frame, BlockSet &blkset) {
    off_t base_offset;
    base_offset = index->get_frame_location(frame);
    blkset.begin_read(fd, base_offset);
}

timecode_t ReplayBuffer::write_blockset(const BlockSet &blkset) {
    size_t total_size;
    total_size = blkset.write_all(fd);
    return index->mark_frame(total_size);
}

/* FIXME: these should be refactored into something cleaner */
ReplayFrameData *ReplayBuffer::read_frame(timecode_t frame, int flags) {
    BlockSet blkset;
    read_blockset(frame, blkset);

    ReplayFrameData *ret = new ReplayFrameData;
    ret->free_data_on_destroy( );
    ret->source = this;
    ret->pos = frame;

    if (flags & LOAD_VIDEO) {
        ret->video_data = blkset.load_alloc_block<uint8_t>(
            REPLAY_VIDEO_BLOCK, 
            ret->video_size
        );
    }

    if (flags & LOAD_THUMBNAIL) {
        ret->thumbnail_data = blkset.load_alloc_block<uint8_t>(
            REPLAY_THUMBNAIL_BLOCK, 
            ret->thumbnail_size
        );
    }

    if (flags & LOAD_AUDIO) {
        try {
            ret->audio = 
                blkset.load_alloc_object<IOAudioPacket>(REPLAY_AUDIO_BLOCK);
        } catch (const std::runtime_error &e) {
            ret->audio = NULL;
        }
    }

    /* determine offset and length of next frames and read ahead */
    try {
        off_t readahead_start_offset = index->get_frame_location(frame + 1);
        off_t readahead_end_offset = index->get_frame_location(frame + 9);
        off_t readahead_length = readahead_end_offset - readahead_start_offset;
        if (posix_fadvise(fd, readahead_start_offset, 
                readahead_length, POSIX_FADV_WILLNEED) != 0) {
            perror("posix_fadvise");
        }
    } catch (const ReplayFrameNotFoundException &) {
        /* pass */
    }

    return ret;
}

timecode_t ReplayBuffer::write_frame(const ReplayFrameData &data) {
    BlockSet blkset;

    if (data.video_size == 0) {
        throw std::runtime_error("Tried to write empty frame");
    }

    if (data.video_size > 0) {
        blkset.add_block(
            REPLAY_VIDEO_BLOCK, 
            data.video_data, 
            data.video_size
        );
    }
    if (data.thumbnail_size > 0) {
        blkset.add_block(
            REPLAY_THUMBNAIL_BLOCK, 
            data.thumbnail_data, 
            data.thumbnail_size
        );
    }
    if (data.audio != NULL) {
        blkset.add_object(REPLAY_AUDIO_BLOCK, *(data.audio));
    }

    return write_blockset(blkset);
}

