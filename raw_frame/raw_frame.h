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
#include <stdio.h>

class RawFramePacker;
class RawFrameUnpacker;
class RawFrameDrawOps;
class RawFrameConverter;

class RawFrame {
    public:
        enum PixelFormat { 
            UNDEF,
            RGB8, YCbCr8, CbYCrY8422, 
            RGBAn8, BGRAn8, YCbCrAn8 
        };

        enum FieldDominance {
            UNKNOWN,
            TOP_FIELD_FIRST,
            BOTTOM_FIELD_FIRST,
            PROGRESSIVE,
        };

        RawFrame(coord_t w, coord_t h, PixelFormat pf);
        RawFrame(coord_t w, coord_t h, PixelFormat pf, size_t scanline_size);
        virtual ~RawFrame( );

        RawFrame *copy( );

        uint8_t *scanline(coord_t y) { return _data + _pitch * y; }
        uint8_t *pixel(coord_t x, coord_t y) { 
            if (x >= _w || y >= _h) {
                throw std::runtime_error("invalid pixel access");
            }
            return scanline(y) + pixel_size( ) * x; 
        }
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

        FieldDominance field_dominance( ) const { return _field_dominance; }
        void set_field_dominance(FieldDominance fd) { _field_dominance = fd; }

    	static RawFrame *from_image_file(const char *path);
        static RawFrame *from_png_data(void *data, size_t size);
	    static RawFrame *from_tga_data(const void *data, size_t size);

        bool preview_tally(unsigned int source);
        bool program_tally(unsigned int source);

        uint64_t preview_tally_bits();
        uint64_t program_tally_bits();

        void set_preview_tally(unsigned int source);
        void set_program_tally(unsigned int source);

#ifdef RAWFRAME_POSIX_IO
        ssize_t read_from_fd(int fd);
        ssize_t write_to_fd(int fd);
        ssize_t write_tga_to_fd(int fd);
#endif

        /* 
         * pack methods take raw data in various formats and pack them 
         * into this object e.g. aRawFrame->pack->YCbCr8P422(Y, Cb, Cr)
         */
        RawFramePacker *pack;
        /* 
         * unpack methods do the opposite of pack methods: they take the
         * picture stored in a RawFrame and place a copy in the desired
         * format in the destination given.
         */         
        RawFrameUnpacker *unpack;
        /*
         * draw methods modify this RawFrame by operations such as, e.g.
         * keying in part of another RawFrame.
         */
        RawFrameDrawOps *draw;
        /*
         * convert methods return a new RawFrame object with the same picture
         * as this one, but in a different format.
         */
        RawFrameConverter *convert;


    protected:
        coord_t _w, _h;
        size_t _pitch;
        uint8_t *_data;
        uint8_t _global_alpha;
        PixelFormat _pixel_format;

        RawFrame( );
        RawFrame(PixelFormat pf);

        void initialize_pf(PixelFormat pf);
        size_t pixel_size( ) const;
        size_t minpitch( ) const;
        virtual void alloc( );
        virtual void free_data( );

        void make_ops( );

        void make_packer( );
        void make_unpacker( );
        void make_draw_ops( );
        void make_converter( );
        
        FieldDominance _field_dominance;

        uint64_t tally_program;
        uint64_t tally_preview;

        static int n_frames;
};

#define CHECK(x) check((void *)(x))

class RawFramePacker {
    public:
        RawFramePacker(RawFrame *f_) : f(f_) {
            do_YCbCr8P422 = NULL;
            do_YCbCr8P422A = NULL;
            do_YCbCr10P422A = NULL;
        }

        void YCbCr8P422(uint8_t *Y, uint8_t *Cb, uint8_t *Cr) {
            CHECK(do_YCbCr8P422);
            do_YCbCr8P422(f->size( ), f->data( ), Y, Cb, Cr);
        }

        void YCbCr8P422(uint8_t *Y, uint8_t *Cb, uint8_t *Cr,
            size_t Ysrcpitch, size_t Cbsrcpitch, size_t Crsrcpitch) {
            CHECK(do_YCbCr8P422A);
            do_YCbCr8P422A(
                f->w( ), f->h( ), 
                Ysrcpitch, Cbsrcpitch, Crsrcpitch,
                Y, Cb, Cr, f->data( )
            );
        }

        void YCbCr10P422(
            uint16_t *Y, uint16_t *Cb, uint16_t *Cr,
            size_t Ysrcpitch, size_t Cbsrcpitch, size_t Crsrcpitch) {

            CHECK(do_YCbCr10P422A);
            do_YCbCr10P422A(
                f->w( ), f->h( ),
                Ysrcpitch, Cbsrcpitch, Crsrcpitch,
                Y, Cb, Cr, f->data( )
            );
        }

        void YCbCr8P420(uint8_t *Y, uint8_t *Cb, uint8_t *Cr,
            size_t Ysrcpitch, size_t Cbsrcpitch, size_t Crsrcpitch) {
            CHECK(do_YCbCr8P420A);
            do_YCbCr8P420A(
                f->w( ), f->h( ),
                Ysrcpitch, Cbsrcpitch, Crsrcpitch,
                Y, Cb, Cr, f->data( )
            );
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

        void (*do_YCbCr8P422A)(size_t, size_t, size_t, size_t, size_t,
            uint8_t *, uint8_t *, uint8_t *, uint8_t *);

        void (*do_YCbCr10P422A)(size_t, size_t, size_t, size_t, size_t,
            uint16_t *, uint16_t *, uint16_t *, uint8_t *);

        void (*do_YCbCr8P420A)(size_t, size_t, size_t, size_t, size_t,
            uint8_t *, uint8_t *, uint8_t *, uint8_t *);
};

class RawFrameUnpacker {
    public:
        RawFrameUnpacker(RawFrame *f_) : f(f_) { 
            do_YCbCr8P422 = NULL;
            do_CbYCrY8422 = NULL;
            do_BGRAn8 = NULL;
            do_BGRAn8_scale_1_2 = NULL;
            do_BGRAn8_scale_1_4 = NULL;
            do_CbYCrY8422_scan_double = NULL;
            do_CbYCrY8422_scale_1_4 = NULL;
            do_CbYCrY8422_scan_triple = NULL;
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

        void BGRAn8(uint8_t *data) {
            CHECK(do_BGRAn8);
            do_BGRAn8(f->size( ), f->data( ), data);
        }

        void BGRAn8_scale_1_2(uint8_t *data) {
            CHECK(do_BGRAn8_scale_1_2);
            do_BGRAn8_scale_1_2(f->size( ), f->data( ), data, f->pitch( ));
        }

        void BGRAn8_scale_1_4(uint8_t *data) {
            CHECK(do_BGRAn8_scale_1_4);
            do_BGRAn8_scale_1_4(f->size( ), f->data( ), data, f->pitch( ));
        }

        void CbYCrY8422_scan_double(uint8_t *data) {
            CHECK(do_CbYCrY8422_scan_double);
            do_CbYCrY8422_scan_double(f->size( ), f->data( ), 
                    data, f->pitch( ));
        }

        void CbYCrY8422_scale_1_4(uint8_t *data) {
            CHECK(do_CbYCrY8422_scale_1_4);
            do_CbYCrY8422_scale_1_4(f->size( ), f->data( ),
                    data, f->pitch( ));
        }

        void CbYCrY8422_scan_triple(uint8_t *data) {
            CHECK(do_CbYCrY8422_scan_double);
            do_CbYCrY8422_scan_triple(f->size( ), f->data( ), 
                    data, f->pitch( ));
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
        void (*do_BGRAn8)(size_t, uint8_t *, uint8_t *);
        void (*do_BGRAn8_scale_1_2)(size_t, uint8_t *, 
                uint8_t *, unsigned int);
        void (*do_BGRAn8_scale_1_4)(size_t, uint8_t *, 
                uint8_t *, unsigned int);
        void (*do_CbYCrY8422_scan_double)(size_t, uint8_t *, 
                uint8_t *, unsigned int);
        void (*do_CbYCrY8422_scale_1_4)(size_t, uint8_t *, 
                uint8_t *, unsigned int);
        void (*do_CbYCrY8422_scan_triple)(size_t, uint8_t *,
                uint8_t *, unsigned int);

};

class RawFrameConverter {
    public:
        RawFrameConverter(RawFrame *f_) : f(f_) { }

        RawFrame *BGRAn8( ) {
            RawFrame *ret = match_frame(RawFrame::BGRAn8);
            f->unpack->BGRAn8(ret->data( ));
            return ret;
        }

        RawFrame *BGRAn8_scale_1_2( ) {
            RawFrame *ret = new RawFrame(f->w( ) / 2, f->h( ) / 2, 
                    RawFrame::BGRAn8);
            f->unpack->BGRAn8_scale_1_2(ret->data( ));
            return ret;
        }

        RawFrame *BGRAn8_scale_1_4( ) {
            RawFrame *ret = new RawFrame(f->w( ) / 4, f->h( ) / 4, 
                    RawFrame::BGRAn8);
            f->unpack->BGRAn8_scale_1_4(ret->data( ));
            return ret;
        }

        RawFrame *BGRAn8_540p( ) {
            if (f->w( ) == 960 && f->h( ) == 540) {
                return BGRAn8( );
            } else if (f->w( ) == 1920 && f->h( ) == 1080) {
                return BGRAn8_scale_1_2( );
            } else {
                return NULL; /* fast conversion not supported */
            }
        }

        RawFrame *BGRAn8_270p( ) {
            if (f->w( ) == 480 && f->h( ) >= 270) {
                return BGRAn8( );       
            } else if (f->w( ) == 960 && f->h( ) >= 540) {
                return BGRAn8_scale_1_2( );
            } else if (f->w( ) == 1920 && f->h( ) >= 1080) {
                return BGRAn8_scale_1_4( );
            } else {
                return NULL; /* fast conversion not supported */
            }
        }

        RawFrame *CbYCrY8422( ) {
            RawFrame *ret = match_frame(RawFrame::CbYCrY8422);
            f->unpack->CbYCrY8422(ret->data( ));
            return ret;
        }
        
        RawFrame *CbYCrY8422_scan_double( ) {
            RawFrame *ret = new RawFrame(f->w( ) * 2, f->h( ) * 2,
                    RawFrame::CbYCrY8422);
            f->unpack->CbYCrY8422_scan_double(ret->data( ));
            return ret;
        }

        RawFrame *CbYCrY8422_scan_triple( ) {
            RawFrame *ret = new RawFrame(f->w( ) * 3, f->h( ) * 3,
                    RawFrame::CbYCrY8422);
            f->unpack->CbYCrY8422_scan_triple(ret->data( ));
            return ret;
        }

        RawFrame *CbYCrY8422_1080( ) {
            if (f->w( ) == 960 && f->h( ) >= 540) {
                return CbYCrY8422_scan_double( ); 
            } else {
                return NULL;
            }
        }

        RawFrame *CbYCrY8422_scaled(coord_t w, coord_t h) {
            if (w == f->w( ) / 4 && h == f->h( ) / 4) {
                return CbYCrY8422_scale_1_4( );
            } else {
                return NULL;
            }
        }

        RawFrame *CbYCrY8422_scale_1_4( ) {
            RawFrame *ret = new RawFrame(f->w( ) / 4, f->h( ) / 4,
                    RawFrame::CbYCrY8422);
            f->unpack->CbYCrY8422_scale_1_4(ret->data( ));
            return ret;
        }
    
    protected:
        RawFrame *f;

        RawFrame *match_frame(RawFrame::PixelFormat pf) {
            return new RawFrame(f->w( ), f->h( ), pf);
        }
};

class RawFrameDrawOps {
    public:
        RawFrameDrawOps(RawFrame *f_) : f(f_) { 
            do_alpha_blend = NULL;
            do_alpha_composite = NULL;
            do_blit = NULL;
        }

        void alpha_key(coord_t x, coord_t y, RawFrame *key, 
                uint8_t galpha) {
            CHECK(do_alpha_blend);
            do_alpha_blend(f, key, x, y, galpha);
        }

        void alpha_composite(coord_t x, coord_t y, RawFrame *key,
                coord_t src_x, coord_t src_y, coord_t w, coord_t h,
                uint8_t galpha) {

            CHECK(do_alpha_composite);
            do_alpha_composite(f, key, x, y, galpha, src_x, src_y, w, h);
        }

        void blit(coord_t x, coord_t y, RawFrame *src) {
            CHECK(do_blit);
            do_blit(f, src, x, y);
        }
    protected:
        void check(void *ptr) {
            if (ptr == NULL) {
                throw std::runtime_error("Drawing operation not supported");
            }
        }

        void (*do_alpha_blend)(RawFrame *bkgd, RawFrame *key, 
                coord_t x, coord_t y, uint8_t galpha);

        void (*do_alpha_composite)(RawFrame *bkgd, RawFrame *key, 
                coord_t x, coord_t y, uint8_t galpha, 
                coord_t src_x, coord_t src_y,
                coord_t w, coord_t h);

        void (*do_blit)(RawFrame *bkgd, RawFrame *src, coord_t x, coord_t y);

        RawFrame *f;

};

#undef CHECK

#endif
