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
#include <string.h>

const char *script = 
	"({ \n"
	"	assets : [ load_asset('/opt/exacore/test_files/exavideo.png') ],\n"
	"	render : function() {\n"
	"		draw(this.assets[0], 10, 10, 20, 20, 100, 100, 255); \n"
	"	},\n"
	"	dirty_level : 1\n"
	"})";

void compile_and_run_cg_script( ) {
	int i;
	RawFrame *frame;

	JsCharacterGeneratorScript cg_script(script, strlen(script));	
	fprintf(stderr, "script compiled\n");

	fprintf(stderr, "dirty level = %d\n", cg_script.dirty_level( ));
	
	/* render and discard 1 minute of video */
	for (i = 0; i < 1800; i++) {
		frame = cg_script.render_frame( );
		delete frame;
	}
}

int main( ) {
	int i;

	/* compile and run script 10 times */
	for (i = 0; i < 1000; i++) {
		compile_and_run_cg_script( );
	}
}
