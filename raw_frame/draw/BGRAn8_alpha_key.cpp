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

#ifndef SKIP_ASSEMBLY_ROUTINES
static void BGRAn8_BGRAn8_composite_aligned_sse2(
		RawFrame *dst, RawFrame *src,
		coord_t dst_x, coord_t dst_y,
		uint8_t galpha,
		coord_t src_x, coord_t src_y,
		coord_t w, coord_t h
);

extern "C" void BGRAn8_BGRAn8_composite_chunk_sse2(
	uint8_t *dst, uint8_t *src, size_t count, 
	unsigned int global_alpha
);
#endif

static void fixup_width_height(
	RawFrame *dst, RawFrame *src,
	coord_t dst_x, coord_t dst_y,
	coord_t src_x, coord_t src_y,
	coord_t &w, coord_t &h
) {
	if (src_x + w > src->w( )) {
		w = src->w( ) - src_x;
	}

	if (dst_x + w > dst->w( )) {
		w = dst->w( ) - dst_x;
	}

	if (src_y + h > src->h( )) {
		h = src->h( ) - src_y;
	}

	if (dst_y + h > dst->h( )) {
		h = dst->h( ) - dst_y;
	}
}

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
	
	fixup_width_height(
		bkgd, key,
		x, y,
		src_x, src_y,
		w, h
	);

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

#ifndef SKIP_ASSEMBLY_ROUTINES
void BGRAn8_alpha_composite_sse2(RawFrame *bkgd, RawFrame *key,
		coord_t x, coord_t y, uint8_t galpha,
		coord_t src_x, coord_t src_y,
		coord_t w, coord_t h) {

	coord_t aligned_w;

	fixup_width_height(
		bkgd, key,
		x, y,
		src_x, src_y,
		w, h
	);

	switch (key->pixel_format( )) {
		case RawFrame::BGRAn8:
			/* 
			 * sse2 routine only operates if w is a multiple of 4.
			 * Use it first, then pick up the pieces with the slow
			 * routine.
			 */
			if (w > 4) {
				aligned_w = (w/4)*4;
				BGRAn8_BGRAn8_composite_aligned_sse2(
					bkgd, key, x, y, galpha,
					src_x, src_y, aligned_w, h
				);

				w -= aligned_w;
				x += aligned_w;
				src_x += aligned_w;
			}

			if (w != 0) {
				BGRAn8_BGRAn8_composite(
					bkgd, key, x, y, galpha, 
					src_x, src_y, w, h
				);
			}
			break;
		default:
			throw std::runtime_error("unsupported pixel formats");
	}
}

static void BGRAn8_BGRAn8_composite_aligned_sse2(
		RawFrame *dst, RawFrame *src,
		coord_t dst_x, coord_t dst_y,
		uint8_t galpha,
		coord_t src_x, coord_t src_y,
		coord_t w, coord_t h
) {
	uint8_t *src_scanline;
	uint8_t *dst_scanline;

	while (h > 0) {
		src_scanline = src->scanline(src_y) + 4*src_x;
		dst_scanline = dst->scanline(dst_y) + 4*dst_x;

		BGRAn8_BGRAn8_composite_chunk_sse2( 
			dst_scanline, src_scanline, 
			4*w, galpha
		);

		src_y++;
		dst_y++;
		h--;
	}

}
#endif

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

	float rb, gb, bb, ab;
	float rk, gk, bk, ak;
	float ag = galpha / 255.0f;

	for (int yd = dst_y, ys = src_y; ys < src_y + h; yd++, ys++) {
		src_scanline = src->scanline(ys) + 4*src_x;
		dst_scanline = dst->scanline(yd) + 4*dst_x;

		for (int xd = dst_x, xs = src_x; xs < src_x + w; xd++, xs++) {
			ak = src_scanline[3] * ag / 255.0f;
			
			bk = src_scanline[0] * ak;
			gk = src_scanline[1] * ak;
			rk = src_scanline[2] * ak;

			ab = dst_scanline[3] / 255.0f;
			ab -= ab * ak;

			bb = dst_scanline[0] * ab;
			gb = dst_scanline[1] * ab;
			rb = dst_scanline[2] * ab;

			ab += ak;

			dst_scanline[0] = (bk + bb) / ab;
			dst_scanline[1] = (gk + gb) / ab;
			dst_scanline[2] = (rk + rb) / ab;
			dst_scanline[3] = ab * 255.0;

			src_scanline += 4;
			dst_scanline += 4;
		}
	}

}
