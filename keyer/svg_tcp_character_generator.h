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

#ifndef _KEYDAEMON_SVG_TCP_CHARACTER_GENERATOR_H
#define _KEYDAEMON_SVG_TCP_CHARACTER_GENERATOR_H

#include "character_generator.h"

class SvgTcpCharacterGenerator : public CharacterGenerator {
    public:
        SvgTcpCharacterGenerator(int tcp_port);
        ~SvgTcpCharacterGenerator( );

    protected:
        virtual void run_thread(void);
};

#endif
