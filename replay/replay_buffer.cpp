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

#include <linux/fs.h> /* for block device size ioctls */
#include <sys/ioctl.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdexcept>

//#define ENABLE_MSYNC

struct msync_req {
    void *base;
    size_t size;
};

class ReplayBuffer::MsyncBackground : public Thread {
    public:
        MsyncBackground( ) : request_queue(256) {
            start_thread( );        
        };

        ~MsyncBackground( ) {
        
        };

        Pipe<msync_req> request_queue;

    protected:
        void run_thread( ) {
            msync_req req;

            for (;;) {
                req = request_queue.get( );
                msync(req.base, req.size, MS_SYNC);
            }
        };
};

ReplayBuffer::ReplayBuffer(const char *path, size_t buffer_size, 
        size_t frame_size, const char *name) {
    int error;
    struct stat stat;
    
#ifdef ENABLE_MSYNC
    mst = new MsyncBackground( );
#else
    mst = NULL;
#endif

    _field_dominance = RawFrame::UNKNOWN;

    /* open and allocate (if necessary) buffer file */
    fd = open(path, O_CREAT | O_RDWR, 0644);
    if (fd < 0) {
        throw POSIXError("open");
    }

    if (fstat(fd, &stat) != 0) {
        throw POSIXError("fstat");
    }

    if (S_ISREG(stat.st_mode)) {
        error = posix_fallocate(fd, 0, buffer_size);
        if (error != 0) {
            throw POSIXError("posix_fallocate", error);
        }
    } else if (S_ISBLK(stat.st_mode)) {
        __u64 disk_size;
        ioctl(fd, BLKGETSIZE64, &disk_size);
        buffer_size = disk_size;
        fprintf(stderr, "using raw disk io (buffer size %zu)\n", buffer_size);
    } else {
        throw std::runtime_error("cannot use this thing as a buffer");
    }

    data = NULL;

    tc_current = 0;
    this->buffer_size = buffer_size;
    this->frame_size = frame_size;
    this->n_frames = buffer_size / frame_size;
    this->name = strdup(name);

    if (this->name == NULL) {
        throw std::runtime_error("allocation failure");
    }

    this->locks = new int[this->n_frames];
    memset(locks, 0, this->n_frames * sizeof(int));
}

ReplayBuffer::~ReplayBuffer( ) {
    if (close(fd) != 0) {
        throw POSIXError("close");
    }

    free(name);

    delete locks;
}

ReplayShot *ReplayBuffer::make_shot(timecode_t offset, whence_t whence) {
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
    unsigned int frame_index = tc_current % n_frames;

    frame_data.source = this;
    frame_data.pos = tc_current + 1;
    frame_data.data_size = frame_size;
    frame_data.data_ptr = mmap(
        NULL, frame_size, PROT_READ | PROT_WRITE, 
        MAP_SHARED | MAP_LOCKED, fd, frame_index * frame_size
    );

    if (frame_data.data_ptr == MAP_FAILED) {
        throw POSIXError("mmap failed in get_writable_frame");
    }
}

void ReplayBuffer::finish_frame_write(ReplayFrameData &rfd) {
#ifdef ENABLE_MSYNC
    unsigned int frame_index = tc_current % n_frames;

    unsigned int block_index = frame_index & ~0x3fU;
    unsigned int block_size = frame_size * 0x40;

    /* have our background thread msync() this stuff */
    if ((frame_index & 0x3fU) == 0x3fU) {
        void *data_ptr = data + block_index * frame_size;
        msync_req req;
        req.base = data_ptr;
        req.size = block_size;
        mst->request_queue.put(req);
    }
#endif
    
    if (munmap(rfd.data_ptr, rfd.data_size) != 0) {
        throw std::runtime_error("munmap failed in finish_frame_write");
    }
    
    tc_current++;
}

void ReplayBuffer::get_readable_frame(timecode_t tc, 
        ReplayFrameData &frame_data) {
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

    void *dp = mmap(
        NULL, frame_size, PROT_READ, MAP_SHARED | MAP_LOCKED, 
        fd, frame_index * frame_size
    );

    if (dp == MAP_FAILED) {
        throw POSIXError("mmap failed in get_readable_frame");
    }

    frame_data.data_ptr = dp;
    frame_data.data_size = frame_size;

    //try_readahead(tc, 128);
}

void ReplayBuffer::finish_frame_read(ReplayFrameData &frame_data) {
    if (munmap(frame_data.data_ptr, frame_data.data_size) != 0) {
        throw POSIXError("munmap failed in finish_frame_read");
    }
}

const char *ReplayBuffer::get_name( ) {
    return name;
}

void ReplayBuffer::lock_frame(timecode_t frame) {
#ifdef ENABLE_MLOCK
    bool flag;
    unsigned int frame_offset = frame % n_frames;

    if (frame_offset >= n_frames) {
        return; 
    }

    { MutexLock l(m);
        if (locks[frame_offset] == 0) {
            flag = true;
        } else {
            flag = false;
        }
        locks[frame_offset]++;
    }

    if (flag) {
        if (mlock(data + frame_offset * frame_size, frame_size) != 0) {
            perror("mlock");
        }
    }
#else
    (void) frame;
#endif
}

void ReplayBuffer::unlock_frame(timecode_t frame) {
#ifdef ENABLE_MLOCK
    bool flag;
    unsigned int frame_offset = frame % n_frames;

    if (frame_offset >= n_frames) {
        return; 
    }

    { MutexLock l(m);
        locks[frame_offset]--;
        if (locks[frame_offset] == 0) {
            flag = true;
        } else {
            flag = false;
        }
    }

    if (flag) {
        munlock(data + frame_offset * frame_size, frame_size);
    }
#else
    (void) frame;
#endif
}

ReplayBufferLocker::ReplayBufferLocker( ) {
    buf = NULL;
    start = end = 0;

    start_thread( );
}

ReplayBufferLocker::~ReplayBufferLocker( ) {
    
}

void ReplayBufferLocker::run_thread( ) {
    timecode_t current_start = 0, current_end = 0;
    ReplayBuffer *current_buf = NULL;
    
    timecode_t next_start = 0, next_end = 0;
    ReplayBuffer *next_buf = NULL;

    for (;;) {
        { MutexLock l(m);
            if (start == current_start && end == current_end 
                    && buf == current_buf) {
                c.wait(m);
                continue;
            } else {
                next_start = start;
                next_end = end;
                next_buf = buf;
            }
        }

        /* something has changed if we fall through here */
        if (next_buf != NULL) {
            if (next_buf != current_buf) {
                /* we changed buffers so we have to unlock everything */
                /* (but only if it's not NULL) */
                if (current_buf != NULL) {
                    unlock_all_frames(current_buf, current_start, current_end);
                }
                current_buf = next_buf;
                current_start = next_start;
                current_end = next_end;
                lock_all_frames(current_buf, current_start, current_end);
            } else {
                /* check for overlap and move the range if possible */
                move_range(current_buf, current_start, current_end, 
                        next_start, next_end);
                current_start = next_start;
                current_end = next_end;
            }
        }
    }

}

void ReplayBufferLocker::set_position(ReplayBuffer *buf, timecode_t tc) {
    MutexLock l(m);

    this->buf = buf;
    start = tc - 10;
    end = tc + 10;

    c.signal( );
}

void ReplayBufferLocker::lock_all_frames(ReplayBuffer *buf, timecode_t start, 
        timecode_t end) {
    for (timecode_t i = start; i < end; i++) {
        buf->lock_frame(i);
    }
}

void ReplayBufferLocker::unlock_all_frames(ReplayBuffer *buf, 
        timecode_t start, timecode_t end) {
    for (timecode_t i = start; i < end; i++) {
        buf->unlock_frame(i);
    }
}

void ReplayBufferLocker::move_range(ReplayBuffer *buf,
        timecode_t s1, timecode_t e1, 
        timecode_t s2, timecode_t e2) {
    if (s1 > e2 || s2 > e1) {
        /* no overlap */
        unlock_all_frames(buf, s1, e1);
        lock_all_frames(buf, s2, e2);
    } else {
        if (s1 < s2) {
            /* unlock all frames from s1 up to s2 */
            unlock_all_frames(buf, s1, s2);
        } else {
            /* lock all frames from s2 to s1 */
            lock_all_frames(buf, s2, s1);
        }

        if (e1 < e2) {
            /* lock all frames from e1 to e2 */
            lock_all_frames(buf, e1, e2);
        } else {
            /* unlock all frames from e2 to e1 */
            unlock_all_frames(buf, e2, e1);
        }

    }
}

void ReplayBuffer::try_readahead(timecode_t tc, unsigned int n) {
    for (unsigned int i = 0; i < n; i++) {
        try_readahead(tc + i);
        try_readahead(tc - i);
    }
}

void ReplayBuffer::try_readahead(timecode_t tc) {
    unsigned int frame_index = tc % n_frames;
//    int ret;
    madvise(data + frame_index * frame_size, frame_size, MADV_WILLNEED);
#if 0
    if (ret != 0) {
        perror("madvise");
    }
#endif
}
