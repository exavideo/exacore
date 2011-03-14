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
#include "libjpeg_glue.h"
#include "xmalloc.h"
#include <string.h>
#include <assert.h>

Mjpeg422Encoder::Mjpeg422Encoder(coord_t w_, coord_t h_, 
        size_t max_frame_size) {
    w = w_;
    h = h_;
    jpeg_alloc_size = max_frame_size;

    jpeg_data = (uint8_t *)
        xmalloc(jpeg_alloc_size, "Mjpeg422Encoder", "jpeg_data");

    y_plane = (uint8_t *)
        xmalloc(2 * w * h, "Mjpeg422Encoder", "YCbCr planes");

    /* put the Cb and Cr planes in the same memory chunk as the y plane */
    cb_plane = y_plane + w * h;
    cr_plane = cb_plane + w * h / 2;
    
    /* allocate arrays of pointers to feed to libjpeg */
    y_scans = (JSAMPARRAY)
        xmalloc(sizeof(JSAMPROW) * h, "Mjpeg422Encoder", "y_scans");
    cb_scans = (JSAMPARRAY)
        xmalloc(sizeof(JSAMPROW) * h, "Mjpeg422Encoder", "cb_scans");
    cr_scans = (JSAMPARRAY)
        xmalloc(sizeof(JSAMPROW) * h, "Mjpeg422Encoder", "cr_scans");

    for (int i = 0; i < h; i++) {
        y_scans[i] = (JSAMPROW) (y_plane + i * w);
        cb_scans[i] = (JSAMPROW) (cb_plane + i * w/2);
        cr_scans[i] = (JSAMPROW) (cr_plane + i * w/2);
    }

    libjpeg_init( );

    jpeg_finished_size = 0;
}

Mjpeg422Encoder::~Mjpeg422Encoder( ) {
    free(jpeg_data);
    free(y_plane);
    free(y_scans);
    free(cb_scans);
    free(cr_scans);

    jpeg_destroy_compress(&cinfo);
}

void Mjpeg422Encoder::libjpeg_init( ) {
    memset(&cinfo, 0, sizeof(cinfo));

    cinfo.err = jpeg_throw_on_error(&jerr);
    jpeg_create_compress(&cinfo);

    /* test if things properly match the DCT size */
    assert(w % 16 == 0);
    assert(h % 8 == 0);

    cinfo.image_width = w;
    cinfo.image_height = h;

    cinfo.input_components = 3;
    jpeg_set_defaults(&cinfo);
    jpeg_set_colorspace(&cinfo, JCS_YCbCr);

    cinfo.raw_data_in = TRUE;
    cinfo.dct_method = JDCT_FASTEST;

    /* Y */
    cinfo.comp_info[0].v_samp_factor = 1;
    cinfo.comp_info[0].h_samp_factor = 2;
    /* Cb */
    cinfo.comp_info[1].v_samp_factor = 1;
    cinfo.comp_info[1].h_samp_factor = 1;
    /* Cr */
    cinfo.comp_info[2].v_samp_factor = 1;
    cinfo.comp_info[2].h_samp_factor = 1;
}

void Mjpeg422Encoder::encode(RawFrame *f) {
    size_t jpeg_size = jpeg_alloc_size;

    JDIMENSION scanlines_consumed = 0;

    JSAMPARRAY planes[3];

    f->unpack->YCbCr422p(y_plane, cb_plane, cr_plane);    
   
    jpeg_mem_dest(&cinfo, jpeg_data, &jpeg_size);
    jpeg_start_compress(&cinfo, TRUE);

    while (scanlines_consumed < h) {
        planes[0] = y_scans + scanlines_consumed;
        planes[1] = cb_scans + scanlines_consumed;
        planes[2] = cr_scans + scanlines_consumed;
        scanlines_consumed += jpeg_write_raw_data(&cinfo, planes, 
                h - scanlines_consumed);
    }

    jpeg_finish_compress(&cinfo);

    jpeg_finished_size = jpeg_size;
}
