/*
 * Copyright 2011 Andrew H. Armenia.
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
#include <stdexcept>
#include "xmalloc.h"
#include <assert.h>
#include "pack_CbYCrY8422.h"
#include "unpack_CbYCrY8422.h"
#include "draw_CbYCrY8422.h"
#include "draw_BGRAn8.h"
#include <string.h>

RawFrame::RawFrame( ) {
    _pixel_format = UNDEF;
    _w = 0;
    _h = 0;
    _pitch = 0;
    _data = NULL;
    _global_alpha = 0xff;
    pack = NULL;
    unpack = NULL;
    draw = NULL;
    convert = NULL;
}

RawFrame::RawFrame(PixelFormat pf) {
    initialize_pf(pf);
}

void RawFrame::initialize_pf(PixelFormat pf) {
    _pixel_format = pf;
    _global_alpha = 0xff;
    make_ops( );
}

RawFrame::RawFrame(coord_t w, coord_t h, PixelFormat pf) {
    _w = w;
    _h = h;
    _global_alpha = 0xff;
    _pixel_format = pf;
    _pitch = minpitch( );
    alloc( );
    make_ops( );
}

RawFrame::RawFrame(coord_t w, coord_t h, PixelFormat pf, size_t pitch) {
    _w = w;
    _h = h;
    _global_alpha = 0xff;
    _pixel_format = pf;
    _pitch = pitch;
    if (_pitch < minpitch( )) {
        throw std::runtime_error(
            "Scanline pitch insufficient for width and pixel format"
        );
    }
    alloc( );
    make_ops( );
}

void RawFrame::make_ops(void) {
    make_packer( );
    make_unpacker( );
    make_draw_ops( );
    make_converter( );
}

RawFrame::~RawFrame( ) {
    free_data( );
    delete pack;
    delete unpack;
    delete draw;
    delete convert;
}

size_t RawFrame::minpitch( ) const {
    switch (_pixel_format) {
        case RGB8:
        case YCbCr8:
            return 3 * _w;

        case CbYCrY8422:
            return 2 * _w;

        case RGBAn8:
        case BGRAn8:
        case YCbCrAn8:
            return 4 * _w;

        default:
            throw std::runtime_error("invalid pixel format");
            
    }
}

void RawFrame::alloc( ) { 
    assert(_w > 0);
    assert(_h > 0);
    assert(_pitch >= minpitch( ));

    _data = (uint8_t *)xmalloc(_h * _pitch, "RawFrame", "_data");
    if (_data == NULL) {
        throw std::runtime_error("RawFrame allocation failed");
    }
}

void RawFrame::free_data( ) {
    if (_data) {
        free(_data);
    }
}

void RawFrame::make_converter(void) {
    convert = new RawFrameConverter(this);
}

void RawFrame::make_packer(void) {
    switch (_pixel_format) {
        case CbYCrY8422:
            pack = new CbYCrY8422Packer(this);
            break;

        default:
            pack = new RawFramePacker(this);
            break;
    }
}

void RawFrame::make_unpacker(void) {
    switch (_pixel_format) {
        /* specific handlers go here */
        case CbYCrY8422:
            unpack = new CbYCrY8422Unpacker(this);
            break;

        default:
            unpack = new RawFrameUnpacker(this);
            break;
    }
}

void RawFrame::make_draw_ops(void) {
    switch (_pixel_format) {
        /* specific handlers go here */
        case CbYCrY8422:
            draw = new CbYCrY8422DrawOps(this);
            break;

        case BGRAn8:
            draw = new BGRAn8DrawOps(this);
            break;

        default:
            draw = new RawFrameDrawOps(this);
            break;
    }
}

#ifdef RAWFRAME_POSIX_IO

#include "posix_util.h"

ssize_t RawFrame::read_from_fd(int fd) {
    return read_all(fd, _data, size( ));
}

ssize_t RawFrame::write_to_fd(int fd) {
    return write_all(fd, _data, size( ));
}

ssize_t RawFrame::write_tga_to_fd(int fd) {
    uint8_t tga_header[18];

    memset(tga_header, 0, sizeof(tga_header));

    tga_header[0] = 0;  /* no image ID */
    tga_header[1] = 0;  /* no color map */
    tga_header[2] = 2;  /* uncompressed, true-color */
    /* 3 through 7 are for color map data */
    tga_header[12] = _w & 0xff;     /* image width */
    tga_header[13] = (_w >> 8) & 0xff;
    tga_header[14] = _h & 0xff;     /* image height */
    tga_header[15] = (_h >> 8) & 0xff;
    tga_header[16] = minpitch( ) / _w * 8;
    switch (_pixel_format) {
        case RGBAn8:
        case BGRAn8:
        case YCbCrAn8:
            tga_header[17] = 0x28;
        default:
            tga_header[17] = 0x20;
    }

    write_all(fd, tga_header, sizeof(tga_header));
    return write_all(fd, _data, size( ));
}

#endif

