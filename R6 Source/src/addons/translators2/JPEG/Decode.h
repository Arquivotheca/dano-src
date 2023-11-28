//****************************************************************************************
//
//	File:		Decode.h
//
//  Written by:	Ficus Kirkpatrick
//	Revised by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#ifndef DECODE_H
#define DECODE_H

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

#endif