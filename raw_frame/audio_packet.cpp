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

    npackets++;
    if (npackets > 1000) {
        fprintf(stderr,
            "More than 1000 AudioPackets are outstanding!\n"
            "This is a sign of a memory leak.\n"
            "Please check your code!\n"
        );
    }

    if (_data == NULL) {
        throw std::runtime_error("AudioPacket: failed to allocate data");
    }
}

AudioPacket::~AudioPacket( ) {
    npackets--;
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

#ifdef RAWFRAME_POSIX_IO

#include "posix_util.h"

ssize_t AudioPacket::read_from_fd(int fd) {
    return read_all(fd, _data, _size);
}

ssize_t AudioPacket::write_to_fd(int fd) {
    return write_all(fd, _data, _size);
}

#endif
