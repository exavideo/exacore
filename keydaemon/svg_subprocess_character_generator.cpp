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

#include "svg_subprocess_character_generator.h"
#include "posix_util.h"
#include "rsvg_frame.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

SvgSubprocessCharacterGenerator::SvgSubprocessCharacterGenerator(
    const char *cmd, unsigned int dirty_level
) : CharacterGenerator(1), _dirty_level(dirty_level) {

    _cmd = strdup(cmd);
    send_fd = -1;
    recv_fd = -1;
    start_thread( );
}

SvgSubprocessCharacterGenerator::~SvgSubprocessCharacterGenerator( ) {
    if (send_fd != -1) {
        close(send_fd);
    }

    if (recv_fd != -1) {
        close(recv_fd);
    }

    free(_cmd);
}

void SvgSubprocessCharacterGenerator::request( ) {
    const char *dummy = "";

    if (write_all(send_fd, dummy, 1) != 1) {
        throw std::runtime_error("dead subprocess?");
    }
}

template <typename T>
static T read_item_from_fd(int fd) {
    T ret;

    if (read_all(fd, &ret, sizeof(ret)) != 1) {
        fprintf(stderr, "it's dead??\n");
        throw std::runtime_error("dead subprocess?");
    }

    return ret;
}

char *SvgSubprocessCharacterGenerator::read_svg(size_t length) {
    char *ret = (char *) malloc(length);

    if (ret == NULL) {
        throw std::runtime_error("memory allocator fail-whaling");
    }

    if (read_all(recv_fd, ret, length) != 1) {
        throw std::runtime_error("dead subprocess?");
    }

    return ret;
}

void SvgSubprocessCharacterGenerator::do_fork( ) {
    pid_t child;
    int send_pipe[2];
    int recv_pipe[2];

    if (pipe(send_pipe) < 0) {
        throw std::runtime_error("pipe send_pipe failed");
    }

    if (pipe(recv_pipe) < 0) {
        throw std::runtime_error("pipe recv_pipe failed");
    }

    child = fork( );

    if (child == -1) {
        throw std::runtime_error("fork failed");
    } else if (child == 0) {
        /* TODO proper error handling here */
        /* make the send pipe stdin and the receive pipe stdout */
        dup2(send_pipe[0], STDIN_FILENO);
        dup2(recv_pipe[1], STDOUT_FILENO);
        /* close the unused ends of the pipes */
        close(send_pipe[1]);
        close(recv_pipe[0]);

        execlp("/bin/sh", "/bin/sh", "-c", _cmd, (char *) NULL);

        /* if we fall through the exec an error has occurred, so die */
        perror("execlp");
        exit(1);
    } else {
        /* close unused pipe ends */
        close(recv_pipe[1]);
        close(send_pipe[0]);

        send_fd = send_pipe[1];
        recv_fd = recv_pipe[0];
    }
}

void SvgSubprocessCharacterGenerator::run_thread( ) {
    RawFrame *frame;
    size_t svg_size;
    uint8_t alpha;
    char *svg_data;
    
    /* fork subprocess */
    do_fork( );

    for (;;) {
        /* request a SVG from the subprocess */
        request( );

        /* get the SVG */
        svg_size = read_item_from_fd<uint32_t>(recv_fd);
        alpha = read_item_from_fd<uint8_t>(recv_fd);
        _dirty_level = read_item_from_fd<uint8_t>(recv_fd);

        if (svg_size > 0) {
            svg_data = read_svg(svg_size);

            /* render SVG to frame */
            frame = RsvgFrame::render_svg(svg_data, svg_size);
            frame->set_global_alpha(alpha);
            free(svg_data);

            /* put frame down the pipe */
            _output_pipe.put(frame);
        } else {
            _output_pipe.put(NULL);
        }
    }
}
