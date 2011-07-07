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
#include "replay_playout.h"
#include "decklink.h"
#include "raw_frame.h"
#include "pipe.h"

int main( ) {
    InputAdapter *iadp;
    OutputAdapter *oadp;

    ReplayShot *shot;
    
    /* set up replay buffer */
    ReplayBuffer buf("d1", 2L << 30, 256 << 10);
    
    /* start input/output */
    iadp = create_decklink_input_adapter(2, 0, 0, RawFrame::CbYCrY8422);
    oadp = create_decklink_output_adapter(0, 0, RawFrame::CbYCrY8422);

    /* start ingest and playout threads */
    ReplayIngest ingest(iadp, &buf);
    ReplayPlayout playout(oadp);

    /* wait a while */
    sleep(10);

    /* roll replay! */
    fprintf(stderr, "ROLL REPLAY...");
    shot = buf.make_shot(-150); /* five seconds back from now */
    playout.roll_shot(shot);
    fprintf(stderr, "rolling!\n");

    /* let it roll for a bit before we exit unceremoniously */
    for (;;)    sleep(10);
}
