//****************************************************************************************
//
//	File:		Encode.h
//
//  Written by:	Ficus Kirkpatrick
//	Revised by:	Daniel Switkin
//
//	Copyright 1999, Be Incorporated
//
//****************************************************************************************

#ifndef ENCODE_H
#define ENCODE_H

extern "C" {
#include "jinclude.h"
#include "jpeglib.h"
#include "jerror.h"
}

#include <setjmp.h>
#include <DataIO.h>

/* output stuff */

#define OUTPUT_BUFSIZ	4096

typedef struct {
	struct jpeg_destination_mgr pub;
	BPositionIO *io;
	JOCTET *buffer;
} be_destination_mgr;

void be_position_io_dst(j_compress_ptr cinfo, BPositionIO *io);

#endif
