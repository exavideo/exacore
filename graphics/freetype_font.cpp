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
#include <assert.h>

FT_Library FreetypeFont::library = NULL;

static void FTCHK(FT_Error err) {
    if (err != 0) {
        throw FreetypeError( );
    }
}

FreetypeFont::FreetypeFont(const char *font_file, FT_Long index) {
    if (library == NULL) {
        do_library_init( );
    }
    _h = 0;
    face = NULL;
    FTCHK(FT_New_Face(library, font_file, index, &face));

    rb = gb = bb = ab = 0;
    rf = gf = bf = af = 255;
}

void FreetypeFont::set_size(unsigned int n_pixels) {
    /* 
     * FreeType expects the size to be in units of 1/64 points.
     * We will get it to use pixels instead by telling it that the dpi is 72.
     * Thus, 1 point = 1/72 in = 1 pixel.
     * We must then multiply the n_pixels passed in by 64 to get into units of
     * 1/64 pixels = 1/64 points.
     */
    FTCHK(FT_Set_Char_Size(face, 0, n_pixels * 64, 72, 72));
    _h = face->size->metrics.height / 64;
    _baseline = _h + face->size->metrics.descender / 64;
}

void FreetypeFont::set_fgcolor(int r, int g, int b, int a) {
    rf = r;
    gf = g;
    bf = b;
    af = a;
}

void FreetypeFont::set_bgcolor(int r, int g, int b, int a) {
    rb = r;
    gb = g;
    bb = b;
    ab = a;
}

RawFrame *FreetypeFont::render_string(const char *string) {
    int x;
    RawFrame *ret;
    FT_GlyphSlot slot = face->glyph;
    FT_Bool use_kerning = FT_HAS_KERNING(face);
    FT_UInt glyph_index, previous;

    uint8_t *glyph_scanline;

    x = 0;
    previous = 0;

    /* first compute the size of the resulting image */
    const char *scan_ptr = string;
    while (*scan_ptr != '\0') {
        glyph_index = FT_Get_Char_Index(face, *scan_ptr);
        scan_ptr++;

        if (use_kerning && previous != 0 && glyph_index != 0) {
            FT_Vector delta;
            FT_Get_Kerning(face, previous, glyph_index, 
                    FT_KERNING_DEFAULT, &delta);
            x += delta.x / 64;
        }

        FTCHK(FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT));

        x += slot->advance.x / 64;

        previous = glyph_index;
    }

    /* initialize a raw frame */
    ret = new RawFrame(x, _h, RawFrame::BGRAn8);
    
    /* second pass: draw it */
    scan_ptr = string;
    int xd = 0;
    previous = 0;
    uint8_t *dest_scanline = ret->data( );

    for (unsigned int i = 0; i < ret->size( ); i += 4) {
        dest_scanline[i] = bb;
        dest_scanline[i+1] = gb;
        dest_scanline[i+2] = rb;
        dest_scanline[i+3] = ab;
    }

    while (*scan_ptr != '\0') {
        glyph_index = FT_Get_Char_Index(face, *scan_ptr);
        scan_ptr++;

        if (use_kerning && previous != 0 && glyph_index != 0) {
            FT_Vector delta;
            FT_Get_Kerning(face, previous, glyph_index,
                    FT_KERNING_DEFAULT, &delta);
            xd += delta.x / 64;
        }

        FTCHK(FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER));

        //int yd = -(slot->bitmap_top);
        int yd = _baseline - slot->bitmap_top;
        for (int y = 0; y < slot->bitmap.rows && yd < _h; y++, yd++) {
            if (yd >= 0) {
                glyph_scanline = ((uint8_t *)slot->bitmap.buffer) 
                        + slot->bitmap.pitch * y;
                dest_scanline = ret->scanline(yd) + 4*xd;
                int xd2 = xd;
                for (int x = 0; x < slot->bitmap.width && xd2 < ret->w( ); 
                        x++, xd2++) {

                    dest_scanline[0] = (bf * glyph_scanline[x] 
                            + bb * (255 - glyph_scanline[x])) / 255;
                    dest_scanline[1] = (gf * glyph_scanline[x]
                            + gb * (255 - glyph_scanline[x])) / 255;
                    dest_scanline[2] = (rf * glyph_scanline[x]
                            + rb * (255 - glyph_scanline[x])) / 255;
                    dest_scanline[3] = (af * glyph_scanline[x]
                            + ab * (255 - glyph_scanline[x])) / 255;
                    dest_scanline += 4;
                }
            }
        }

        xd += slot->advance.x / 64;
        previous = glyph_index;
    }

    return ret;
}

void FreetypeFont::do_library_init( ) {
    FTCHK(FT_Init_FreeType(&library));
}

FreetypeFont::~FreetypeFont( ) {
    /* FIXME this leaks */
}

