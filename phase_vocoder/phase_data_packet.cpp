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

#include "phase_data_packet.h"
#include <fftw3.h>
#include <math.h>

PhaseDataPacket::PhaseDataPacket(AudioPacket *apkt, size_t n_points_) {
    n_channels = apkt->channels( );
    n_points = n_points_;
    n_samples = apkt->n_frames( );

    real_samples = new float[n_channels * n_samples];
    fft1_results = new float[n_channels * n_points];
    fft2_results = new float[n_channels * n_points];

    fill_sample_array(apkt);
    do_ffts( );
    process_data( );
}

void PhaseDataPacket::fill_sample_array(AudioPacket *apkt) {
    /* FIXME: we don't really need all these samples converted to float */
    for (size_t i = 0; i < apkt->n_frames( ); i++) {
        for (unsigned j = 0; j < n_channels; j++) {
            real_samples[j*n_samples+i] = apkt->sample(i)[j];
        }
    }
}

static void float_fft(int n, float *src, float *dst) {
    fftwf_plan p = fftwf_plan_r2r_1d(n, src, dst, FFTW_R2HC, FFTW_ESTIMATE);
    fftwf_execute(p);
    fftwf_destroy_plan(p);
}

static void convert_to_magnitude_phase(size_t n, float *fftw_result) {
    /* there is no phase term for DC (i=0) nor Nyquist (i=n/2) */
    float real, imag, mag, angle;
    for (size_t i = 1; i < n/2; i++) {
        real = fftw_result[i];
        imag = fftw_result[n-i];
        mag = sqrtf(real*real + imag*imag);
        angle = atan2f(imag, real);
        fftw_result[i] = mag;
        fftw_result[n-i] = angle;
    }
}

void PhaseDataPacket::do_ffts( ) {
    size_t midpoint = n_samples / 2;
    size_t overlap = n_points / 2;
    size_t total_length = 2 * n_points - overlap;
    size_t start1 = midpoint - (total_length / 2);
    size_t start2 = start1 + overlap;

    for (unsigned i = 0; i < n_channels; i++) {
        float *input_channel_base = real_samples + i * n_samples;
        float *output_channel_base1 = fft1_results + i * n_points;
        float *output_channel_base2 = fft2_results + i * n_points;
        float_fft(n_points, input_channel_base + start1, output_channel_base1);
        float_fft(n_points, input_channel_base + start2, output_channel_base2);
    }
}

void PhaseDataPacket::process_data( ) {
    /* compute phase differences in fft2_results */
    for (unsigned i = 0; i < n_channels; i++) {
        float *fft1base = fft1_results + n_points * i;
        float *fft2base = fft2_results + n_points * i;

        convert_to_magnitude_phase(n_points, fft1base);
        convert_to_magnitude_phase(n_points, fft2base);
        for (size_t i = n_points/2 + 1; i < n_points; i++) {
            fft2base[i] -= fft1base[i];
        }
    }
}

PhaseDataPacket::~PhaseDataPacket( ) {
    delete [] real_samples;
    delete [] fft1_results;
    delete [] fft2_results;
}
