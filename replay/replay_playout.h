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
#include "replay_playout_filter.h"
#include "replay_playout_bars_source.h"

#include <list>
#include <vector>
#include <atomic>

typedef std::list<const char *> StringList;

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

        /*
         * Register filter.
         * Filters can be used e.g. to add graphics to the video output.
         */
        void register_filter(ReplayPlayoutFilter *filt);

        /*
         * Stop, or more precisely, return to idle source.
         */
        void stop( );

        /*
         * Map audio channels to buffers.
         */
        void map_channel(unsigned int ch, ReplayBuffer *buf);

        /*
         * Clear audio channel mappings.
         */
        void clear_channel_map( );

        /*
         * Roll out file via AvspipeInputAdapter using e.g. ffmpeg
         */
        void avspipe_playout(const char *cmd);
        /*
         * Roll out file using libavformat.
         */
        void lavf_playout(const char *cmd);
        void lavf_playout(const char *cmd, int64_t offset);
        /*
         * Roll out list of files.
         */
        void lavf_playout_list(const StringList &files);

        /*
         * Get information about the current playout source state
         */
        timecode_t source_position( );
        timecode_t source_duration( );

        /* Multiviewer ports. */
        AsyncPort<ReplayRawFrame> monitor;
        AsyncPort<ReplayRawFrame> *get_monitor( ) { return &monitor; }

    protected:
        void run_thread( );

        struct SourceState {
            timecode_t position;
            timecode_t duration;
        };

        struct ChannelMapEntry {
            unsigned int no;
            ReplayBuffer *buf;
        };

        OutputAdapter *oadp;
        ReplayPlayoutBarsSource *idle_source;
        std::vector<ReplayPlayoutFilter *> filters;
        std::atomic<ReplayPlayoutSource *> playout_source;
        std::atomic<Rational *> new_speed;
        std::atomic<timecode_t> _source_position;
        std::atomic<timecode_t> _source_duration;
        Mutex filters_mutex;

        std::vector<ChannelMapEntry> channel_map;
};

#endif

