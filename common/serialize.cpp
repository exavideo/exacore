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

#include "serialize.h"
#include "posix_util.h"
#include <string.h>

SerializeStream::SerializeStream( ) {
    current_buffer = NULL;
    total_bytes = 0;
}

SerializeStream::~SerializeStream( ) {
    for (Buffer *&buf : buffers) {
        delete buf;
    }
}

void SerializeStream::write_bytes(const void *bytes, size_t size) {
    if (current_buffer == NULL) {
        current_buffer = new AppendableBuffer;
    }

    current_buffer->append(bytes, size);
    total_bytes += size;
}

void SerializeStream::write_bytes_byref(const void *bytes, size_t size) {
    if (current_buffer != NULL) {
        buffers.push_back(current_buffer);
        current_buffer = NULL;
    }

    UnownedBuffer *newbuf = new UnownedBuffer(bytes, size);
    buffers.push_back(newbuf);
    total_bytes += size;
}

size_t SerializeStream::writeout(int fd) {
    size_t total = 0;
    if (current_buffer) {
        buffers.push_back(current_buffer);
        current_buffer = NULL;
    }

    for (Buffer *&buf : buffers) {
        if (write_all(fd, buf->data( ), buf->size( )) <= 0) {
            throw POSIXError("write_all");
        }
        total += buf->size( );
    }

    return total;
}

DeserializeStream::DeserializeStream(uint8_t *data, size_t size) {
    this->data = data;
    this->ptr = data;
    this->bytes_remaining = size;
}

DeserializeStream::~DeserializeStream( ) {
    delete [] data;
}

void DeserializeStream::read_bytes(void *bytes, size_t size) {
    if (size > bytes_remaining) {
        throw std::runtime_error("Tried to deserialize too much");
    }

    memcpy(bytes, ptr, size);
    bytes_remaining -= size;
    ptr += size;
}

SerializeStream &operator<< (SerializeStream &str, const std::string &s) {
    const std::string::value_type *data = s.data( );
    str << s.size( );
    str.write_array_byval(data, s.size( ));
    return str;
}

DeserializeStream &operator>> (DeserializeStream &str, std::string &s) {
    std::string::value_type *data;
    size_t n_elements;
    str >> n_elements;
    data = new std::string::value_type[n_elements];
    str.read_array(data, n_elements);
    s.assign(data, n_elements);
    delete data;
    return str;
}

