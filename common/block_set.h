/*
 * Copyright 2011, 2012, 2013 Exavideo LLC.
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

#ifndef _BLOCK_SET_H
#define _BLOCK_SET_H

#include <stddef.h>
#include <unistd.h>
#include <list>

#include "serialize.h"

class BlockSet {
    public:
        BlockSet( );
        ~BlockSet( );

        /* 
         * The add_block functions do not take ownership of the pointer.
         * Caller is expected to keep the object around during the life
         * of the BlockSet, and to free the object sometime thereafter.
         */
        template <class T>
        void add_block(const char *label, const T *data, size_t count) {
            add_block(label, (void *)data, count * sizeof(T));
        }

        void add_block(const char *label, void *data, size_t size);
        void add_block(const char *label, SerializeStream *sstr);

        /* serialize object and add as a block */
        template <class T>
        void add_object(const char *label, const T &object) {
            SerializeStream *sstr = new SerializeStream;
            *sstr << object;
            add_block(label, sstr);
        }

        /* 
         * load_alloc_block will allocate the necessary space with
         * new[]. Must be freed by the caller using delete[].
         */
        template <class T>
        T *load_alloc_block(const char *label, size_t &count) {
            BlockData &block = find_block(label);
            count = block.size / sizeof(T);
            T *data = new T[count];
            read_data(label, data, count * sizeof(T));
            return data;
        };

        /* 
         * unserialize object from block with given label
         * return object allocated by new T(dsstr) 
         * where dsstr is a DeserializeStream 
         */
        template <class T>
        T *load_alloc_object(const char *label) {
            size_t size;
            uint8_t *serdata = load_alloc_block<uint8_t>(label,size);
            DeserializeStream dsstr(serdata, size);
            return new T(dsstr);
        }

        void begin_read(int fd, off_t start);
        size_t write_all(int fd) const;

        off_t end_offset( );

    protected:
        struct BlockHeader {
            char label[8];
            size_t size;
        };

        struct BlockData {
            char label[8];
            size_t size;
            off_t offset;
            void *data;
            SerializeStream *sstr;
        };

        size_t read_data(const char *label, void *data, size_t size);
        BlockData &find_block(const char *label);
        bool have_block(const char *label);

        std::list<BlockData> blocks;
        int fd;
};


#endif
