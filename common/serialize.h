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

#ifndef _SERIALIZE_H
#define _SERIALIZE_H

#include "buffer.h"
#include <stdint.h>
#include <list>
#include <string>
#include <vector>
#include <typeinfo>
#include <type_traits>

class Serializable { };

class SerializeStream {
    public:
        SerializeStream( );
        ~SerializeStream( );

        /* Write bytes (by copy) */
        void write_bytes(const void *bytes, size_t size);
        /* Write bytes (by reference) */
        void write_bytes_byref(const void *bytes, size_t size);

        /* for convenience */
        template <class T>
        void write_array_byval(const T *array, size_t size) {
            write_bytes(array, size * sizeof(T));
        }

        template <class T>
        void write_array_byref(const T *array, size_t size) {
            write_bytes_byref(array, size * sizeof(T));
        }

        size_t writeout(int fd);
        size_t bytes( ) { return total_bytes; }

    protected:
        AppendableBuffer *current_buffer;
        std::list<Buffer *> buffers;
        size_t total_bytes;
};

class DeserializeStream {
    public:
        /* by default, takes ownership of and will delete [] data. */
        DeserializeStream(uint8_t *data, size_t size);
        ~DeserializeStream( );

        void read_bytes(void *bytes, size_t size);

        template <class T>
        void read_array(T *array, size_t size) {
            read_bytes(array, size * sizeof(T));
        }
    protected:
        uint8_t *data;
        uint8_t *ptr;
        size_t bytes_remaining;
};

SerializeStream &operator<< (SerializeStream &str, const std::string &obj);
DeserializeStream &operator>> (DeserializeStream &str, std::string &obj);

template <class T>
typename std::enable_if<std::is_fundamental<T>::value, SerializeStream &>::type
operator<<(SerializeStream &str, const T &obj) {
    str.write_bytes(&obj, sizeof(obj));
    return str;
}

template <class T>
typename std::enable_if<std::is_fundamental<T>::value, DeserializeStream &>::type
operator>>(DeserializeStream &str, T &obj) {
    str.read_bytes(&obj, sizeof(obj));
    return str;
}

template <class T>
typename std::enable_if<
    std::is_base_of<Serializable, T>::value, 
    SerializeStream &
>::type operator<<(SerializeStream &str, const T &obj) {
    obj.serialize(str);
    return str;
}

template <class T>
typename std::enable_if<
    std::is_base_of<Serializable, T>::value,
    DeserializeStream &
>::type operator>>(DeserializeStream &str, T &obj) {
    obj.deserialize(str);
    return str;
}

/* Now, fun with vectors! */
template <class U>
SerializeStream &operator<<(SerializeStream &str, const std::vector<U> &v) {
    str << v.size( );
    for (const U& u : v) {
        str << u;
    }

    return str;
}

template <class U>
DeserializeStream &operator>>(DeserializeStream &str, std::vector<U> &v) {
    size_t sz;
    str >> sz;
    v.clear( );

    while (sz > 0) {
        /* construct new vector elements by deserializing them in turn */
        if (std::is_trivial<U>::value) {
            /* deserialize trivial things using the >> operator */
            U new_u;
            str >> new_u;
            v.push_back(new_u);
        } else {
            /* use deserialize constructor on object (fails if not present) */
            v.emplace_back(str);
        }
        sz--;
    }

    return str;
}


#endif
