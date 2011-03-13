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

#include "mjpeg_codec.h"

Mjpeg422Encoder::Mjpeg422Encoder(coord_t w_, coord_t h_, 
        size_t max_frame_size) {
    w = w_;
    h = h_;
    jpeg_alloc_size = max_frame_size;

    jpeg_data = xmalloc(jpeg_alloc_size, "Mjpeg422Encoder", "jpeg_data");
    y_plane = xmalloc(2 * w * h, "Mjpeg422Encoder", "YCbCr planes");
    /* put the Cb and Cr planes in the same memory chunk as the y plane */
    cb_plane = y_plane + w * h;
    cr_plane = cb_plane + w * h / 2;

    libjpeg_init( );
}

Mjpeg422Encoder::~Mjpeg422Encoder( ) {
    free(jpeg_data);
    free(y_plane);
}

void Mjpeg422Encoder::libjpeg_init( ) {
    memset(&cinfo, 0, sizeof(cinfo));

    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = throw_encode_exception;
    jpeg_create_compress(&cinfo);
}

void Mjpeg422Encoder::encode(RawFrame *f) {
    f->unpack->YCbCr422p(y_frame, cb_plane, cr_plane);    


}
