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

#include "svg_subprocess_character_generator.h"
#include "raw_frame.h"

int main(int argc, char **argv) {
    int i;
    RawFrame *out = NULL;
    SvgSubprocessCharacterGenerator cg("scoreboard/scoreboard_cg.rb");

    UNUSED(argc);
    UNUSED(argv);

    for (i = 0; i < 30; i++) {
        if (cg.output_pipe( ).get(out) == 0) {
            fprintf(stderr, "ruby died??\n");
        } else if (out == NULL) {
            fprintf(stderr, "no frame??\n");
        } else {
            fprintf(stderr, "w=%d h=%d\n", out->w( ), out->h( ));
            out->write_to_fd(STDOUT_FILENO);
        }
    }
}
