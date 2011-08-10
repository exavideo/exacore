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

#ifndef _REPLAY_PREVIEW_H
#define _REPLAY_PREVIEW_H

#include "replay_data.h"
#include "thread.h"
#include "mutex.h"
#include "condition.h"
#include "async_port.h"
#include <stdexcept>

/* ReplayPreview objects edit ReplayShots, providing a live preview for the multiviewer. */
class ReplayPreview : public Thread {
    public:
        ReplayPreview( );
        ~ReplayPreview( );

        void change_shot(const ReplayShot &shot);
        void get_shot(ReplayShot &shot);

        void seek(timecode_t delta); 
        void mark_in( );
        void mark_out( );

        AsyncPort<ReplayRawFrame> monitor;
        AsyncPort<ReplayRawFrame> *get_monitor( ) { return &monitor; }

        class IllegalMarkOutError : public std::exception {
            const char *what( ) const throw() { 
                return "Cannot place mark out before mark in";
            }
        };
    protected:
        void run_thread( );
        void wait_update(ReplayFrameData &rfd);

        ReplayShot current_shot;
        timecode_t current_pos;

        Mutex m;
        Condition updated;

        bool update_monitor;
};

#endif

