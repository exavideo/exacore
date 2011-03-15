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

#ifndef _OPENREPLAY_UNPACK_UYVY_H
#define _OPENREPLAY_UNPACK_UYVY_H

#include "raw_frame.h"
#include "cpu_dispatch.h"

void CbYCrY8422_YCbCr8P422_default(size_t, uint8_t *, uint8_t *, 
        uint8_t *, uint8_t *);

void CbYCrY8422_CbYCrY8422_default(size_t, uint8_t *, uint8_t *);

extern "C" {
    void CbYCrY8422_YCbCr8P422_sse3(size_t, uint8_t *, uint8_t *, 
            uint8_t *, uint8_t *);
    void CbYCrY8422_YCbCr8P422_ssse3(size_t, uint8_t *, uint8_t *, 
            uint8_t *, uint8_t *);
}


class CbYCrY8422Unpacker : public RawFrameUnpacker {
    public:
        CbYCrY8422Unpacker(RawFrame *f) : RawFrameUnpacker(f) {
            /* CPU dispatched routines */
            if (cpu_ssse3_available( )) {
                do_YCbCr8P422 = CbYCrY8422_YCbCr8P422_ssse3;
            } else if (cpu_sse3_available( )) {
                do_YCbCr8P422 = CbYCrY8422_YCbCr8P422_sse3;
            } else {
                do_YCbCr8P422 = CbYCrY8422_YCbCr8P422_default;
            }
            
            /* Non CPU-dispatched routines */
            do_CbYCrY8422 = CbYCrY8422_CbYCrY8422_default;
        }
};

#endif
