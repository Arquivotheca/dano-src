/* mjpega_interface.cpp */

#include "mjpega_interface.h"

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>

#include "jpeglib.h"

#include "jpeg_io_routines.h"

void InitMJPEGA(	uint32		width,
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

void DecodeMJPEGA(void 		*inData,
				void		*outData,
				uint32		width,
				uint32		height,
				size_t		inSize,
				jpeg_stream	*stream)
{
	uint32					i;
	jpeg_decompress_struct 	dinfo;
	my_error_mgr 			jerr;
	int32					fieldOffset, fieldSize;
	int32					y = 0;
	
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
   	dinfo.dct_method = JDCT_IFAST;

  	jpeg_start_decompress(&dinfo);

	fieldOffset = dinfo.mjpegHeader.offsetToNextField;
	fieldSize = dinfo.mjpegHeader.fieldSize;

	/* should of read app1 marker in by now */
	if (fieldOffset > 0) {
		/* twin fields */
	  	for (i = 0;i < height; i++) {
			stream->linePtrs[i] = (uint8 *)outData + ((width * 4) * y);
			y += 2;
		}	
		
	} else {
	  	for (i = 0;i< height; i++) {
			stream->linePtrs[i] = (uint8 *)outData + ((width * 4) * i);
		}		
	}

	while (dinfo.output_scanline < dinfo.output_height) {
		jpeg_read_scanlines(&dinfo, stream->linePtrs + dinfo.output_scanline, dinfo.output_height);
    }
    	
  	jpeg_destroy_decompress(&dinfo);
  	
//  	fieldOffset = 0;
  	
  	if (fieldOffset > 0) {
		/* decompress other field */
	  	jerr.pub.error_exit = my_error_exit;
	
	  	if (setjmp(jerr.setjmp_buffer)) {
	    	jpeg_destroy_decompress(&dinfo);
			return;
	  	}
	
	  	jpeg_create_decompress(&dinfo);	
		jpeg_init_buffer(&dinfo, (uint8 *)inData + fieldOffset, inSize - fieldSize);
	
	  	jpeg_read_header(&dinfo, true);
	
		dinfo.do_fancy_upsampling = false;
	   	dinfo.dct_method = JDCT_IFAST;
	
	  	jpeg_start_decompress(&dinfo);

		y = 1;

	  	for (i = 0;i < height; i++) {
			stream->linePtrs[i] = (uint8 *)outData + ((width * 4) * y);
			y += 2;
		}	

		while (dinfo.output_scanline < dinfo.output_height) {
			jpeg_read_scanlines(&dinfo, stream->linePtrs + dinfo.output_scanline, height);
	    }
	    	
	  	jpeg_destroy_decompress(&dinfo);
  	}
}

status_t EncodeMJPEGA(	void 		*inData,
						void		*outData,
						size_t		*outSize,
						uint32		width,
						uint32		height,
						int32		quality,
						jpeg_stream	*stream)
{
	jpeg_compress_struct 	cinfo;
	jpeg_error_mgr 			jerr;
	int32					lines, y; 
	uint32					i;
	mjpeg_header			*header;
	size_t					headerOffset;

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
 
 	memset(&cinfo.mjpegHeader, 0, sizeof(mjpeg_header));
 
 	cinfo.mjpegHeader.outputHeader = true;
 
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

	headerOffset = dest->bufferSize - cinfo.mjpegHeader.bufferOffset;
	header = (mjpeg_header *)((char *)outData + headerOffset);

	jpeg_destroy_compress(&cinfo);

	return B_OK;
}
