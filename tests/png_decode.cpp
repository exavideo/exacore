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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mjpeg_codec.h"
#include "raw_frame.h"
#include "posix_util.h"
#include "cpu_dispatch.h"
#include "hex_dump.h"

int main(int argc, char **argv) {
    int fd;
    struct stat st;

    if (argc < 2) {
        fprintf(stderr, "usage: %s image.png\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        throw POSIXError("open png file");
    }

    if (fstat(fd, &st) != 0) {
        throw POSIXError("stat png file");
    }

    void *buf = malloc(st.st_size);
    if (read_all(fd, buf, st.st_size) <= 0) {
        throw POSIXError("read png file");
    }

    close(fd);

    RawFrame *frame = RawFrame::from_png_data(buf, st.st_size);
    frame->write_tga_to_fd(STDOUT_FILENO);
    delete frame;
}
