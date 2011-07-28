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

#include "replay_ingest.h"
#include "replay_buffer.h"
#include "replay_preview.h"
#include "replay_playout.h"
#include "replay_multiviewer.h"
#include "framebuffer_display_surface.h"
#include "decklink.h"
#include "raw_frame.h"
#include "pipe.h"
#include "posix_util.h"
#include <linux/input.h>

int main( ) {
    InputAdapter *iadp;
    OutputAdapter *oadp;
    FramebufferDisplaySurface ds;
    
    /* set up multiviewer */
    ReplayMultiviewer mv(&ds);
    ReplayMultiviewer::SourceParams mvsrc;
    
    /* set up replay buffer */
    ReplayBuffer buf("d1", 2L << 30, 256 << 11, "Cam 1");
    
    /* start input/output */
    iadp = create_decklink_input_adapter(1, 0, 0, RawFrame::CbYCrY8422);
    oadp = create_decklink_output_adapter(0, 0, RawFrame::CbYCrY8422);

    /* start ingest and playout threads */
    ReplayIngest ingest(iadp, &buf);
    mvsrc.source = &ingest.monitor;
    mvsrc.source_name = "Source 1";
    mvsrc.x = 0;
    mvsrc.y = 540;
    mv.add_source(mvsrc);

    ReplayPreview preview;
    mvsrc.source = &preview.monitor;
    mvsrc.source_name = "Preview";
    mvsrc.x = 0;
    mvsrc.y = 0;
    mv.add_source(mvsrc);

    ReplayPlayout playout(oadp);
    mvsrc.source = &playout.monitor;
    mvsrc.source_name = "Program";
    mvsrc.x = 960;
    mvsrc.y = 0;
    mv.add_source(mvsrc);

    mv.start( );

    struct input_event evt;
    int last_value = -1;

    for (;;) {
        if (read_all(STDIN_FILENO, &evt, sizeof(evt)) != 1) {
            break;
        }

        if (evt.type == 1 && evt.code == 260 && evt.value == 1) {
            /* make shot, put into preview */
            ReplayShot *shot = buf.make_shot(-30);
            preview.change_shot(*shot);
            delete shot;
        } else if (evt.type == 1 && evt.code == 257 && evt.value == 1) {
            playout.stop( );
        } else if (evt.type == 1 && evt.code == 259 && evt.value == 1) {
            /* roll out current preview shot */
            ReplayShot shot;
            preview.mark_in( );
            preview.get_shot(shot);
            playout.roll_shot(&shot);
        } else if (evt.type == 2 && evt.code == 7) {
            /* seek preview */
            if (last_value != -1) {
                preview.seek(evt.value - last_value);
            }
            last_value = evt.value;
        }
    }   
}
