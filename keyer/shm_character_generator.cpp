/*
 * Copyright 2016 Andrew H. Armenia.
 * 
 * This file is part of exacore.
 * 
 * exacore is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * exacore is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with exacore.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "shm_character_generator.h"
#include "posix_util.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


ShmCharacterGenerator::ShmCharacterGenerator(
    const char *cmd, unsigned int dirty_level
) : CharacterGenerator(1), _dirty_level(dirty_level), shm_buf(8294400) {
    _cmd = strdup(cmd);
    start_thread( );
}

ShmCharacterGenerator::~ShmCharacterGenerator( ) {
    free(_cmd);
}

void ShmCharacterGenerator::do_fork( ) {
    pid_t child;
    child = fork( );

    if (child == -1) {
        throw std::runtime_error("fork failed");
    } else if (child == 0) {
        /* pass the child process the shm buffer as fd 9 */
        dup2(shm_buf.get_fd(), 9);
        execl("/bin/sh", "/bin/sh", "-c", _cmd, (char *) NULL);

        /* if we fall through the exec an error has occurred, so die */
        perror("execlp");
        exit(1);
    } 
}

void ShmCharacterGenerator::run_thread( ) {
    RawFrame *frame;
    const void *data;

    /* fork subprocess */
    do_fork( );

    for (;;) {
        /* just copy frames from the shm_buf as fast as we can */
        frame = new RawFrame(1920, 1080, RawFrame::BGRAn8);
    
        shm_buf.begin_read(data);
        memcpy(frame->data(), data, frame->size());
        shm_buf.end_read();
        
        _output_pipe.put(frame);

    }
}

