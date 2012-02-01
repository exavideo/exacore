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

#ifndef _MPLAYER_DISPLAY_SURFACE_H
#define _MPLAYER_DISPLAY_SURFACE_H

#include "display_surface.h"

class MplayerDisplaySurface : public DisplaySurface {
    public:
        MplayerDisplaySurface(const char *cmd = "mplayer -demuxer rawvideo -rawvideo bgra:w=1920:h=1080 -");
        ~MplayerDisplaySurface( );
        virtual void flip( );
    protected:
        int _fd;
        int fork_mplayer(const char *cmd);
};


#endif
