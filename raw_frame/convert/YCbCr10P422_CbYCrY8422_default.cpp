/*
 * Copyright 2011, 2016 Exavideo LLC.
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

void YCbCr10P422_CbYCrY8422_A_default(
    size_t w, size_t h, 
    size_t Ypitch, size_t Cbpitch, size_t Crpitch,
    uint16_t *Y, uint16_t *Cb, uint16_t *Cr,
    uint8_t *dst
) {

    Ypitch -= w;
    Cbpitch -= w/2;
    Crpitch -= w/2;

    while (h > 0) {
        for (size_t i = 0; i < w; i += 2) {
            *(dst++) = *(Cb++) >> 2;
            *(dst++) = *(Y++) >> 2;
            *(dst++) = *(Cr++) >> 2;
            *(dst++) = *(Y++) >> 2;
        }

        /* skip extra bytes on source */
        Y += Ypitch;
        Cb += Cbpitch;
        Cr += Crpitch;

        h--;
    }
}
