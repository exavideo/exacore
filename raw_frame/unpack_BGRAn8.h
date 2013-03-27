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

#ifndef _UNPACK_BGRAN8_H
#define _UNPACK_BGRAN8_H

void BGRAn8_BGRAn8_default(size_t, uint8_t *src, uint8_t *dst);

class BGRAn8Unpacker : public RawFrameUnpacker {
    public:
        BGRAn8Unpacker(RawFrame *f) : RawFrameUnpacker(f) {
            do_BGRAn8 = BGRAn8_BGRAn8_default;
        }
};

#endif
