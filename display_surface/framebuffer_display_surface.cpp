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

#include "framebuffer_display_surface.h"

#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>

#include "posix_util.h"


FramebufferDisplaySurface::FramebufferDisplaySurface(const char *fb) {
    _ttyfd = -1;
    _fd = -1;
    screensize = 0;

    /* open the framebuffer device */
    _fd = open(fb, O_RDWR);
    if (_fd == -1) {
        throw POSIXError("open framebuffer");
    }

    xioctl(_fd, FBIOGET_VSCREENINFO, &_vinfo);
    memcpy(&_old_vinfo, &_vinfo, sizeof(struct fb_var_screeninfo));

    fprintf(stderr, "red: offset %d length %d msb_right %d\n",
        _vinfo.red.offset, _vinfo.red.length, _vinfo.red.msb_right);
    fprintf(stderr, "green: offset %d length %d msb_right %d\n",
        _vinfo.green.offset, _vinfo.green.length, _vinfo.green.msb_right);
    fprintf(stderr, "blue: offset %d length %d msb_right %d\n",
        _vinfo.blue.offset, _vinfo.blue.length, _vinfo.blue.msb_right);

    fprintf(stderr, "bits_per_pixel: %d\n", _vinfo.bits_per_pixel);

    screensize = _vinfo.bits_per_pixel * _vinfo.xres_virtual 
            * _vinfo.yres_virtual / 8;

    fprintf(stderr, "trying to mmap: %d bytes", (int) (screensize));
    _real_data = (uint8_t *) mmap(0, screensize, PROT_READ | PROT_WRITE, 
            MAP_SHARED, _fd, 0);

    if ((void *)_real_data == MAP_FAILED) {
        throw POSIXError("mmap framebuffer");
    }

    memset(_real_data, 0xff, screensize);

    fprintf(stderr, "_real_data=%p\n", _real_data);

    _w = _vinfo.xres_virtual;
    _h = _vinfo.yres_virtual;
    _pitch = _w * _vinfo.bits_per_pixel / 8;
    _data = _real_data;
    
    /* FIXME we have no guarantee this is the real pixel format */
    initialize_pf(RawFrame::BGRAn8);
}

FramebufferDisplaySurface::~FramebufferDisplaySurface( ) {
    if (_fd != -1) {
        /* FIXME this could conceivably put bad VSCREENINFO into the FB */
        munmap(_real_data, screensize);
        close(_fd);
    }
}

void FramebufferDisplaySurface::flip( ) {
    /* pageflipping crashes some kernel drivers so don't do it??? */
}

void FramebufferDisplaySurface::free_data( ) {
    /* do nothing since we'll munmap() it later */
}
