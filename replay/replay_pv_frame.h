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

#ifndef _REPLAY_PV_FRAME_H
#define _REPLAY_PV_FRAME_H

#include "serialize.h"
#include <fftw3.h>

class ReplayPvFrame : public Serializable {
    public:
        ReplayPvFrame( );
        ReplayPvFrame(
            const ReplayPvFrame &before, 
            const ReplayPvFrame &after, 
            float interp
        );

        ~ReplayPvFrame( );
        void set_fft(float *data, size_t fft_size);
        bool is_polar( ) { return format == POLAR; }
        bool is_rect( ) { return format == RECTANGULAR; }
        
    protected:
        float *src_data;
        float *data;
        size_t size;

        void make_polar( );
        void make_rectangular( );

        enum { RECTANGULAR, POLAR } format;
};

#endif
