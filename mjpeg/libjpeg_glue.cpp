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

#include "jpeglib.h"
#include <stdexcept>

typedef struct {
    struct jpeg_source_mgr pub;
} mem_source_mgr;

// jpeglib seems to be crudely implementing C++ using C...
// and it doesn't get along with my coding style well.
METHODDEF(void) init_source(j_decompress_ptr cinfo) {
    /* do nothing - it's ready to go already */
}

METHODDEF(boolean) fill_input_buffer(j_decompress_ptr cinfo) {
    /* if we need more data we're doing it wrong */
    ERREXIT(cinfo, JERR_INPUT_EMPTY);
    return false;
}

METHODDEF(void) skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
    if (cinfo->src->bytes_in_buffer < num_bytes) {
        ERREXIT(cinfo, JERR_INPUT_EMPTY);
    } else {
        cinfo->src->next_input_byte += num_bytes;
        cinfo->src->bytes_in_buffer -= num_bytes;
    }
}

METHODDEF(void) term_source(j_decompress_ptr cinfo) {
    /* caller responsible for the memory so don't worry about it */
}

GLOBAL(void) jpeg_mem_src(j_decompress_ptr cinfo, void *data, size_t len) {
    if (cinfo->src == NULL) {
        cinfo->src = (struct jpeg_source_mgr *)
            malloc(sizeof(struct jpeg_source_mgr));

    }
        
    /* just give it the damn pointer */
    cinfo->src->next_input_byte = (JOCTET *)data;
    cinfo->src->bytes_in_buffer = len;

    /* fill in the (quasi-dummy) functions */
    cinfo->src->init_source = init_source;
    cinfo->src->fill_input_buffer = fill_input_buffer;
    cinfo->src->skip_input_data = skip_input_data;
    cinfo->src->resync_to_restart = jpeg_resync_to_restart;
    cinfo->src->term_source = term_source;
}

struct mem_destination_mgr {
    struct jpeg_destination_mgr pub;
    uint8_t *data_ptr;
    size_t total_len;
    size_t *len_ptr;
};

METHODDEF(void) mem_init_destination(j_compress_ptr cinfo) {
    mem_destination_mgr *dest = (mem_destination_mgr *)cinfo->dest;
    dest->pub.next_output_byte = dest->data_ptr;
    dest->pub.free_in_buffer = dest->total_len;
}

METHODDEF(boolean) mem_empty_output_buffer(j_compress_ptr cinfo) {
    ERREXIT(cinfo, JERR_OUT_OF_MEMORY);
    return FALSE;
}

METHODDEF(void) mem_term_destination(j_compress_ptr cinfo) {
    mem_destination_mgr *dest = (mem_destination_mgr *)cinfo->dest;
    *(dest->len_ptr) = dest->total_len - dest->pub.free_in_buffer;
}

GLOBAL(void) jpeg_mem_dest(j_compress_ptr cinfo, void *data, size_t *len) {
    mem_destination_mgr *dest;

    if (cinfo->dest == NULL) {
        cinfo->dest = (struct jpeg_destination_mgr *)
            malloc(sizeof(mem_destination_mgr));
    }

    dest = (mem_destination_mgr *)cinfo->dest;

    dest->pub.init_destination = mem_init_destination;
    dest->pub.empty_output_buffer = mem_empty_output_buffer;
    dest->pub.term_destination = mem_term_destination;
    dest->data_ptr = (uint8_t *)data;
    dest->total_len = *len;
    dest->len_ptr = len;
}

METHODDEF(void) throw_on_error_exit(j_common_ptr cinfo) {
    (*cinfo->err->output_message)(cinfo);
    /* return control to exception handler (or maybe crash spectacularly) */
    throw std::runtime_error("JPEG decode error");
}

struct jpeg_error_mgr *jpeg_throw_on_error(struct jpeg_error_mgr *error_mgr) {
    jpeg_error_mgr *ret;
    ret = jpeg_std_error(error_mgr);
    ret->error_exit = throw_on_error_exit;
    return ret;
}

