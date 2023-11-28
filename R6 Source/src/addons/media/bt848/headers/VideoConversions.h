//******************************************************************************
//
//	File:		VideoConversions.h
//
//	Description:	Video Conversion library header.
//	
//	Copyright 1998, Be Incorporated, All Rights Reserved.
//
//******************************************************************************


#ifndef	_VIDEO_CONVERSION_H
#define	_VIDEO_CONVERSION_H

#include <stdio.h>
#include <Bitmap.h>
#include <byteorder.h>
#include <SupportDefs.h>
#include <GraphicsDefs.h>
#include <InterfaceDefs.h>

#include "Timecode.h"
#include "VideoImage.h"

BTimecode		FrameNumberToTimecode(uint32 frameNumber, timecodetype type);
uint32			TimecodeToFrameNumber(BTimecode tc);

status_t		VideoImageToBitmap(	BVideoImage *videoImage, BBitmap *bitmap);


#endif


