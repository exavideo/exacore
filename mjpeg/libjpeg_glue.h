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

#ifndef _OPENREPLAY_LIBJPEG_GLUE_H
#define _OPENREPLAY_LIBJPEG_GLUE_H

#include <stdio.h>
#include "jpeglib.h"
#include <stdint.h>

/* 
 * Some simple routines to set up libjpeg to compress to and decompress from
 * memory blobs, and to get it to throw C++ exceptions.
 */
GLOBAL(void) jpeg_mem_src(j_decompress_ptr cinfo, void *data, size_t len); 
/* 
 * Here len is a pointer to a variable that initially contains the full size 
 * of the data memory region. After compression is done it will contain the
 * number of bytes written to the data buffer.
 */
GLOBAL(void) jpeg_mem_dest(j_compress_ptr cinfo, void *data, size_t *len);
struct jpeg_error_mgr *jpeg_throw_on_error(struct jpeg_error_mgr *error_mgr);

#endif
