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

#include "clocks.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>

int main( ) {
    uint64_t msec;

    msec = clock_monotonic_msec( );
    fprintf(stderr, "clock=%lu\n", msec);
    usleep(500000);
    msec = clock_monotonic_msec( );
    fprintf(stderr, "clock=%lu\n", msec);

    return 0;
}
