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
#include "posix_util.h"
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

ssize_t read_all(int fd, void *data, size_t size) {
    ssize_t nread;
    uint8_t *buf = (uint8_t *) data;

    while (size > 0) {
        nread = read(fd, buf, size);
        if (nread > 0) {
            size -= nread;
            buf += nread;
        } else if (nread == 0) {
            /* EOF */
            return 0;
        } else {
            if (errno == EAGAIN || errno == EINTR) {
                /* don't worry about these */
            } else {
                /* return with error to caller */
                return -1;
            }
        }
    }
    return 1;
}

ssize_t write_all(int fd, const void *data, size_t size) {
    ssize_t written;
    uint8_t *buf = (uint8_t *) data;

    while (size > 0) {
        written = write(fd, buf, size);
        if (written > 0) {
            size -= written;
            buf += written;
        } else if (written == 0) {
            /* what exactly does this mean? we'll try again */
        } else {
            if (errno == EAGAIN || errno == EINTR) {
                /* don't worry about these */
            } else {
                /* exit (pass errno through to caller) */
                return -1;
            }
        }
    }

    return 1;
}
