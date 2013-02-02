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

#ifndef _REPLAY_DATA_H
#define _REPLAY_DATA_H

#include <stddef.h>
#include <stdint.h>

#include "raw_frame.h"
#include "rational.h"

class ReplayBuffer;

typedef int_fast32_t timecode_t;

/*
 * A shot, or a sequence of frames residing in one of the replay buffers.
 */
struct ReplayShot {
    ReplayBuffer *source;
    timecode_t start;
    timecode_t length;
};

/*
 * Data representing a compressed M-JPEG frame in a buffer.
 */
struct ReplayFrameData {
    ReplayBuffer *source;
    timecode_t pos;
    bool use_first_field;

    ReplayFrameData( ) {
        data_ptr = NULL;
        data_size = 0;
    }

    ReplayFrameData(const ReplayFrameData &from) {
        data_ptr = from.data_ptr;
        data_size = from.data_size;
    }

    const ReplayFrameData &operator=(const ReplayFrameData &from) {
        data_ptr = from.data_ptr;
        data_size = from.data_size;
        return *this;
    }

    void *main_jpeg( ) {
        return data_ptr;    
    }

    size_t main_jpeg_size( ) {
        return data_size - sizeof(aux_data);
    }

    void *thumb_jpeg( ) {
        return aux( )->thumbnail;
    }

    size_t thumb_jpeg_size( ) {
        return sizeof(aux( )->thumbnail);
    }

    void *audio( ) {
        return aux( )->audio;
    }

    size_t audio_size( ) {
        return sizeof(aux( )->audio);
    }

    bool has_audio( ) {
        return aux( )->has_audio;
    }

    void enable_audio( ) {
        aux( )->has_audio = true;
    }

    void no_audio( ) {
        aux( )->has_audio = false;
    }

    bool valid( ) {
        return (data_ptr != NULL);
    }

    void clear( ) {
        data_ptr = NULL;
    }

    friend class ReplayBuffer;
protected:
    struct aux_data {
        bool has_audio;
        uint8_t audio[8192];
        uint8_t thumbnail[81920];
        uint8_t game_data[4096];
    };

    struct aux_data *aux( ) {
        /* FIXME: maybe nonportable due to alignment? */
        return (aux_data *)
                ((uint8_t *)data_ptr + data_size - sizeof(aux_data));
    }

    void *data_ptr;
    size_t data_size;
};

/*
 * Wrap a reference to a RawFrame object used in the monitor ports.
 * ReplayRawFrames own their corresponding RawFrames and will delete
 * them when they go out of scope.
 */
struct ReplayRawFrame {
    ReplayRawFrame(RawFrame *f) : fractional_tc(0) { 
        frame_data = f; 
        source_name = NULL;
        source_name2 = NULL;
        bgra_data = NULL;
        tc = 0;
    }
    ~ReplayRawFrame( ) { 
        delete frame_data;
    }

    RawFrame *frame_data;
    RawFrame *bgra_data;
    /* other stuff goes here */
    const char *source_name;
    const char *source_name2;
    timecode_t tc;
    Rational fractional_tc;
};

#endif
