/* jpeg_io_routines.h */

#ifndef __JPEG_IO_ROUTINES_H
#define __JPEG_IO_ROUTINES_H

#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"

/* special IO routined for processing a buffer rather than a file */

typedef struct {

	struct jpeg_source_mgr 	pub;	/* public fields */
	void					*ourBuffer;
	size_t					bufferSize;

} my_source_mgr;

typedef my_source_mgr * my_src_ptr;

typedef struct {
  	struct jpeg_destination_mgr pub; /* public fields */

	void					*ourBuffer;
	size_t					bufferSize;
	size_t					finalSize;

} my_destination_mgr;

typedef my_destination_mgr * my_dest_ptr;

void jpeg_init_buffer(j_decompress_ptr cinfo, void *buffer, size_t bufferSize);
void jpeg_init_out_buffer(j_compress_ptr cinfo, void *outBuffer, size_t bufferSize);

#endif
