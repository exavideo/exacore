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

#ifndef _OPENREPLAY_WRAP_DRIVERS_HPP
#define _OPENREPLAY_WRAP_DRIVERS_HPP

#include "raw_frame.h"
#include "decklink.h"

class DecklinkConfigProxy {
    protected:
        unsigned int card_index_;
        unsigned int norm_;
        RawFrame::PixelFormat pf_;

    public:
        DecklinkConfigProxy( ) {
            card_index_ = 0;
            norm_ = 0;
            pf_ = RawFrame::CbYCrY8422;
        }

        void card_index(unsigned int ci) {
            card_index_ = ci;
        }

        void norm(unsigned int n) {
            norm_ = n;
        }

        /* void norm(const char *name) { ... } */

        /* void pixel_format(...) */

};

class DecklinkOutputConfigProxy : public DecklinkConfigProxy {
    public:
        DecklinkOutputConfigProxy( ) : DecklinkConfigProxy( ) {
            
        }

        OutputAdapter *construct( ) {
            return create_decklink_output_adapter(card_index_,
                    norm_, pf_);
        }
};


class DecklinkInputConfigProxy : public DecklinkConfigProxy {
    protected:
        unsigned int input_;

    public:
        DecklinkInputConfigProxy( ) : DecklinkConfigProxy( ) {
            input_ = 0;
        }

        void input(unsigned int in) {
            input_ = in;
        }

        InputAdapter *construct( ) {
            return create_decklink_input_adapter(card_index_,
                    norm_, input_, pf_);
        }

        /* void input(const char *name) { ... } */
};

Data_Object<InputAdapter>
construct_decklink_input(void) {
    Data_Object<DecklinkInputConfigProxy> p(new DecklinkInputConfigProxy);
    rb_yield(p);
    return Data_Object<InputAdapter>(p->construct( ));
}

Data_Object<OutputAdapter>
construct_decklink_output(void) {
    Data_Object<DecklinkOutputConfigProxy> p(new DecklinkOutputConfigProxy);
    rb_yield(p);
    return Data_Object<OutputAdapter>(p->construct( ));
}

static void Init_drivers( ) {
    Module rb_mDriver = define_module("Driver");

    rb_mDriver.define_module_function("decklink_input", 
            &construct_decklink_input);

    rb_mDriver.define_module_function("decklink_output",
            &construct_decklink_output);

    rb_mDriver.define_class<DecklinkConfigProxy>("DecklinkConfigProxy")
        .define_method("card_index", &DecklinkConfigProxy::card_index)
        .define_method("norm", &DecklinkConfigProxy::norm);

    rb_mDriver.define_class<InputAdapter>("InputAdapter");
    rb_mDriver.define_class<OutputAdapter>("OutputAdapter");

    rb_mDriver.define_class<DecklinkOutputConfigProxy, DecklinkConfigProxy>
        ("DecklinkOutputConfigProxy");

    rb_mDriver.define_class<DecklinkInputConfigProxy, DecklinkConfigProxy>
        ("DecklinkInputConfigProxy")
            .define_method("input", &DecklinkInputConfigProxy::input);

};

#endif
