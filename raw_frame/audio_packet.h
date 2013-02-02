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

#ifndef _AUDIO_PACKET_H
#define _AUDIO_PACKET_H

#include "types.h"
#include <stdlib.h>
#include <assert.h>

class AudioPacket {
    public:
        /* create new AudioPacket */
        AudioPacket(unsigned int rate, unsigned int channels,
                size_t sample_size, size_t n_frames);

        /* reconstitute serialize()'d AudioPacket */
        AudioPacket(void *src, size_t size);

        virtual ~AudioPacket( );

        AudioPacket *copy( );

        /* save audio packet to a block of memory */
        void serialize(void *dest, size_t size);

        uint8_t *data( ) { return _data; }
        size_t size( ) { return _size; }
        size_t n_frames( ) { return _size / (_sample_size * _channels); }
        /* return pointer to tne n'th sample frame */
        uint8_t *sample(unsigned int n) { 
            assert(n < n_frames( ));
            return _data + (n * _sample_size * _channels);
        }

#ifdef RAWFRAME_POSIX_IO
        ssize_t read_from_fd(int fd);
        ssize_t write_to_fd(int fd);
#endif

    protected:
        struct sdata {
            size_t _size;
            unsigned int _rate;
            unsigned int _channels;
            size_t _sample_size;
            uint8_t data[0];
        };

        uint8_t *_data;
        size_t _size;
        unsigned int _rate;
        unsigned int _channels;
        size_t _sample_size;
        static size_t npackets;
};

#endif
