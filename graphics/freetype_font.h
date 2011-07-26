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

#ifndef _OPENREPLAY_FREETYPE_FONT_H
#define _OPENREPLAY_FREETYPE_FONT_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include "raw_frame.h"
#include <stdexcept>

class FreetypeError : public virtual std::exception {
    public:
        const char *what( ) { return "FreeType error"; }
};

class FreetypeFont {
    public:
        FreetypeFont(const char *font_file, FT_Long index=0);
        ~FreetypeFont( );

        void set_size(unsigned int n_pixels);
        void set_fgcolor(int r, int g, int b, int a);
        void set_bgcolor(int r, int g, int b, int a); 
        RawFrame *render_string(const char *string);
    private:
        static void do_library_init( );
        static FT_Library library;
        FT_Face face;
        int _h, _baseline;

        int rb, gb, bb, ab;
        int rf, gf, bf, af;
};

#endif
