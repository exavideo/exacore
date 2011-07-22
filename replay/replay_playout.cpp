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

#include "replay_playout.h"
#include "raw_frame.h"
#include "mjpeg_codec.h"
#include <string.h>

/* FIXME hardcoded 1920x1080 decoding */
ReplayPlayout::ReplayPlayout(OutputAdapter *oadp_) : 
        current_pos(0), field_rate(0), dec(1920, 1080) {
    oadp = oadp_;
    current_source = NULL;
    running = false;
    start_thread( );
}

ReplayPlayout::~ReplayPlayout( ) {
    
}

void ReplayPlayout::roll_shot(ReplayShot *shot) {
    MutexLock l(m);
    current_source = shot->source;
    field_rate = Rational(1,12);
    current_pos = Rational((int) shot->start);
}

void ReplayPlayout::stop( ) {
    MutexLock l(m);
    field_rate = Rational(0);
}

void ReplayPlayout::run_thread( ) {
    ReplayRawFrame *monitor_frame;

    Rational pos(0);
    ReplayFrameData rfd1, rfd2;
    ReplayFrameData rfd_cache;
    RawFrame *f_cache = NULL;
    RawFrame *out = NULL;
    rfd_cache.data_ptr = NULL;

    for (;;) {
        get_and_advance_current_fields(rfd1, rfd2, pos);
        
        out = new RawFrame(1920, 1080, RawFrame::CbYCrY8422);

        if (rfd1.data_ptr == NULL) {
            /* out = something... */
        } else {
            decode_field(out, rfd1, rfd_cache, f_cache, true);
            decode_field(out, rfd2, rfd_cache, f_cache, false);
        }

        /* scale down to BGRAn8 and send to monitor port */
        monitor_frame = new ReplayRawFrame(
            out->convert->BGRAn8_scale_1_2( )
        );
        monitor.put(monitor_frame);

        /* send the full CbYCrY frame to output */
        oadp->input_pipe( ).put(out);
    }
}

void ReplayPlayout::get_and_advance_current_fields(ReplayFrameData &f1,
        ReplayFrameData &f2, Rational &pos) {
    MutexLock l(m);
    timecode_t tc;

    if (current_source != NULL) {
        pos = current_pos;
        tc = current_pos.integer_part( );
        current_source->get_readable_frame(tc, f1);

        /* FIXME: this assumes bottom-field-first format */
        if (current_pos.fractional_part( ).less_than_one_half( )) {
            f1.use_top_field = false;  
        } else {
            f1.use_top_field = true;
        }

        current_pos += field_rate;
        
        tc = current_pos.integer_part( );
        current_source->get_readable_frame(tc, f2);
        
        if (current_pos.fractional_part( ).less_than_one_half( )) {
            f2.use_top_field = false;
        } else {
            f2.use_top_field = true;
        }

        current_pos += field_rate;
    } else {
        f1.data_ptr = NULL;
        f2.data_ptr = NULL;
        tc = 0;
    }
}


void ReplayPlayout::decode_field(RawFrame *out, ReplayFrameData &field,
        ReplayFrameData &cache_data, RawFrame *&cache_frame,
        bool is_first_field) {

    /* decode the field data if we don't have it cached */
    if (field.data_ptr != cache_data.data_ptr) {
        delete cache_frame;
        cache_frame = dec.decode(field.data_ptr, field.data_size);
        cache_data = field;
    }

    /* FIXME: this logic assumes bottom-field first video */
    /* FIXME: need to implement scanline interpolation for case 3 and 4 */
    /* FIXME: all this memcpy-ing is looking ugly (but maybe hard to avoid) */
    if (!is_first_field && field.use_top_field) {
        /* case 1: copying a top field to a top field */
        for (coord_t i = 0; i < cache_frame->h( ); i += 2) {
            memcpy(out->scanline(i), cache_frame->scanline(i), 
                    cache_frame->pitch( )); 
        }
    } else if (is_first_field && !field.use_top_field) {
        /* case 2: copying a bottom field to a bottom field */
        for (coord_t i = 1; i < cache_frame->h( ); i += 2) {
            memcpy(out->scanline(i), cache_frame->scanline(i),
                    cache_frame->pitch( ));
        }
    } else if (is_first_field && field.use_top_field) {
        /* case 3: copying a top field into a bottom field */
        for (coord_t i = 0; i < cache_frame->h( ); i += 2) {
            memcpy(out->scanline(i + 1), cache_frame->scanline(i),
                    cache_frame->pitch( ));
        }
    } else if (!is_first_field && !field.use_top_field) {
        /* case 4: copying a bottom field into a top field */
        for (coord_t i = 1; i < cache_frame->h( ); i += 2) {
            memcpy(out->scanline(i - 1), cache_frame->scanline(i),
                    cache_frame->pitch( ));
        }
    }
}       
