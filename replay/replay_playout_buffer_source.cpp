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

#include "replay_playout_buffer_source.h"
#include <algorithm> // min
#include <string.h>

ReplayPlayoutBufferSource::ReplayPlayoutBufferSource(const ReplayShot &shot) {
    source = shot.source;
    pos = shot.start;
}

ReplayPlayoutBufferSource::~ReplayPlayoutBufferSource( ) {

}

void ReplayPlayoutBufferSource::read_frame(
        ReplayPlayoutFrame &frame_data, 
        Rational speed
) {
    RawFrame *src;
    /* for speed of 1/1 we should advance pos by 1/2 between fields */
    speed = speed / 2; 
    
    try {
        frame_data.tc = pos.integer_part( );
        frame_data.fractional_tc = pos.fractional_part( );
        frame_data.audio_data = NULL;

        /* retrieve the first field and weave it in */
        src = cache.get_frame(source, pos.integer_part( ));
        frame_data.video_data = new RawFrame(
            src->w( ), src->h( ), 
            src->pixel_format( )
        );
        
        if (pos.fractional_part( ).less_than_one_half( )) {
            /* weave src first field to out first field */
            weave_field(frame_data.video_data, 0, src, 0);
        } else {
            /* weave src second field to out first field */
            weave_field(frame_data.video_data, 0, src, 1);
        }

        pos += speed;

        /* now get the second field and weave it in */
        src = cache.get_frame(source, pos.integer_part( ));
        if (pos.fractional_part( ).less_than_one_half( )) {
            /* weave src first field to out second field */
            weave_field(frame_data.video_data, 1, src, 0);
        } else {
            /* weave src second field to out second field */
            weave_field(frame_data.video_data, 1, src, 1);
        }

        pos += speed;

        if (speed == Rational(1,2)) {
            IOAudioPacket *ap = cache.get_audio(source, pos.integer_part( ));
            if (ap) {
                frame_data.audio_data = ap->clone( );
            }
        } 

        if (frame_data.audio_data == NULL) {
            frame_data.audio_data = audio_allocator.allocate( );
            frame_data.audio_data->zero( );
        }

        frame_data.source_name = source->get_name( );
    } catch (const ReplayFrameNotFoundException &) {
        frame_data.video_data = NULL;
    }
}

/* figure out which scanline begins the first or second field */
coord_t ReplayPlayoutBufferSource::first_scanline(
        RawFrame::FieldDominance dom, 
        int field
) {
    switch (dom) {
        case RawFrame::TOP_FIELD_FIRST:
        case RawFrame::PROGRESSIVE:
            if (field == 0) return 0; else return 1;
        case RawFrame::BOTTOM_FIELD_FIRST:
        default:
            if (field == 0) return 1; else return 0;
    }
}

void ReplayPlayoutBufferSource::weave_field(
    RawFrame *dst, int dst_field,
    RawFrame *src, int src_field
) {
    coord_t src_scanline, dst_scanline, h, minpitch;
    src_scanline = first_scanline(source->field_dominance( ), src_field);
    dst_scanline = first_scanline(output_dominance, dst_field);
    h = src->h( );
    minpitch = std::min(src->pitch( ), dst->pitch( ));

    while (src_scanline < h) {
        memcpy(
            dst->scanline(dst_scanline), 
            src->scanline(src_scanline), 
            minpitch
        );

        src_scanline += 2;
        dst_scanline += 2;
    }
}

timecode_t ReplayPlayoutBufferSource::position( ) {
    return pos.integer_part( );
}

timecode_t ReplayPlayoutBufferSource::duration( ) {
    return -1;
}
