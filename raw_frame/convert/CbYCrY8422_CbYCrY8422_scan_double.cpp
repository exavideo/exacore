#include <stdint.h>
#include <stdlib.h>

static void CbYCrY8422_double_scanline(uint8_t *dst, uint8_t *src, 
        size_t src_length) {
    uint8_t y1, y2, cb, cr;
    size_t i, dp;

    dp = 0;

    for (i = 0; i < src_length; i += 4) {
        cb = src[i];
        y1 = src[i+1];
        cr = src[i+2];
        y2 = src[i+3];

        dst[dp+0] = cb;
        dst[dp+1] = y1;
        dst[dp+2] = cr;
        dst[dp+3] = y1;
        dst[dp+4] = cb;
        dst[dp+5] = y2;
        dst[dp+6] = cr;
        dst[dp+7] = y2;

        dp += 8;
    }
}

void CbYCrY8422_CbYCrY8422_scan_double(size_t src_size, 
        uint8_t *src, uint8_t *dst, unsigned int src_pitch) {
    size_t n_scanlines = src_size / src_pitch;
    size_t i;

    for (i = 0; i < n_scanlines; i++) {
        CbYCrY8422_double_scanline(dst, src, src_pitch);
        CbYCrY8422_double_scanline(dst + 2*src_pitch, src, src_pitch);
        dst += 4*src_pitch;
        src += src_pitch;
    }
}
