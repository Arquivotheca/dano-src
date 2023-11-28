/* jpeg_interface.cpp */

#include "jpeg_interface.h"

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <OS.h>

#include "jpeglib.h"

#include "jpeg_io_routines.h"

#define TIMING 0

void InitJPEG(	uint32		width,
				uint32		height,
				jpeg_stream	*stream)
{
	stream->linePtrs = (uint8 **)malloc( 4 * height );
}

struct my_error_mgr {
		struct jpeg_error_mgr pub;	/* "public" fields */
  		jmp_buf setjmp_buffer;	/* for return to caller */
};

typedef struct my_error_mgr * my_error_ptr;

METHODDEF(void) my_error_exit (j_common_ptr cinfo)
{
	my_error_ptr myerr = (my_error_ptr) cinfo->err;
	(*cinfo->err->output_message) (cinfo);
	longjmp(myerr->setjmp_buffer, 1);
}

void DecodeJPEG(void 		*inData,
				void		*outData,
				uint32		width,
				uint32		height,
				size_t		inSize,
				jpeg_stream	*stream)
{
	uint32					i;
	jpeg_decompress_struct 	dinfo;
	my_error_mgr 			jerr;

#if TIMING
	bigtime_t 				s = system_time(), e;
#endif
	
  	for (i = 0;i< height; i++) {
		stream->linePtrs[i] = (uint8 *)outData + ((width * 4) * i);
	}

  	dinfo.err = jpeg_std_error(&jerr.pub);
  	jerr.pub.error_exit = my_error_exit;

  	if (setjmp(jerr.setjmp_buffer)) {
    	jpeg_destroy_decompress(&dinfo);
		return;
  	}

  	jpeg_create_decompress(&dinfo);	
	jpeg_init_buffer(&dinfo, inData, inSize);

  	jpeg_read_header(&dinfo, true);

	dinfo.do_fancy_upsampling = false;
	dinfo.do_block_smoothing = false;
   	dinfo.dct_method = JDCT_IFAST;

  	jpeg_start_decompress(&dinfo);

	while (dinfo.output_scanline < dinfo.output_height) {
		jpeg_read_scanlines(&dinfo, stream->linePtrs + dinfo.output_scanline, height);
    }
    	
  	jpeg_destroy_decompress(&dinfo);

#if TIMING
	e = system_time();
	printf("PJPEGNode = %Ld (%.2f)\n", e - s, (double)1000000 / (double)(e - s));
#endif
}

status_t EncodeJPEG(	void 		*inData,
						void		*outData,
						size_t		*outSize,
						uint32		width,
						uint32		height,
						int32		quality,
						jpeg_stream	*stream)
{
	jpeg_compress_struct 	cinfo;
	jpeg_error_mgr 			jerr;
	uint32					lines, y, i;

  	for (i = 0;i< height; i++) {
		stream->linePtrs[i] = (uint8 *)inData + ((width * 4) * i);
	}

  	cinfo.err = jpeg_std_error(&jerr);
  	jpeg_create_compress(&cinfo);	

	jpeg_init_out_buffer(&cinfo, outData, width * height * 4);

	cinfo.image_width = width;
  	cinfo.image_height = height;
	cinfo.input_components = 4;
  	cinfo.in_color_space = JCS_RGB;
   	cinfo.dct_method = JDCT_IFAST;

	jpeg_set_defaults(&cinfo); 	

	jpeg_set_quality(&cinfo, quality, true); 	

	/* not motion jpeg */
 	cinfo.mjpegHeader.outputHeader = false;

 	jpeg_start_compress(&cinfo, true);

	y = 0;

	while (cinfo.next_scanline < cinfo.image_height) {
		lines = jpeg_write_scanlines(&cinfo, stream->linePtrs, height);
		y += lines;
	}

	jpeg_finish_compress(&cinfo);

	my_dest_ptr 		dest;
	dest = (my_dest_ptr) cinfo.dest;

	/* how big was the encoded file we produced */
	*outSize = dest->finalSize;

	jpeg_destroy_compress(&cinfo);

	return B_OK;
}
