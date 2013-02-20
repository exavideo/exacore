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


const char *ReplayBuffer::get_name( ) {
    return name;
}

/* FIXME: these should be refactored into something cleaner */
ReplayFrameData *ReplayBuffer::read_frame(timecode_t frame, LoadFlags flags) {
    FrameHeader header; 
    off_t base_offset, video_offset, thumbnail_offset, audio_offset;
    ReplayFrameData *ret = new ReplayFrameData;
    ret->free_data_on_destroy( );
    ret->source = this;
    ret->pos = frame;

    /* determine frame offset */
    base_offset = index->get_frame_location(frame);

    /* read frame header */
    if (pread_all(fd, &header, sizeof(header), base_offset) <= 0) {
        throw POSIXError("pread_all");
    }

    if ((flags & LOAD_VIDEO) && header.video_size > 0) {
        video_offset = base_offset + sizeof(header);
        ret->video_size = header.video_size;
        ret->video_data = malloc(ret->video_size);
        if (pread_all(fd, ret->video_data, ret->video_size, 
                video_offset) <= 0) {
            throw POSIXError("pread_all");
        }
    }

    if ((flags & LOAD_THUMBNAIL) && header.thumbnail_size > 0) {
        thumbnail_offset = base_offset + sizeof(header) + header.video_size;
        ret->thumbnail_size = header.thumbnail_size;
        ret->thumbnail_data = malloc(ret->thumbnail_size);
        if (pread_all(fd, ret->thumbnail_data, ret->thumbnail_size,
                thumbnail_offset) <= 0) {
            throw POSIXError("pread_all");
        }
    }

    if ((flags & LOAD_AUDIO) && header.audio_size > 0) {
        audio_offset = base_offset + sizeof(header) 
                + header.video_size + header.thumbnail_size;
        ret->audio_size = header.audio_size;
        ret->audio_data = malloc(ret->audio_size);
        if (pread_all(fd, ret->audio_data, ret->audio_size,
                audio_offset) <= 0) {
            throw POSIXError("pread_all");
        }
    }

    return ret;
}

timecode_t ReplayBuffer::write_frame(const ReplayFrameData &data) {
    FrameHeader header;
    size_t total_size;

    if (data.video_size == 0) {
        throw std::runtime_error("Tried to write empty frame");
    }

    header.version = 1;
    header.video_size = data.video_size;
    header.thumbnail_size = data.thumbnail_size;
    header.audio_size = data.audio_size;
    total_size = data.video_size + data.thumbnail_size + data.audio_size
        + sizeof(header);

    if (write_all(fd, &header, sizeof(header)) <= 0) {
        throw POSIXError("failed to write frame header");
    }

    if (write_all(fd, data.video_data, data.video_size) <= 0) {
        throw POSIXError("failed to write video data");
    }

    if (data.thumbnail_size > 0) {
        if (write_all(fd, data.thumbnail_data, data.thumbnail_size) <= 0) {
            throw POSIXError("failed to write thumbnail data");
        }
    }

    if (data.audio_size > 0) {
        if (write_all(fd, data.audio_data, data.audio_size) <= 0) {
            throw POSIXError("failed to write audio data");
        }
    }

    return index->mark_frame(total_size);
}

