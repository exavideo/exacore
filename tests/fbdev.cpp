/*
 * Copyright 2011 Exavideo LLC.
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

#include "posix_util.h"
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>

int main( ) {
    int fd = 0;

    struct fb_var_screeninfo vinfo, vinfo2;
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo_cache;

    fd = open("/dev/fb0", O_RDWR);
    if (fd == -1) {
        throw POSIXError("open");
    }

    if (ioctl(fd, FBIOGET_FSCREENINFO, &finfo) != 0) {
        throw POSIXError("FBIOGET_FSCREENINFO");
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo) != 0) {
        throw POSIXError("FBIOGET_VSCREENINFO");
    }

    fprintf(stderr, "xres=%d yres=%d\n", vinfo.xres, vinfo.yres);
    fprintf(stderr, "xres_virtual=%d yres_virtual=%d\n", vinfo.xres_virtual, vinfo.yres_virtual);

    memcpy(&vinfo_cache, &vinfo, sizeof(struct fb_var_screeninfo));

    vinfo.yres_virtual *= 2;
    if (ioctl(fd, FBIOPUT_VSCREENINFO, &vinfo) != 0) {
        throw POSIXError("FBIOPUT_VSCREENINFO");
    }

    if (ioctl(fd, FBIOGET_VSCREENINFO, &vinfo2) != 0) {
        throw POSIXError("FBIOGET_VSCREENINFO");
    }

    fprintf(stderr, "xres_virtual=%d yres_virtual=%d yoffset=%d\n", vinfo2.xres_virtual, vinfo2.yres_virtual, vinfo2.yoffset);

    close(fd);
    return 0;
}
