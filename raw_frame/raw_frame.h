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
#include <stdexcept>

class RawFramePacker;
class RawFrameUnpacker;
class RawFrameDrawOps;

class RawFrame {
    public:
        enum PixelFormat { 
            UNDEF,
            RGB8, YCbCr8, CbYCrY8422, 
            RGBAn8, BGRAn8, YCbCrAn8 
        };
        RawFrame(coord_t w, coord_t h, PixelFormat pf);
        RawFrame(coord_t w, coord_t h, PixelFormat pf, size_t scanline_size);
        virtual ~RawFrame( );

        uint8_t *scanline(coord_t y) { return _data + _pitch * y; }
        uint8_t *data( ) { return _data; }

        uint8_t global_alpha( ) { return _global_alpha; }
        void set_global_alpha(uint8_t a) { _global_alpha = a; }
        
        coord_t w( ) const { return _w; }
        coord_t h( ) const { return _h; }
        /* This is "scanline pitch" or the size in bytes of one scanline */
        size_t pitch( ) const { return _pitch; }
        /* This is the size of a whole frame */
        size_t size( ) const { return _pitch * _h; }
        PixelFormat pixel_format( ) const { return _pixel_format; }

#ifdef RAWFRAME_POSIX_IO
        ssize_t read_from_fd(int fd);
        ssize_t write_to_fd(int fd);
#endif

        RawFramePacker *pack;
        RawFrameUnpacker *unpack;
        RawFrameDrawOps *draw;

    protected:
        coord_t _w, _h;
        size_t _pitch;
        uint8_t *_data;
        uint8_t _global_alpha;
        PixelFormat _pixel_format;

        RawFrame( );
        RawFrame(PixelFormat pf);

        void initialize_pf(PixelFormat pf);
        size_t minpitch( ) const;
        virtual void alloc( );
        virtual void free_data( );
        void make_packer( );
        void make_unpacker( );
        void make_draw_ops( );
};

#define CHECK(x) check((void *)(x))

class RawFramePacker {
    public:
        RawFramePacker(RawFrame *f_) : f(f_) {
            do_YCbCr8P422 = NULL;
        }

        void YCbCr8P422(uint8_t *Y, uint8_t *Cb, uint8_t *Cr) {
            CHECK(do_YCbCr8P422);
            do_YCbCr8P422(f->size( ), f->data( ), Y, Cb, Cr);
        }

    protected:
        void check(void *ptr) {
            if (ptr == NULL) {
                throw std::runtime_error("Conversion not supported");
            }
        }

        RawFrame *f;

        void (*do_YCbCr8P422)(size_t, uint8_t *, uint8_t *, 
                uint8_t *, uint8_t *);
};

class RawFrameUnpacker {
    public:
        RawFrameUnpacker(RawFrame *f_) : f(f_) { 
            do_YCbCr8P422 = NULL;
            do_CbYCrY8422 = NULL;
        }
    
        /* TODO: provide routines for each desired output format here! */
        void YCbCr8P422(uint8_t *Y, uint8_t *Cb, uint8_t *Cr) {
            CHECK(do_YCbCr8P422);
            do_YCbCr8P422(f->size( ), f->data( ), Y, Cb, Cr);
        }

        void CbYCrY8422(uint8_t *data) {
            CHECK(do_CbYCrY8422);
            do_CbYCrY8422(f->size( ), f->data( ), data);
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
        void (*do_YCbCr8P422)(size_t, uint8_t *, uint8_t *, 
                uint8_t *, uint8_t *);
        void (*do_CbYCrY8422)(size_t, uint8_t *, uint8_t *);
};

class RawFrameDrawOps {
    public:
        RawFrameDrawOps(RawFrame *f_) : f(f_) { 
            do_alpha_blend = NULL;
        }

        void alpha_key(coord_t x, coord_t y, RawFrame *key, 
                uint8_t galpha) {
            CHECK(do_alpha_blend);
            do_alpha_blend(f, key, x, y, galpha);
        }
    protected:
        void check(void *ptr) {
            if (ptr == NULL) {
                throw std::runtime_error("Conversion not supported");
            }
        }

        void (*do_alpha_blend)(RawFrame *bkgd, RawFrame *key, 
                coord_t x, coord_t y, uint8_t galpha);

        RawFrame *f;

};

#undef CHECK

#endif
