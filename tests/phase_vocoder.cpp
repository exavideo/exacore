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

#include "audio_packet.h"
#include "phase_data_packet.h"
#include "phase_vocoder_synth.h"
#include "posix_util.h"
#include <unistd.h>

int main( ) {
    AudioPacket *src = new AudioPacket(48000, 2, 2, 1600);
    AudioPacket *out = new AudioPacket(48000, 2, 2, 1600);
    PhaseVocoderSynth pv(512, 2);

    while (read_all(STDIN_FILENO, src->data( ), src->size( )) > 0) {
        PhaseDataPacket phase_data(src, 512);
        pv.set_phase_data(phase_data);
        pv.render_samples(out);
        write_all(STDOUT_FILENO, out->data( ), out->size( ));
    }
}
