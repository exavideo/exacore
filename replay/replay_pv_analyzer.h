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

#ifndef _REPLAY_PV_ANALYZER_H
#define _REPLAY_PV_ANALYZER_H

#include "audio_fifo.h"

class ReplayPvFrameSet;

class ReplayPvAnalyzer {
    public:
        ReplayPvAnalyzer( );
        ReplayPvAnalyzer(ReplayBuffer *buf);
        ~ReplayPvAnalyzer( );

        void analyze(IOAudioPacket *apkt);
    protected:
        AudioFIFO<float> *channel_fifos;
        ReplayBuffer *buffer;
        ReplayPvFrameSet *output_frames;
        size_t fft_size;
        size_t hop_size;

        void emit_frame( );
};

#endif
