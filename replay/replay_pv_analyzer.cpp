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

ReplayPvAnalyzer::ReplayPvAnalyzer( ) {
    channel_fifos = NULL;
    output_frames = NULL;
    buffer = buf;
    fft_size = 512;
    hop_size = 128;
}

timecode_t ReplayPvAnalyzer::analyze(PackedAudioPacket<int16_t> *apkt) {
    timecode_t time = -1;

    if (fifo == NULL) {
        fifo = new AudioFIFO<float>(apkt->n_channels);
    }

    fifo->add_packet(apkt);

    while (fifo.samples( ) > fft_size) {
        time = emit_frame( );
    }

    return time;
}

timecode_t ReplayPvAnalyzer::emit_frame( ) {
    ReplayFrameData rfd;
    PlanarAudioPacket<float> pkt(fft_size, fifo->channels( ));
    fifo->peek_packet(&pkt);

    if (output_frames == NULL) {
        output_frames = new ReplayPvFrameSet(n_channels, fft_size);
    }

    for (size_t i = 0; i < n_channels; i++) {
        output_frames->frame(i).set_fft(pkt.planes(i), fft_size);
    }

    fifo->pop(hop_size);

    /* write output_frames to buffer... */
}
