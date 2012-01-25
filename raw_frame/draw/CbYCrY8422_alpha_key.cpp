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
#include <assert.h>

static void CbYCrY8422_RGBAn8_key_default(RawFrame *bkgd, RawFrame *key,
        coord_t x, coord_t y, uint8_t galpha); 

static void CbYCrY8422_BGRAn8_key_default(RawFrame *bkgd, RawFrame *key,
        coord_t x, coord_t y, uint8_t galpha);

void CbYCrY8422_alpha_key_default(RawFrame *bkgd, RawFrame *key,
        coord_t x, coord_t y, uint8_t galpha) {

    assert(bkgd->pixel_format( ) == RawFrame::CbYCrY8422);

    switch (key->pixel_format( )) {
        case RawFrame::RGBAn8:
            CbYCrY8422_RGBAn8_key_default(bkgd, key, x, y, galpha);
            break;

        case RawFrame::BGRAn8:
            CbYCrY8422_BGRAn8_key_default(bkgd, key, x, y, galpha);
            break;

        default:
            throw std::runtime_error("Unsupported key requested");
    }

}

void CbYCrY8422_RGBAn8_key_default(RawFrame *bkgd, RawFrame *key, 
        coord_t x, coord_t y, uint8_t galpha) {
    
    int i, j, x0;
    uint8_t *bp, *kp;

    int32_t y0, y1, cb, cr;
    int32_t fy0, fy1, fcb, fcr;
    int32_t ka0, ka1, ka01, ka11;
    int32_t fr0, fg0, fb0, fr1, fg1, fb1;
    int32_t fra, fga, fba, kaa, kaa1;
    uint16_t frp, fgp, fbp, kap;

    for (i = 0; i < key->h( ) && y < bkgd->h( ); i++, y++) {
        bp = bkgd->scanline(y);
        kp = key->scanline(i);

        frp = kp[0];
        fgp = kp[1];
        fbp = kp[2];
        kap = (kp[3] * galpha + 511) / 256;

        /* FIXME this loop condition is not quite correct. */
        for (x0 = x, j = 0; j < key->w( ) && x + j < bkgd->w( ); j += 2, x0 += 2) {
            cb = bp[2*x0];
            y0 = bp[2*x0+1];
            cr = bp[2*x0+2];
            y1 = bp[2*x0+3];


            fr0 = kp[4*j];
            fg0 = kp[4*j+1];
            fb0 = kp[4*j+2];
            ka0 = kp[4*j+3];
            ka0 = (ka0 * galpha + 511) / 256;
            ka01 = 0xff - ka0;

            fr1 = kp[4*j+4];
            fg1 = kp[4*j+5];
            fb1 = kp[4*j+6];
            ka1 = kp[4*j+7];
            ka1 = (ka1 * galpha + 511) / 256;
            ka11 = 0xff - ka1;            

            /* chroma filtering */
            fra = (frp + 2*fr0 + fr1) / 4;
            fga = (fgp + 2*fg0 + fg1) / 4;
            fba = (fbp + 2*fb0 + fb1) / 4;
            kaa = (kap + 2*ka0 + ka1) / 4;
            kaa1 = 0xff - kaa;

            frp = fr1;
            fgp = fg1;
            fbp = fb1;
            kap = ka1;

            fy0 = (4096  +  66*fr0 + 129*fg0 +  25*fb0) / 256;
            fy1 = (4096  +  66*fr1 + 129*fg1 +  25*fb1) / 256;
            fcb = (32768 -  38*fra -  74*fga + 112*fba) / 256;
            fcr = (32768 + 112*fra -  94*fga -  18*fba) / 256;


            y0 = (y0 * ka01 + fy0 * ka0 + 256) / 256;
            y1 = (y1 * ka11 + fy1 * ka1 + 256) / 256;

            cr = (cr * kaa1 + fcr * kaa + 256) / 256;
            cb = (cb * kaa1 + fcb * kaa + 256) / 256;

            bp[2*x0]   = cb;
            bp[2*x0+1] = y0;
            bp[2*x0+2] = cr;
            bp[2*x0+3] = y1;
        }
    }
}

void CbYCrY8422_BGRAn8_key_default(RawFrame *bkgd, RawFrame *key, 
        coord_t x, coord_t y, uint8_t galpha) {
    
    int i, j, x0;
    uint8_t *bp, *kp;

    int32_t y0, y1, cb, cr;
    int32_t fy0, fy1, fcb, fcr;
    int32_t ka0, ka1, ka01, ka11;
    int32_t fr0, fg0, fb0, fr1, fg1, fb1;
    int32_t fra, fga, fba, kaa, kaa1;
    uint16_t frp, fgp, fbp, kap;

    for (i = 0; i < key->h( ) && y < bkgd->h( ); i++, y++) {
        bp = bkgd->scanline(y);
        kp = key->scanline(i);

        fbp = kp[0];
        fgp = kp[1];
        frp = kp[2];
        kap = (kp[3] * galpha + 511) / 256;

        /* FIXME this loop condition is not quite correct. */
        for (x0 = x, j = 0; j < key->w( ) && x + j < bkgd->w( ); j += 2, x0 += 2) {
            cb = bp[2*x0];
            y0 = bp[2*x0+1];
            cr = bp[2*x0+2];
            y1 = bp[2*x0+3];


            fb0 = kp[4*j];
            fg0 = kp[4*j+1];
            fr0 = kp[4*j+2];
            ka0 = kp[4*j+3];
            ka0 = (ka0 * galpha + 511) / 256;
            ka01 = 0xff - ka0;

            fb1 = kp[4*j+4];
            fg1 = kp[4*j+5];
            fr1 = kp[4*j+6];
            ka1 = kp[4*j+7];
            ka1 = (ka1 * galpha + 511) / 256;
            ka11 = 0xff - ka1;            

            /* chroma filtering */
            fra = (frp + 2*fr0 + fr1) / 4;
            fga = (fgp + 2*fg0 + fg1) / 4;
            fba = (fbp + 2*fb0 + fb1) / 4;
            kaa = (kap + 2*ka0 + ka1) / 4;
            kaa1 = 0xff - kaa;

            frp = fr1;
            fgp = fg1;
            fbp = fb1;
            kap = ka1;

            if (ka0 != 0 || ka1 != 0) {
                fy0 = (4096  +  66*fr0 + 129*fg0 +  25*fb0) / 256;
                fy1 = (4096  +  66*fr1 + 129*fg1 +  25*fb1) / 256;
                fcb = (32768 -  38*fra -  74*fga + 112*fba) / 256;
                fcr = (32768 + 112*fra -  94*fga -  18*fba) / 256;


                y0 = (y0 * ka01 + fy0 * ka0 + 256) / 256;
                y1 = (y1 * ka11 + fy1 * ka1 + 256) / 256;

                cr = (cr * kaa1 + fcr * kaa + 256) / 256;
                cb = (cb * kaa1 + fcb * kaa + 256) / 256;

                bp[2*x0]   = cb;
                bp[2*x0+1] = y0;
                bp[2*x0+2] = cr;
                bp[2*x0+3] = y1;
            }
        }
    }
}
