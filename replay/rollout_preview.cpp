/*
 * Copyright 2011, 2013 Exavideo LLC.
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

#include "rollout_preview.h"
#include "replay_playout_lavf_source.h"
#include <stdio.h>

RolloutPreview::RolloutPreview( ) {
    start_thread( );
}

RolloutPreview::~RolloutPreview( ) {

}

void RolloutPreview::load_file(const char *fn) {
    MutexLock l(m);
    filename = fn;
    pos = 0;
    updated.signal( );
}


void RolloutPreview::seek(int64_t delta) {
    MutexLock l(m);
    pos += delta;
    
    if (pos < 0) {
        pos = 0;
    }

    updated.signal( );
}

void RolloutPreview::run_thread( ) {
    std::string current_file;
    std::string new_file;
    ReplayPlayoutLavfSource *current_source = NULL;
    ReplayPlayoutFrame frame_data;
    Rational speed(1,1);
    RawFrame *img;
    ReplayRawFrame *monitor_frame;

    int64_t current_pos, new_pos;
    bool do_update;

    for (;;) {
        /* wait for some work to do */
        wait_update(new_file, new_pos);

        do_update = false;
        /* if we are going to a new file, open the file */
        if (new_file != current_file) {
            if (current_source != NULL) {
                delete current_source;
                current_source = NULL;
            }

            current_source = new ReplayPlayoutLavfSource(new_file.c_str());
            current_file = new_file;
            current_pos = 0;
            do_update = true;
        }

        /* if we have a new position, seek the file */
        if (new_pos != current_pos) {
            current_pos = new_pos;
            do_update = true;

            if (current_source != NULL) {
                current_source->seek(new_pos);
            }
        }

        if (do_update && current_source != NULL) {
            /* decode a frame of video */
            current_source->read_frame(frame_data, speed);
            
            img = frame_data.video_data;
            
            /* put 1/2 scale version to multiviewer */
            monitor_frame = new ReplayRawFrame(img->convert->BGRAn8_scale_1_2());
            monitor_frame->source_name = "Preview";
            monitor_frame->source_name2 = current_file.c_str();
            monitor_frame->tc = 0;
            monitor.put(monitor_frame);

            /* delete decoded data so we don't leak it */
            delete frame_data.video_data;
            delete frame_data.audio_data;
        }
    }
}


void RolloutPreview::wait_update(std::string &fn, int64_t &new_pos) {
    MutexLock l(m);
    
    /* wait until there is some work to be done */
    while (filename == "") {
        updated.wait(m);
    }


    fn = filename;
    new_pos = pos;
}

void RolloutPreview::get(std::string &fn, int64_t &p) {
    MutexLock l(m);

    fn = filename;
    p = pos;
}
