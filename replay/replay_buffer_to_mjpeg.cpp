/*
 * Copyright 2013 Exavideo LLC.
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
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "posix_util.h"
#include "block_set.h"

#define REPLAY_VIDEO_BLOCK "ReplJpeg"

void process(int input_fd, int output_fd);

int main(int argc, const char **argv) {
    int input_fd, output_fd;

    if (argc != 3) {
        fprintf(stderr, "usage: replay_buffer_to_mjpeg <replay_buffer> <mjpeg_file>");
    }

    /* input must be seekable - i.e. a file */
    input_fd = open(argv[1], O_RDONLY);
    if (input_fd < 0) {
        fprintf(stderr, "cannot open buffer file %s: %s\n",
            argv[1], strerror(errno));
        exit(1);
    }

    /* output can be to stdout or file */
    if (!strcmp(argv[2], "-")) {
        output_fd = STDOUT_FILENO;
    } else {
        output_fd = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (output_fd < 0) {
            fprintf(stderr, "cannot open output file %s: %s\n",
                argv[2], strerror(errno));
            exit(1);
        }
    }

    process(input_fd, output_fd);

    return 0;
}

void process(int input_fd, int output_fd) {
    off_t current_offset = 0;
    uint8_t *mjpeg_data;
    size_t size;

    for (;;) {
        BlockSet blkset;
        try {
            blkset.begin_read(input_fd, current_offset);
            mjpeg_data = blkset.load_alloc_block<uint8_t>(REPLAY_VIDEO_BLOCK, size);
        } catch (...) {
            /* 
             * if it bombs out we are probably past end of file but who knows
             * so we just close the output_fd and rethrow. This is really ugly
             * but will make sure that any really bad errors get printed
             * rather than ignored.
             */
            close(input_fd);
            close(output_fd);
            throw;
        }
        
        write_all(output_fd, mjpeg_data, size);
        delete [] mjpeg_data;
        current_offset = blkset.end_offset( );
    }
}

