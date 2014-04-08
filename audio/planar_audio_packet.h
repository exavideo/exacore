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

#ifndef _PLANAR_AUDIO_PACKET_H
#define _PLANAR_AUDIO_PACKET_H

#include <stdexcept>

template <class T>
class PackedAudioPacket;

template <class T>
class PlanarAudioPacket {
    public:
        PlanarAudioPacket(size_t n_samples, size_t n_channels);
        ~PlanarAudioPacket( );

        template <class U> PlanarAudioPacket<U> *copy( ) const;
        template <class U> PackedAudioPacket<U> *make_packed( ) const;

        T *data( ) { return _data; }
        const T *data( ) const { return _data; }

        /* return start of plane for channel i */
        T *channel(size_t i) { 
            if (i >= _channels) {
                throw std::runtime_error("that channel does not exist");
            }

            return _data + _samples * i;
        }

        size_t channels( ) { return _channels; }
        size_t size_samples( ) { return _samples; }
        size_t size_words( ) { return _samples * _channels; }
        size_t size_bytes( ) { return _samples * _channels * sizeof(T); }

    protected:
        size_t _samples;
        size_t _channels;
        T *_data;
};

#include "planar_audio_packet.cpp"

#endif
