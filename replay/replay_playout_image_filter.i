/*
 * Copyright 2013 Exavideo LLC.
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

%{
    #include "replay_playout_image_filter.h"
%}

%include "typemaps.i"
%include "replay_playout_filter.i"

class ReplayPlayoutImageFilter : public ReplayPlayoutFilter {
    public:
        ReplayPlayoutImageFilter(RawFrame *, coord_t, coord_t);
        static ReplayPlayoutImageFilter *from_png(
            const char *INPUT, coord_t, coord_t
        );
        static ReplayPlayoutImageFilter *from_svg(
            const char *INPUT, coord_t, coord_t
        );
        virtual void enable( );
        virtual void disable( );
        virtual bool is_enabled( );
};


