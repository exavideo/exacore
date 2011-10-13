/*
 * Copyright 2011 Andrew H. Armenia.
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


#ifndef _OPENREPLAY_DRAW_CBYCRY8422_H
#define _OPENREPLAY_DRAW_CBYCRY8422_H

#include "raw_frame.h"

void CbYCrY8422_alpha_key_default(RawFrame *bkgd, RawFrame *key,
        coord_t x, coord_t y, uint8_t galpha);


#ifndef SKIP_ASSEMBLY_ROUTINES 
void CbYCrY8422_alpha_key_sse2(RawFrame *bkgd, RawFrame *key, 
        coord_t x, coord_t y, uint8_t galpha);
#endif

class CbYCrY8422DrawOps : public RawFrameDrawOps {
    public:
        CbYCrY8422DrawOps(RawFrame *f_) : RawFrameDrawOps(f_) {

#ifdef SKIP_ASSEMBLY_ROUTINES
            do_alpha_blend = CbYCrY8422_alpha_key_default;
#else
            if (cpu_sse3_available( )) {
                do_alpha_blend = CbYCrY8422_alpha_key_sse2;
            } else {
                do_alpha_blend = CbYCrY8422_alpha_key_default;
            }
#endif
        }
};

#endif
