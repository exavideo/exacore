/*
 * Copyright 2011, 2014 Exavideo LLC.
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

static void BGRAn8_BGRAn8_composite(
		RawFrame *dst, RawFrame *src,
		coord_t dst_x, coord_t dst_y,
		uint8_t galpha,
		coord_t src_x, coord_t src_y,
		coord_t w, coord_t h
);

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

void BGRAn8_alpha_composite_default(RawFrame *bkgd, RawFrame *key,
		coord_t x, coord_t y, uint8_t galpha,
		coord_t src_x, coord_t src_y,
		coord_t w, coord_t h) {
	
	switch (key->pixel_format( )) {
		case RawFrame::BGRAn8:
			BGRAn8_BGRAn8_composite(
				bkgd, key, x, y, galpha, 
				src_x, src_y, w, h
			);
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

static void BGRAn8_BGRAn8_composite(
		RawFrame *dst, RawFrame *src,
		coord_t dst_x, coord_t dst_y,
		uint8_t galpha,
		coord_t src_x, coord_t src_y,
		coord_t w, coord_t h
) {
	uint8_t *src_scanline;
	uint8_t *dst_scanline;

	int rb, gb, bb, ab;
	int rk, gk, bk, ak;

	coord_t act_w, act_h;

	act_w = w;

	if (src_x + w > src->w( )) {
		act_w = src->w( ) - src_x;
	}

	if (dst_x + w > dst->w( )) {
		act_w = dst->w( ) - dst_x;
	}

	act_h = h;

	if (src_y + h > src->h( )) {
		act_h = src->h( ) - src_y;
	}

	if (dst_y + h > dst->h( )) {
		act_h = dst->h( ) - dst_y;
	}

	if (src_x > src->w( )) {
		fprintf(stderr, "src_x > src w...?\n");
		return;
	}

	if (src_y > src->h( )) {
		fprintf(stderr, "src_y > src h...?\n");
		return;
	}
	
	for (int yd = dst_y, ys = src_y; ys < src_y + act_h; yd++, ys++) {
		src_scanline = src->scanline(ys) + 4*src_x;
		dst_scanline = dst->scanline(yd) + 4*dst_x;

		for (int xd = dst_x, xs = src_x; xs < src_x + act_w; 
				xd++, xs++) {

			bk = src_scanline[0];
			gk = src_scanline[1];
			rk = src_scanline[2];
			ak = src_scanline[3];

			ak = ak * galpha / 255;

			bb = dst_scanline[0];
			gb = dst_scanline[1];
			rb = dst_scanline[2];
			ab = dst_scanline[3];

			/* note that here the alpha is blended between the two as well */
			dst_scanline[0] = (bk * ak + bb * (255 - ak)) / 255;
			dst_scanline[1] = (gk * ak + gb * (255 - ak)) / 255;
			dst_scanline[2] = (rk * ak + rb * (255 - ak)) / 255;
			dst_scanline[3] = (ak * ak + ab * (255 - ak)) / 255;

			src_scanline += 4;
			dst_scanline += 4;
		}
	}

}
