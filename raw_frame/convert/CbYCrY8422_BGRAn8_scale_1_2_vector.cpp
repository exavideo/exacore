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
#include <stdio.h>

extern "C" void CbYCrY8422_BGRAn8_scale_line_1_2_vector(size_t src_size, 
        uint8_t *src, uint8_t *dst);


void CbYCrY8422_BGRAn8_scale_1_2_vector(size_t n, uint8_t *src,
        uint8_t *dst, unsigned int s_pitch) {
    size_t n_scans = n / s_pitch;
    size_t j = 0;

    while (j < n_scans) {
        CbYCrY8422_BGRAn8_scale_line_1_2_vector(s_pitch, src, dst);
        /* we generate 1/2 the pixels at 2x the size = same number of bytes */
        src += 2*s_pitch;
        dst += s_pitch;

        j += 2;
    }
}
