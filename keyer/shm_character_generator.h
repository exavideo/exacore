/*
 * Copyright 2016 Andrew H. Armenia.
 * 
 * This file is part of exacore.
 * 
 * exacore is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * exacore is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with exacore.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _KEYDAEMON_SHM_CHARACTER_GENERATOR_H
#define _KEYDAEMON_SHM_CHARACTER_GENERATOR_H

#include "character_generator.h"
#include "shm_double_buffer.h"

/* 
 * Overlay a frame buffer provided by another process via a double-buffered
 * shared memory area.
 */
class ShmCharacterGenerator : public CharacterGenerator {
    public:
        ShmCharacterGenerator(const char *cmd, unsigned int dirty_level = 0);
        virtual ~ShmCharacterGenerator( );
        unsigned int dirty_level( ) { return _dirty_level; }
    protected:
        virtual void run_thread( );
        void do_fork( );

        char *_cmd;
        unsigned int _dirty_level;
        ShmDoubleBuffer shm_buf;
};

#endif
