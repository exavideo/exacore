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

#ifndef _OPENREPLAY_OUTPUT_ADAPTER_H
#define _OPENREPLAY_OUTPUT_ADAPTER_H

#include "pipe.h"
#include "raw_frame.h"

class OutputAdapter {
    public:
        virtual Pipe<RawFrame *> &input_pipe( ) = 0;
};

OutputAdapter *create_decklink_output_adapter(unsigned int card_index,
        unsigned int decklink_norm);

#endif
