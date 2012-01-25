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
#include "replay_gamedata.h"
#include "async_port.h"
#include "rational.h"
#include "mjpeg_codec.h"
#include "avspipe_input_adapter.h"

#include <list>
#include <vector>

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

        /* Play input from a pipe */
        void avspipe_playout(const char *cmd);

        void show_clock( );
        void hide_clock( );
        void position_clock(coord_t x, coord_t y);

        AsyncPort<ReplayRawFrame> monitor;
        AsyncPort<ReplayRawFrame> *get_monitor( ) { return &monitor; }

        unsigned int add_svg_dsk(const std::string &svg, 
            coord_t xoffset = 0, coord_t yoffset = 0);

    protected:
        void run_thread( );
        void get_and_advance_current_fields(ReplayFrameData &f1, 
                ReplayFrameData &f2, Rational &pos);

        void decode_field(RawFrame *out, ReplayFrameData &field, 
                ReplayFrameData &cache_data, RawFrame *&cache_frame,
                bool is_first_field);

        void roll_next_shot( );

        void apply_dsks(RawFrame *target);
        void add_clock(RawFrame *target);

        OutputAdapter *oadp;

        ReplayBuffer *current_source;
        AvspipeInputAdapter *next_avspipe;

        ReplayBufferLocker lock;

        Rational current_pos;
        Rational field_rate;
        timecode_t shot_end;

        struct dsk {
            RawFrame *key;
            coord_t x;
            coord_t y;
        };

        std::list<ReplayShot> next_shots;
        std::vector<dsk> dsks;

        Mutex m;
        Mutex dskm;
        Mutex clockm;

        Mjpeg422Decoder dec;

        bool render_clock;
        coord_t clock_x;
        coord_t clock_y;

        ReplayGameData game_data;
};

#endif

