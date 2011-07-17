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

FramebufferDisplaySurface::FramebufferDisplaySurface(const char *fb) {
    _ttyfd = -1;
    _fd = -1;
    screensize = 0;

    /* open the framebuffer device */
    _fd = open(fb, O_RDWR);
    if (_fd == -1) {
        throw POSIXError("open framebuffer");
    }

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
        xioctl(_ttyfd, KDSETMODE, KD_GRAPHICS); 
    } catch (...) {
        fprintf(stderr, "could not set graphics mode..."
                " running from within xterm or via SSH?\n");
        throw;
    }


    xioctl(_fd, FBIOGET_VSCREENINFO, &_vinfo);
    memcpy(&_old_vinfo, &_vinfo, sizeof(struct fb_var_screeninfo));

    /* set up 2x vertical resolution for page flipping */
    _vinfo.xres_virtual = _vinfo.xres;
    _vinfo.yres_virtual = 2*_vinfo.yres;
    _vinfo.yoffset = 0;
    screensize = _vinfo.xres * _vinfo.yres * _vinfo.bits_per_pixel / 8;

    xioctl(_fd, FBIOPUT_VSCREENINFO, &vinfo);

    _real_data = (uint8_t *) mmap(0, 2*screensize, PROT_READ | PROT_WRITE, 
            MAP_SHARED, _fd, 0);

    if ((void *)_real_data == MAP_FAILED) {
        throw POSIXError("mmap framebuffer");
    }

    _w = _vinfo.xres_virtual;
    _h = _vinfo.yres_virtual;
    _pitch = _w * _vinfo.bits_per_pixel / 8;
    _data = _real_data;
    
    /* FIXME we have no guarantee this is the real pixel format */
    initialize_pf(RawFrame::BGRAn8);
}

FramebufferDisplaySurface::~FramebufferDisplaySurface( ) {
    if (_ttyfd != -1) {
        xioctl(_ttyfd, KDSETMODE, KD_TEXT);
        close(_ttyfd);
    }

    if (_fd != -1) {
        /* FIXME this could conceivably put bad VSCREENINFO into the FB */
        munmap(_real_data, 2*screensize);
        xioctl(_fd, FBIOPUT_VSCREENINFO, &_old_vinfo);
        close(_fd);
    }
}

void FramebufferDisplaySurface::flip( ) {
    if (_vinfo.yoffset == 0) {
        _vinfo.yoffset = _vinfo.yres;
        _data = _real_data + _pitch * _h;
    } else {
        _vinfo.yoffset = 0;
        _data = _real_data;
    }

    xioctl(_fd, FBIOPUT_VSCREENINFO, &_vinfo);
}

void FramebufferDisplaySurface::free_data( ) {
    /* do nothing since we'll munmap() it later */
}
