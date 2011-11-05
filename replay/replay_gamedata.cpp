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

#include "replay_gamedata.h"

ReplayGameData::ReplayGameData( ) {
    clock = ":01.2";
}

ReplayGameData::ReplayGameData(const std::string &com) {
    clock = com;
}

ReplayGameData::~ReplayGameData( ) {

}

void ReplayGameData::set_clock(const std::string &new_clock) {
    MutexLock l(m);
    clock = new_clock;
}

void ReplayGameData::get_clock(std::string &clock_value) {
    MutexLock l(m);
    clock_value = clock;
}

void ReplayGameData::as_jpeg_comment(std::string &com) {
    MutexLock l(m);
    com = clock;
}
