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

class ReplayMultiviewer : public Thread {
    public:
        ReplayMultiviewer(DisplaySurface *dpy_);
        ~ReplayMultiviewer( );

        struct SourceParams {
            AsyncPort<ReplayRawFrame> *source;
            const char *source_name;
            coord_t x, y;
        };

        void add_source(const SourceParams &params);
        void start( );

    protected:
        void run_thread( );
        DisplaySurface *dpy;

        std::vector<SourceParams> sources;
};
