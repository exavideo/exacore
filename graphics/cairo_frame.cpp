
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

#include "cairo_frame.h"
#include <stdexcept>

CairoFrame::CairoFrame(coord_t w, coord_t h) 
        : RawFrame(w, h, RawFrame::BGRAn8, 
        cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w)) {

    crs = cairo_image_surface_create_for_data(_data, CAIRO_FORMAT_ARGB32,
            _w, _h, _pitch);

    if (cairo_surface_status(crs) != CAIRO_STATUS_SUCCESS) {
        throw std::runtime_error("Cairo surface creation failed");
    }
}

cairo_t *CairoFrame::cairo_create( ) {
    cairo_t *ret;

    ret = ::cairo_create(crs);

    if (cairo_status(ret) != CAIRO_STATUS_SUCCESS) {
        throw std::runtime_error("cairo_create failed");
    }

    return ret;
}

CairoFrame::~CairoFrame( ) {
    cairo_surface_destroy(crs);
}
