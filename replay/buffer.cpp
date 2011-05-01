/*
 * Copyright 2011 Andrew H. Armenia.
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

#include "buffer.h"

#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdexcept>
#include <assert.h>

Buffer::Buffer(const char *filename, size_t block_size) {
    UNUSED(filename);
    UNUSED(block_size);

    throw std::runtime_error("stub"); /* FIXME */
}

Buffer::Buffer(const char *filename, size_t n_blocks, size_t block_size) {
    this->n_blocks = n_blocks;
    this->block_size = block_size;
    /* FIXME this is still stubbed */
    UNUSED(filename);
}

Buffer::~Buffer( ) {
    /* FIXME once mmap stuff is working */
}

void *Buffer::block_ptr(Buffer::offset_t ofs) const {
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

        /* FIXME? is this really necessary or a good idea? */
        if (madvise(block_ptr(ofs), block_size, MADV_DONTNEED) != 0) {
            perror("madvise");
            throw std::runtime_error("madvise( ) failed");
        }
    }
}

Buffer::offset_t Buffer::write_offset(void) const {
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

Buffer::offset_t Buffer::rel_offset(Buffer::offset_t k, int n) const {
    offset_t nofs;

    /* 
     * FIXME this is not my best work, and slightly byzantine. 
     * But it is an exact specification of how relative offset computation
     * should go down.
     */
    if (n >= 0) {
        nofs = n;
    
        k += nofs;
        k = k % n_blocks;
    } else {
        nofs = -n;
        nofs = nofs % n_blocks; 
        
        if (nofs <= k) {
            k -= nofs;
        } else {
            k = n_blocks - (nofs - k);
        }
    }

    return k;
}

bool Buffer::in_range(Buffer::offset_t l, Buffer::offset_t r, 
        Buffer::offset_t t) const {

    assert(l < n_blocks);
    assert(r < n_blocks);
    assert(t < n_blocks);

    /* 
     * note: in these diagrams: x represents a block that is in the
     * range of interest but is not pointed at by x, r, or t.
     */

    if (l <= r) {
        /* lxxxxxr */
        if (l <= t && t <= r) {
            /* lxxxtxxr */
            return true;
        } else {
            /* t  lxxxxxr; lxxxxxr t */
            return false;
        }
    } else {
        /* xxr     lxx */
        if (t >= l || t <= r) {
            /* txr     lxx; xxr     ltx */
            return true;
        } else {
            /* xxr t lxx */
            return false;
        }
    }

}

unsigned int Buffer::intersect(Buffer::offset_t a1, Buffer::offset_t a2,
        Buffer::offset_t b1, Buffer::offset_t b2) const {
    
    assert(a1 < n_blocks);
    assert(a2 < n_blocks);
    assert(b1 < n_blocks);
    assert(b2 < n_blocks);

    if (in_range(a1, a2, b1) && in_range(a1, a2, b2)) {
        if (in_range(a1, b2, b1)) {
            return span(b1, b2);
        } else {
            return span(a1, b2) + span(b1, a2);
        }
    } else if (in_range(b1, b2, a1) && in_range(b1, b2, a2)) {
        if (in_range(b1, a2, a1)) {
            return span(a1, a2);
        } else {
            return span(b1, a2) + span(a1, b2);
        }
    } else if (in_range(a1, a2, b1)) {
        return span(b1, a2);
    } else if (in_range(a1, a2, b2)) {
        return span(a1, b2);
    } else {
        return 0;
    }

}

unsigned int Buffer::span(Buffer::offset_t a1, Buffer::offset_t a2) const {
    assert(a1 < n_blocks);
    assert(a2 < n_blocks);

    if (a1 <= a2) {
        /* straightforward */
        return a2 - a1 + 1;
    } else {
        /* 
         * e.g. n_blocks = 8:
         * xx    xx
         *  ^a1  ^a2
         *    1    6
         * span = 4: 8 - 6 = 2, 1 + 1 = 2
         */
        return a2 + (n_blocks - a1) + 1;
    }
}

Buffer::offset_t Buffer::tc_offset(Buffer::timecode_t tc) {
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

Buffer::offset_t Buffer::live_offset( ) {
    { MutexLock l(m);
        return last_written;
    }
}



void RegionLocker::set_region(Buffer::offset_t pos, int before, int after) {
    { MutexLock l(m);
        first_locked = buffer_->rel_offset(pos, -before);
        last_locked = buffer_->rel_offset(pos, after);
    }

    needs_update.signal( );
}

void RegionLocker::run_thread(void) {
    /* 
     * lr = "left side of range", rr = "right side of range" 
     * This is the range we currently want locked.
     */
    Buffer::offset_t lr = 0, rr = 0, cr;

    /* 
     * lf = "left fencepost", rf = "right fencepost"
     * These represent what is already locked.
     */
    Buffer::offset_t lf, rf;

    while (!stop) {
        /* update fenceposts */
        lf = locks_held.front( ).block( );
        rf = locks_held.back( ).block( );

        { MutexLock l(m);
            lr = first_locked;
            rr = last_locked;

            /* check if we need to update */
            if (!locks_held.empty( )) {
                if (lr == lf && rr == rf) {
                    /* block if we don't */
                    needs_update.wait(m);
                }
            }
        }

        /* Set left fencepost correctly */
        if (locks_held.empty( )) {
            /* start from zero and lock everything */
            cr = lr;
            for (;;) {
                locks_held.push_back( Buffer::LockHelper(buffer_, cr) );
                progress( );
                cr = buffer_->rel_offset(cr, 1);
                if (cr == rr) {
                    break;
                }
            }
        } else {
            lf = locks_held.front( ).block( );
            if (buffer_->in_range(lr, rr, lf)) {
                /* 
                 * move left fencepost left by locking more frames.
                 * Keep doing this until it corresponds with left side
                 * of the desired range.
                 */
                while (lf != lr) {
                    lf = buffer_->rel_offset(lf, 1);
                    locks_held.push_front( Buffer::LockHelper(buffer_, lf) );
                    progress( );
                }
            } else {
                /* move left fencepost right until it coincides with lr */
                while (lf != lr && !locks_held.empty( )) {
                    locks_held.pop_front( );
                    lf = locks_held.front( ).block( );
                    progress( );
                }
            }
        }

        /* Now the right fencepost */
        if (locks_held.empty( )) {
            cr = lr;
            for (;;) {
                locks_held.push_back( Buffer::LockHelper(buffer_, cr) );
                progress( );
                cr = buffer_->rel_offset(cr, 1);
                if (cr == rr) {
                    break;
                }
            }
        } else {
            rf = locks_held.back( ).block( );
            if (!buffer_->in_range(lr, rr, rf)) {
                /* 
                 * Right fencepost is outside our intended range.
                 * Move left until it is.
                 */
                while (rf != rr) {
                    /* 
                     * It should now be impossible for this operation to 
                     * empty the list. In the case where it would,
                     * it would have already been emptied above and
                     * this code should not be reached.
                     */
                    assert(!locks_held.empty( ));
                    locks_held.pop_back( );
                    rf = locks_held.back( ).block( );
                    progress( );
                }
            } else {
                /* Right fencepost is inside the range. Move it right. */
                while (rf != rr) {
                    rf = buffer_->rel_offset(rf, 1);
                    locks_held.push_back( Buffer::LockHelper(buffer_, rf) );
                    progress( );
                }
            }
        }
    }
}

void RegionLocker::progress( ) {
    Buffer::offset_t lf, rf, lr, rr;
    if (locks_held.empty( )) {
        MutexLock l(m);
        n_locked_ = 0;
    } else {
        lf = locks_held.front( ).block( );
        rf = locks_held.front( ).block( );
        { MutexLock l(m); 
            lr = first_locked;
            rr = last_locked;
            /* compute overlap between ranges */
            n_locked_ = buffer_->intersect(lf, rf, lr, rr);
        }
    }
}

unsigned int RegionLocker::n_locked( ) {
    { MutexLock l(m);
        return n_locked_;
    }
}

unsigned int RegionLocker::n_requested( ) {
    { MutexLock l(m);
        return buffer_->span(first_locked, last_locked);
    }
}
