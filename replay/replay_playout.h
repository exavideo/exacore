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

#ifndef _REPLAY_PLAYOUT_H
#define _REPLAY_PLAYOUT_H

#include "thread.h"
#include "mutex.h"
#include "adapter.h"
#include "replay_data.h"
#include "replay_buffer.h"
#include "async_port.h"
#include "rational.h"
#include "mjpeg_codec.h"

#include <list>

class ReplayPlayout : public Thread {
    public:
        ReplayPlayout(OutputAdapter *oadp_);
        ~ReplayPlayout( );

        /* Roll a shot and clear the queue of shots to follow. */
        void roll_shot(const ReplayShot &shot);
        /* Queue a shot to roll after the current shot passes its end */
        void queue_shot(const ReplayShot &shot);
        /* Stop the playout right now */
        void stop( );
        /* Adjust the playout rate */
        void set_speed(int num, int denom);

        AsyncPort<ReplayRawFrame> monitor;
        AsyncPort<ReplayRawFrame> *get_monitor( ) { return &monitor; }

    protected:
        void run_thread( );
        void get_and_advance_current_fields(ReplayFrameData &f1, 
                ReplayFrameData &f2, Rational &pos);

        void decode_field(RawFrame *out, ReplayFrameData &field, 
                ReplayFrameData &cache_data, RawFrame *&cache_frame,
                bool is_first_field);

        void roll_next_shot( );

        OutputAdapter *oadp;

        ReplayBuffer *current_source;
        Rational current_pos;
        Rational field_rate;
        timecode_t shot_end;

        std::list<ReplayShot> next_shots;

        Mutex m;

        Mjpeg422Decoder dec;

};

#endif

