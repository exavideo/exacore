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

#ifndef _OPENREPLAY_UNPACK_CBYCRY8422_H
#define _OPENREPLAY_UNPACK_CBYCRY8422_H

#include "raw_frame.h"
#include "cpu_dispatch.h"

void CbYCrY8422_YCbCr8P422_default(size_t, uint8_t *, uint8_t *, 
        uint8_t *, uint8_t *);

void CbYCrY8422_CbYCrY8422_default(size_t, uint8_t *, uint8_t *);

#ifndef SKIP_ASSEMBLY_ROUTINES
extern "C" {
    void CbYCrY8422_YCbCr8P422_sse3(size_t, uint8_t *, uint8_t *, 
            uint8_t *, uint8_t *);
    void CbYCrY8422_YCbCr8P422_ssse3(size_t, uint8_t *, uint8_t *, 
            uint8_t *, uint8_t *);
}
#endif

void CbYCrY8422_BGRAn8_default(size_t, uint8_t *, uint8_t *);
void CbYCrY8422_BGRAn8_scale_1_2_default(size_t, uint8_t *, 
        uint8_t *, unsigned int);
void CbYCrY8422_BGRAn8_scale_1_4_default(size_t, uint8_t *, 
        uint8_t *, unsigned int);

void CbYCrY8422_CbYCrY8422_scan_double(size_t, uint8_t *,
        uint8_t *, unsigned int);

void CbYCrY8422_CbYCrY8422_scale_1_4(size_t, uint8_t *,
        uint8_t *, unsigned int);

void CbYCrY8422_CbYCrY8422_scan_triple(size_t, uint8_t *,
        uint8_t *, unsigned int);

class CbYCrY8422Unpacker : public RawFrameUnpacker {
    public:
        CbYCrY8422Unpacker(RawFrame *f) : RawFrameUnpacker(f) {
            /* CPU dispatched routines */
#ifdef SKIP_ASSEMBLY_ROUTINES
            do_YCbCr8P422 = CbYCrY8422_YCbCr8P422_default;
#else
            if (cpu_ssse3_available( )) {
                do_YCbCr8P422 = CbYCrY8422_YCbCr8P422_ssse3;
            } else if (cpu_sse3_available( )) {
                do_YCbCr8P422 = CbYCrY8422_YCbCr8P422_sse3;
            } else {
                do_YCbCr8P422 = CbYCrY8422_YCbCr8P422_default;
            }
            do_YCbCr8P422 = CbYCrY8422_YCbCr8P422_default;
#endif
            
            /* Non CPU-dispatched routines */
            do_CbYCrY8422 = CbYCrY8422_CbYCrY8422_default;
            do_BGRAn8 = CbYCrY8422_BGRAn8_default;
            do_BGRAn8_scale_1_2 = CbYCrY8422_BGRAn8_scale_1_2_default;
            do_BGRAn8_scale_1_4 = CbYCrY8422_BGRAn8_scale_1_4_default;
            do_CbYCrY8422_scan_double = CbYCrY8422_CbYCrY8422_scan_double;
            do_CbYCrY8422_scale_1_4 = CbYCrY8422_CbYCrY8422_scale_1_4;
            do_CbYCrY8422_scan_triple = CbYCrY8422_CbYCrY8422_scan_triple;
        }
};

#endif
