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

#include <stdlib.h>
#include <new>

void *operator new(std::size_t sz) throw(std::bad_alloc) {
    if (sz == 0) {
        sz = 1;
    }

    void *ret = malloc(sz);

    if (ret == NULL) {
        throw std::bad_alloc( );
    } else {
        return ret;
    }
}

void operator delete(void *ptr) throw() {
    free(ptr);
}
