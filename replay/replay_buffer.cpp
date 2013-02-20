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
    fd = open(path, O_CREAT | O_WRONLY, 0644);
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
    writer = NULL;
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

ReplayBufferWriter *ReplayBuffer::make_writer( ) {
    if (writer != NULL) {
        throw std::runtime_error("Someone is already writing to this ReplayBuffer!");
    }

    writer = new ReplayBufferWriter(this, index, fd);
    return writer;
}

void ReplayBuffer::release_writer(ReplayBufferWriter *wr) {
    if (wr == writer) {
        writer = NULL;
    } else {
        fprintf(stderr, "release_writer called with wrong writer\n"
            "this should never happen, check your code\n");
        throw std::runtime_error("tried to release_writer the wrong writer?");
    }
}

ReplayBufferReader *ReplayBuffer::make_reader( ) {
    int newfd = open(path, O_RDONLY);

    if (newfd == -1) {
        throw POSIXError("could not open buffer for reading");
    }

    return new ReplayBufferReader(this, index, newfd);
}
