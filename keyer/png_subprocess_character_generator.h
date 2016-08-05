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

#ifndef _KEYDAEMON_PNG_SUBPROCESS_CHARACTER_GENERATOR_H
#define _KEYDAEMON_PNG_SUBPROCESS_CHARACTER_GENERATOR_H

#include "subprocess_character_generator.h"

class PngSubprocessCharacterGenerator : public SubprocessCharacterGenerator {
    public:
        PngSubprocessCharacterGenerator(const char *cmd, 
                unsigned int dirty_level = 0);
    protected:
        RawFrame *do_render(void *data, size_t size);
};

#endif
