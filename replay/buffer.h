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

class Buffer {
    public:
        typedef size_t offset_t;
        typedef size_t timecode_t;

        Buffer(const char *filename, size_t block_size);
        Buffer(const char *filename, size_t size, size_t block_size);
        ~Buffer( );

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
        void *block_ptr(offset_t ofs);

        /* Obtain the offset of the next writable block. */
        offset_t write_offset(void);

        /* 
         * Call when done writing to the currently writable block.
         * This advances the internal pointers.
         */
        timecode_t write_done(void);

        /*
         * Return the offset of the block n blocks after block k.
         * Negative "n" values are allowed and represent blocks 
         * occurring before block k in time.
         */
        offset_t rel_offset(offset_t k, int n);

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
                    parent_ = rhs->parent_;
                    block_ = rhs->block_;

                    if (parent != NULL) {
                        parent_->block_lock(block_);
                    }
                }

                const LockHelper &operator=(const LockHelper &rhs) {
                    if (parent != NULL) {
                        parent->block_unlock(block_);
                    }

                    parent_ = rhs->parent_;
                    block_ = rhs->block_;

                    if (parent != NULL) {
                        parent->block_lock(block_);
                    }
                }

                ~LockHelper( ) {
                    if (parent != NULL) {
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


class BufferReader : public Thread {

};

#endif
