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

/* 
 * Important Note!
 * This frame synchronizer has a non-zero probability of failure.
 * The probability of a major failure is related to the amount of time
 * for which a reference to a contained object is held onto. So don't hold
 * onto them for long! The probability of failure can be reduced by increasing
 * the argument "n" to the constructor to a suitably large value.
 */

/* FIXME: re-implement using a proper reference counting scheme */

template <class T>
class AsyncPort {
    public:
        AsyncPort(int n = 8) {
            buf = new T*[n];
            for (int i = 0; i < n; i++) { 
                buf[i] = NULL;
            }
            j = 0;
            sz = n;
        }

        /* 
         * Put a new object into the buffer. 
         * delete anything we overwrite.
         */
        void put(T* ptr) {
            MutexLock l(m);
            if (buf[j] != NULL) {
                delete buf[j];
            }

            buf[j] = ptr;
            j++;

            if (j >= sz) {
                j = 0;
            }
        }

        /* 
         * Return the last object written to the buffer.
         * Return NULL if nothing has been written yet.
         */
        T* get( ) {
            MutexLock l(m);
            return buf[(j == 0) ? (sz - 1) : (j - 1)];
        }
    protected:
        T **buf;
        Mutex m;
        int j;
        int sz;
};

#endif

