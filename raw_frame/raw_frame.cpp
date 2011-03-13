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
#include "raw_frame.h"
#include <stdexcept>

RawFrame::RawFrame(coord_t w, coord_t h, PixelFormat pf) {
    _w = w;
    _h = h;
    _pixel_format = pf;
    _pitch = minpitch( );
    alloc( );
    make_unpacker( );
}

RawFrame::RawFrame(coord_t w, coord_t h, PixelFormat pf, size_t pitch) {
    _w = w;
    _h = h;
    _pixel_format = pf;
    _pitch = pitch;
    if (_pitch < minpitch( )) {
        throw std::runtime_error(
            "Scanline pitch insufficient for width and pixel format"
        );
    }
    alloc( );
    make_unpacker( );
}

RawFrame::~RawFrame( ) {
    free(_data);
    delete unpack;
}

size_t RawFrame::minpitch( ) {
    switch (_pixel_format) {
        case RGB8:
        case YUV8:
            return 3 * _w;

        case UYVY8:
            return 2 * _w;

        case RGBA8:
        case BGRA8:
        case YUVA8:
            return 4 * _w;

        default:
            throw std::runtime_error("invalid pixel format");
            
    }
}

void RawFrame::alloc( ) { 
    assert(_w > 0);
    assert(_h > 0);
    assert(_pitch > minpitch( ));

    _data = malloc(_h * _pitch);
    if (_data == NULL) {
        throw std::runtime_error("RawFrame allocation failed");
    }
}

void RawFrame::make_unpacker(void) {
    switch (_pixel_format) {
        /* specific handlers go here */

        default:
            unpack = new RawFrameUnpacker(this);
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

