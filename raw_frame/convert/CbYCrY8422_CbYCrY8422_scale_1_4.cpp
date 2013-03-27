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

#include <stddef.h>
#include <stdint.h>

static void CbYCrY8422_quarter_scanline(uint8_t *dst, uint8_t *src, 
        size_t src_length) {
    uint8_t y1, y2, cb, cr;
    size_t i, dp;

    dp = 0;

    for (i = 0; i < src_length; i += 16) {
        cb = src[i];
        y1 = src[i+1];
        cr = src[i+2];
        y2 = src[i+9];

        dst[dp+0] = cb;
        dst[dp+1] = y1;
        dst[dp+2] = cr;
        dst[dp+3] = y2;

        dp += 4;
    }
}

void CbYCrY8422_CbYCrY8422_scale_1_4(size_t src_size, 
        uint8_t *src, uint8_t *dst, unsigned int src_pitch) {
    size_t n_scanlines = src_size / src_pitch;
    size_t i;

    for (i = 0; i < n_scanlines; i += 4) {
        CbYCrY8422_quarter_scanline(dst, src, src_pitch);
        src += 4*src_pitch;
        dst += src_pitch / 4;
    }
}

