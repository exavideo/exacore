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

int16_t *y_offset_lookup_table;
int16_t *y_interp_lookup_table;
int16_t *c_offset_lookup_table;
int16_t *c_interp_lookup_table;

void build_lookup_tables(void) {
    int i;

    y_interp_lookup_table = new int16_t[1920];
    y_offset_lookup_table = new int16_t[1920];
    c_interp_lookup_table = new int16_t[1920];
    c_offset_lookup_table = new int16_t[1920];

    for (i = 0; i < 1920; i++) {
        /* compute the "stretching" function */  
        float opos = (i / 1919.0f) * 2.0f - 1.0f; /* output position on [-1..1] */
        float ipos = (4.0f * opos - powf(opos, 3.0f)) / 3.0f; /* input position on [-1..1] */
        float iofs = (ipos + 1.0f) / 2.0f * 719.0f; /* input position on [0,720) */
        float interp = iofs - floorf(iofs);

        y_offset_lookup_table[i] = 2 * floorf(iofs) + 1;
        y_interp_lookup_table[i] = 256 * interp;

        if (i % 2 == 0) {
            /* adjust the values for the 2x-subsampled chroma */
            c_offset_lookup_table[i] = 2 * (floorf(iofs / 2.0f) * 2);
            c_interp_lookup_table[i] = iofs / 2.0f - floorf(iofs / 2.0f);
        }
    }
}

void interpolate_scanline(uint8_t *out, uint8_t *in1, uint8_t *in2, int frac2) {
    for (int i = 0; i < 2*1920; i++) {
        uint16_t l1 = in1[i];
        uint16_t l2 = in2[i];

        uint16_t result = (l1 * (9-frac2) + l2 * frac2) / 9;
        out[i] = result;
    }
}

void upscale_scanline(uint8_t *out, uint8_t *in1) {
    uint16_t interp, y_in0, y_in1, cbin0, crin0, cbin1, crin1, ofs;

    for (int i = 0; i < 1920; i++) {
        /* use lookup table to find out which two pixels to look at */
        ofs = y_offset_lookup_table[i];
        /* fetch luma value from the left pixel */
        y_in0 = in1[ofs];

        /* 
         * fetch luma value from right pixel
         * (or reuse left one if at an edge)
         */
        if (ofs < 1438) {
            y_in1 = in1[ofs + 2];
        } else {
            y_in1 = y_in0;
        }

        /* fetch linear interpolation value from lookup table */
        interp = y_interp_lookup_table[i];
        /* do luma interpolation */
        out[2*i+1] = (y_in0 * (256 - interp) + y_in1 * interp) / 256;
    
        /* if this sample has cosited chroma samples deal with those here */
        if (i % 2 == 0) {
            ofs = c_offset_lookup_table[i];
            cbin0 = in1[ofs];
            crin0 = in1[ofs+2];

            if (ofs < 1436) {
                cbin1 = in1[ofs+4];
                crin1 = in1[ofs+6];
            } else {
                cbin1 = cbin0;
                crin1 = crin0;
            }


            interp = c_interp_lookup_table[i];
            out[2*i] = (cbin1 * interp + cbin0 * (256 - interp)) / 256;
            out[2*i+2] = (crin1 * interp + crin0 * (256 - interp)) / 256;
        }
    }
}

RawFrame *upscale(RawFrame *in) {
    RawFrame *out = new RawFrame(1920, 1080, RawFrame::CbYCrY8422);
    uint8_t *scaled_1 = new uint8_t[2*1920];
    uint8_t *scaled_2 = new uint8_t[2*1920];
    for (int i = 0; i < out->h( ); i++) {
        int src_scan = i * 4 / 9;
        uint8_t *scanline_1 = in->scanline(src_scan);
        uint8_t *scanline_2;
        if (src_scan + 1 < in->h( )) {
            scanline_2 = in->scanline(src_scan + 1);
        } else {
            scanline_2 = in->scanline(src_scan);
        }
        
        upscale_scanline(scaled_1, scanline_1);
        upscale_scanline(scaled_2, scanline_2);
        
        interpolate_scanline(out->scanline(i), scaled_1, scaled_2, (4 * i) % 9);
    }

    return out;
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "-n") == 0) {
        cpu_force_no_simd( );
    }

    build_lookup_tables( );

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
