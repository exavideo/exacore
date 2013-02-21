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

#include "replay_playout_bars_source.h"
#include "posix_util.h"
#include <math.h>
#include <unistd.h>
#include <fcntl.h>

ReplayPlayoutBarsSource::ReplayPlayoutBarsSource( ) {
    int barsfd;

    bars = new RawFrame(1920, 1080, RawFrame::CbYCrY8422);

    barsfd = open("../files/1080p_bars.uyvy", O_RDONLY);
    if (barsfd == -1) {
        throw POSIXError("cannot load color bars");
    }

    bars->read_from_fd(barsfd);
    close(barsfd);

    phase = 0;
}

ReplayPlayoutBarsSource::~ReplayPlayoutBarsSource( ) {
    delete bars;
}

void ReplayPlayoutBarsSource::read_frame(ReplayPlayoutFrame &frame_data, 
        Rational speed) {
    (void) speed;

    frame_data.video_data = bars->copy( );
    frame_data.audio_data = audio_allocator.allocate( );
    frame_data.tc = 0;
    frame_data.fractional_tc = 0;
    frame_data.source_name = "No Source";
    oscillate(frame_data.audio_data, 440);
}

void ReplayPlayoutBarsSource::oscillate(AudioPacket *pkt, float frequency) {
    /* 
     * convert frequency to delta phase per sample 
     * frequency = Hz, multiply by 2*pi to get rad/sec, then divide by sample
     * rate to get rad/sample.
     */
    const double pi = 4*atan(1); /* atan(1) == pi/4 */
    double phase_rate = frequency * 2 * pi / 48000.0;
    int16_t *sample, value;
    for (unsigned int i = 0; i < pkt->n_frames( ); i++) {
        /* FIXME: this blindly assumes stereo samples */
        value = 30000 * sin(phase);
        sample = pkt->sample(i);
        sample[0] = value;
        sample[1] = value;
        phase += phase_rate;
        if (phase > 2 * pi) {
            phase -= 2 * pi;
        }
    }
}
