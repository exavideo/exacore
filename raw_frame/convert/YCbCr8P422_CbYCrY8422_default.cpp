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
#include <stdlib.h>

void YCbCr8P422_CbYCrY8422_default(size_t n, uint8_t *dst, uint8_t *Y,
        uint8_t *Cb, uint8_t *Cr) {

    while (n > 0) {
        *(dst++) = *(Cb++);
        *(dst++) = *(Y++);
        *(dst++) = *(Cr++);
        *(dst++) = *(Y++);
        n -= 4;
    }

}

void YCbCr8P422_CbYCrY8422_A_default(
    size_t w, size_t h, 
    size_t Ypitch, size_t Cbpitch, size_t Crpitch,
    uint8_t *Y, uint8_t *Cb, uint8_t *Cr,
    uint8_t *dst
) {

    Ypitch -= w;
    Cbpitch -= w/2;
    Crpitch -= w/2;

    while (h > 0) {
        for (size_t i = 0; i < w; i += 2) {
            *(dst++) = *(Cb++);
            *(dst++) = *(Y++);
            *(dst++) = *(Cr++);
            *(dst++) = *(Y++);
        }

        /* skip extra bytes on source */
        Y += Ypitch;
        Cb += Cbpitch;
        Cr += Crpitch;

        h--;
    }
}
