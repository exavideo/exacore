/*
 * Copyright 2011 Andrew H. Armenia.
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

#ifndef _KEYDAEMON_CHARACTER_GENERATOR_H
#define _KEYDAEMON_CHARACTER_GENERATOR_H

#include "types.h"
#include "pipe.h"
#include "thread.h"
#include "raw_frame.h"

/* 
 * Something that generates drawable overlay images.
 */
class CharacterGenerator : public Thread {
    public:
        CharacterGenerator( );
        virtual ~CharacterGenerator( );
        Pipe<RawFrame *> &output_pipe( ) { return _output_pipe; }

        coord_t x( ) { return _x; }
        coord_t y( ) { return _y; }
        void set_x(coord_t x) { _x = x; }
        void set_y(coord_t y) { _y = y; }
        void set_position(coord_t x, coord_t y) { _x = x; _y = y; }
        virtual unsigned int dirty_level( ) { return 0; }

    protected:
        CharacterGenerator(int dummy); /* construct without starting thread */
        virtual void run_thread(void); /* override from Thread */

        coord_t _x, _y; 
        Pipe<RawFrame *> _output_pipe; 
};

#endif
