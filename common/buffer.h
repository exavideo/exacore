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

#ifndef _BUFFER_H
#define _BUFFER_H

#include <stddef.h>

class Buffer {
    public:
        virtual ~Buffer( );
        const void *data( ) const;
        size_t size( ) const;
    protected:
        Buffer( );

        const void *_data;
        size_t _size;
};

class OwnedBuffer : public Buffer {
    public:
        OwnedBuffer(size_t sz);
        ~OwnedBuffer( );
    protected:
        void *nc_data;
};

class AppendableBuffer : public OwnedBuffer {
    public:
        AppendableBuffer( );
        void append(const void *data, size_t sz);
    protected:
        size_t realsize;
        
};

class UnownedBuffer : public Buffer {
    public:
        UnownedBuffer(const void *d, size_t sz);
        ~UnownedBuffer( );
};

#endif
