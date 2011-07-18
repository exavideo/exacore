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
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

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

POSIXError::POSIXError( ) throw() {
    format_message("unknown source", errno);
}

POSIXError::POSIXError(const char *msg) throw() {
    format_message(msg, errno);
}

POSIXError::POSIXError(const char *msg, int en) throw() {
    format_message(msg, en);
}

void POSIXError::format_message(const char *msg, int en) throw() {
    const char *error_str;

    if (en < 0 || en >= sys_nerr) {
        error_str = "Unknown error";
    } else {
        error_str = sys_errlist[en];
    }

    memset(what_, 0, sizeof(what_));
    snprintf(what_, sizeof(what_) - 1, "%s: OS error: %s", msg, error_str);
}

void xioctl(int fd, int req, void *param) {
    if (ioctl(fd, req, param) == -1) {
        throw POSIXError("ioctl");
    }
}
