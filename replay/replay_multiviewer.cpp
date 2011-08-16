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
#include "freetype_font.h"
#include <stdio.h>

ReplayMultiviewer::ReplayMultiviewer(DisplaySurface *dpy_) {
    dpy = dpy_;
    large_font = new FreetypeFont("../fonts/Inconsolata.otf");
    large_font->set_size(30);
    small_font = new FreetypeFont("../fonts/Inconsolata.otf");
    small_font->set_size(20);
}

ReplayMultiviewer::~ReplayMultiviewer( ) {
    delete large_font;
    delete small_font;
}

void ReplayMultiviewer::add_source(
        const ReplayMultiviewerSourceParams &params) {
    sources.push_back(params);
}

void ReplayMultiviewer::start( ) {
    start_thread( ); 
}

void ReplayMultiviewer::run_thread( ) {
    for (;;) {
        for (unsigned int i = 0; i < sources.size( ); i++) {
            const ReplayMultiviewerSourceParams &src = sources[i];
            ReplayRawFrame *f = src.source->get( );
            if (f != NULL) {
                render_text(f);
                dpy->draw->blit(src.x, src.y, f->frame_data);
            }
        }
        dpy->flip( );
    }
}

void ReplayMultiviewer::render_text(ReplayRawFrame *f) {
    int w = f->frame_data->w( );
    int h = f->frame_data->h( );

    int xt, yt;

    /* source names */
    if (f->source_name != NULL) {
        RawFrame *text;
        if (f->source_name2 == NULL) {
            /* 
             * a simple source, probably an ingest
             * just render its name centered at the bottom
             */
            text = small_font->render_string(f->source_name);
        } else {
            /*
             * complex source like program or preview
             * render its name larger... we'll render the subtitle later
             */
            text = large_font->render_string(f->source_name);
        }
        xt = w / 2 - text->w( ) / 2;
        yt = h - text->h( );
        f->frame_data->draw->alpha_key(xt, yt, text, 255);
        delete text;

        if (f->source_name2 != NULL) {
            /*
             * render the second source name on top of the primary
             */
            text = small_font->render_string(f->source_name2);
            xt = w / 2 - text->w( ) / 2;
            yt = yt - text->h( );
            f->frame_data->draw->alpha_key(xt, yt, text, 255);
            delete text;
        }
    }

    /* timecode */
    #define TIMECODE_BUF_SIZE 80
    #define TIMECODE_FPS 30
    char timecode_buf[TIMECODE_BUF_SIZE];
    timecode_buf[TIMECODE_BUF_SIZE - 1] = 0;
    
    int hours, minutes, seconds, frames;
    /* decompose integer timecode part */
    frames = f->tc;
    
    seconds = frames / TIMECODE_FPS;
    frames %= TIMECODE_FPS;

    minutes = seconds / 60;
    seconds %= 60;
    
    hours = minutes / 60;
    minutes %= 60;
    
    if (f->fractional_tc == Rational(0)) {
        /* no fractional part so don't take up space */
        snprintf(timecode_buf, TIMECODE_BUF_SIZE - 1,
                "%02d:%02d:%02d:%02d",
                hours, minutes, seconds, frames);
    } else {
        /* include the fractional part when present */
        snprintf(timecode_buf, TIMECODE_BUF_SIZE - 1,
                "%02d:%02d:%02d:%02d %d/%d",
                hours, minutes, seconds, frames,
                f->fractional_tc.num( ),
                f->fractional_tc.denom( )
        );
    }

    RawFrame *text = small_font->render_string(timecode_buf);
    /* draw timecode at top left corner */
    f->frame_data->draw->alpha_key(0, 0, text, 255);
    delete text;
}
