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

#ifndef _OPENREPLAY_BUFFER_H
#define _OPENREPLAY_BUFFER_H

#include "types.h"
#include <stdlib.h>
#include <list>

#include "mutex.h"
#include "condition.h"
#include "thread.h"

#include <stdexcept>

class Buffer {
    public:
        typedef size_t offset_t;
        typedef size_t timecode_t;

        Buffer(const char *filename, size_t block_size);
        Buffer(const char *filename, size_t n_blocks, size_t block_size);
        ~Buffer( );

        /*
         * IMPORTANT NOTE
         *
         * "Locking" blocks is not a means of obtaining exclusive access!
         * This "locking" refers to keeping the data in main memory,
         * and not allowing it to be swapped out to disk. That's important.
         */

        /*
         * Lock a block, forcing it to be kept available for fast access.
         */
        void block_lock(offset_t ofs);

        /*
         * Unlock a block, allowing it to be swapped out to disk.
         */
        void block_unlock(offset_t ofs);
        
        /* 
         * Obtain a readable and writable pointer to the block 
         * with the given offset. 
         */
        void *block_ptr(offset_t ofs) const;

        /* Obtain the offset of the next writable block. */
        offset_t write_offset(void) const;

        /* Timecode helpers */

        /* 
         * Call when done writing to the currently writable block.
         * This advances the internal pointers.
         */
        timecode_t write_done(void);

        /*
         * Return the offset of a block containing data for the given 
         * timecode. Throws an exception if the timecode is not present
         * in the buffer.
         */
        offset_t tc_offset(timecode_t tc);

        /*
         * Return the offset of the block most recently written.
         */
        offset_t live_offset(void);

        /*
         * Return the total size of the buffer, in blocks.
         */
        offset_t size(void) const {
            return n_blocks;
        }

        /* Things to make the modular arithmetic easier */

        /* 
         * Return true if block t lies between blocks l and r.
         */
        bool in_range(offset_t l, offset_t r, offset_t t) const;

        /*
         * Return the offset of the block n blocks after block k.
         * Negative "n" values are allowed and represent blocks 
         * occurring before block k in time.
         */
        offset_t rel_offset(offset_t k, int n) const;

        /*
         * Return the cardinality/size of the intersection of the intervals
         * [a1..a2], [b1..b2].
         */
        unsigned int intersect(offset_t a1, offset_t a2, 
                offset_t b1, offset_t b2) const;

        /*
         * Return the cardinality/size of the interval [a1..a2].
         */
        unsigned int span(offset_t a1, offset_t a2) const;

        /*
         * A simple RAII wrapper around locking and unlocking blocks.
         */
        class LockHelper {
            public:
                LockHelper( ) {
                    parent_ = NULL;
                }

                LockHelper(Buffer *parent, offset_t block) {
                    parent_ = parent;
                    block_ = block;
                    parent_->block_lock(block_);
                }

                LockHelper(const LockHelper &rhs) {
                    parent_ = rhs.parent_;
                    block_ = rhs.block_;

                    if (parent_ != NULL) {
                        parent_->block_lock(block_);
                    }
                }

                offset_t block(void) { 
                    return block_;
                }

                const LockHelper &operator=(const LockHelper &rhs) {
                    if (parent_ != NULL) {
                        parent_->block_unlock(block_);
                    }

                    parent_ = rhs.parent_;
                    block_ = rhs.block_;

                    if (parent_ != NULL) {
                        parent_->block_lock(block_);
                    }

                    return *this;
                }

                ~LockHelper( ) {
                    if (parent_ != NULL) {
                        parent_->block_unlock(block_);
                    }
                }

            protected:
                Buffer *parent_;
                offset_t block_;
        };

    protected:
        Mutex m;

        offset_t last_written;
        timecode_t last_written_tc;

        offset_t n_blocks;
        size_t block_size;

        uint8_t *data;
        uint32_t *lock_counts;

        /* initially true, once a frame is written this becomes permanently false */
        bool empty; 
};

class RegionLocker : public Thread {
    public:
        RegionLocker(Buffer *buf);
        ~RegionLocker( );

        void set_region(Buffer::offset_t pos, int before, int after);
        unsigned int n_locked( );
        unsigned int n_requested( );

    protected:
        Mutex m;
        Condition needs_update;
        
        /* These values represent what we want. */
        Buffer::offset_t first_locked;
        Buffer::offset_t last_locked;

        bool stop;
        bool update;

        unsigned int n_locked_;

        /* 
         * These values represent what we've got. 
         * This is accessed only by the worker, and should always contain
         * lock helpers for logically consecutive blocks.
         */
        std::list<Buffer::LockHelper> locks_held;

        void unlock_unneeded(void);
        void progress(void);

        void run_thread(void); /* override from Thread */

        Buffer *buffer_;
};

class BufferReader : public Thread {

};

/* An exception to throw when the timecode is unavailable. */
class TimecodeNotPresent : public std::runtime_error {
    public:
        TimecodeNotPresent( ) : std::runtime_error("Timecode not found") { }
};

#endif
