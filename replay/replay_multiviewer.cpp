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
#include "rsvg_frame.h"
#include <stdio.h>
#include <unistd.h>

ReplayMultiviewer::ReplayMultiviewer(DisplaySurface *dpy_) {
    dpy = dpy_;
    large_font = new FreetypeFont("../fonts/Inconsolata.otf");
    large_font->set_size(30);
    small_font = new FreetypeFont("../fonts/Inconsolata.otf");
    small_font->set_size(20);

    waveform_graticule = RsvgFrame::render_svg_file("../assets/waveform.svg");
    vector_graticule = RsvgFrame::render_svg_file("../assets/vectorscope.svg");

    overlay_mode = NONE;
}

ReplayMultiviewer::~ReplayMultiviewer( ) {
    delete large_font;
    delete small_font;
    delete waveform_graticule;
    delete vector_graticule;
}

void ReplayMultiviewer::add_source(
        const ReplayMultiviewerSourceParams &params) {
    sources.push_back(params);
}

void ReplayMultiviewer::start( ) {
    start_thread( ); 
}

void ReplayMultiviewer::change_mode( ) {
    MutexLock l(m);

    switch (overlay_mode) {
        case NONE:
            overlay_mode = WAVEFORM;
            break;

        case WAVEFORM:
            overlay_mode = VECTORSCOPE;
            break;

        case VECTORSCOPE:
        default:
            overlay_mode = NONE;
            break;
    }
}

void ReplayMultiviewer::run_thread( ) {
    overlay_mode_t overlay;

    for (;;) {
        { MutexLock l(m);
            overlay = overlay_mode;
        }
        for (unsigned int i = 0; i < sources.size( ); i++) {
            const ReplayMultiviewerSourceParams &src = sources[i];
            ReplayRawFrame *f = src.source->get( );

            if (f != NULL) {
                f->bgra_data = f->frame_data->convert->BGRAn8( );

                if (f->frame_data->pixel_format( ) == RawFrame::CbYCrY8422) {
                    if (overlay == VECTORSCOPE) {
                        render_vector(f);
                    } else if (overlay == WAVEFORM) {
                        render_waveform(f);
                    }
                }

                render_text(f);
                
                dpy->draw->blit(src.x, src.y, f->bgra_data);
            }
        }
        dpy->flip( );
    }
    usleep(20000);
}

void ReplayMultiviewer::render_vector(ReplayRawFrame *f) {
    /* make a copy of the graticule */
    RawFrame *vector = vector_graticule->convert->BGRAn8( );
    RawFrame *src = f->frame_data;

    /* plot each Cb/Cr vector */
    uint8_t cb, cr;
    size_t s = src->size( );
    uint8_t *d = src->data( );
    for (size_t i = 0; i < s; i += 4) {
        cb = d[i];
        cr = d[i+2];

        vector->scanline(255 - cb)[4*cr] = 0xff;
        vector->scanline(255 - cb)[4*cr+1] = 0xff;
        vector->scanline(255 - cb)[4*cr+2] = 0xff;
    }

    f->bgra_data->draw->alpha_key(112, 7, vector, 255);
    delete vector;
}

void ReplayMultiviewer::render_waveform(ReplayRawFrame *f) {
    RawFrame *wfm = waveform_graticule->convert->BGRAn8( );
    RawFrame *src = f->frame_data;

    coord_t h = src->h( );      /* height */
    coord_t w = src->w( );      /* source width */
    coord_t st = w / 256;       /* stride */
    uint8_t *d = src->data( );
    uint8_t y;                  /* luma */
    const coord_t xofs = 16;

    for (coord_t line = 0; line < h; y++) {
        d = src->scanline(line);
        for (coord_t x = 0; x < w; x += st) {
            y = d[2*x];
            wfm->scanline(255 - y)[4*x + 4*xofs] = 0xff;
            wfm->scanline(255 - y)[4*x + 4*xofs + 1] = 0xff;
            wfm->scanline(255 - y)[4*x + 4*xofs + 2] = 0xff;
        }
    }

    f->bgra_data->draw->alpha_key(112, 72, wfm, 255);
    delete wfm;
}

void ReplayMultiviewer::render_text(ReplayRawFrame *f) {
    int w = f->bgra_data->w( );
    int h = f->bgra_data->h( );

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
        f->bgra_data->draw->alpha_key(xt, yt, text, 255);
        delete text;

        if (f->source_name2 != NULL) {
            /*
             * render the second source name on top of the primary
             */
            text = small_font->render_string(f->source_name2);
            xt = w / 2 - text->w( ) / 2;
            yt = yt - text->h( );
            f->bgra_data->draw->alpha_key(xt, yt, text, 255);
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
    f->bgra_data->draw->alpha_key(0, 0, text, 255);
    delete text;
}
