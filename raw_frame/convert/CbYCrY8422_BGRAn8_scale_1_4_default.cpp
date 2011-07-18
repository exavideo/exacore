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

#include <stdint.h>
#include <stddef.h>
#include "clamp.h"

void CbYCrY8422_BGRAn8_scale_1_4_default(size_t n, uint8_t *src, 
        uint8_t *dst, unsigned int s_pitch) {
    int32_t cb, y1, cr;
    int32_t r, g, b;
    
    uint8_t *src_scan;

    while (n > 0) {
        src_scan = src;
        for (unsigned int i = 0; i < s_pitch; i += 8) {
            cb = src_scan[0];
            y1 = src_scan[1];
            cr = src_scan[2];

            y1 -= 16;
            cb -= 128;
            cr -= 128;

            y1 *= 298;

            r = y1 + 459 * cr;
            r /= 256;

            g = y1 - 55 * cb - 136 * cr;
            g /= 256;

            b = y1 + 541 * cb;
            b /= 256;

            dst[0] = CLAMP(b);
            dst[1] = CLAMP(g);
            dst[2] = CLAMP(r);
            dst[3] = 0xff;

            dst += 4;
            src_scan += 8;
        }

        src += 4*s_pitch;
        n -= 4*s_pitch;
    }
}
