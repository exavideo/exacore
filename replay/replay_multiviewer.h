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

#ifndef _REPLAY_MULTIVIEWER_H
#define _REPLAY_MULTIVIEWER_H

#include <vector>
#include "thread.h"
#include "display_surface.h"
#include "async_port.h"
#include "replay_data.h"

class FreetypeFont;

struct ReplayMultiviewerSourceParams {
    AsyncPort<ReplayRawFrame> *source;
    coord_t x, y;
};

class ReplayMultiviewer : public Thread {
    public:
        ReplayMultiviewer(DisplaySurface *dpy_);
        ~ReplayMultiviewer( );


        void add_source(const ReplayMultiviewerSourceParams &params);
        void start( );

    protected:
        void run_thread( );
        void render_text(ReplayRawFrame *f);
        DisplaySurface *dpy;
        
        FreetypeFont *large_font;
        FreetypeFont *small_font;

        std::vector<ReplayMultiviewerSourceParams> sources;
};

#endif
