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

#include "replay_preview.h"
#include "replay_buffer.h"
#include "mjpeg_codec.h"
#include <stdio.h>

ReplayPreview::ReplayPreview( ) {
    current_shot.source = NULL;
    start_thread( );
}

ReplayPreview::~ReplayPreview( ) {

}

void ReplayPreview::change_shot(const ReplayShot &shot) {
    fprintf(stderr, "Preview: updating shot\n");

    MutexLock l(m);
    current_shot = shot;
    current_pos = shot.start;
    update_monitor = true;
    updated.signal( );
}

void ReplayPreview::get_shot(ReplayShot &shot) {
    MutexLock l(m);
    shot = current_shot;
}

void ReplayPreview::seek(timecode_t delta) {
    /* FIXME: do some more error checking here */
    fprintf(stderr, "Preview: seeking\n");

    MutexLock l(m);
    current_pos += delta;
    update_monitor = true;
    updated.signal( );
}

void ReplayPreview::mark_in( ) {
    MutexLock l(m);
    current_shot.start = current_pos;
    /* FIXME: what to do about length here? */
}

void ReplayPreview::mark_out( ) {
    MutexLock l(m);
    if (current_pos > current_shot.start) {
        current_shot.length = current_pos - current_shot.start;
    } else {
        throw IllegalMarkOutError( );
    }
}

void ReplayPreview::run_thread( ) {
    Mjpeg422Decoder dec(1920, 1080);
    ReplayFrameData rfd;
    ReplayRawFrame *monitor_frame;
    RawFrame *new_frame;

    for (;;) {
        /* wait for some work to do */
        wait_update(rfd);

        try {
            /* decode at 1/2 size */
            new_frame = dec.decode(rfd.data_ptr, rfd.data_size, 2);
            fprintf(stderr, "replay monitor frame: %dx%d", (int) new_frame->w( ), (int) new_frame->h( ));
            monitor_frame = new ReplayRawFrame(new_frame->convert->BGRAn8( ));
            delete new_frame;

            monitor.put(monitor_frame);
        } catch (ReplayFrameNotFoundException &e) {
            fprintf(stderr, "replay preview: frame not found\n");
            /* FIXME: put some indication up on the monitor (or get back on the end) */;
        }
    }
}

void ReplayPreview::wait_update(ReplayFrameData &rfd) {
    MutexLock l(m);
    
    /* wait until there is some work to be done */
    while (!update_monitor || current_shot.source == NULL) {
        update_monitor = false;
        updated.wait(m);
    }

    update_monitor = false;

    fprintf(stderr, "updating monitor\n");
    /* grab the frame from the buffer */
    current_shot.source->get_readable_frame(current_pos, rfd);
}
