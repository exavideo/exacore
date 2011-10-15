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


void interpolate_scanline(uint8_t *out, uint8_t *in1, uint8_t *in2, int frac2) {
    for (int i = 0; i < 2*1920; i++) {
        uint16_t l1 = in1[i];
        uint16_t l2 = in2[i];

        uint16_t result = (l1 * (9-frac2) + l2 * frac2) / 9;
        out[i] = result;
    }
}

void upscale_scanline(uint8_t *out, uint8_t *in1) {
    for (int i = 0; i < 1920; i++) {
        float opos = (i / 1919.0f) * 2.0f - 1.0f; /* range from -1..1 */
        float ipos = (4.0f*opos - powf(opos,3.0f)) / 3.0f;
        float iofs = (ipos + 1.0f) / 2.0f * 720.0f;
        float interp = iofs - floorf(iofs);
        int iofs0 = floorf(iofs);
        uint8_t y_in0 = in1[2*iofs0 + 1];
        uint8_t y_in1;
        uint8_t &out_luma = out[2*i + 1];

        if (iofs0 < 719) {
            y_in1 = in1[2*iofs0 + 3];
        } else {
            y_in1 = y_in0;
        }

        out_luma = (uint8_t) (y_in1 * interp + y_in0 * (1.0f - interp));

        if (i % 2 == 0) {
            int cofs0 = floorf(iofs/2.0f) * 2;
            float interp = iofs / 2.0f - floorf(iofs / 2.0f);
            uint8_t cbin0 = in1[2*cofs0];
            uint8_t crin0 = in1[2*cofs0+2];
            uint8_t cbin1, crin1;
            uint8_t &out_cb = out[2*i];
            uint8_t &out_cr = out[2*i+2];

            if (cofs0 < 718) {
                cbin1 = in1[2*cofs0+4];
                crin1 = in1[2*cofs0+6];
            } else {
                cbin1 = cbin0;
                crin1 = crin0;
            }


            /* do chroma interpolation... eventually */
            out_cb = (cbin1 * interp + cbin0 * (1.0f - interp));
            out_cr = (crin1 * interp + crin0 * (1.0f - interp));
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
