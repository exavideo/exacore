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

%rename("shot=") ReplayPlayout::roll_shot(const ReplayShot &);
%rename("monitor") ReplayPlayout::get_monitor( );

class ReplayPlayout : public Thread {
    public:
        ReplayPlayout(OutputAdapter *INPUT);
        ~ReplayPlayout( );

        unsigned int add_svg_dsk(const std::string &INPUT,
            coord_t, coord_t);

        void show_clock( );
        void hide_clock( );
        void position_clock(coord_t, coord_t);

        void roll_shot(const ReplayShot &INPUT);
        void queue_shot(const ReplayShot &INPUT);
        void stop( );
        void set_speed(int,int);
        AsyncPort<ReplayRawFrame> *get_monitor( );
};


