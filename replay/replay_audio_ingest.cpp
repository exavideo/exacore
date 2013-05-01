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

#include "replay_audio_ingest.h"

ReplayAudioIngest::ReplayAudioIngest(InputAdapter *iadp) {
    if (iadp->audio_output_pipe( ) == NULL) {
        throw std::runtime_error("no audio from input adapter");
    }

    pipe = iadp->audio_output_pipe( );
    running = false;
    stop = false;
    set_fft_parameters( );

    iadp->start( );
}

ReplayAudioIngest::ReplayAudioIngest(Pipe<IOAudioPacket *> *ipipe) {
    if (ipipe == NULL) {
        throw std::runtime_error("cannot use NULL input pipe");
    }

    pipe = ipipe;
    running = false;
    stop = false;
    set_fft_parameters( );
}

ReplayAudioIngest::~ReplayAudioIngest( ) {
    stop = true;
    join_thread( );

    /* clean up per-channel buffers */
    for (channel_entry &e : channel_map) {
        delete [] e.current_frame;
        delete [] e.last_frame;
        delete e.fifo;
    }
} 

void ReplayAudioIngest::start( ) {
    running = true;
    start_thread( );
}

void ReplayAudioIngest::set_fft_parameters( ) {
    fft_size = 1024;
    fft_hop = 256;
    window = new float[fft_size];
    fft = new FFT<float>(fft_size);
    output_frame = new std::complex<float>[fft_size];
    /* FIXME: need to actually create a window function and use it... */
}


void ReplayAudioIngest::map_channel(
    unsigned int channel_no, 
    ReplayBuffer *buffer
) {
    channel_entry e;

    /* check for invalid mapping requests */
    if (running) {
        throw std::runtime_error("cannot add channel mappings while running");
    }

    for (channel_entry &i : channel_map) {
        if (channel_no == i.channel_no) {
            throw std::runtime_error("cannot map same channel twice");
        }
    }

    e.channel_no = channel_no;
    e.buffer = buffer;
    e.current_frame = new std::complex<float>[fft_size];
    e.last_frame = new std::complex<float>[fft_size];
    e.fifo = new AudioFIFO<float>;

    channel_map.push_back(e);
}

void ReplayAudioIngest::run_thread( ) {
    IOAudioPacket *in;
    for (;;) {
        in = pipe->get( );
        process_packet(in);
        delete in;
    }
}

void ReplayAudioIngest::process_packet(IOAudioPacket *pkt) {
    PlanarAudioPacket<int16_t> *planar = pkt->make_planar<int16_t>( );
    
    /* 
     * iterate through our map and pull out channels. Load FIFO for each. 
     * Then, emit transformed frames until the FIFO can't provide 
     * any more data.
     */
    for (channel_entry &ch : channel_map) {
        /* ignore mappings for channels above the number we have */
        if (ch.channel_no < planar->channels( )) {
            int16_t *ch_samples = planar->channel(ch.channel_no);
            size_t n_samples = planar->size_samples( );
            ch.fifo->add_packed_samples(ch_samples, n_samples);
        }

        while (ch.fifo->fill_samples( ) >= fft_size) {
            emit_frame(ch);
        }
    }
}

void ReplayAudioIngest::emit_frame(channel_entry &ch) {
    BlockSet bset;
    size_t i;
    if (ch.fifo->fill_samples( ) < fft_size) {
        throw std::runtime_error("cannot emit frame, not enough samples");
    }

    /* take FFT of ch.fifo->data( ) into ch.current_frame */
    fft->compute(ch.current_frame, ch.fifo->data( ));

    /* subtract phase of last_frame from phase of current_frame to get output_frame */
    for (i = 0; i < fft_size; i++) {
        output_frame[i] = std::polar(
            std::abs(ch.current_frame[i]),
            std::arg(ch.current_frame[i]) - std::arg(ch.last_frame[i])
        );
    }

    /* 
     * swap last_frame and current_frame pointers 
     * (this makes current_frame the next last_frame)
     */
    std::swap(ch.last_frame, ch.current_frame);

    /* write output_frame to buffer */
    bset.add_block(REPLAY_PVOC_BLOCK, output_frame, fft_size);
    bset.add_block("Debug001", ch.fifo->data( ), fft_size);
    ch.buffer->write_blockset(bset);
    ch.fifo->pop_samples(fft_hop);
}
