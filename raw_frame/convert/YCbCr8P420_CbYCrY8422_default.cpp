/*
 * Copyright 2013 Exavideo LLC.
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

void YCbCr8P420_CbYCrY8422_A_default(
    size_t w, size_t h,
    size_t Ypitch, size_t Cbpitch, size_t Crpitch,
    uint8_t *Y, uint8_t *Cb, uint8_t *Cr, uint8_t *dst
) {
    uint8_t *sY, *sCb, *sCr;
    unsigned int i;

    while (h-- > 0) {
        sY = Y;
        sCb = Cb;
        sCr = Cr;

        for (i = 0; i < w; i += 2) {
            *(dst++) = *(sCb++);
            *(dst++) = *(sY++);
            *(dst++) = *(sCr++);
            *(dst++) = *(sY++);
        }

        Y += Ypitch;
        /* move Cb and Cr to next line only every other output line */
        if (h & 0x01) {
            Cb += Cbpitch;
            Cr += Crpitch;
        }
    }
}
