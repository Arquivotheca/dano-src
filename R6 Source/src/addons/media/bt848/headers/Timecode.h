//******************************************************************************
//
//	File:		Timecode.h
//
//	Description:	Timecode class header.
//	
//	Copyright 1997, Be Incorporated, All Rights Reserved.
//
//******************************************************************************


#ifndef	_TIMECODE_H
#define	_TIMECODE_H

#include <SupportDefs.h>

#include "VideoDefs.h"

typedef enum
{
	B_VIDEO_NONDROP_NTSC,
	B_VIDEO_NONDROP_PAL,
	B_VIDEO_NONDROP_PAL_M,
	B_VIDEO_DROP_NTSC,
	B_VIDEO_DROP_PAL_M,
	B_30_FPS,
	B_30_DROP_FPS,
	B_25_FPS,
	B_24_FPS,
} timecodetype;

class BTimecode {

public:
		int8				hour;
		int8				minute;
		int8				second;
		int8				frame;
		timecodetype		type;

		// constructors
		BTimecode();
		BTimecode(	int8 h,
						int8 m,
						int8 s,
						int8 f,
						timecodetype t = B_VIDEO_NONDROP_NTSC);
		BTimecode(const BTimecode& tc);
		
		// assignment
		BTimecode	&operator=(const BTimecode &from);
		void			Set(int8 h,
							int8 m,
							int8 s,
							int8 f,
							timecodetype t = B_VIDEO_NONDROP_NTSC);

		// arithmetic
		BTimecode	operator+(const BTimecode&) const;
		BTimecode	operator-(const BTimecode&) const;
		BTimecode&	operator++();
		BTimecode&	operator--();
		BTimecode&	operator+=(const BTimecode&);
		BTimecode&	operator-=(const BTimecode&);

		// relational
		bool		operator!=(const BTimecode&) const;
		bool		operator==(const BTimecode&) const;
		
};

inline BTimecode::BTimecode()
{
	hour = 0;
	minute = 0;
	second = 0;
	frame = 0;
	type = B_VIDEO_NONDROP_NTSC;
}

inline BTimecode::BTimecode(	int8 h,
								int8 m,
								int8 s,
								int8 f,
								timecodetype t)
{
	hour = h;
	minute = m;
	second = s;
	frame = f;
	type = t;
}

inline BTimecode::BTimecode(const BTimecode& tc)
{
	hour = tc.hour;
	minute = tc.minute;
	second = tc.second;
	frame = tc.frame;
	type = tc.type;
}

inline BTimecode &BTimecode::operator=(const BTimecode& from)
{
	// don't need to worry about "this==from"
	hour = from.hour;
	minute = from.minute;
	second = from.second;
	frame = from.frame;
	type = from.type;
	return *this;
}

inline void BTimecode::Set(	int8 h,
							int8 m,
							int8 s,
							int8 f,
							timecodetype t)

{
	hour = h;
	minute = m;
	second = s;
	frame = f;
	type = t;
}

#endif
