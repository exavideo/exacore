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

#include "packed_audio_packet.h"
#include "planar_audio_packet.h"
#include "numarray_copy.h"

template <class T>
PlanarAudioPacket<T>::PlanarAudioPacket(size_t n_samples, size_t n_channels) {
    _channels = n_channels;
    _samples = n_samples;
    _data = new T[_channels * _samples];
}

template <class T>
PlanarAudioPacket<T>::~PlanarAudioPacket( ) {
    delete _data;
}

template <class T> template <class U>
PlanarAudioPacket<U> *PlanarAudioPacket<T>::copy( ) const {
    PlanarAudioPacket<U> *ret = new PlanarAudioPacket<U>(_samples, _channels);
    numarray_copy(ret->_data, _data, _samples * _channels);
}

template <class T> template <class U>
PackedAudioPacket<U> *PlanarAudioPacket<T>::make_packed( ) const {
    PackedAudioPacket<U> *ret = new PackedAudioPacket<U>(_samples, _channels);
    size_t i, j;

    /* convert audio from planar to packed format */
    for (i = 0; i < _samples; i++) {
        for (j = 0; j < _channels; j++) {
            ret->data( )[i*_channels+j] = _data[j*_samples+i];
        }
    }
}
