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

#ifndef _REPLAY_PLAYOUT_H
#define _REPLAY_PLAYOUT_H

#include "thread.h"
#include "mutex.h"
#include "adapter.h"
#include "async_port.h"
#include "rational.h"
#include "replay_data.h"
#include "replay_playout_source.h"

#include <list>
#include <vector>
#include <atomic>

class ReplayPlayout : public Thread {
    public:
        ReplayPlayout(OutputAdapter *oadp_);
        ~ReplayPlayout( );

        /* 
         * Change the source of media to be played out. 
         * We will read frames from this source until it runs out.
         * Any playout source that is currently in use will be deleted.
         */
        void set_source(ReplayPlayoutSource *src);

        /* 
         * Change playout speed to the given fraction of full speed.
         * Note: Not all possible sources support variable speed.
         * These sources always play out at full speed.
         */
        void set_speed(int num, int denom);

        /*
         * Roll shot.
         * This is a wrapper around ReplayBufferPlayoutSource
         * and set_source, provided for convenience and compatibility.
         */
        void roll_shot(const ReplayShot &replay);

        /* Multiviewer ports. */
        AsyncPort<ReplayRawFrame> monitor;
        AsyncPort<ReplayRawFrame> *get_monitor( ) { return &monitor; }


        //unsigned int add_svg_dsk(const std::string &svg, 
        //    coord_t xoffset = 0, coord_t yoffset = 0);

        //unsigned int add_png_file_dsk(const std::string &path, 
        //    coord_t xoffset = 0, coord_t yoffset = 0);

    protected:
        void run_thread( );

        OutputAdapter *oadp;
        ReplayPlayoutSource *idle_source;

        struct dsk {
            RawFrame *key;
            coord_t x;
            coord_t y;
            bool enabled;
        };

        std::atomic<ReplayPlayoutSource *> playout_source;
        std::atomic<Rational *> new_speed;
};

#endif

