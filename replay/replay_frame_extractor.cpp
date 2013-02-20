/*
 * Copyright 2011, 2013 Exavideo LLC.
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
#include "audio_packet.h"

ReplayFrameExtractor::ReplayFrameExtractor( ) 
        : dec(1920, 1080), enc(1920, 1080) {

}

ReplayFrameExtractor::~ReplayFrameExtractor( ) {

}

void ReplayFrameExtractor::extract_raw_jpeg(const ReplayShot &shot, 
        timecode_t offset, std::string &jpeg) {
    /* FIXME: this ends up creating a lot of reader objects */
    ReplayFrameData *rfd;
    ReplayBufferReader *reader;
    reader = shot.source->make_reader( );
    rfd = reader->read_frame(shot.start + offset);
    jpeg.assign((char *)rfd->video_data, rfd->video_size);
    delete reader;
    delete rfd;
}

void ReplayFrameExtractor::extract_thumbnail_jpeg(const ReplayShot &shot,
        timecode_t offset, std::string &jpeg) {
    ReplayFrameData *rfd;
    ReplayBufferReader *reader;
    reader = shot.source->make_reader( );
    rfd = reader->read_frame(shot.start + offset);
    jpeg.assign((char *)rfd->thumbnail_data, rfd->thumbnail_size);
    delete reader;
    delete rfd;
}

void ReplayFrameExtractor::extract_scaled_jpeg(const ReplayShot &shot,
        timecode_t offset, std::string &jpeg, int scale_down) {
    
    ReplayFrameData *rfd;
    ReplayBufferReader *reader;
    reader = shot.source->make_reader( );
    rfd = reader->read_frame(shot.start + offset);

    RawFrame *rf = dec.decode(rfd->video_data, rfd->video_size, scale_down);
    enc.encode(rf);
    delete rf;
    delete reader;
    delete rfd;

    jpeg.assign((char *)enc.get_data( ), enc.get_data_size( ));
}

void ReplayFrameExtractor::extract_raw_audio(const ReplayShot &shot,
        timecode_t offset, std::string &data) {
    ReplayFrameData *rfd;
    ReplayBufferReader *reader;
    reader = shot.source->make_reader( );
    rfd = reader->read_frame(shot.start + offset);

    if (rfd->audio_size > 0) {
        AudioPacket apkt(rfd->audio_data, rfd->audio_size);
        data.assign((char *)apkt.data( ), apkt.size( ));
    }
}
