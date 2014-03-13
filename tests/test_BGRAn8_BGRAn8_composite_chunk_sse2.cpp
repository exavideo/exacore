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

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

extern "C" void BGRAn8_BGRAn8_composite_chunk_sse2(
	uint8_t *dst, uint8_t *src, size_t count, 
	unsigned int global_alpha
);

uint8_t background[] = {
	0, 0, 0, 255,
	0, 0, 0, 255,
	0, 0, 0, 255,
	0, 0, 0, 255
};

uint8_t key[] = {
	0, 0, 0, 255,
	255, 0, 0, 255,
	0, 255, 0, 255,
	0, 0, 255, 255
};

int main( ) {
	BGRAn8_BGRAn8_composite_chunk_sse2(background, key, sizeof(key), 255);

	for (size_t i = 0; i < sizeof(key); i += 4) {
		printf("r=%d g=%d b=%d a=%d\n", 
			background[i+2],
			background[i+1],
			background[i+0],
			background[i+3]
		);
	}

	return 0;
}

