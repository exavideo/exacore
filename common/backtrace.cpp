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

#include <execinfo.h>
#include <unistd.h>

void print_backtrace( ) {
    const unsigned int TRACE_SIZE = 64;
    void *trace[TRACE_SIZE];
    int size;

    size = backtrace(trace, TRACE_SIZE);
    backtrace_symbols_fd(trace, size, STDERR_FILENO);
}

