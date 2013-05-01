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

#ifndef _REPLAY_AUDIO_BUFFER_PLAYOUT_H
#define _REPLAY_AUDIO_BUFFER_PLAYOUT_H

#include "ajfft.h"
#include "packed_audio_packet.h"
#include "audio_fifo.h"
#include "replay_buffer.h"
#include "rational.h"
#include <complex>
#include <vector>

class ReplayAudioBufferPlayout {
    public:
        ReplayAudioBufferPlayout( );
        ~ReplayAudioBufferPlayout( );

        void set_position(uint64_t timestamp);
        void map_channel(unsigned int channel_no, ReplayBuffer *buf);
        void fill_packet(IOAudioPacket *apkt, Rational speed);
        void clear_channel_map( );

    protected:
        struct channel_data {
            unsigned int channel_no;
            timecode_t origin_timecode;
            ReplayBuffer *buf;
            Rational pos_offset;

            std::complex<float> *phase_accumulator;
            float *overlap_add_buffer;
            AudioFIFO<float> *fifo;
        };
        
        FFT<float> *ifft;
        std::complex<float> *ifft_result;
        float *ifft_realpart;
        std::vector<channel_data> channel_map;
        unsigned int max_channel_no;
        size_t fft_size, fft_hop;
        float scale_factor;

        void initialize_fft( );
        void synthesize_samples(channel_data &ch);
};

#endif
