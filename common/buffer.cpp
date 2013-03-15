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

#include "buffer.h"
#include <string.h>
#include <stdlib.h>
#include <algorithm>
#include <stdexcept>

Buffer::Buffer( ) {
    _data = NULL;
    _size = 0;
}

Buffer::~Buffer( ) {

}

const void *Buffer::data( ) const {
    return _data;
}

size_t Buffer::size( ) const {
    return _size;
}

OwnedBuffer::OwnedBuffer(size_t sz) {
    nc_data = malloc(sz);
    _data = nc_data;
    _size = sz;
}

OwnedBuffer::~OwnedBuffer( ) {
    free(nc_data);
}

UnownedBuffer::UnownedBuffer(const void *d, size_t sz) {
    _data = d;
    _size = sz;
}

UnownedBuffer::~UnownedBuffer( ) {

}

AppendableBuffer::AppendableBuffer( ) : OwnedBuffer(64) {
    realsize = _size;
    _size = 0;
}

void AppendableBuffer::append(const void *d, size_t sz) {
    if (_size + sz > realsize) {
        realsize = std::max(2*realsize, sz + _size);
        nc_data = realloc(nc_data, realsize);
        if (nc_data == NULL) {
            throw std::runtime_error("Reallocation failure");
        }
    }

    memcpy(((uint8_t *) nc_data) + _size, d, sz);
    _size += sz;
}
