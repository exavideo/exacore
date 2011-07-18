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

#if 0
    /* open TTY and switch to graphics mode */
    _ttyfd = open("/dev/tty", O_RDWR);
    if (_ttyfd == -1) {
        throw POSIXError("open controlling terminal");
    }
    
    /* 
     * FIXME: we should be able to start on a VC 
     * even if running on another controlling terminal 
     */
    try {
        xioctl(_ttyfd, KDSETMODE, (void *)KD_GRAPHICS); 
    } catch (...) {
        fprintf(stderr, "could not set graphics mode..."
                " running from within xterm or via SSH?\n");
        throw;
    }
#endif

    xioctl(_fd, FBIOGET_VSCREENINFO, &_vinfo);
    memcpy(&_old_vinfo, &_vinfo, sizeof(struct fb_var_screeninfo));

    /* set up 2x vertical resolution for page flipping */
    _vinfo.xres_virtual = _vinfo.xres;
    _vinfo.yres_virtual = 2*_vinfo.yres;
    _vinfo.yoffset = 0;
    screensize = _vinfo.xres * _vinfo.yres * _vinfo.bits_per_pixel / 8;

    xioctl(_fd, FBIOPUT_VSCREENINFO, &_vinfo);

    fprintf(stderr, "red: offset %d length %d msb_right %d\n",
        _vinfo.red.offset, _vinfo.red.length, _vinfo.red.msb_right);
    fprintf(stderr, "green: offset %d length %d msb_right %d\n",
        _vinfo.green.offset, _vinfo.green.length, _vinfo.green.msb_right);
    fprintf(stderr, "blue: offset %d length %d msb_right %d\n",
        _vinfo.blue.offset, _vinfo.blue.length, _vinfo.blue.msb_right);

    fprintf(stderr, "bits_per_pixel: %d\n", _vinfo.bits_per_pixel);


    fprintf(stderr, "trying to mmap: %d bytes", (int) (2*screensize));
    _real_data = (uint8_t *) mmap(0, screensize, PROT_READ | PROT_WRITE, 
            MAP_SHARED, _fd, 0);

    _real_data2 = (uint8_t *) mmap(0, screensize, PROT_READ | PROT_WRITE,
            MAP_SHARED, _fd, screensize);


    if ((void *)_real_data == MAP_FAILED) {
        throw POSIXError("mmap framebuffer");
    }

    if ((void *)_real_data2 == MAP_FAILED) {
        throw POSIXError("mmap framebuffer");
    }

    memset(_real_data, 0xff, screensize);
    //memset(_real_data2, 0x00, screensize);

    fprintf(stderr, "_real_data=%p\n", _real_data);
    fprintf(stderr, "_real_data2=%p\n", _real_data2);

    _w = _vinfo.xres_virtual;
    _h = _vinfo.yres_virtual;
    _pitch = _w * _vinfo.bits_per_pixel / 8;
    _data = _real_data;
    
    /* FIXME we have no guarantee this is the real pixel format */
    initialize_pf(RawFrame::BGRAn8);
}

FramebufferDisplaySurface::~FramebufferDisplaySurface( ) {
#if 0
    if (_ttyfd != -1) {
        xioctl(_ttyfd, KDSETMODE, KD_TEXT);
        close(_ttyfd);
    }
#endif
    if (_fd != -1) {
        /* FIXME this could conceivably put bad VSCREENINFO into the FB */
        munmap(_real_data, screensize);
        munmap(_real_data2, screensize);
        xioctl(_fd, FBIOPUT_VSCREENINFO, &_old_vinfo);
        close(_fd);
    }
}

void FramebufferDisplaySurface::flip( ) {
#if 0
    if (_vinfo.yoffset == 0) {
        _vinfo.yoffset = _vinfo.yres;
        _data = _real_data;
    } else {
        _vinfo.yoffset = 0;
        _data = _real_data2;
    }

    xioctl(_fd, FBIOPAN_DISPLAY, &_vinfo);
#endif
}

void FramebufferDisplaySurface::free_data( ) {
    /* do nothing since we'll munmap() it later */
}
