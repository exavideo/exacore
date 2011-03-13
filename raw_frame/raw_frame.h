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
#ifndef _OPENREPLAY_RAW_FRAME_H
#define _OPENREPLAY_RAW_FRAME_H
    
#include "types.h"

class RawFrameUnpacker;

class RawFrame {
    public:
        enum PixelFormat { RGB8, YUV8, UYVY8, RGBA8, BGRA8, YUVA8 };
        RawFrame(coord_t w, coord_t h, PixelFormat pf);
        RawFrame(coord_t w, coord_t h, PixelFormat pf, size_t scanline_size);
        ~RawFrame( );

        uint8_t *scanline(coord_t y) { return _data + _pitch * y; }
        uint8_t *data( ) { return _data; }
        
        const coord_t w( ) const { return _w; }
        const coord_t h( ) const { return _h; }
        /* This is "scanline pitch" or the size in bytes of one scanline */
        const size_t pitch( ) const { return _pitch; }
        /* This is the size of a whole frame */
        const size_t size( ) const { return _pitch * _h; }

#ifdef RAWFRAME_POSIX_IO
        void read_from_fd(int fd);
        void write_to_fd(int fd);
#endif

        RawFrameUnpacker *unpack;

    protected:
        coord_t _w, _h;
        size_t _pitch;
        uint8_t *_data;

        size_t minpitch( ) const;
        void alloc( );
        void make_unpacker( );
};

class RawFrameUnpacker {
    public:
        RawFrameUnpacker(RawFrame *f_) : f(f_) { 
            do_YCbCr422p = NULL;
        }
    
        /* TODO: provide routines for each desired output format here! */
        void YCbCr422p(uint8_t *Y, uint8_t *Cb, uint8_t *Cr) {
            check(do_YCbCr422p);
            do_YCbCr422p(f->size( ), f->data( ), Y, Cb, Cr);
        }
    protected:
        void check(void *ptr) {
            if (ptr == NULL) {
                throw std::runtime_error("Conversion not supported");
            }
        }

        RawFrame *f;
        /*
         * These get filled in with pointers to the "meat" of the operation.
         * This part is likely implemented in SSE3 assembly.
         */

        /* do_YCbCr422p(in_size, packed_data, y, cb, cr */
        void (*do_YCbCr422p)(size_t, uint8_t *, uint8_t *, uint8_t *, uint8_t *);
};

#endif
