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

#include "phase_vocoder_synth.h"
#include <fftw3.h>
#include <math.h>
#include <stdexcept>
#include <string.h>

PhaseVocoderSynth::PhaseVocoderSynth(size_t n_points_, size_t n_channels_) {
    n_channels = n_channels_;
    n_points = n_points_;
    n_samples_ready = 0;
    n_samples = 3200;

    float_samples = new float[n_samples * n_channels];
    phase_data = new float[n_points * n_channels];
    phase_accumulator = new float[n_samples * n_channels];
    realimag = new float[n_points];
    fft_result = new float[n_points];
}

static void p2c(float *src, float *dest, size_t n_points) {
    float mag, angle, real, imag;

    for (size_t i = 1; i < n_points/2; i++) {
        mag = src[i];
        angle = src[n_points - i];
        real = mag * cosf(angle);
        imag = mag * sinf(angle);
        dest[i] = real;
        dest[n_points - i] = imag;
    }
}

static void accumulate(float *src, float *dest, size_t n_points) {
    const float pi = 4*atan(1);

    /* copy magnitudes */
    for (size_t i = 0; i <= n_points/2; i++) {
        dest[i] = src[i];
    }

    /* accumulate phase */
    for (size_t i = n_points/2 + 1; i < n_points; i++) {
        dest[i] += src[i];
        if (dest[i] >= 2*pi) {
            dest[i] -= 2*pi;
        }
    }
}

static void ifft(float *src, float *dest, size_t n_points) {
    fftwf_plan plan;

    plan = fftwf_plan_r2r_1d(n_points, src, dest, FFTW_HC2R, FFTW_ESTIMATE);
    fftwf_execute(plan);
    fftwf_destroy_plan(plan);
}

void PhaseVocoderSynth::render_more_samples( ) {
    float *accumulator_ptr, *phasedata_ptr, *dest_ptr;

    for (size_t i = 0; i < n_channels; i++) {
        phasedata_ptr = phase_data + n_points * i;
        accumulator_ptr = phase_accumulator + n_points * i;
        dest_ptr = float_samples + i*n_samples + n_samples_ready;
        /* accumulate phase for this channel */
        accumulate(phasedata_ptr, accumulator_ptr, n_points);
        /* convert to Cartesian form */
        p2c(accumulator_ptr, realimag, n_points);
        /* do inverse FFT of the points */
        ifft(realimag, fft_result, n_points);

        /* copy samples */
        for (size_t j = 0; j < n_points/2; j++) {
            *dest_ptr += fft_result[j];
            dest_ptr++;
        }

        for (size_t j = n_points/2; j < n_points; j++) {
            *dest_ptr = fft_result[j];
            dest_ptr++;
        }
    }

    n_samples_ready += n_points/2;
}

AudioPacket *PhaseVocoderSynth::render_samples(size_t n_samples) {
    AudioPacket *ret = new AudioPacket(48000, n_channels, 2, n_samples);
    render_samples(ret);
    return ret;
}

void PhaseVocoderSynth::render_samples(AudioPacket *ret) {
    size_t n_samples = ret->n_frames( );
    while (n_samples_ready < n_samples) {
        render_more_samples( );
    }

    for (size_t i = 0; i < n_samples; i++) {
        for (size_t j = 0; j < n_channels; j++) {
            /* we divide by n_points to compensate for the unbalanced FFT */
            ret->sample(i)[j] = float_samples[i+j*n_samples] / n_points;
        }
    }

    for (size_t i = 0; i < n_channels; i++) {
        memmove(
            /* FIXME we have a naming problem here */
            float_samples+i*this->n_samples, 
            float_samples+i*this->n_samples+n_samples,
            (this->n_samples - n_samples) * sizeof(float)
        );
    }

    n_samples_ready -= n_samples;
}

void PhaseVocoderSynth::set_phase_data(const PhaseDataPacket &data) {
    if (n_points != data.points( ) || n_channels != data.channels( )) {
        throw std::runtime_error("Phase data does not conform");
    }

    memcpy(phase_data, data.phase_data( ), n_points * n_channels);
}
