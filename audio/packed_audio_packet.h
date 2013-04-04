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

#ifndef _PACKED_AUDIO_PACKET_H
#define _PACKED_AUDIO_PACKET_H

#include <stddef.h>
#include "serialize.h"

template <class T>
class PlanarAudioPacket;

template <class T>
class PackedAudioPacket : public Serializable {
    public:
        PackedAudioPacket(size_t n_samples, size_t n_channels);
        PackedAudioPacket(void *data, size_t n_bytes);
        PackedAudioPacket(DeserializeStream &str);
        ~PackedAudioPacket( );

        template <class U> PackedAudioPacket<U> *copy( ) const;
        template <class U> PlanarAudioPacket<U> *make_planar( ) const;
        PackedAudioPacket<T> *clone( ) { return copy<T>( ); }
        PackedAudioPacket<T> *change_channels(size_t n_out_channels);
        void zero( );

        T *data( ) { return _data; }
        const T *data( ) const { return _data; }

        size_t channels( ) const { return _channels; }
        size_t size_samples( ) const { return _samples; }
        size_t size_words( ) const { return _samples * _channels; }
        size_t size_bytes( ) const { return _samples * _channels * sizeof(T); }
        ssize_t read_from_fd(int fd);
        ssize_t write_to_fd(int fd);

        void serialize(SerializeStream &str) const;
        void deserialize(DeserializeStream &str);

    private:
        size_t _samples;
        size_t _channels;
        T *_data;
};

#include "packed_audio_packet.cpp"

typedef PackedAudioPacket<int16_t> IOAudioPacket;

#endif
