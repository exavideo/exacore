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

#include "raw_frame.h"

static void BGRAn8_BGRAn8_key(RawFrame *bkgd, RawFrame *key,
        coord_t x, coord_t y, uint8_t galpha);

void BGRAn8_alpha_key_default(RawFrame *bkgd, RawFrame *key,
        coord_t x, coord_t y, uint8_t galpha) {
    switch (key->pixel_format( )) {
        case RawFrame::BGRAn8:
            BGRAn8_BGRAn8_key(bkgd, key, x, y, galpha);
            break;
        default:
            throw std::runtime_error("unsupported pixel formats");
    }
}

static void BGRAn8_BGRAn8_key(RawFrame *bkgd, RawFrame *key,
        coord_t x, coord_t y, uint8_t galpha) {

    uint8_t *src_scanline;
    uint8_t *dst_scanline;

    int rb, gb, bb;
    int rk, gk, bk, ak;

    for (int yd = y, ys = 0; yd < bkgd->h( ) && ys < key->h( ); yd++, ys++) {
        src_scanline = key->scanline(ys);
        dst_scanline = bkgd->scanline(yd) + 4*x;
        for (int xd = x, xs = 0; xd < bkgd->w( ) && xs < key->w( ); 
                xd++, xs++) {
            bk = src_scanline[0];
            gk = src_scanline[1];
            rk = src_scanline[2];
            ak = src_scanline[3];

            ak = ak * galpha / 255;

            bb = dst_scanline[0];
            gb = dst_scanline[1];
            rb = dst_scanline[2];

            dst_scanline[0] = (bk * ak + bb * (255 - ak)) / 255;
            dst_scanline[1] = (gk * ak + gb * (255 - ak)) / 255;
            dst_scanline[2] = (rk * ak + rb * (255 - ak)) / 255;

            src_scanline += 4;
            dst_scanline += 4;
        }
    }
}
