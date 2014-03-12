/*
 * Copyright 2011, 2012, 2013 Exavideo LLC.
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

#include "js_character_generator_script.h"
#include "posix_util.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

int main(int argc, const char *argv[]) {
	RawFrame *frame;
	char *script;
	int script_fd;
	struct stat st;

	if (argc != 2) {
		fprintf(stderr, "usage: %s script.js\n", argv[0]);
		return 1;
	}

	script_fd = open(argv[1], O_RDONLY);
	if (script_fd < 0) {
		perror("open");
		return 1;
	}
	
	if (fstat(script_fd, &st) < 0) {
		perror("fstat");
		return 1;
	}

	script = (char *) malloc(st.st_size + 1);
	if (script == NULL) {
		fprintf(stderr, "failed to allocate memory for script\n");
		return 1;
	}

	memset(script, 0, st.st_size + 1);

	if (read_all(script_fd, script, st.st_size) != 1) {
		fprintf(stderr, "failed to read script\n");
		return 1;
	}

	JsCharacterGeneratorScript cg_script(script, strlen(script));	
	for (int i = 0; i < 30; i++) {
		frame = cg_script.render_frame( );
		frame->write_to_fd(STDOUT_FILENO);
		delete frame;
	}

	return 0;
}
