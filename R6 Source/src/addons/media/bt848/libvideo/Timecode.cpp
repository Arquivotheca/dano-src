//******************************************************************************
//
//	File:		Timecode.cpp
//
//	Description:	Timecode class
//	
//	Copsecondright 1997, Be Incorporated, All Rights Reserved.
//
//******************************************************************************


#include "Timecode.h"
#include "VideoConversions.h"

//------------------------------------------------------------------------------

BTimecode BTimecode::operator+(const BTimecode& tc) const
{
	BTimecode 	aTc;
	uint32			aFrameNumber;
	uint32			thisFrameNumber;

	switch (type)
	{
		case B_30_FPS:
		case B_VIDEO_NONDROP_NTSC:
		case B_VIDEO_NONDROP_PAL_M:
			aTc.frame  = frame  + tc.frame;
			aTc.second = second + tc.second + aTc.frame/30;
			aTc.minute = minute + tc.minute + aTc.second/60;
			aTc.hour   = hour   + tc.hour   + aTc.minute/60;
			
			aTc.hour   = aTc.hour   % 24;
			aTc.minute = aTc.minute % 60;
			aTc.second = aTc.second % 60;
			aTc.frame  = aTc.frame  % 30;
			break;
		case B_25_FPS:
		case B_VIDEO_NONDROP_PAL:
			aTc.frame  = frame  + tc.frame;
			aTc.second = second + tc.second + aTc.frame/25;
			aTc.minute = minute + tc.minute + aTc.second/60;
			aTc.hour   = hour   + tc.hour   + aTc.minute/60;
			
			aTc.hour   = aTc.hour   % 24;
			aTc.minute = aTc.minute % 60;
			aTc.second = aTc.second % 60;
			aTc.frame  = aTc.frame  % 25;
			break;
		case B_24_FPS:
			aTc.frame  = frame  + tc.frame;
			aTc.second = second + tc.second + aTc.frame/24;
			aTc.minute = minute + tc.minute + aTc.second/60;
			aTc.hour   = hour   + tc.hour   + aTc.minute/60;
			
			aTc.hour   = aTc.hour   % 24;
			aTc.minute = aTc.minute % 60;
			aTc.second = aTc.second % 60;
			aTc.frame  = aTc.frame  % 24;
			break;
		case B_30_DROP_FPS:
		case B_VIDEO_DROP_NTSC:
		case B_VIDEO_DROP_PAL_M:
			thisFrameNumber	=	TimecodeToFrameNumber(*this);
			aFrameNumber 	= 	TimecodeToFrameNumber(tc);
			
			aTc = FrameNumberToTimecode(thisFrameNumber + aFrameNumber, type);
			break;	
	}
	return(aTc);
}

//------------------------------------------------------------------------------

BTimecode BTimecode::operator-(const BTimecode& tc) const
{
	BTimecode aTc;
	uint32			aFrameNumber;
	uint32			thisFrameNumber;
	
	switch (type)
	{
		case B_30_FPS:
		case B_VIDEO_NONDROP_NTSC:
		case B_VIDEO_NONDROP_PAL_M:
			aTc.frame  = frame  - tc.frame;
			aTc.second = second - tc.second - (aTc.frame  < 0 ? 1 : 0);
			aTc.minute = minute - tc.minute - (aTc.second < 0 ? 1 : 0);
			aTc.hour   = hour   - tc.hour   - (aTc.minute < 0 ? 1 : 0);
		
			aTc.hour   = (aTc.hour   < 0 ? aTc.hour   + 60 : aTc.hour);
			aTc.minute = (aTc.minute < 0 ? aTc.minute + 60 : aTc.minute);
			aTc.second = (aTc.second < 0 ? aTc.second + 60 : aTc.second);
			aTc.frame  = (aTc.frame  < 0 ? aTc.frame  + 30 : aTc.frame);
			break;
		case B_25_FPS:
		case B_VIDEO_NONDROP_PAL:
			aTc.frame  = frame  - tc.frame;
			aTc.second = second - tc.second - (aTc.frame  < 0 ? 1 : 0);
			aTc.minute = minute - tc.minute - (aTc.second < 0 ? 1 : 0);
			aTc.hour   = hour   - tc.hour   - (aTc.minute < 0 ? 1 : 0);
		
			aTc.hour   = (aTc.hour   < 0 ? aTc.hour   + 60 : aTc.hour);
			aTc.minute = (aTc.minute < 0 ? aTc.minute + 60 : aTc.minute);
			aTc.second = (aTc.second < 0 ? aTc.second + 60 : aTc.second);
			aTc.frame  = (aTc.frame  < 0 ? aTc.frame  + 25 : aTc.frame);
			break;
		case B_24_FPS:
			aTc.frame  = frame  - tc.frame;
			aTc.second = second - tc.second - (aTc.frame  < 0 ? 1 : 0);
			aTc.minute = minute - tc.minute - (aTc.second < 0 ? 1 : 0);
			aTc.hour   = hour   - tc.hour   - (aTc.minute < 0 ? 1 : 0);
		
			aTc.hour   = (aTc.hour   < 0 ? aTc.hour   + 60 : aTc.hour);
			aTc.minute = (aTc.minute < 0 ? aTc.minute + 60 : aTc.minute);
			aTc.second = (aTc.second < 0 ? aTc.second + 60 : aTc.second);
			aTc.frame  = (aTc.frame  < 0 ? aTc.frame  + 24 : aTc.frame);
			break;
		case B_30_DROP_FPS:
		case B_VIDEO_DROP_NTSC:
		case B_VIDEO_DROP_PAL_M:
			thisFrameNumber	=	TimecodeToFrameNumber(*this);
			aFrameNumber 	= 	TimecodeToFrameNumber(tc);
			
			aTc = FrameNumberToTimecode(thisFrameNumber - aFrameNumber, type);
			break;	
	}
	return(aTc);
}

//------------------------------------------------------------------------------

BTimecode& BTimecode::operator++()
{

	frame++;
	
	switch(type)
	{
		case B_30_FPS:
		case B_25_FPS:
		case B_24_FPS:
		case B_30_DROP_FPS:
		case B_VIDEO_NONDROP_NTSC:
		case B_VIDEO_NONDROP_PAL_M:
		case B_VIDEO_DROP_NTSC:
		case B_VIDEO_DROP_PAL_M:
			second = second + frame/30;
			break;
		case B_VIDEO_NONDROP_PAL:
			second = second + frame/25;
			break;
	}
	
	minute = minute + second/60;
	hour   = hour   + minute/60;

	hour   = hour   % 24;	
	minute = minute % 60;
	second = second % 60;
	switch(type)
	{
		case B_30_FPS:
		case B_VIDEO_NONDROP_NTSC:
		case B_VIDEO_NONDROP_PAL_M:
			frame = frame % 30;
			break;
		case B_25_FPS:
		case B_VIDEO_NONDROP_PAL:
			frame = frame % 25;
			break;
		case B_24_FPS:
			frame = frame % 24;
			break;
		case B_30_DROP_FPS:
		case B_VIDEO_DROP_NTSC:
			if ((frame = frame % 30) == 0)
				switch(minute)
				{
					case 0:
					case 10:
					case 20:
					case 30:
					case 40:
					case 50:
						break;						
					default:
						frame = 2;
						break;
				}
			break;
		case B_VIDEO_DROP_PAL_M:
			if ((frame = frame % 30) == 0)
				if (minute%2 == 0)
					switch(minute)
					{
						case 0:
						case 20:
						case 40:
							break;						
						default:
							frame = 4;
							break;
					}
			break;
	}

	return(*this);
}

//------------------------------------------------------------------------------

BTimecode& BTimecode::operator--()
{

	frame--;
	second = second - (frame < 0 ? 1 : 0);
	minute = minute - (second < 0 ? 1 : 0);
	hour   = hour   - (minute < 0 ? 1 : 0);

	hour   = (hour   < 0 ? hour   + 24 : hour);
	minute = (minute < 0 ? minute + 60 : minute);
	second = (second < 0 ? second + 60 : second);

	switch(type)
	{
		case B_30_FPS:
		case B_VIDEO_NONDROP_NTSC:
		case B_VIDEO_NONDROP_PAL_M:
			frame = (frame < 0 ? frame + 30 : frame);
			break;
		case B_25_FPS:
		case B_VIDEO_NONDROP_PAL:
			frame = (frame < 0 ? frame + 25 : frame);
			break;
		case B_24_FPS:
			frame = (frame < 0 ? frame + 24 : frame);
			break;
		case B_30_DROP_FPS:
		case B_VIDEO_DROP_NTSC:
			frame = (frame < 0 ? frame + 30 : frame);
			if (frame == 1)
				switch(minute)
				{
					case 0:
					case 10:
					case 20:
					case 30:
					case 40:
					case 50:
						break;						
					default:
						frame = 0;
						--(*this);
						break;
				}
			break;
		case B_VIDEO_DROP_PAL_M:
			frame = (frame < 0 ? frame + 30 : frame);
			if (frame == 3)
					switch(minute)
					{
						case 0:
						case 20:
						case 40:
							break;						
						default:
							frame = 0;
							--(*this);
							break;
					}
			break;
	}

	return(*this);
}

//------------------------------------------------------------------------------

BTimecode& BTimecode::operator+=(const BTimecode& tc)
{
	uint32			aFrameNumber;
	uint32			thisFrameNumber;

	switch (type)
	{
		case B_30_FPS:
		case B_VIDEO_NONDROP_NTSC:
		case B_VIDEO_NONDROP_PAL_M:
			frame  = frame  + tc.frame;
			second = second + tc.second + frame/30;
			minute = minute + tc.minute + second/60;
			hour   = hour   + tc.hour   + minute/60;
			
			hour   = hour   % 24;
			minute = minute % 60;
			second = second % 60;
			frame  = frame  % 30;
			break;
		case B_25_FPS:
		case B_VIDEO_NONDROP_PAL:
			frame  = frame  + tc.frame;
			second = second + tc.second + frame/25;
			minute = minute + tc.minute + second/60;
			hour   = hour   + tc.hour   + minute/60;
			
			hour   = hour   % 24;
			minute = minute % 60;
			second = second % 60;
			frame  = frame  % 25;
			break;
		case B_24_FPS:
			frame  = frame  + tc.frame;
			second = second + tc.second + frame/24;
			minute = minute + tc.minute + second/60;
			hour   = hour   + tc.hour   + minute/60;
			
			hour   = hour   % 24;
			minute = minute % 60;
			second = second % 60;
			frame  = frame  % 24;
			break;
		case B_30_DROP_FPS:
		case B_VIDEO_DROP_NTSC:
		case B_VIDEO_DROP_PAL_M:
			thisFrameNumber	=	TimecodeToFrameNumber(*this);
			aFrameNumber 	= 	TimecodeToFrameNumber(tc);
			
			*this = FrameNumberToTimecode(thisFrameNumber + aFrameNumber, type);
			break;	
	}
	
	return(*this);
}

//------------------------------------------------------------------------------

BTimecode& BTimecode::operator-=(const BTimecode& tc)
{
	uint32			aFrameNumber;
	uint32			thisFrameNumber;

	switch (type)
	{
		case B_30_FPS:
		case B_VIDEO_NONDROP_NTSC:
		case B_VIDEO_NONDROP_PAL_M:
			frame  = frame  - tc.frame;
			second = second - tc.second - (frame < 0 ? 1 : 0);
			minute = minute - tc.minute - (second < 0 ? 1 : 0);
			hour   = hour   - tc.hour   - (minute < 0 ? 1 : 0);
			
			hour   = (hour   < 0 ? hour   + 24 : hour);
			minute = (minute < 0 ? minute + 60 : minute);
			second = (second < 0 ? second + 60 : second);
			frame  = (frame  < 0 ? frame  + 30 : frame);		
			break;
		case B_25_FPS:
		case B_VIDEO_NONDROP_PAL:
			frame  = frame  - tc.frame;
			second = second - tc.second - (frame < 0 ? 1 : 0);
			minute = minute - tc.minute - (second < 0 ? 1 : 0);
			hour   = hour   - tc.hour   - (minute < 0 ? 1 : 0);
			
			hour   = (hour   < 0 ? hour   + 24 : hour);
			minute = (minute < 0 ? minute + 60 : minute);
			second = (second < 0 ? second + 60 : second);
			frame  = (frame  < 0 ? frame  + 25 : frame);		
			break;			
		case B_24_FPS:
			frame  = frame  - tc.frame;
			second = second - tc.second - (frame < 0 ? 1 : 0);
			minute = minute - tc.minute - (second < 0 ? 1 : 0);
			hour   = hour   - tc.hour   - (minute < 0 ? 1 : 0);
			
			hour   = (hour   < 0 ? hour   + 24 : hour);
			minute = (minute < 0 ? minute + 60 : minute);
			second = (second < 0 ? second + 60 : second);
			frame  = (frame  < 0 ? frame  + 24 : frame);		
			break;			
		case B_30_DROP_FPS:
		case B_VIDEO_DROP_NTSC:
		case B_VIDEO_DROP_PAL_M:
			thisFrameNumber	=	TimecodeToFrameNumber(*this);
			aFrameNumber 	= 	TimecodeToFrameNumber(tc);
			
			*this = FrameNumberToTimecode(thisFrameNumber - aFrameNumber, type);
			break;	
	}
	return(*this);
}

//------------------------------------------------------------------------------

bool BTimecode::operator!=(const BTimecode& tc) const
{
	return(	frame  != tc.frame ||
			second != tc.second ||
			minute != tc.minute ||
			hour   != tc.hour ||
			type   != tc.type);
}

//------------------------------------------------------------------------------

bool BTimecode::operator==(const BTimecode& tc) const
{
	return(	frame  == tc.frame &&
			second == tc.second &&
			minute == tc.minute &&
			hour   == tc.hour &&
			type   == tc.type);
}

