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

#include "replay_multiviewer.h"
#include <stdio.h>

ReplayMultiviewer::ReplayMultiviewer(DisplaySurface *dpy_) {
    dpy = dpy_;
}

ReplayMultiviewer::~ReplayMultiviewer( ) {

}

void ReplayMultiviewer::add_source(const SourceParams &params) {
    sources.push_back(params);
}

void ReplayMultiviewer::start( ) {
    start_thread( ); 
}

void ReplayMultiviewer::run_thread( ) {
    for (;;) {
        for (unsigned int i = 0; i < sources.size( ); i++) {
            const SourceParams &src = sources[i];
            ReplayRawFrame *f = src.source->get( );
            if (f != NULL) {
                dpy->draw->blit(src.x, src.y, f->frame_data);
                #if 0
                for (int j = 0; i < f->frame_data->h( ); j++) {
                    fprintf(stderr, "scribbling on the framebuffer??\n");
                    uint8_t *scan = dpy->scanline(src.y + j);
                    scan += 4 * src.x;

                    for (int i = 0; i < f->frame_data->w( ); i++) {
                        scan[0] = 0xff;
                        scan[1] = 0x00;
                        scan[2] = 0x00;
                        scan[3] = 0xff;

                        scan += 4;
                    }
                }
                #endif
            } else {
                fprintf(stderr, "no frame was available??\n");
            }
        }
        
        dpy->flip( );
    }
}
