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

#include "png_subprocess_character_generator.h"
#include "raw_frame.h"

PngSubprocessCharacterGenerator::PngSubprocessCharacterGenerator(
    const char *cmd, unsigned int dirty_level
) : SubprocessCharacterGenerator(cmd, dirty_level) {

}

RawFrame *PngSubprocessCharacterGenerator::do_render(void *data, size_t size) {
    try {
        return RawFrame::from_png_data(data, size);
    } catch(...) {
        fprintf(stderr, "PngSubprocessCharacterGenerator: "
            "failed to load PNG image from data!\n");
        return NULL;
    }
}

