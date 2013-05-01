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

#include "replay_audio_buffer_playout.h"
#include <iostream>

ReplayAudioBufferPlayout::ReplayAudioBufferPlayout( ) {
    max_channel_no = 0;

    /*
     * FIXME
     * these need to agree between playout and ingest, but they're
     * defined separately in both places.
     */
    fft_size = 1024;
    fft_hop = 256;
    scale_factor = float(fft_hop) / (float(fft_size) * float(fft_size));

    initialize_fft( );
}

void ReplayAudioBufferPlayout::initialize_fft( ) {
    ifft = new FFT<float>(fft_size, FFT<float>::INVERSE);
    ifft_result = new std::complex<float>[fft_size];
    ifft_realpart = new float[fft_size];
}

void ReplayAudioBufferPlayout::clear_channel_map( ) {
    for (channel_data &ch : channel_map) {
        delete [] ch.phase_accumulator;
        delete [] ch.overlap_add_buffer;
        delete ch.fifo;
    }

    channel_map.clear( );
}

ReplayAudioBufferPlayout::~ReplayAudioBufferPlayout( ) {
    clear_channel_map( );

    delete [] ifft_result;
    delete [] ifft_realpart;
    delete ifft;
}

void ReplayAudioBufferPlayout::set_position(uint64_t timestamp) {
    for (channel_data &ch : channel_map) {
        ch.origin_timecode = ch.buf->get_frame_timecode(timestamp);
        ch.pos_offset = 0;
    }
}

void ReplayAudioBufferPlayout::map_channel(
    unsigned int channel_no,
    ReplayBuffer *buf
) {
    channel_data chnew;
    
    /* 
     * check if we already have a mapping for this channel 
     * if so, we will just overwrite it
     */
    for (channel_data &ch : channel_map) {
        if (ch.channel_no == channel_no) {
            ch.buf = buf;
            return;
        }
    }

    /* no mapping exists for this channel, so add it */
    chnew.channel_no = channel_no;
    chnew.buf = buf;
    chnew.phase_accumulator = new std::complex<float>[fft_size];
    chnew.overlap_add_buffer = new float[fft_size];
    chnew.fifo = new AudioFIFO<float>;

    channel_map.push_back(chnew);
}

void ReplayAudioBufferPlayout::fill_packet(
    IOAudioPacket *apkt, 
    Rational speed
) {
    /* 
     * synthesize enough data for each channel to fill apkt 
     * note that we don't do any synthesis on channels that do not exist
     * in the apkt.
     */
    for (channel_data &ch : channel_map) {
        while (ch.fifo->fill_samples( ) < apkt->size_samples( )) {
            if (ch.channel_no < apkt->channels( )) {            
                synthesize_samples(ch);
            }
            ch.pos_offset += speed;
        }
        
        if (ch.channel_no < apkt->channels( )) {
            ch.fifo->fill_channel(apkt, ch.channel_no);
        }
    }
}

template <class T>
static void numarray_pop(T *array, size_t n, size_t s) {
    size_t i;
    for (i = 0; i < (s - n); i++) {
        array[i] = array[i+n];
    }

    for (i = (s - n); i < s; i++) {
        array[i] = 0;
    }
}

template <class T>
static void numarray_dump(T *array, size_t n) {
    for (size_t i = 0; i < n; i++) {
        std::cout << array[i] << std::endl;
    }
}

void ReplayAudioBufferPlayout::synthesize_samples(channel_data &ch) {
    BlockSet bset;
    std::complex<float> *frame_data;
    size_t count;
    timecode_t frame = ch.origin_timecode + ch.pos_offset.integer_part( );
     
    /* load FFT frame from buffer file */
    ch.buf->read_blockset(frame, bset);
    frame_data = bset.load_alloc_block<std::complex<float> >(
        REPLAY_PVOC_BLOCK, count
    );
    
    if (count != fft_size) {
        throw std::runtime_error("FFT size mismatch");
    }

    /* phase accumulation */
    for (size_t i = 0; i < fft_size; i++) {
        ch.phase_accumulator[i] = std::polar(
            std::abs(frame_data[i]),
            std::arg(ch.phase_accumulator[i]) + std::arg(frame_data[i])
        );
    }

    delete [] frame_data;

    /* compute the inverse FFT */
    ifft->compute(ifft_result, ch.phase_accumulator);

    /* now get real part, scale, and overlap-add */
    for (size_t i = 0; i < fft_size; i++) {
        ch.overlap_add_buffer[i] += std::real(ifft_result[i]) * scale_factor;
    }

    /* shift off first fft_hop samples from overlap_add_buffer into fifo */
    ch.fifo->add_packed_samples(ch.overlap_add_buffer, fft_hop);
    numarray_pop(ch.overlap_add_buffer, fft_hop, fft_size);
}
