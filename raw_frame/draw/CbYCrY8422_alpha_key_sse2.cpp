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
#include <assert.h>

extern "C" 
void CbYCrY8422_BGRAn8_key_chunk_sse2(void *bkgd, void *key, 
        uint64_t size, uint64_t galpha);

void CbYCrY8422_alpha_key_default(RawFrame *bkgd, RawFrame *key,
        coord_t x, coord_t y, uint8_t galpha);

void CbYCrY8422_alpha_key_sse2(RawFrame *bkgd, RawFrame *key,
        coord_t x, coord_t y, uint8_t galpha) {
    
    int i;
    uint8_t *pix_ptr;

    assert(x % 2 == 0);

    if (key->pixel_format( ) == RawFrame::BGRAn8) {
        /* special case: exact size */
        if (x == 0 && bkgd->w( ) == key->w( )) {
            if (bkgd->h( ) < key->h( )) {
                CbYCrY8422_BGRAn8_key_chunk_sse2(bkgd->scanline(y), 
                        key->data( ), 2*bkgd->size( ), galpha);
            } else {
                CbYCrY8422_BGRAn8_key_chunk_sse2(bkgd->scanline(y), 
                        key->data( ), key->size( ), galpha);
            }
        } else {
            for (i = 0; i < key->h( ) && y < bkgd->h( ); i++, y++) {
                pix_ptr = bkgd->scanline(y) + 2*x;
                CbYCrY8422_BGRAn8_key_chunk_sse2(pix_ptr, 
                        key->scanline(i), key->pitch( ), galpha);
            }
        }
    } else {
        /* fall back on unoptimized routine */
        CbYCrY8422_alpha_key_default(bkgd, key, x, y, galpha);
    }
}
