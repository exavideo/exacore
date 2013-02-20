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
    reader = NULL;
    cached_compressed_frame = NULL;
    cached_raw_frame = NULL;
}

ReplayFrameCache::~ReplayFrameCache( ) {
    delete reader;
    delete cached_compressed_frame;
    delete cached_raw_frame;
}

RawFrame *ReplayFrameCache::get_frame(ReplayBuffer *source, timecode_t tc) {
    if (reader == NULL || reader->source( ) != source) {
        /* cache miss type 1: we don't have a reader */
        delete reader;
        reader = source->make_reader( );
        delete cached_compressed_frame;
        delete cached_raw_frame;
        cached_compressed_frame = NULL;
        cached_raw_frame = NULL;
    }

    if (cached_compressed_frame == NULL 
            || cached_compressed_frame->pos != tc) {
        /* 
         * cache miss type 2: we have a reader (maybe just created) 
         * but no frame or the wrong frame
         */
        delete cached_compressed_frame;
        delete cached_raw_frame;
        cached_compressed_frame = NULL;
        cached_raw_frame = NULL;
        load_frame(tc);
    }

    return cached_raw_frame;
}

void ReplayFrameCache::load_frame(timecode_t tc) {
    /* we must already have a reader to load a frame */
    if (reader == NULL) {
        throw std::runtime_error("tried to load frame before we knew source");
    }

    cached_compressed_frame = reader->read_frame(tc);
    cached_raw_frame = decoder.decode(
        cached_compressed_frame->video_data,
        cached_compressed_frame->video_size
    );
}
