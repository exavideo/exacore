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

#ifndef _REPLAY_GAMEDATA_H
#define _REPLAY_GAMEDATA_H

#include "mutex.h"
#include <string>

class ReplayGameData {
    public:
        ReplayGameData( );
        ReplayGameData(const std::string &com);
        ~ReplayGameData( );

        void set_clock(const std::string &new_clock);
        void get_clock(std::string &clock_value);

        void as_jpeg_comment(std::string &com);
        void from_jpeg_comment(const std::string &com);
    protected:
        std::string clock;
        Mutex m;
};

#endif
