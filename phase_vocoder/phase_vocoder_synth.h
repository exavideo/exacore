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

#ifndef _PHASE_VOCODER_SYNTH_H
#define _PHASE_VOCODER_SYNTH_H

#include "phase_data_packet.h"
#include <stddef.h>

class PhaseVocoderSynth {
    public:
        PhaseVocoderSynth(size_t n_points, size_t n_channels);
        void set_phase_data(const PhaseDataPacket &phase_data);
        AudioPacket *render_samples(size_t n_samples);
        void render_samples(AudioPacket *pkt);

    protected:
        float *phase_data;
        float *phase_accumulator;
        float *realimag;

        float *float_samples;
        float *fft_result;
        size_t n_samples_ready;
        size_t n_samples;
        size_t n_points;
        size_t n_channels;

        void render_more_samples( );
};

#endif
