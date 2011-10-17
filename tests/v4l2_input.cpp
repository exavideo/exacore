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

#include "v4l2_input.h"
#include "mjpeg_codec.h"
#include "raw_frame.h"
#include "pipe.h"
#include "posix_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main( ) {
    InputAdapter *iadp;

    iadp = create_v4l2_upscaled_input_adapter("/dev/video0");

    for (;;) {
        RawFrame *f = iadp->output_pipe( ).get( );
        f->write_to_fd(STDOUT_FILENO);
        //Mjpeg422Encoder enc(1920, 1080);
        //enc.encode(f);
        delete f;
        //write_all(STDOUT_FILENO, enc.get_data( ), enc.get_data_size( ));
    }
}

