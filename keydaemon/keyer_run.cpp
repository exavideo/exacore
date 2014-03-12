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

#include "keyer_app.h"
#include "svg_subprocess_character_generator.h"
#include "js_character_generator.h"
#include "decklink.h"

int main( ) {
	KeyerApp keyer;

	SvgSubprocessCharacterGenerator scbd(
		"cd /opt/scoreboard; ruby scoreboard_server.rb", 1
	);

	scbd.set_x(150);
	scbd.set_y(50);
	keyer.cg(&scbd);

	SvgSubprocessCharacterGenerator bug(
		"cd ..; ruby scoreboard/static_svg.rb /home/rpitv/bug.svg"
	);

	bug.set_x(0);
	bug.set_y(0);
	keyer.cg(&bug);

	JsCharacterGenerator graphics("ruby js_keyer_server.rb");
	graphics.set_x(0);
	graphics.set_y(0);
	keyer.cg(&graphics);

	InputAdapter *inp = create_decklink_input_adapter_with_audio
		(0, 0, 0, RawFrame::CbYCrY8422, 8);
	OutputAdapter *out1 = create_decklink_output_adapter_with_audio
		(1, 0, RawFrame::CbYCrY8422, 8);
	OutputAdapter *out2 = create_decklink_output_adapter_with_audio
		(2, 0, RawFrame::CbYCrY8422, 8);

	keyer.input(inp);
	keyer.output(out1);
	keyer.output(out2);

	keyer.run( );
	return 0;	
}
