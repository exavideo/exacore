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

#ifndef _OPENREPLAY_ASYNC_PORT_H
#define _OPENREPLAY_ASYNC_PORT_H

#include "mutex.h"
#include <atomic>

template <class T>
class AsyncPort {
    public:
        AsyncPort( ) {
            buf = NULL;
            readbuf = NULL;
        }

        /* 
         * Put new value into buf. If something is already in buf,
         * it has not yet been picked up by the receiving side of
         * the port. Thus, we delete it.
         */
        void put(T* ptr) {
            T* old_value = buf.exchange(ptr);
            if (old_value != NULL) {
                delete old_value;
            }
        }

        /*
         * Read value from buf/readbuf.
         * We atomically read buf and set it to null. Thus, a pointer in
         * the read side of the port cannot be deleted except by another read.
         */
        T* get( ) {
            T* new_value = buf.exchange(NULL);
            if (new_value != NULL) {
                delete readbuf;
                readbuf = new_value;
            }

            return readbuf;
        }
    protected:
        T* readbuf;
        std::atomic<T*> buf;
};

#endif

