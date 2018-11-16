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

#include <vector>

struct CgOutputFrame {
    RawFrame *frame;
    bool tie_to_tally;

    CgOutputFrame() {
        frame = NULL;
        tie_to_tally = false;
    }

    CgOutputFrame(RawFrame *f) {
        frame = f;
        tie_to_tally = false;
    }
};

/* 
 * Something that generates drawable overlay images.
 */
class CharacterGenerator : public Thread {
    public:
        CharacterGenerator( );
        virtual ~CharacterGenerator( );
        Pipe<CgOutputFrame> &output_pipe( ) { return _output_pipe; }

        coord_t x( ) { return _x; }
        coord_t y( ) { return _y; }
        void set_x(coord_t x) { _x = x; }
        void set_y(coord_t y) { _y = y; }
        void set_position(coord_t x, coord_t y) { _x = x; _y = y; }
        virtual unsigned int dirty_level( ) { return 0; }

        /*
         * Tells the keyer to inhibit this graphic if the given source's tally
         * is active on the input frame.
         */
        virtual void inhibit_on_source(unsigned int source);
        /*
         * Returns a list of tally sources for which we should inhibit.
         */
        virtual std::vector<unsigned int> inhibited_sources( );
        

    protected:
        CharacterGenerator(int dummy); /* construct without starting thread */
        virtual void run_thread(void); /* override from Thread */

        coord_t _x, _y; 
        Pipe<CgOutputFrame> _output_pipe; 
        std::vector<unsigned int> _inhibited_sources;
};

#endif
