/*
 * Copyright 2011 Exavideo LLC.
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

#include "replay_frame_extractor.h"
#include "replay_buffer.h"

ReplayFrameExtractor::ReplayFrameExtractor( ) 
        : dec(1920, 1080), enc(1920, 1080) {

}

ReplayFrameExtractor::~ReplayFrameExtractor( ) {

}

void ReplayFrameExtractor::extract_raw_jpeg(const ReplayShot &shot, 
        timecode_t offset, std::string &jpeg) {
    ReplayFrameData rfd;

    shot.source->get_readable_frame(shot.start + offset, rfd);
    uint8_t *data = (uint8_t *) rfd.main_jpeg( );
    size_t size = rfd.main_jpeg_size( );
    for (size_t i = 0; i < size - 1; i++) {
        if (data[i] == 0xff && data[i + 1] == 0xd9) {
            jpeg.assign((char *)data, i + 2);
            return;
        }
    }

    throw std::runtime_error("no valid JPEG frame found");
}

void ReplayFrameExtractor::extract_scaled_jpeg(const ReplayShot &shot,
        timecode_t offset, std::string &jpeg, int scale_down) {
    
    ReplayFrameData rfd;
    shot.source->get_readable_frame(shot.start + offset, rfd);

    RawFrame *rf = dec.decode(rfd.main_jpeg( ), 
            rfd.main_jpeg_size( ), scale_down);
    enc.encode(rf);
    delete rf;

    jpeg.assign((char *)enc.get_data( ), enc.get_data_size( ));
}
