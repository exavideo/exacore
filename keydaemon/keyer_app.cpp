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

#include "keyer_app.h"

KeyerApp::KeyerApp( ) {
    iadp = NULL;
}

KeyerApp::~KeyerApp( ) {

}

void KeyerApp::clear_all_flags( ) {
    unsigned int i;

    for (i = 0; i < flags.size( ); i++) {
        flags[i] = false;
    }
}

void KeyerApp::input(InputAdapter *iadp_) {
    if (iadp != NULL) {
        throw std::runtime_error("cannot define multiple inputs");
    }
    
    iadp = iadp_;
}

void KeyerApp::output(OutputAdapter *oadp_) {
    oadps.push_back(oadp_);
}

void KeyerApp::cg(CharacterGenerator *cg) {
    cgs.push_back(cg);
    flags.push_back(false);
}

void KeyerApp::run( ) {
    RawFrame *frame = NULL;
    RawFrame *cgout = NULL;
    AudioPacket *audio = NULL;

    if (iadp == NULL) {
        throw std::runtime_error("cannot run with no input adapter");
    }

    if (oadps.size( ) == 0) {
        throw std::runtime_error("cannot run with no output adapter");
    }

    if (cgs.size( ) == 0) {
        fprintf(stderr, 
                "Keyer warning: no CGs, just passing through video\n");
    }

    /* main loop */
    try {
        iadp->start( );
        for (;;) {
            /* get incoming frame */
            frame = iadp->output_pipe( ).get( );
            if (iadp->audio_output_pipe( ) != NULL) {
                audio = iadp->audio_output_pipe( )->get( );
            }

            clear_all_flags( ); /* we haven't keyed anything yet */

            std::vector<RawFrame *> stale_frames(oadps.size( ));

            for (unsigned int j = 0; j < oadps.size( ); j++) {
                /* get overlay from each CG */
                for (unsigned int i = 0; i < cgs.size( ); i++) {
                    CharacterGenerator *cg = cgs[i];

                    /*
                     * dirty_level determines which outputs will
                     * get keyed with this CG's output
                     * so skip this CG if its dirty level is higher
                     * than that of the output.
                     * (For now, the output dirty level is just the
                     * order in which the output was added.
                     *
                     * Also, if the key was already added in an
                     * earlier pass, skip it.
                     */
                    if (cg->dirty_level( ) > j || flags[i]) {
                        continue;
                    }
                    
#if 0
                    if (!cg->output_pipe( ).data_ready( )) {
                        if (stale_frames[i] == NULL) {
                            fprintf(stderr, "not keying this frame on account of staleness\n");
                            continue;
                        } else {
                            cgout = stale_frames[i];
                        }
                    } else {
                        delete stale_frames[i];
                        cgout = cg->output_pipe( ).get( );
                        stale_frames[i] = cgout;
                    }
#endif

                    if (!cg->output_pipe( ).data_ready( )) {
                        fprintf(stderr, "not keying this frame on account of staleness\n");
                        continue;
                    } else {
                        cgout = cg->output_pipe( ).get( );
                    }

                    /*
                     * If no overlay is being rendered by this CG right now, the CG
                     * will output a NULL frame. We can safely ignore those.
                     */
                    if (cgout != NULL) {
                        frame->draw->alpha_key(cg->x( ), cg->y( ), 
                                cgout, cgout->global_alpha( ));

                        delete cgout;
                    }
                    /* 
                     * mark this CG as "done" so we don't waste
                     * time later on subsequent passes 
                     */
                    flags[i] = true;
                }

                /* Lastly, send output to the output adapter. */
                oadps[j]->input_pipe( ).put(frame->convert->CbYCrY8422( ));

                if (oadps[j]->audio_input_pipe( ) != NULL && audio != NULL) {
                    oadps[j]->audio_input_pipe( )->put(audio->copy( ));
                }
            }

            delete frame;
            if (audio != NULL) {
                delete audio;
            }
        }
    } catch (BrokenPipe &) {
        fprintf(stderr, "Unexpected component shutdown\n");
    }
}
