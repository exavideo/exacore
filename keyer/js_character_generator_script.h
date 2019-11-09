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

#include "raw_frame.h"

#include <vector>
#include <v8.h>

class JsCharacterGeneratorScript {
	public:
		JsCharacterGeneratorScript(const char *data, size_t size);
		~JsCharacterGeneratorScript( );

		unsigned int dirty_level( );
		RawFrame *render_frame( );
		void send_message(const char *data, size_t size);

		int load_asset(const char *path);
		void draw(
			int asset, coord_t src_x, coord_t src_y, 
			coord_t dest_x, coord_t dest_y, coord_t w, coord_t h,
			uint8_t galpha
		);

	protected:
		RawFrame *current_frame;
		std::vector<RawFrame *> assets;

		v8::Persistent<v8::Context> v8_context;
		v8::Persistent<v8::Script> v8_script;
		v8::Persistent<v8::Object> v8_object;
};


