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

#include "posix_util.h"

/*
 * A channel for communicating objects between threads.
 * Based on a Unix pipe.
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
        Pipe( ) { 
            int pfds[2];

            if (pipe(pfds) != 0) {
                perror("pipe");
                throw std::runtime_error("pipe failed");
            }

            read_fd = pfds[0];
            write_fd = pfds[1];
        }

        int get(T& obj) {
            ssize_t ret = read_all(read_fd, (void *)&obj, sizeof(T));
            if (ret < 0) {
                perror("pipe read");
                throw std::runtime_error("pipe read failed");
            }
            return ret;
        }

        int put(const T& obj) {
            ssize_t ret = write_all(write_fd, (const void *)&obj, sizeof(T));
            if (ret < 0) {
                if (errno == EPIPE) {
                    return 0; /* pipe was closed */
                } else {
                    perror("pipe write");
                    throw std::runtime_error("pipe write failed");
                }
            }
            return ret;
        }

        void done_reading(void) {
            if (close(read_fd) != 0) {
                perror("pipe read end close");
                throw std::runtime_error("pipe read-end close failed");
            }
        }

        void done_writing(void) {
            if (close(write_fd) != 0) {
                perror("pipe write end close");
                throw std::runtime_error("pipe write-end close failed");
            }
        }

        bool data_ready(void) {
            struct pollfd pfd;
            int ret;

            pfd.fd = read_fd;
            pfd.events = POLLIN;

            ret = poll(&pfd, 1, 0);

            if (ret < 0) {
                perror("poll");
                throw std::runtime_error("poll failed");
            }
            
            if (pfd.revents & POLLIN || pfd.revents & POLLHUP) {
                return true;
            } else {
                return false;
            }
        }

        ~Pipe( ) { 

        }

    protected:
        int write_fd, read_fd;
};

#endif
