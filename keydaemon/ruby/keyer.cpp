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

#include "rice/Module.hpp"
#include "rice/Data_Type.hpp"
#include "rice/Constructor.hpp"

#include "character_generator.h"
#include "svg_subprocess_character_generator.h"
#include "decklink.h"

#include <vector>
#include <stdio.h>
#include <stdlib.h>

using namespace Rice;

#include "wrap_drivers.hpp"

class Keyer {
    protected:
        std::vector<Data_Object<CharacterGenerator> *> cgs;
        Data_Object<InputAdapter> *iadp;
        Data_Object<OutputAdapter> *oadp;

        void do_mark(void) {
            unsigned int i;

            for (i = 0; i < cgs.size( ); i++) {
                cgs[i]->mark( );
            }

            if (iadp != NULL) {
                iadp->mark( );
            }
            
            if (oadp != NULL) {
                oadp->mark( );
            }
        }

    public:
        Keyer( ) {
            iadp = NULL;
            oadp = NULL;
        }

        ~Keyer( ) {
            unsigned int i;

            delete iadp;
            delete oadp;
            for (i = 0; i < cgs.size( ); i++) {
                delete cgs[i];
            }
        }

        void input(Object iadp_) {
            if (iadp != NULL) {
                throw std::runtime_error("cannot define multiple inputs");
            }
            
            iadp = new Data_Object<InputAdapter>(iadp_);
        }

        void output(Object oadp_) {
            if (oadp != NULL) {
                throw std::runtime_error("cannot define multiple outputs");
            }

            oadp = new Data_Object<OutputAdapter>(oadp_);
        }

        void cg(Object cg) {
            cgs.push_back(new Data_Object<CharacterGenerator>(cg));
        }

        void run( ) {
            RawFrame *frame = NULL;
            RawFrame *cgout = NULL;

            if (iadp == NULL) {
                throw std::runtime_error("cannot run with no input adapter");
            }

            if (oadp == NULL) {
                throw std::runtime_error("cannot run with no output adapter");
            }

            if (cgs.size( ) == 0) {
                fprintf(stderr, 
                        "Keyer warning: no CGs, just passing through video\n");
            }

            /* main loop */
            try {
                for (;;) {
                    /* get incoming frame */
                    frame = (*iadp)->output_pipe( ).get( );

                    /* get overlay from each CG */
                    for (unsigned int i = 0; i < cgs.size( ); i++) {
                        Data_Object<CharacterGenerator> cg = *(cgs[i]);
                        
                        if (!cg->output_pipe( ).data_ready( )) {
                            fprintf(stderr, "not keying this frame on account of staleness\n");
                            continue;
                        }

                        cgout = cg->output_pipe( ).get( );

                        /*
                         * If no overlay is being rendered by this CG right now, the CG
                         * will output a NULL frame. We can safely ignore those.
                         */
                        if (cgout != NULL) {
                            frame->draw->alpha_key(cg->x( ), cg->y( ), 
                                    cgout, cgout->global_alpha( ));
                            delete cgout;
                        }
                    }

                    /* Lastly, send output to the output adapter. */
                    (*oadp)->input_pipe( ).put(frame);
                }
            } catch (BrokenPipe &) {
                fprintf(stderr, "Unexpected component shutdown\n");
            }
        }
};

/* 
 * Take a block. 
 * Create a new Keyer and pass it to the block for configuration.
 * Then, run the Keyer.
 */
void run_keyer(void) {
    Data_Object<Keyer> k(new Keyer);
    rb_yield(k);
    k->run( );
}

class CGConfigProxy {
    protected:
        coord_t x_, y_;

        virtual void base_configure(CharacterGenerator *cg) {
            cg->set_x(x_);
            cg->set_y(y_);
        }

    public:
        CGConfigProxy( ) {
            x_ = 0;
            y_ = 0;
        }

        void x(coord_t xn) {
            x_ = xn;
        }

        void y(coord_t yn) {
            y_ = yn;
        }
};

/*
 * A proxy for configuring the SvgSubprocessCharacterGenerator.
 */
class SvgSubprocessCGConfigProxy : public CGConfigProxy {
    public:
        SvgSubprocessCGConfigProxy( ) {
            _cmd = NULL;
        }

        void command(const char *cmd) {
            if (cmd == NULL || strlen(cmd) == 0) {
                throw std::runtime_error("cannot use null or empty command");
            }
            fprintf(stderr, "accepted command: %s\n", cmd);
            _cmd = strdup(cmd);
        }

        CharacterGenerator *construct( ) {
            if (_cmd == NULL) {
                throw std::runtime_error("command not specified");
            }
            CharacterGenerator *cg = new SvgSubprocessCharacterGenerator(_cmd);
            base_configure(cg);
            return cg;
        }

        ~SvgSubprocessCGConfigProxy( ) {
            free(_cmd);
        }
    protected:
        char *_cmd;

};

/* 
 * Take a block. 
 * Create a new Keyer and pass it to the block for configuration.
 * Then, run the Keyer.
 */
CharacterGenerator *construct_svg_sp_cg(void) {
    Data_Object<SvgSubprocessCGConfigProxy> p(new SvgSubprocessCGConfigProxy);
    rb_yield(p);
    return p->construct( );
    /* p should be GC'd by Ruby */
}

Data_Type<CharacterGenerator> rb_mCharacterGenerator;

template<>
coord_t from_ruby<coord_t>(Object x) {
    VALUE v = x.value( );
    Check_Type(v, T_FIXNUM);
    return (coord_t) FIX2INT(v);
}

extern "C" void Init_keyer( ) {
    /* export our Keyer class to ruby? */
    Init_drivers( );

    Module rb_mKeyer = define_module("Keyer");

    rb_mKeyer.define_module_function("keyer", &run_keyer);
    rb_mKeyer.define_module_function("svg_subprocess_cg", 
            &construct_svg_sp_cg);

    rb_mCharacterGenerator =
        rb_mKeyer.define_class<CharacterGenerator>("CharacterGenerator");

    rb_mKeyer.define_class<Keyer>("KeyerApp")
        .define_constructor(Constructor<Keyer>( ))
        .define_method("input", &Keyer::input)
        .define_method("output", &Keyer::output)
        .define_method("cg", &Keyer::cg);

    rb_mKeyer
        .define_class<CGConfigProxy>("CGConfigProxy")
            .define_method("x", &CGConfigProxy::x)
            .define_method("y", &CGConfigProxy::y);

    rb_mKeyer
        .define_class<SvgSubprocessCGConfigProxy, CGConfigProxy>
                ("SvgSubprocessCGConfigProxy")
            .define_constructor(Constructor<SvgSubprocessCGConfigProxy>( ))
            .define_method("command", &SvgSubprocessCGConfigProxy::command);
}
