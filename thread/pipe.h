#ifndef _PIPE_H
#define _PIPE_H

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

#include <unistd.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <stdexcept>
#include <assert.h>

#include "posix_util.h"
#include "mutex.h"
#include "condition.h"

/*
 * Thrown when a broken pipe condition is detected.
 */
class BrokenPipe : public virtual std::exception {
    const char *what( ) const throw( ) { 
        return "Broken pipe between threads\n";
    }
};

/*
 * A channel for communicating objects between threads.
 * 
 * Methods:
 *
 * int get(T& obj): retrieve an object from the pipe.
 *  This blocks until an object is available.
 *  Returns 1 if an object was received, zero if the pipe has been closed.
 *
 * int put(T& obj): put object onto the pipe.
 *  This may also block until a thread tries to get( ).
 *  Returns 1 when the object is put. Zero if the pipe has been closed.
 *
 * void close_read(void):
 *  Close reader end of the pipe.
 *
 * void close_write(void):
 *  Close writer end of the pipe.
 */
template <class T>
class Pipe {
    public:
        Pipe(unsigned int buf_len_) { 
            buf_len = buf_len_;
            read_ptr = 0;
            write_ptr = 0;

            buf = new T*[buf_len];

            for (unsigned int i = 0; i < buf_len; ++i) {
                buf[i] = NULL;
            }

            read_done = false;
            write_done = false;
        }

        T get() {
            T obj;
            { MutexLock lock(mut);
                while (empty( )) {
                    if (write_done) {
                        throw BrokenPipe( );
                    } else {
                        pipe_not_empty.wait(mut);
                    }
                }

                /* get object out and adjust state */
                assert(buf[read_ptr] != NULL);
                obj = *(buf[read_ptr]);
                delete buf[read_ptr];
                buf[read_ptr] = NULL;

                read_ptr = advance(read_ptr);

                /* signal not full */
                pipe_not_full.signal( );
            }
            return obj;
        }

        void put(const T& obj) {
            { MutexLock lock(mut);
                if (read_done) {
                    throw BrokenPipe( );
                }

                while (full( )) {
                    if (read_done) {
                        throw BrokenPipe( );
                    } else {
                        pipe_not_full.wait(mut);
                    }
                }

                /* put object in and adjust state */
                assert(buf[write_ptr] == NULL);
                buf[write_ptr] = new T(obj);

                write_ptr = advance(write_ptr);

                /* signal not empty */
                pipe_not_empty.signal( );
            }
        }

        void done_reading(void) {
            { MutexLock lock(mut);
                read_done = true;
                /* signal "not full" so producer does not deadlock */
                pipe_not_full.signal( );
            }
        }

        void done_writing(void) {
            { MutexLock lock(mut);
                write_done = true;
                /* signal "not empty" so consumer does not deadlock */
                pipe_not_empty.signal( );
            }
        }

        bool data_ready(void) {
            bool ret;

            { MutexLock lock(mut);
                if (write_ptr != read_ptr) {
                    ret = true;
                } else {
                    ret = false;
                }
            }

            return ret;
        }

        bool can_put(void) {
            MutexLock lock(mut);

            if (full( )) {
                return false;
            } else {
                return true;
            }
        }

        ~Pipe( ) { 
            for (unsigned int i = 0; i < buf_len; i++) {
                if (buf[i] != NULL) {
                    delete buf[i];
                }
            }
            delete [] buf;
        }

    protected:
        /* Return the position one past "i". */
        unsigned int advance(unsigned int i) {
            assert(i < buf_len);

            if (i == buf_len - 1) {
                return 0;
            } else {
                return i + 1;
            }
        }

        /* WARNING!  These assume the mutex is already locked. */
        bool empty(void) {
            if (read_ptr == write_ptr) {
                return true;
            } else { 
                return false;
            }
        }


        bool full(void) {
            if (advance(write_ptr) == read_ptr) {
                return true;
            } else {
                return false;
            }
        }

        /* 
         * read_ptr points to the first full slot. 
         * write_ptr points to the next writable slot.
         * special case: empty when read_ptr == write_ptr.
         * That means that full is when write_ptr == read_ptr - 1.
         * And in that condition, there is a bubble.
         */
        unsigned int buf_len, read_ptr, write_ptr;

        T **buf;
        Mutex mut;
        Condition pipe_not_full, pipe_not_empty;

        bool read_done, write_done;
};

#endif
