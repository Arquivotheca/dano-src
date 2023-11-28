/* jpeg_io_routines.cpp */

#include <stdio.h>
#include <stdlib.h>

#include "jpeg_io_routines.h"

METHODDEF(void)
init_source (j_decompress_ptr cinfo)
{
}

METHODDEF(boolean)
fill_input_buffer (j_decompress_ptr cinfo)
{
	/* fill input buffer once! */

  	my_src_ptr 		src = (my_src_ptr)cinfo->src;

	if (!src->bufferSize) {
		/* called again to reload buffer */
		printf("JPEG asked for reload!!\n");
	}

  	src->pub.next_input_byte = (const JOCTET *)src->ourBuffer;
	src->pub.bytes_in_buffer = src->bufferSize;

	src->bufferSize = 0;	

	return true;
}

METHODDEF(void)
skip_input_data (j_decompress_ptr cinfo, long num_bytes)
{
	my_src_ptr 		src = (my_src_ptr)cinfo->src;

  	if (num_bytes > 0) {
	    src->pub.next_input_byte += (size_t) num_bytes;
	    src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}

METHODDEF(void)
term_source (j_decompress_ptr cinfo)
{
	/* no work necessary here */
}

/* build io device */

void jpeg_init_buffer(j_decompress_ptr cinfo, void *buffer, size_t bufferSize)
{
  	my_src_ptr 	src;

  	if (cinfo->src == NULL) {	/* first time for this JPEG object? */
    	cinfo->src = (struct jpeg_source_mgr *)(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,SIZEOF(my_source_mgr));
		src = (my_src_ptr)cinfo->src; 
  	}

	src = (my_src_ptr) cinfo->src;
	src->pub.init_source = init_source;
	src->pub.fill_input_buffer = fill_input_buffer;
	src->pub.skip_input_data = skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->pub.term_source = term_source;

	src->ourBuffer = buffer;
	src->bufferSize = bufferSize;

	src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
	src->pub.next_input_byte = NULL; /* until buffer loaded */
}

/* output routines */

METHODDEF(void)
init_destination (j_compress_ptr cinfo)
{
  	my_dest_ptr 		dest = (my_dest_ptr)cinfo->dest;

  	dest->pub.next_output_byte = (JOCTET *)dest->ourBuffer;
  	dest->pub.free_in_buffer = dest->bufferSize;
}

METHODDEF(boolean)
empty_output_buffer (j_compress_ptr cinfo)
{
  	my_dest_ptr 		dest = (my_dest_ptr)cinfo->dest;

  	dest->pub.next_output_byte = (JOCTET *)dest->ourBuffer;
  	dest->pub.free_in_buffer = dest->bufferSize;

  	return true;
}

METHODDEF(void)
term_destination (j_compress_ptr cinfo)
{
  	my_dest_ptr 		dest = (my_dest_ptr)cinfo->dest;

	dest->finalSize = dest->bufferSize - dest->pub.free_in_buffer;
}

void jpeg_init_out_buffer(j_compress_ptr cinfo, void *outBuffer, size_t bufferSize)
{
  my_dest_ptr dest;

  /* The destination object is made permanent so that multiple JPEG images
   * can be written to the same file without re-executing jpeg_stdio_dest.
   * This makes it dangerous to use this manager and a different destination
   * manager serially with the same JPEG object, because their private object
   * sizes may be different.  Caveat programmer.
   */
  if (cinfo->dest == NULL) {	/* first time for this JPEG object? */
    cinfo->dest = (struct jpeg_destination_mgr *)
      (*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
				  SIZEOF(my_destination_mgr));
  }

  	dest = (my_dest_ptr) cinfo->dest;
  	dest->pub.init_destination = init_destination;
  	dest->pub.empty_output_buffer = empty_output_buffer;
	dest->pub.term_destination = term_destination;

	dest->ourBuffer = outBuffer;
	dest->bufferSize = bufferSize;
}
