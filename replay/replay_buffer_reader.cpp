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

#include "replay_buffer.h"
#include "xmalloc.h"
#include "posix_util.h"
#include <sys/types.h>
#include <unistd.h>

ReplayBufferReader::ReplayBufferReader(ReplayBuffer *buf_, 
        ReplayBufferIndex *index_, int fd_) {
    buf = buf_;
    index = index_;
    fd = fd_;
    seek_to(0);
    timecode = 0;
}

ReplayBufferReader::~ReplayBufferReader( ) {
    close(fd);
}

ReplayFrameData *ReplayBufferReader::read_frame(timecode_t tc) {
    seek_to(index->get_frame_location(tc)); 
    timecode = tc;
    return read_next_frame( );
}

ReplayFrameData *ReplayBufferReader::read_next_frame( ) {
    ReplayBuffer::FrameHeader header; 
    ReplayFrameData *ret = new ReplayFrameData;
    ret->free_data_on_destroy( );

    const size_t BIGFRAME_SIZE = 2*1024*1024;
    
    /* read frame header */
    if (read_all(fd, &header, sizeof(header)) <= 0) {
        throw POSIXError("read_all");
    }

    /* check that header makes sense */
    if (header.version != 1) {
        fprintf(stderr, "bad header - expected version 1, got version %d\n", header.version);
        throw std::runtime_error("bad frame header\n");
    }

    if (header.video_size == 0) {
        throw std::runtime_error("frame with no video loaded from buffer");
    }

    if (header.video_size > BIGFRAME_SIZE) {
        fprintf(stderr, "Warning: a very large frame is being loaded\n");
    }

    ret->video_size = header.video_size;
    ret->video_data = xmalloc(ret->video_size, 
        "ReplayBufferReader", "video_data");
    if (read_all(fd, ret->video_data, ret->video_size) <= 0) {
        delete ret;
        throw POSIXError("read_all");
    }

    ret->thumbnail_size = header.thumbnail_size;
    if (ret->thumbnail_size > 0) {
        ret->thumbnail_data = xmalloc(ret->thumbnail_size,
            "ReplayBufferReader", "thumbnail_data");
        if (read_all(fd, ret->thumbnail_data, ret->thumbnail_size) <= 0) {
            delete ret;
            throw POSIXError("read_all");
        }
    }

    ret->audio_size = header.audio_size;
    if (ret->audio_size > 0) {
        ret->audio_data = xmalloc(ret->audio_size,
            "ReplayBufferReader", "audio_data");
        if (read_all(fd, ret->audio_data, ret->audio_size) <= 0) {
            delete ret;
            throw POSIXError("read_all");
        }
    }

    ret->source = buf;
    ret->pos = timecode;
    timecode++;

    return ret;
}

void ReplayBufferReader::seek_to(off_t where, int whence) {
    if (lseek(fd, where, whence) == (off_t) -1) {
        throw POSIXError("lseek");
    }
}
