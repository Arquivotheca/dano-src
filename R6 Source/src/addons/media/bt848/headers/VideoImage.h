/*
	
	VideoImage.h
	
	Copyright 1997 Be Incorporated, All Rights Reserved.
	
*/

#ifndef _VIDEO_IMAGE_H
#define _VIDEO_IMAGE_H

#include <OS.h>
#include <Point.h>
#include <SupportDefs.h>
#include <GraphicsDefs.h>

#include "Timecode.h"
#include "VideoDefs.h"

enum video_layout { 
	B_INTERLEAVED,
	B_NONINTERLEAVED,
	B_F1,
	B_F2
};

//------------------------------------------------------------------------------

class BVideoImage
{

public:
							BVideoImage(BPoint size,
										color_space color = B_RGB32,
										video_layout layout = B_INTERLEAVED,
										buffer_orientation orientation = B_BUFFER_TOP_TO_BOTTOM,
										bool allocateBuffer = true);
virtual						~BVideoImage();
		bool				IsValid() const;
		void				SetImageSize(BPoint size);
		BPoint				ImageSize() const;
		void				SetColorSpace(color_space color);
		color_space			ColorSpace() const;
		void				SetLayout(video_layout layout);
		video_layout		Layout() const;
		void				SetOrientation(buffer_orientation orientation);
		buffer_orientation	Orientation() const;
		void				SetBytesPerRow(int32 rowbytes);
		int32				BytesPerRow() const;
		void				SetFrameNumber(uint32 framenumber);
		uint32				FrameNumber() const;
		void				SetTimestamp(bigtime_t timestamp);
		bigtime_t			Timestamp() const;
		void				SetTimecode(BTimecode timecode);
		BTimecode			Timecode() const;
		void				SetBuffer(const void *buffer, bool logicalAddress = true);
		void				*Buffer() const;
		void				SetLogical(bool logicalAddress);
		bool				IsLogical() const;
		void				SetStatus(status_t status);
		status_t			Status() const;
		int32				BitsPerPixel();
//-----------------------------------------------------------------------

private:
virtual	void				_ReservedVideoImage1();
virtual	void				_ReservedVideoImage2();
virtual	void				_ReservedVideoImage3();

							BVideoImage(const BVideoImage &);
		BVideoImage			&operator=(const BVideoImage &);
		
		BPoint				fImageSize;
		color_space			fColorSpace;
		video_layout		fLayout;
		buffer_orientation	fOrientation;
		int32				fRowBytes;
		uint32				fFrameNumber;
		bigtime_t			fTimestamp;
		BTimecode			fTimecode;
		const void			*fBuffer;
		bool				fLogical;
		area_id				fBufferAreaID;
		bool				fAllocateBuffer;
		bool				fIsValid;
		status_t			fStatus;
		uint32				_reserved[3];
};

#endif
