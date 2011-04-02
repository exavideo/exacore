/*
 * Copyright 2011 Andrew H. Armenia.
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

#include <sys/mman.h>
#include <stdio.h>
#include <stdexcept>

void *Buffer::block_ptr(Buffer::offset_t ofs) {
    assert(ofs < n_blocks);

    return (void *)(data + block_size * ofs);
}

void Buffer::block_lock(Buffer::offset_t ofs) {
    uint32_t old_lock_count;

    assert(ofs < n_blocks);

    old_lock_count = __sync_fetch_and_add(&lock_counts[ofs], 1);

    if (old_lock_count == 0) {
        if (mlock(block_ptr(ofs), block_size) != 0) {
            perror("mlock");
            throw std::runtime_error("mlock( ) failed");
        }
    }
}

void Buffer::block_unlock(offset_t ofs) {
    uint32_t new_lock_count;

    assert(ofs < n_blocks);
    
    new_lock_count = __sync_sub_and_fetch(&lock_counts[ofs], 1);
    
    if (new_lock_count == 0) {
        if (munlock(block_ptr(ofs), block_size) != 0) {
            perror("munlock");
            throw std::runtime_error("munlock( ) failed");
        }
    }
}

Buffer::offset_t Buffer::write_offset(void) {
    if (empty) {
        return 0;
    } else {
        return last_written + 1;
    }
}

Buffer::timecode_t Buffer::write_done(void) {
    timecode_t ret;

    { MutexLock l(m);
        if (empty) {
            last_written = 0;
            last_written_tc = 0;
        } else {
            last_written = rel_offset(last_written, 1);
            last_written_tc = last_written_tc + 1;
        }

        ret = last_written_tc;
    }

    return ret;
}

Buffer::offset_t Buffer::rel_offset(Buffer::offset_t k, int n) {
    offset_t k;
    offset_t nofs;

    /* 
     * FIXME this is not my best work, and slightly byzantine. 
     * But it is an exact specification of how relative offset computation
     * should go down.
     */
    if (n > 0) {
        nofs = n;
        k += nofs;
        while (k >= n_blocks) {
            k -= n_blocks;
        }
    } else {
        nofs = -n;
        while (nofs >= n_blocks) {
            nofs -= n_blocks;
        }
        
        if (nofs < k) {
            k -= nofs;
        } else {
            k = n_blocks - (nofs - k);
        }
    }

    return k;
}

Buffer::offset_t tc_offset(Buffer::timecode_t tc) {
    { MutexLock l(m);
        if (tc > last_written_tc) {
            throw TimecodeNotPresent( );
        }

        if (last_written_tc - tc >= n_blocks - 1) {
            throw TimecodeNotPresent( );
        }

        return rel_offset(last_written, -(last_written_tc - tc));
    }
}

Buffer::offset_t live_offset(Buffer::timecode_t tc) {
    { MutexLock l(m);
        return last_written;
    }
}
