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

#ifndef _AUDIO_FIFO_H
#define _AUDIO_FIFO_H

#include "packed_audio_packet.h"
#include <stddef.h>

/* buffer for audio data */
template <class T>
class AudioFIFO {
    public:
        AudioFIFO(size_t n_channels = 1);
        ~AudioFIFO( );
    
        /* 
         * n is number of samples per channel here 
         * e.g. if n = 2000 and n_channels = 2 then 4000 
         * samples will be read from idata
         */
        template <class U> void add_packed_samples(const U *idata, size_t n);

        template <class U> void add_packet(const PackedAudioPacket<U> *apkt);
        template <class U> void add_packet(const PlanarAudioPacket<U> *apkt);
        template <class U> void fill_packet(PackedAudioPacket<U> *apkt);
        template <class U> void fill_packet(PlanarAudioPacket<U> *apkt);
        template <class U> void peek_packet(PackedAudioPacket<U> *apkt) const;
        template <class U> void peek_packet(PlanarAudioPacket<U> *apkt) const;
        size_t samples( ) const { return fill_level; }
        void pop(size_t n_samples);
        const T *data( ) const { return _data; }
        /* decklink's ScheduleAudioSamples() takes non-const void* */
        T *data( ) { return _data; }
    protected:
        T *_data;
        size_t size;
        size_t fill_level;
        size_t n_channels;

        void reallocate(size_t new_size);
        void copy_data(T *dst, const T *src, size_t n);
};

#include "audio_fifo.cpp"

#endif
