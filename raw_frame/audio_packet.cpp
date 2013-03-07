/*
 * Copyright 2011 Andrew H. Armenia.
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

#include "audio_packet.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdexcept>

size_t AudioPacket::npackets = 0;

AudioPacket::AudioPacket(unsigned int rate, unsigned int channels,
        size_t sample_size, size_t n_frames) {
    
    _rate = rate;
    _channels = channels;
    _sample_size = sample_size;

    _size = _channels * _sample_size * n_frames;

    _data = (uint8_t *)malloc(_size);
    memset(_data, 0, _size);

    leak_detect( );

    if (_data == NULL) {
        throw std::runtime_error("AudioPacket: failed to allocate data");
    }
}

void AudioPacket::leak_detect( ) {
    //npackets++;
    size_t current_npackets = __sync_add_and_fetch(&npackets, 1);

    if (current_npackets > 1000) {
        fprintf(stderr,
            "More than 1000 (%zd) AudioPackets are outstanding!\n"
            "This is a sign of a memory leak.\n"
            "Please check your code!\n",
            npackets
        );
    }
}

AudioPacket::AudioPacket(void *src, size_t size) {
    struct sdata *sd = (struct sdata *)src;
    _rate = sd->_rate;
    _channels = sd->_channels;
    _size = sd->_size;
    _sample_size = sd->_sample_size;
    _data = (uint8_t *)malloc(_size);

    leak_detect( );

    if (size < sizeof(struct sdata) + _size) {
        throw std::runtime_error("Tried to deserialize invalid AudioPacket");
    }

    memcpy(_data, sd->_data, _size);
}

AudioPacket::~AudioPacket( ) {
    //npackets--;
    __sync_sub_and_fetch(&npackets, 1);

    if (_data != NULL) {
        free(_data);
    }
}

AudioPacket *AudioPacket::copy( ) {
    AudioPacket *ret = new AudioPacket(_rate, _channels, 
            _sample_size, n_frames( ));

    memcpy(ret->_data, _data, _size);
    return ret;
}

void AudioPacket::serialize(void *dest, size_t size) {
    if (size < sizeof(struct sdata) + _size) {
        throw std::runtime_error("cannot serialize audio, not enough space");
    }

    struct sdata *sd = (struct sdata *)dest;
    sd->_size = _size;
    sd->_rate = _rate;
    sd->_channels = _channels;
    sd->_sample_size = _sample_size;
    memcpy(sd->_data, _data, _size);
}

#ifdef RAWFRAME_POSIX_IO

#include "posix_util.h"

ssize_t AudioPacket::read_from_fd(int fd) {
    return read_all(fd, _data, _size);
}

ssize_t AudioPacket::write_to_fd(int fd) {
    return write_all(fd, _data, _size);
}

#endif
