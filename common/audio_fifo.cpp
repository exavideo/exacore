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

#include "audio_fifo.h"
#include <algorithm>
#include <stdexcept>
#include <string.h>

AudioFIFO::AudioFIFO( ) {
    const size_t initial_size = 65536;

    size = initial_size;
    data = new uint8_t[size];
    fill_level = 0;
    sample_size = 4; /* fixed at 16-bit stereo for now */
}

AudioFIFO::~AudioFIFO( ) {
    delete [] data;
}

void AudioFIFO::add_samples(size_t n_samples, uint8_t *idata) {
    size_t n_bytes = n_samples * sample_size;

    if (fill_level + n_bytes > size) {
        reallocate(std::max(2*size, fill_level + n_bytes));        
    }

    memcpy(data + fill_level, idata, n_bytes);
    fill_level += n_bytes;
}

void AudioFIFO::fill_packet(AudioPacket *apkt) {
    if (fill_level < apkt->size( )) {
        throw std::runtime_error("not enough data in AudioFIFO");
    }

    /* copy out first data from buffer */
    memcpy(apkt->data( ), data, apkt->size( ));
    /* move next data down */
    memmove(data, data + apkt->size( ), fill_level - apkt->size( ));
    fill_level -= apkt->size( );
}

void AudioFIFO::reallocate(size_t new_size) {
    uint8_t *new_data;
    if (new_size > size) {
        new_data = new uint8_t[new_size];
        memcpy(new_data, data, fill_level);
        delete [] data;
        data = new_data;
        size = new_size;
    }
}
