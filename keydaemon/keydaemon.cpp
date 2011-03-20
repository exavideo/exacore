/*
 * Copyright 2011 Andrew H. Armenia.
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

#include "character_generator.h"
#include "svg_subprocess_character_generator.h"
#include "decklink.h"

#include <vector>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    std::vector<CharacterGenerator *> cgs;
    RawFrame *frame = NULL;
    RawFrame *cgout = NULL;
    InputAdapter *iadp;
    OutputAdapter *oadp;

    /* initialize the various keyers */
        /* load some configuration, parse command line args or something */

    /* FIXME this is ugly hardcoding */
    cgs.push_back(new SvgSubprocessCharacterGenerator
            ("scoreboard/scoreboard_cg.rb"));


    /* start input and output adapters */
    iadp = create_decklink_input_adapter(0, 0, 0, RawFrame::CbYCrY8422);
    oadp = create_decklink_output_adapter(1, 0, RawFrame::CbYCrY8422);

    /* main loop */
    for (;;) {
        /* get incoming frame */
        if (iadp->output_pipe( ).get(frame) == 0) {
            goto dead_pipe;
        }

        /* get overlay from each CG */
        for (unsigned int i = 0; i < cgs.size( ); i++) {
            CharacterGenerator *cg = cgs[i];

            if (cg->output_pipe( ).get(cgout) == 0) {
                goto dead_pipe;
            }

            /*
             * If no overlay is being rendered by this CG right now, the CG
             * will output a NULL frame. We can safely ignore those.
             */
            if (cgout != NULL) {
                frame->draw->alpha_key(cg->x( ), cg->y( ), cgout, 255);
                delete cgout;
            }
        }

        /* Lastly, send output to the output adapter. */
        if (oadp->input_pipe( ).put(frame) == 0) {
            goto dead_pipe;
        }
    }

dead_pipe:
    /* The program should only reach this point if something goes wrong. */
    fprintf(stderr, "A component has died unceremoniously\n");
    exit(1);
}


