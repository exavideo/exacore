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

#include "character_generator.h"

CharacterGenerator::CharacterGenerator( ) : Thread( ), _output_pipe(2) { 
    start_thread( );
}

CharacterGenerator::CharacterGenerator(int dummy) : Thread( ), _output_pipe(2) {
    /* don't start the thread */
    UNUSED(dummy);
}

CharacterGenerator::~CharacterGenerator( ) {

}

void CharacterGenerator::run_thread(void) {
    for (;;) {
        /* just dump lots of null frames */
        _output_pipe.put(NULL);
    }
}
