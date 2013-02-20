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
#include "posix_util.h"

ReplayBufferWriter::ReplayBufferWriter(ReplayBuffer *buf_,
        ReplayBufferIndex *index_, int fd_) {
    fd = fd_;
    buf = buf_;
    index = index_;
};

ReplayBufferWriter::~ReplayBufferWriter( ) {
    buf->release_writer(this);
    /* we do not close our fd here as it will be reused by future writers */
}

timecode_t ReplayBufferWriter::write_frame(const ReplayFrameData &data) {
    ReplayBuffer::FrameHeader header;
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
