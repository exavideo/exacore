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
}

RawFrame::RawFrame(PixelFormat pf) {
    initialize_pf(pf);
}

void RawFrame::initialize_pf(PixelFormat pf) {
    _pixel_format = pf;
    _global_alpha = 0xff;
    make_packer( );
    make_unpacker( );
    make_draw_ops( );
}

RawFrame::RawFrame(coord_t w, coord_t h, PixelFormat pf) {
    _w = w;
    _h = h;
    _global_alpha = 0xff;
    _pixel_format = pf;
    _pitch = minpitch( );
    alloc( );
    make_packer( );
    make_unpacker( );
    make_draw_ops( );
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
    make_packer( );
    make_unpacker( );
    make_draw_ops( );
}

RawFrame::~RawFrame( ) {
    free_data( );
    delete pack;
    delete unpack;
    delete draw;
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

#endif

