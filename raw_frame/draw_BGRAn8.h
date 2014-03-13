/*
 * Copyright 2011, 2014 Exavideo LLC.
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

#ifndef _OPENREPLAY_DRAW_BGRAN8_H
#define _OPENREPLAY_DRAW_BGRAN8_H

void BGRAn8_blit_default(RawFrame *bkgd, RawFrame *src, coord_t x, coord_t y);
void BGRAn8_alpha_key_default(RawFrame *bkgd, RawFrame *key, 
        coord_t x, coord_t y, uint8_t galpha);
void BGRAn8_alpha_composite_default(RawFrame *bkgd, RawFrame *key,
        coord_t x, coord_t y, uint8_t galpha,
        coord_t src_x, coord_t src_y,
        coord_t w, coord_t h);

#ifndef SKIP_ASSEMBLY_ROUTINES
void BGRAn8_alpha_composite_sse2(RawFrame *bkgd, RawFrame *key,
        coord_t x, coord_t y, uint8_t galpha,
        coord_t src_x, coord_t src_y,
        coord_t w, coord_t h);
#endif

class BGRAn8DrawOps : public RawFrameDrawOps {
    public:
        BGRAn8DrawOps(RawFrame *f_) : RawFrameDrawOps(f_) {
            do_blit = BGRAn8_blit_default;
            do_alpha_blend = BGRAn8_alpha_key_default;
#ifndef SKIP_ASSEMBLY_ROUTINES
            if (cpu_sse2_available( )) {
                do_alpha_composite = BGRAn8_alpha_composite_sse2;
            } else {
                do_alpha_composite = BGRAn8_alpha_composite_default;
            }
#else
            do_alpha_composite = BGRAn8_alpha_composite_default;
#endif
        }
};

#endif

