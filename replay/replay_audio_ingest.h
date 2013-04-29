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

#ifndef _REPLAY_AUDIO_INGEST_H
#define _REPLAY_AUDIO_INGEST_H

#include "packed_audio_packet.h"
#include "audio_fifo.h"
#include "thread.h"
#include "pipe.h"
#include "replay_buffer.h"
#include "ajfft.h"
#include "adapter.h"
#include <vector>
#include <atomic>

class ReplayAudioIngest : public Thread {
    public:
        ReplayAudioIngest(InputAdapter *iadp);
        ReplayAudioIngest(Pipe<IOAudioPacket *> *ipipe);
        ~ReplayAudioIngest( );

        /* 
         * map channel to a ReplayBuffer where the vocoder frames will
         * be stored. Unmapped channels do not get recorded at all.
         */
        void map_channel(unsigned int channel_no, ReplayBuffer *buffer);

        /*
         * start worker thread
         */
        void start( );

    protected:
        struct channel_entry {
            unsigned int channel_no;
            ReplayBuffer *buffer;

            std::complex<float> *current_frame;
            /* 
             * keep track of the last vocoded frame 
             * so we can use it to compute phase delta
             */
            std::complex<float> *last_frame;
            /*
             * use FIFO for overlapping windows
             */
            AudioFIFO<float> *fifo;
        };

        void run_thread( );
        void set_fft_parameters( );
        void process_packet(IOAudioPacket *pkt);
        void emit_frame(channel_entry &ch);


        Pipe<IOAudioPacket *> *pipe;
        std::vector<channel_entry> channel_map;
        FFT<float> *fft;
        float *window;
        size_t fft_size, fft_hop;

        std::complex<float> *output_frame;

        std::atomic<bool> stop;
        bool running;
};

#endif
