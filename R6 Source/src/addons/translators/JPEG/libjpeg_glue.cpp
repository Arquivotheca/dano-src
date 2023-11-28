/* libjpeg_glue.cpp
 *
 * functions for sticking libjpeg and the translation kit together
 * (source and destination managers and whatever else comes about)
 *
 */

#include "libjpeg_glue.h"

/* source manager for decompression */

static void
init_source(j_decompress_ptr cinfo)
{
	be_source_mgr *m = (be_source_mgr *)cinfo->src;
	m->beginning = TRUE;
}

static boolean
fill_input_buffer(j_decompress_ptr cinfo)
{
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

static void
skip_input_data(j_decompress_ptr cinfo, long nbytes)
{
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

static void
term_source(j_decompress_ptr cinfo)
{
	/* do nothing */
}

void
be_position_io_src(j_decompress_ptr cinfo, BPositionIO *io)
{
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

/* destination manager for compression */

static void
init_destination(j_compress_ptr cinfo)
{
	be_destination_mgr *dst = (be_destination_mgr *)cinfo->dest;

	dst->buffer = (JOCTET *)
		(*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_IMAGE,
				OUTPUT_BUFSIZ * sizeof(JOCTET));
	dst->pub.next_output_byte = dst->buffer;
	dst->pub.free_in_buffer = OUTPUT_BUFSIZ;
}

static boolean
empty_output_buffer(j_compress_ptr cinfo)
{
	be_destination_mgr *dst = (be_destination_mgr *)cinfo->dest;

	if (dst->io->Write(dst->buffer, OUTPUT_BUFSIZ) != (size_t)OUTPUT_BUFSIZ)
		ERREXIT(cinfo, JERR_FILE_WRITE);

	dst->pub.next_output_byte = dst->buffer;
	dst->pub.free_in_buffer = OUTPUT_BUFSIZ;
	
	return TRUE;
}

static void
term_destination(j_compress_ptr cinfo)
{
	be_destination_mgr *dst = (be_destination_mgr *)cinfo->dest;
	size_t n = OUTPUT_BUFSIZ - dst->pub.free_in_buffer;

	/* write any pending data */
	if (n > 0) {
		if (dst->io->Write(dst->buffer, n) != n)
			ERREXIT(cinfo, JERR_FILE_WRITE);
	}
}

void
be_position_io_dst(j_compress_ptr cinfo, BPositionIO *io)
{
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

/* error handling stuff */

static void
be_error_exit(j_common_ptr cinfo)
{
	be_error_mgr *mgr = (be_error_mgr *)cinfo->err;
	(*cinfo->err->output_message)(cinfo);
	longjmp(mgr->setjmp_buffer, 1);
}

struct jpeg_error_mgr *
be_std_error(struct jpeg_error_mgr *err)
{
	struct jpeg_error_mgr *ret;
	
	ret = jpeg_std_error(err);
	ret->error_exit = be_error_exit;
	/* XXXficus probably want to make output_message do a BAlert */

	return err;
}
