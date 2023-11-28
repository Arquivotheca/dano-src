#include "jpeg6b.h"
#include <stdio.h>
#include "jpeglib.h"
#include <setjmp.h>
#include <malloc.h>
#include <Debug.h>
#include <string.h>

struct my_error_mgr {
	struct jpeg_error_mgr pub;	/* "public" fields */
	
	jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

/*
 * Here's the routine that will replace the standard error_exit method:
 */

static void
my_error_exit (j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr) cinfo->err;
	
	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	// (*cinfo->err->output_message) (cinfo);
	
	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}

void 
JPEGDecompressor::init_source(jpeg_decompress_struct *)
{
}

boolean 
JPEGDecompressor::fill_input_buffer(jpeg_decompress_struct *)
{
	return FALSE;
}

void 
JPEGDecompressor::skip_input_data(jpeg_decompress_struct *cinfo, long num_bytes)
{
	if (num_bytes > 0)
	{
		jpeg_source_mgr *src = cinfo->src;
		JPEGDecompressor *that = (JPEGDecompressor *)cinfo->client_data;
		if (src->bytes_in_buffer <= (size_t)num_bytes)
		{
			that->fSkipMore = (size_t)num_bytes - src->bytes_in_buffer;
			src->bytes_in_buffer = 0;
			src->next_input_byte = 0;
		}
		else
		{
			src->bytes_in_buffer -= num_bytes;
			src->next_input_byte += num_bytes;
			that->fSkipMore = 0;
		}
	}
}

void 
JPEGDecompressor::term_source(jpeg_decompress_struct *)
{
}

JPEGDecompressor::JPEGDecompressor(void)
	: fSkipMore(0), fBuffer(new uint8[BUFFER_SIZE]), fBytesInBuffer(0), fStartDecompressor(false)
{
	fScans[0] = 0;
	fScans[1] = 0;
	fScans[2] = 0;
	fScans[3] = 0;
	// create a new decompressor
	fDecoder = static_cast<jpeg_decompress_struct *>(malloc(sizeof(jpeg_decompress_struct)));
	if (fDecoder)
	{
		jpeg_source_mgr *src = static_cast<jpeg_source_mgr *>(malloc(sizeof(jpeg_source_mgr)));
		if (src)
		{
			fDecoder->err = static_cast<jpeg_error_mgr *>(malloc(sizeof(my_error_mgr)));
			if (fDecoder->err)
			{
				// init the structs
				/* We set up the normal JPEG error routines, then override error_exit. */
				fDecoder->err = jpeg_std_error(fDecoder->err);
				fDecoder->err->error_exit = my_error_exit;
				/* create the decoder */
				jpeg_create_decompress(fDecoder);
				/* remember "this" */
				fDecoder->client_data = (void *)this;
				/* Init our custom source manager */
				fDecoder->src = src;
				src->init_source = init_source;
				src->fill_input_buffer = fill_input_buffer;
				src->skip_input_data = skip_input_data;
				src->resync_to_restart = jpeg_resync_to_restart; // use default
				src->term_source = term_source;
				src->bytes_in_buffer = 0;
				src->next_input_byte = 0;
				// bail out
				return;
			}
			free(src);
		}
		free(fDecoder);
		fDecoder = 0;
	}
}

JPEGDecompressor::~JPEGDecompressor()
{
	if (fDecoder)
	{
		jpeg_destroy_decompress(fDecoder);
		free(fDecoder->err);
		free(fDecoder->src);
		free(fDecoder);
	}
	delete fScans[0];
}

status_t 
JPEGDecompressor::WriteData(const uint8 *buffer, size_t bytes)
{
	if (!fDecoder) return B_ERROR;
	size_t orig_bytes = bytes;
	status_t result = B_OK;

	// prepare the longjump return vector
	if (setjmp((reinterpret_cast<my_error_ptr>(fDecoder->err))->setjmp_buffer))
	{
		// oops, fatal error
		DEBUGGER("fatal error in JPEG decoder");
		return B_ERROR;
	}
	// while more bytes to consume
	while (bytes)
	{
		// any bytes to skip?
		if (bytes <= fSkipMore)
		{
			fSkipMore -= bytes;
			return orig_bytes;
		}
		if (fSkipMore)
		{
			bytes -= fSkipMore;
			buffer += fSkipMore;
			fSkipMore = 0;
		}
		ASSERT(fSkipMore == 0);
		// make sure the decoder has data to work with
		if (fBytesInBuffer < BUFFER_SIZE)
		{
			size_t copybytes = BUFFER_SIZE - fBytesInBuffer;
			if (copybytes > bytes) copybytes = bytes;
			memcpy(fBuffer+fBytesInBuffer, buffer, copybytes);
			bytes -= copybytes;
			fBytesInBuffer += copybytes;
			buffer += copybytes;
		}
		fDecoder->src->bytes_in_buffer = fBytesInBuffer;
		fDecoder->src->next_input_byte = fBuffer;

		// while more scanlines to process
		result = B_OK;
		while (result == B_OK)
		{
			// ensure we've read the header
			if (!fScans[0])
			{
				if (jpeg_read_header(fDecoder, TRUE) == JPEG_HEADER_OK)
				{
					// set the output color space
					switch (fDecoder->jpeg_color_space)
					{
						case JCS_GRAYSCALE:
							fDecoder->out_color_space = JCS_GRAYSCALE; break;
						case JCS_YCbCr:
							fDecoder->out_color_space = JCS_RGB; break;
						case JCS_YCCK:
							fDecoder->out_color_space = JCS_CMYK; break;
						default:
							fDecoder->out_color_space = fDecoder->jpeg_color_space;
							//DEBUGGER("pass through conversion");
							break;
					}
					// 1:1 scaling
					fDecoder->scale_num = 1;
					fDecoder->scale_denom = 1;
					// plain RGB, please
					fDecoder->quantize_colors = FALSE;
					// prep the output_* fields ahead of jpeg_start_decompress()
					jpeg_calc_output_dimensions(fDecoder);
					ASSERT(fDecoder->rec_outbuf_height <= 4);
					// build a buffer to hold the scanlines
					fScans[0] = new uint8[fDecoder->rec_outbuf_height * fDecoder->output_components * fDecoder->output_width];
					fScans[1] = fScans[0] + (fDecoder->output_components * fDecoder->output_width);
					fScans[2] = fScans[1] + (fDecoder->output_components * fDecoder->output_width);
					fScans[3] = fScans[2] + (fDecoder->output_components * fDecoder->output_width);
					// notify the subclass of the image size
					result = ProcessHeader(fDecoder->output_width, fDecoder->output_height, fDecoder->output_components);
					fStartDecompressor = true;
					continue;
				}
				break;
			}
			// start the decompression process
			if (fStartDecompressor)
			{
				if (jpeg_start_decompress(fDecoder) == TRUE)
				{
					fStartDecompressor = false;
					continue;
				}
				break;
			}
			// no more scan lines?
			if (fDecoder->output_scanline >= fDecoder->output_height)
			{
				ImageComplete();
				bytes = 0;
				break;
			}
			// try and read some scanlines
			if (uint rows = jpeg_read_scanlines(fDecoder, fScans, fDecoder->rec_outbuf_height))
			{
				// pawn it off on our descendants
				result = ProcessScanLine(fScans[0], rows);
			}
			else
			{
				ASSERT(fDecoder->src->bytes_in_buffer != BUFFER_SIZE);
				break;
			}
		}
		// move any remaing data to the front of the buffer
		fBytesInBuffer = fDecoder->src->bytes_in_buffer;
		if (fBytesInBuffer)
		{
			memcpy(fBuffer, fDecoder->src->next_input_byte, fBytesInBuffer);
		}
	}
	// we always take all the data, or die trying
	return result == B_OK ? orig_bytes : result;
}

status_t 
JPEGDecompressor::ProcessHeader(uint , uint , uint)
{
	return B_OK;
}

status_t 
JPEGDecompressor::ProcessScanLine(const uint8 *, uint )
{
	return B_OK;
}

void 
JPEGDecompressor::ImageComplete(void)
{
}

