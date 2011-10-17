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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "raw_frame.h"
#include "posix_util.h"
#include "cpu_dispatch.h"

void upscale_scanline(uint8_t *dst_scanline, uint8_t *src_scanline) {
    const coord_t offset = 40;
    uint16_t y0, y1, cb0, cb1, cr0, cr1;
    /* copy luma values */
    for (coord_t i = 0; i < 640; i++) {
        /* luma */
        dst_scanline[6*i + 1] = src_scanline[2*(i + offset) + 1];
    }

    /* copy chroma values */
    for (coord_t i = 0; i < 320; i++) {
        dst_scanline[12*i + 0] = src_scanline[4*i + 2*offset]; /* Cb */
        dst_scanline[12*i + 2] = src_scanline[4*i + 2*offset + 2]; /* Cr */
    }

    /* luma interpolation */
    for (coord_t i = 0; i < 639; i++) {
        y0 = dst_scanline[6*i + 1];
        y1 = dst_scanline[6*i + 7];
        dst_scanline[6*i + 3] = (2 * y0 + y1) / 3;
        dst_scanline[6*i + 5] = (2 * y1 + y0) / 3;
    }

    /* chroma interpolation */
    for (coord_t i = 0; i < 319; i++) {
        /* Cb */
        cb0 = dst_scanline[12 * i + 0];
        cb1 = dst_scanline[12 * i + 12];

        dst_scanline[12*i + 4] = (2 * cb0 + cb1) / 3;
        dst_scanline[12*i + 8] = (2 * cb1 + cb0) / 3;

        /* Cr */
        cr0 = dst_scanline[12 * i + 2];
        cr1 = dst_scanline[12 * i + 14];

        dst_scanline[12*i +  6] = (2 * cr0 + cr1) / 3;
        dst_scanline[12*i + 10] = (2 * cr1 + cr0) / 3;
    }
}

void interpolate_scanline(uint8_t *dst, uint8_t *s1, uint8_t *s2, int interp) {
    for (coord_t i = 0; i < 2*1920; i++) {
        dst[i] = (s1[i] * (3 - interp) + s2[i] * interp) / 3;
    }
}

RawFrame *upscale(RawFrame *in) {
    RawFrame *out = new RawFrame(1920, 1080, RawFrame::CbYCrY8422);
    coord_t offset = in->h( ) / 2 - 180;

    /* pass 1: up-scale all scanlines that are direct copies */
    for (coord_t i = 0; i < 180; i++) {
        /* keep interlaced pairs together */
        upscale_scanline(out->scanline(6*i), in->scanline(2*i + offset));        
        upscale_scanline(out->scanline(6*i + 1), in->scanline(2*i+1 + offset));
    }

    /* pass 2: interpolate scanlines in between */
    for (coord_t i = 0; i < 179; i++) {
        interpolate_scanline(out->scanline(6*i+2), /* dst. scanline */
            out->scanline(6*i), out->scanline(6*i+6), /* source scanlines */
            1 /* interpolant */
        );
        interpolate_scanline(out->scanline(6*i+3), /* dst. scanline */
            out->scanline(6*i+1), out->scanline(6*i+7), /* source scanlines */
            1 /* interpolant */
        );

        interpolate_scanline(out->scanline(6*i+4), /* dst. scanline */
            out->scanline(6*i), out->scanline(6*i+6), /* source scanlines */
            2 /* interpolant */
        );
        interpolate_scanline(out->scanline(6*i+5), /* dst. scanline */
            out->scanline(6*i+1), out->scanline(6*i+7), /* source scanlines */
            2 /* interpolant */
        );
            
    }

    /* close enough for now */

    return out;
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "-n") == 0) {
        cpu_force_no_simd( );
    }

    /* Read 1080p UYVY422 frames on stdin. Dump M-JPEG stream on stdout. */
    RawFrame frame(720, 480, RawFrame::CbYCrY8422);

    ssize_t ret;


    for (;;) {
        ret = frame.read_from_fd(STDIN_FILENO);

        if (ret < 0) {
            perror("frame.read_from_fd");
            exit(1);
        } else if (ret == 0) {
            break;
        } else {
            RawFrame *out = upscale(&frame);

            if (out->write_to_fd(STDOUT_FILENO) < 0) {
                perror("write_to_fd");
                break;
            }
            delete out;
        }            
    }
}
