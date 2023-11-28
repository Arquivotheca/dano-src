//****************************************************************************************
//
//	File:		Decode.cpp
//
//  Written by:	Ficus Kirkpatrick
//	Revised by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#include "Decode.h"

/* source manager for decompression */

static void init_source(j_decompress_ptr cinfo) {
	be_source_mgr *m = (be_source_mgr *)cinfo->src;
	m->beginning = TRUE;
}

static boolean fill_input_buffer(j_decompress_ptr cinfo) {
	be_source_mgr *src = (be_source_mgr *)cinfo->src;
	size_t n;

	n = src->io->Read(src->buffer, INPUT_BUFSIZ);
	if (n <= 0) {
		if (src->beginning) {
			ERREXIT(cinfo, JERR_INPUT_EMPTY);
		}

		WARNMS(cinfo, JWRN_JPEG_EOF);
		src->buffer[0] = (JOCTET)0xff;
		src->buffer[1] = (JOCTET)JPEG_EOI;
		n = 2;
	}

	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = n;
	src->beginning = FALSE;
	
	return TRUE;
}

static void skip_input_data(j_decompress_ptr cinfo, long nbytes) {
	be_source_mgr *src = (be_source_mgr *)cinfo->src;
	if (nbytes > 0) {
		while (nbytes > (long)src->pub.bytes_in_buffer) {
			nbytes -= (long)src->pub.bytes_in_buffer;
			fill_input_buffer(cinfo);
		}
		src->pub.next_input_byte += (size_t)nbytes;
		src->pub.bytes_in_buffer -= (size_t)nbytes;
	}
}

static void term_source(j_decompress_ptr cinfo) {
	/* do nothing */
}

void be_position_io_src(j_decompress_ptr cinfo, BPositionIO *io) {
	be_source_mgr *src;

	if (cinfo->src == NULL) {
		/* if this is the first time for this object, set things up
		 * the jpeg shutdown calls will automagically free this
		 * memory for us...bleagh
		 */
		cinfo->src = (struct jpeg_source_mgr *)
			(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT,
				sizeof(be_source_mgr));
		src = (be_source_mgr *)cinfo->src;
		src->buffer = (JOCTET *)
			(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT,
				INPUT_BUFSIZ * sizeof(JOCTET));
	}

	src = (be_source_mgr *)cinfo->src;
	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart;
	src->pub.term_source = term_source;
	src->io = io;
	src->pub.bytes_in_buffer = 0;
	src->pub.next_input_byte = NULL;
}