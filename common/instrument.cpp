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

#include "instrument.h"
#include "clocks.h"
#include <stdio.h>

Instrument::Instrument(const char *fname, uint64_t threshold_ms) 
        : fn(fname), threshold(threshold_ms) {
    
    start_ms = clock_monotonic_msec( );
}

Instrument::~Instrument() {
    uint64_t time_taken;

    time_taken = clock_monotonic_msec( ) - start_ms;

    if (time_taken > threshold) {
        fprintf(
            stderr, "function %s took %lu msec to finish\n", 
            fn, time_taken
        );
    }
}
