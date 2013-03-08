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

#include "audio_packet.h"

/* buffer for (16-bit, stereo) audio data */
class AudioFIFO {
    public:
        AudioFIFO( );
        ~AudioFIFO( );

        void fill_packet(AudioPacket *apkt);
        void add_samples(size_t n_samples, uint8_t *data);
        size_t samples( ) { return fill_level / sample_size; }
    protected:
        uint8_t *data;
        size_t size;
        size_t fill_level;
        size_t sample_size;

        void reallocate(size_t new_size);
};

#endif
