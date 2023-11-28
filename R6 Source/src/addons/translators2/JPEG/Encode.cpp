//****************************************************************************************
//
//	File:		Encode.cpp
//
//  Written by:	Ficus Kirkpatrick
//	Revised by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#include "Encode.h"

/* destination manager for compression */

static void init_destination(j_compress_ptr cinfo) {
	be_destination_mgr *dst = (be_destination_mgr *)cinfo->dest;

	dst->buffer = (JOCTET *)
		(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_IMAGE,
				OUTPUT_BUFSIZ * sizeof(JOCTET));
	dst->pub.next_output_byte = dst->buffer;
	dst->pub.free_in_buffer = OUTPUT_BUFSIZ;
}

static boolean empty_output_buffer(j_compress_ptr cinfo) {
	be_destination_mgr *dst = (be_destination_mgr *)cinfo->dest;

	if (dst->io->Write(dst->buffer, OUTPUT_BUFSIZ) != (size_t)OUTPUT_BUFSIZ)
		ERREXIT(cinfo, JERR_FILE_WRITE);

	dst->pub.next_output_byte = dst->buffer;
	dst->pub.free_in_buffer = OUTPUT_BUFSIZ;
	
	return TRUE;
}

static void term_destination(j_compress_ptr cinfo) {
	be_destination_mgr *dst = (be_destination_mgr *)cinfo->dest;
	size_t n = OUTPUT_BUFSIZ - dst->pub.free_in_buffer;

	/* write any pending data */
	if (n > 0) {
		if (dst->io->Write(dst->buffer, n) != n)
			ERREXIT(cinfo, JERR_FILE_WRITE);
	}
}

void be_position_io_dst(j_compress_ptr cinfo, BPositionIO *io) {
	be_destination_mgr *dst;
	
	if (cinfo->dest == NULL) {
		cinfo->dest = (struct jpeg_destination_mgr *)
			(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT,
					sizeof(be_destination_mgr));
	}
	
	dst = (be_destination_mgr *)cinfo->dest;
	dst->pub.init_destination = init_destination;
	dst->pub.empty_output_buffer = empty_output_buffer;
	dst->pub.term_destination = term_destination;
	dst->io = io;
}