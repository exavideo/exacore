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

#include "rsvg_frame.h"

#include "librsvg/rsvg.h"
#include "librsvg/rsvg-cairo.h"

#include <string.h>

static int rsvg_is_init = 0;

RawFrame *RsvgFrame::render_svg(const char *svg_data, size_t size) {
    RsvgHandle *rsvg;
    RsvgDimensionData dim;
    CairoFrame *crf;
    cairo_t *cr;
    
    if (!rsvg_is_init) {
        rsvg_init( );
        rsvg_set_default_dpi_x_y(75.0, 75.0);
    }

    rsvg = rsvg_handle_new_from_data((guint8 *) svg_data, (gsize) size, NULL);

    if (rsvg == NULL) {
        throw std::runtime_error("rsvg_handle_new_from_data failed");
    }

    rsvg_handle_get_dimensions(rsvg, &dim);

    crf = new CairoFrame(dim.width, dim.height);

    /* clear to full transparency initially */
    memset(crf->data( ), 0, crf->size( ));

    /* render SVG */
    cr = crf->cairo_create( );
    if (rsvg_handle_render_cairo(rsvg, cr) == FALSE) {
        cairo_destroy(cr);
        g_object_unref(rsvg);
        throw std::runtime_error("rsvg_handle_render_cairo failed");
    }

    cairo_destroy(cr);
    g_object_unref(rsvg);

    return crf;
}
