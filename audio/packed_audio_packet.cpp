/*
 * Copyright 2013 Exavideo LLC.
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

#include "packed_audio_packet.h"
#include "planar_audio_packet.h"
#include "numarray.h"
#include "posix_util.h"
#include <typeinfo>

template <class T>
PackedAudioPacket<T>::PackedAudioPacket(size_t n_samples, size_t n_channels) {
    _samples = n_samples;
    _channels = n_channels;
    _data = new T[_samples * _channels];
}

template <class T>
PackedAudioPacket<T>::PackedAudioPacket(DeserializeStream &str) {
    _data = NULL;
    deserialize(str);
}

template <class T>
void PackedAudioPacket<T>::deserialize(DeserializeStream &str) {
    std::string theirname;
    delete [] _data;

    /* first check that we are deserializing our own data */
    str >> theirname;    
    if (theirname != typeid(*this).name()) {
        throw std::runtime_error("deserialization of invalid data");
    }

    /* now deserialize it */
    str >> _samples;
    str >> _channels;
    _data = new T[_samples * _channels];
    str.read_array(_data, _samples * _channels);
}

template <class T>
PackedAudioPacket<T>::~PackedAudioPacket( ) {
    delete [] _data;
}


template <class T> template <class U>
PackedAudioPacket<U> *PackedAudioPacket<T>::copy( ) const {
    PackedAudioPacket *ret = new PackedAudioPacket<U>(_samples, _channels);
    numarray_copy(ret->data( ), _data, _samples * _channels);
    return ret;
}

template <class T> template <class U>
PlanarAudioPacket<U> *PackedAudioPacket<T>::make_planar( ) const {
    PlanarAudioPacket<U> *ret = new PlanarAudioPacket<U>(_samples, _channels);
    size_t i, j;

    /* packed to planar audio conversion */
    for (i = 0; i < _samples; i++) {
        for (j = 0; j < _channels; j++) {
            ret->data( )[j*_samples+i] = (U) _data[i*_channels+j];
        }
    }
}

template <class T>
void PackedAudioPacket<T>::zero( ) {
    numarray_set(_data, 0, _samples * _channels);
}

template <class T>
ssize_t PackedAudioPacket<T>::write_to_fd(int fd) {
    return write_all(fd, _data, size_bytes( ));
}

template <class T>
ssize_t PackedAudioPacket<T>::read_from_fd(int fd) {
    return read_all(fd, _data, size_bytes( ));
}

template <class T>
void PackedAudioPacket<T>::serialize(SerializeStream &str) const {
    str << std::string(typeid(*this).name());
    str << _samples;
    str << _channels;
    str.write_array_byref(_data, _samples * _channels);
}
