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

#include <linux/input.h>
#include "posix_util.h"
#include "evdev.h"

/* Wrapper to allow easy reading of events into Ruby */
int read_event(int fd, struct InputEvent &evt) {
    struct input_event real_evt;
    ssize_t ret;

    ret = read_all(fd, &real_evt, sizeof(real_evt)); 
    
    if (ret < 0) {
        throw POSIXError("read_event");
    } else if (ret == 0) {
        return 0;
    } else {
        evt.type = real_evt.type;
        evt.code = real_evt.code;
        evt.value = real_evt.value;
    }

    return 1;
}
