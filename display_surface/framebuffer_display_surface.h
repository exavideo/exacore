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

#ifndef _FRAMEBUFFER_DISPLAY_SURFACE_H
#define _FRAMEBUFFER_DISPLAY_SURFACE_H

#include "display_surface.h"
#include <linux/fb.h>

class FramebufferDisplaySurface : public DisplaySurface {
    public:
        FramebufferDisplaySurface(const char *fb = "/dev/fb0");
        ~FramebufferDisplaySurface( );
        virtual void flip( );
        virtual void free_data( );
    protected:
        int _fd;
        int _ttyfd;

        struct fb_var_screeninfo _vinfo;
        struct fb_var_screeninfo _old_vinfo;

        uint8_t *_real_data;
        size_t screensize;
};


#endif
