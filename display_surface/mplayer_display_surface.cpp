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

#include "mplayer_display_surface.h"

#include <unistd.h>
#include "posix_util.h"

MplayerDisplaySurface::MplayerDisplaySurface(const char *cmd) {
    _w = 1920;
    _h = 1080;
    _pixel_format = BGRAn8;
    _pitch = minpitch( );
    alloc( );
    make_ops( );

    _fd = fork_mplayer(cmd);

    if (_fd == -1) {
        throw std::runtime_error("Failed to fork mplayer!");
    }
}

MplayerDisplaySurface::~MplayerDisplaySurface( ) {
    close(_fd);
}

void MplayerDisplaySurface::flip( ) {
    write_to_fd(_fd);
}

int MplayerDisplaySurface::fork_mplayer(const char *cmd) {
    int pipefd[2];

    if (pipe(pipefd) != 0) {
        throw POSIXError("pipe");
    }

    pid_t child = fork( );
    
    if (child == -1) {
        throw POSIXError("fork");
    } else if (child == 0) {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        
        execl("/bin/sh", "/bin/sh", "-c", cmd, NULL);
        perror("execl");
        _exit(1);
    } else {
        close(pipefd[0]);
        return pipefd[1];
    }
}
