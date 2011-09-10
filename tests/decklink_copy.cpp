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

#include "decklink.h"
#include "raw_frame.h"
#include "pipe.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main( ) {
    InputAdapter *iadp;
    OutputAdapter *oadp;

    iadp = create_decklink_input_adapter(1, 0, 0, RawFrame::CbYCrY8422);
    oadp = create_decklink_output_adapter(0, 0, RawFrame::CbYCrY8422);

    for (;;) {
        oadp->input_pipe( ).put(iadp->output_pipe( ).get( ));
    }
}

