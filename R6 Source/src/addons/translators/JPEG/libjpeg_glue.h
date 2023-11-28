/* libjpeg_glue.h */

#ifndef LIBJPEG_GLUE_H
#define LIBJPEG_GLUE_H

extern "C" {
#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"
}

#include <setjmp.h>
#include <DataIO.h>

/* input stuff */

#define INPUT_BUFSIZ	4096

typedef struct {
	struct jpeg_source_mgr pub;		/* libjpeg stuff */
	boolean beginning;				/* are we at the beginning of the stream? */
	BPositionIO *io;				/* the associated IO stream */
	JOCTET *buffer;					/* duh */
} be_source_mgr;

void be_position_io_src(j_decompress_ptr cinfo, BPositionIO *io);

/* output stuff */

#define OUTPUT_BUFSIZ	4096

typedef struct {
	struct jpeg_destination_mgr pub;
	BPositionIO *io;
	JOCTET *buffer;
} be_destination_mgr;

void be_position_io_dst(j_compress_ptr cinfo, BPositionIO *io);

/* error handling */

typedef struct {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
} be_error_mgr;

jpeg_error_mgr *be_std_error(jpeg_error_mgr *err);

#endif /* LIBJPEG_GLUE_H */
