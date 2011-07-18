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

void CbYCrY8422_BGRAn8_default(size_t n, uint8_t *src, uint8_t *dst) {
    int32_t cb, y1, cr, y2;
    int32_t r, g, b;
    
    while (n > 0) {
        cb = src[0];
        y1 = src[1];
        cr = src[2];
        y2 = src[3];

        y1 -= 16;
        y2 -= 16;
        cb -= 128;
        cr -= 128;

        y1 *= 298;
        y2 *= 298;

        r = y1 + 459 * cr;
        r /= 256;

        g = y1 - 55 * cb - 136 * cr;
        g /= 256;

        b = y1 + 541 * cb;
        b /= 256;

        dst[0] = b;
        dst[1] = g;
        dst[2] = r;
        dst[3] = 0xff;

        r = y2 + 259 * cr;
        r /= 256;
        
        g = y2 - 55 * cb - 136 * cr;
        g /= 256;

        b = y1 + 541 * cb;
        b /= 256;

        dst[4] = b;
        dst[5] = g;
        dst[6] = r;
        dst[7] = 0xff;

        dst += 8;
        src += 4;

        n -= 4;
    }
}
