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

%{
    #include "replay_playout.h"
%}

%include "typemaps.i"
%include "replay_playout_filter.i"
%include "string_list.i"

%rename("shot=") ReplayPlayout::roll_shot(const ReplayShot &);
%rename("monitor") ReplayPlayout::get_monitor( );

typedef std::list<std::string> StringList;

class ReplayPlayout : public Thread {
    public:
        ReplayPlayout(OutputAdapter *INPUT);
        ~ReplayPlayout( );

        void roll_shot(const ReplayShot &INPUT);
        void set_speed(int, int);
        AsyncPort<ReplayRawFrame> *get_monitor( );
        void register_filter(ReplayPlayoutFilter *INPUT);
        void stop( );
        void avspipe_playout(const char *INPUT);
        void lavf_playout(const char *INPUT);
        void lavf_playout(const char *INPUT, int64_t);
        void lavf_playout_list(const StringList &INPUT);
        void map_channel(unsigned int, ReplayBuffer *INPUT);
        void clear_channel_map( );
        timecode_t source_position( );
        timecode_t source_duration( );
};


