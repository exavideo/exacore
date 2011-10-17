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

void ReplayPlayout::roll_shot(const ReplayShot &shot) {
    MutexLock l(m);
    current_source = shot.source;
    /* 1/2 frame of timecode between output fields */
    field_rate = Rational(3, 8);
    current_pos = Rational((int) shot.start);
    shot_end = shot.start + shot.length;

    /* clear out queued shots */
    next_shots.clear( );
}

void ReplayPlayout::roll_next_shot( ) {
    /* no mutex lock, so only call this when mutex is already locked */
    if (!next_shots.empty( )) {
        const ReplayShot &shot = next_shots.front( );
        current_source = shot.source;
        current_pos = Rational((int) shot.start);
        shot_end = shot.start + shot.length;
        next_shots.pop_front( );
    } else {
        shot_end = 0;
    }
}

void ReplayPlayout::queue_shot(const ReplayShot &shot) {
    MutexLock l(m);
    next_shots.push_back(shot);
}

void ReplayPlayout::stop( ) {
    MutexLock l(m);
    field_rate = Rational(0);
}

void ReplayPlayout::set_speed(int num, int denom) {
    MutexLock l(m);
    // divide by two; this is the timecode increment from one *field* to the next
    field_rate = Rational(num, denom) * Rational(1, 2);
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

        /* fill in timecode and other goodies for monitor */
        monitor_frame->source_name = "Program";
        if (rfd1.data_ptr != NULL) {
            monitor_frame->source_name2 = rfd1.source->get_name( );
            monitor_frame->tc = pos.integer_part( );
            monitor_frame->fractional_tc = pos.fractional_part( );
        } else {
            monitor_frame->source_name2 = "No Clip";
        }

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

        if (current_pos.fractional_part( ).less_than_one_half( )) {
            f1.use_first_field = true;
        } else {
            f1.use_first_field = false;  
        }

        current_pos += field_rate;
        
        tc = current_pos.integer_part( );
        current_source->get_readable_frame(tc, f2);
        
        if (current_pos.fractional_part( ).less_than_one_half( )) {
            f2.use_first_field = true;
        } else {
            f2.use_first_field = false;
        }

        current_pos += field_rate;

        if (current_pos.integer_part( ) > shot_end && !next_shots.empty( ) 
                && shot_end > 0) {
            roll_next_shot( );
        }
    } else {
        f1.data_ptr = NULL;
        f2.data_ptr = NULL;
        tc = 0;
    }
}

static coord_t field_start_scan(bool want_first, RawFrame::FieldDominance dom) {
    if (want_first && dom == RawFrame::BOTTOM_FIELD_FIRST) {
        return 1;
    } else if (!want_first && dom == RawFrame::BOTTOM_FIELD_FIRST) {
        return 0;
    } else if (want_first) {
        return 0;    
    } else {
        return 1;
    }
}

void ReplayPlayout::decode_field(RawFrame *out, ReplayFrameData &field,
        ReplayFrameData &cache_data, RawFrame *&cache_frame,
        bool is_first_field) {

    coord_t srcline, dstline;
    RawFrame *tmp;

    /* decode the field data if we don't have it cached */
    if (field.data_ptr != cache_data.data_ptr) {
        delete cache_frame;
        cache_frame = dec.decode(field.data_ptr, field.data_size);
        /* Scale up video to 1920x1080 */
        /* FIXME this assumes we always want 1920x1080 output */
        if (cache_frame->w( ) < 1920) {
            tmp = cache_frame->convert->CbYCrY8422_1080( ); 
            delete cache_frame;
            cache_frame = tmp;
        }
        cache_data = field;
    }

    /* figure which lines we're taking and where they're going */
    srcline = field_start_scan(field.use_first_field, 
            field.source->field_dominance( ));
    dstline = field_start_scan(is_first_field, oadp->output_dominance( ));

    /* copy the scanlines to the destination frame */
    while (srcline < cache_frame->h( ) && dstline < out->h( )) {
        memcpy(out->scanline(dstline), cache_frame->scanline(srcline),
                cache_frame->pitch( ));
        dstline += 2;
        srcline += 2;
    }
}       
