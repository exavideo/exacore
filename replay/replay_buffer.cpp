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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdexcept>

ReplayBuffer::ReplayBuffer(const char *path, size_t buffer_size, 
        size_t frame_size, const char *name) {
    int error;

    /* open and allocate buffer file */
    fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0644);
    if (fd < 0) {
        throw POSIXError("open");
    }

    error = posix_fallocate(fd, 0, buffer_size);
    if (error != 0) {
        throw POSIXError("posix_fallocate", error);
    }

    /* memory map entire buffer file */
    void *mp;
    mp = mmap(NULL, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mp == MAP_FAILED) {
        throw POSIXError("mmap");
    }

    data = (uint8_t *) mp;
    tc_current = 0;
    this->buffer_size = buffer_size;
    this->frame_size = frame_size;
    this->n_frames = buffer_size / frame_size;
    this->name = strdup(name);
    if (this->name == NULL) {
        throw std::runtime_error("allocation failure");
    }
}

ReplayBuffer::~ReplayBuffer( ) {
    if (munmap(data, buffer_size) != 0) {
        throw POSIXError("munmap");
    }

    if (close(fd) != 0) {
        throw POSIXError("close");
    }

    free(name);
}

ReplayShot *ReplayBuffer::make_shot(timecode_t offset, whence_t whence) {
    MutexLock l(m);
    ReplayShot *shot = new ReplayShot;
    timecode_t start_offset;

    shot->source = this;
    switch (whence) {
        case ZERO:
            shot->start = offset;
            break;
        case START:
            if (tc_current < n_frames) {
                start_offset = 0;
            } else {
                /* may break if optimizer is really dumb?? */
                start_offset = (tc_current / n_frames) * n_frames;
            }

            shot->start = start_offset + offset;
            break;
        case END:
            shot->start = tc_current - 1 + offset;
            break;
    }
    shot->length = 0;

    return shot;
}

void ReplayBuffer::get_writable_frame(ReplayFrameData &frame_data) {
    MutexLock l(m);
    unsigned int frame_index = tc_current % n_frames;

    frame_data.source = this;
    frame_data.pos = tc_current + 1;
    frame_data.data_ptr = data + frame_index * frame_size;
    frame_data.data_size = frame_size;
}

void ReplayBuffer::finish_frame_write( ) {
    MutexLock l(m);
    tc_current++;
}

void ReplayBuffer::get_readable_frame(timecode_t tc, 
        ReplayFrameData &frame_data) {
    MutexLock l(m);

    if (tc >= tc_current) {
        fprintf(stderr, "past the end: tc=%d tc_current=%d\n",
                (int) tc, (int) tc_current);
        throw ReplayFrameNotFoundException( );
    } else if (tc < tc_current - n_frames || tc < 0) { 
        fprintf(stderr, "past the beginning: tc=%d tc_current=%d\n", 
                (int) tc, (int) tc_current);
        throw ReplayFrameNotFoundException( );
    }

    unsigned int frame_index = tc % n_frames;
    
    frame_data.source = this;
    frame_data.pos = tc;
    frame_data.data_ptr = data + frame_index * frame_size;
    frame_data.data_size = frame_size;
}

const char *ReplayBuffer::get_name( ) {
    return name;
}
