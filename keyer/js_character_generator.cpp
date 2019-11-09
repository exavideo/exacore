/*
 * Copyright 2014 Exavideo LLC.
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

#include "js_character_generator.h"
#include "js_character_generator_script.h"

#include <string.h>

JsCharacterGenerator::JsCharacterGenerator(const char *cmd) 
		: CharacterGenerator(1) {
	_cmd = strdup(cmd);
	recv_fd = -1;
	script = NULL;
	start_thread( );
}

JsCharacterGenerator::~JsCharacterGenerator( ) {
	if (script != NULL) {
		delete script;
	}

	delete _cmd;
	close(recv_fd);
	/* FIXME: should wait for child here */
}

void JsCharacterGenerator::do_fork( ) {
    pid_t child;
    int recv_pipe[2];

    if (pipe(recv_pipe) < 0) {
        throw std::runtime_error("pipe recv_pipe failed");
    }

    fprintf(stderr, "JsCharacterGenerator forking...\n");
    child = fork( );

    if (child == -1) {
        throw std::runtime_error("fork failed");
    } else if (child == 0) {
        /* TODO proper error handling here */
        /* make the send pipe stdin and the receive pipe stdout */
	close(STDIN_FILENO);
        dup2(recv_pipe[1], STDOUT_FILENO);

        /* close the unused ends of the pipes */
        close(recv_pipe[0]);

        execlp("/bin/sh", "/bin/sh", "-c", _cmd, (char *) NULL);

        /* if we fall through the exec an error has occurred, so die */
        perror("execlp");
        exit(1);
    } else {
        /* close unused pipe ends */
        close(recv_pipe[1]);
        recv_fd = recv_pipe[0];
    }
}

void JsCharacterGenerator::run_thread( ) {
	RawFrame *frame;
	struct pollfd pfd;

	/* fork server subprocess */
	do_fork( );

	pfd.fd = recv_fd;
	pfd.events = POLLIN;

	for (;;) {
		/* poll to see if there is any input from subprocess */
		if (poll(&pfd, 1, 0) < 0) {
			throw POSIXError("poll");
		}

		/* if data was received, process it */
		if ((pfd.revents & POLLIN) != 0) {
			handle_input( );
		}
		
		if (script != NULL) {
			frame = script->render_frame( );
			_output_pipe.put(frame);
		} else {
			_output_pipe.put(NULL);
		}
	}
}

unsigned int JsCharacterGenerator::dirty_level( ) {
	if (script != NULL) {
		return script->dirty_level( );
	} else {
		/* 
		 * it doesn't matter what we return, because without a script,
		 * we are sending NULL frames
		 */
		return 0;
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

void JsCharacterGenerator::handle_input( ) {
	/* read message header and body */
	uint32_t size = read_item_from_fd<uint32_t>(recv_fd);
	uint8_t type = read_item_from_fd<uint32_t>(recv_fd);
	char *data = (char *) malloc(size + 1);
	memset(data, 0, size+1);

	fprintf(stderr, "handle_input(): expecting %d bytes\n", size);
	
	if (read_all(recv_fd, data, size) != 1) {
		throw std::runtime_error("failed to read whole message");
	}

	fprintf(stderr, "finished reading bytes\n");

	switch (type) {
	case 0:
		/* new script received */
		if (script) {
			delete script;
		}
		fprintf(stderr, "JsCharacterGenerator: compiling script\n");
		script = new JsCharacterGeneratorScript(data, size);
		break;
	case 1:
		/* message for script control */
		if (script) {
			script->send_message(data, size);
		}
		break;
	}

	free(data);
}
