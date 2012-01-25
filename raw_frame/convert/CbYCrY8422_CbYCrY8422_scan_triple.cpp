#include <stdint.h>
#include <stdlib.h>

static void CbYCrY8422_triple_scanline(uint8_t *dst, uint8_t *src, 
        size_t src_length) {
    uint8_t y1, y2, cb, cr;
    size_t i, dp;

    dp = 0;

    for (i = 0; i < src_length; i += 4) {
        /* load 2 source pixels */
        cb = src[i];
        y1 = src[i+1];
        cr = src[i+2];
        y2 = src[i+3];

        /* write 6 destination pixels */
        dst[dp+0] = cb;
        dst[dp+1] = y1;
        dst[dp+2] = cr;
        dst[dp+3] = y1;

        dst[dp+4] = cb;
        dst[dp+5] = y1;
        dst[dp+6] = cr;
        dst[dp+7] = y2;

        dst[dp+8] = cb;
        dst[dp+9] = y2;
        dst[dp+10] = cr;
        dst[dp+11] = y2;

        dp += 12;
    }
}

void CbYCrY8422_CbYCrY8422_scan_triple(size_t src_size, 
        uint8_t *src, uint8_t *dst, unsigned int src_pitch) {
    size_t n_scanlines = src_size / src_pitch;
    size_t i;

    /* hack to get a center cut of the 4:3 frame */
    src += 60*src_pitch;
    n_scanlines -= 120;

    for (i = 0; i < n_scanlines; i++) {
        CbYCrY8422_triple_scanline(dst, src, src_pitch);
        CbYCrY8422_triple_scanline(dst + 3*src_pitch, src, src_pitch);
        CbYCrY8422_triple_scanline(dst + 6*src_pitch, src, src_pitch);
        dst += 9*src_pitch;
        src += src_pitch;
    }
}

