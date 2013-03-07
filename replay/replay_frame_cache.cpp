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

#include "replay_frame_cache.h"

ReplayFrameCache::ReplayFrameCache( ) : decoder(1920, 1080) {
    cached_compressed_frame = NULL;
    cached_raw_frame = NULL;
}

ReplayFrameCache::~ReplayFrameCache( ) {
    delete cached_compressed_frame;
    delete cached_raw_frame;
}

RawFrame *ReplayFrameCache::get_frame(ReplayBuffer *source, timecode_t tc) {
    if (cached_compressed_frame == NULL 
            || cached_compressed_frame->source != source 
            || cached_compressed_frame->pos != tc) {

        /* cache miss */
        delete cached_compressed_frame; 
        cached_compressed_frame = NULL;
        
        delete cached_raw_frame;
        cached_raw_frame = NULL;

        cached_compressed_frame = source->read_frame(
            tc, ReplayBuffer::LOAD_VIDEO
        );

        cached_raw_frame = decoder.decode(
            cached_compressed_frame->video_data,
            cached_compressed_frame->video_size
        );
    }

    return cached_raw_frame;
}

