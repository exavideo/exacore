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

#ifndef _PHASE_DATA_PACKET_H
#define _PHASE_DATA_PACKET_H

#include "audio_packet.h"

class PhaseDataPacket {
    public:
        PhaseDataPacket(AudioPacket *apkt, size_t n_points_);
        ~PhaseDataPacket( );
        const float *phase_data( ) const { return fft2_results; } /* see code for why */
        size_t points( ) const { return n_points; }
        size_t channels( ) const { return n_channels; }
    protected:
        size_t n_points, n_channels, n_samples;
        /* 
         * each of these arrays is n_points * n_channels in length,
         * and contains a set of data points for each channel, i.e.
         * l0 l1 l2 l3 ... ln r0 r1 r2 r3 ... rn
         */
        float *real_samples;
        float *fft1_results;
        float *fft2_results;

        void fill_sample_array(AudioPacket *apkt);
        void process_data( );
        void do_ffts( );
};

#endif
