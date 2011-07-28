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

#include "freetype_font.h"
#include <unistd.h>

int main( ) {
    FreetypeFont droid_sans("../fonts/Inconsolata.otf");
    droid_sans.set_size(20);
    droid_sans.set_fgcolor(255, 255, 255, 255);
    droid_sans.set_bgcolor(0, 0, 0, 192);
    RawFrame *text = droid_sans.render_string("01:02:03:04 5/6");

    text->write_tga_to_fd(STDOUT_FILENO);
    delete text;
}
