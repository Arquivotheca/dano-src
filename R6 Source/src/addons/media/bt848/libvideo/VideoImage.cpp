/*
	
	VideoImage.cpp
	
	Copyright 1998 Be Incorporated, All Rights Reserved.
	
*/

#include  "VideoImage.h"

//-----------------------------------------------------------------

BVideoImage::BVideoImage(	BPoint imageSize,
							color_space color,
							video_layout layout,
							buffer_orientation orientation,
							bool allocateBuffer
							)
{
	uint32 buffer_size;
	
	fImageSize = imageSize;
	fColorSpace = color;
	fLayout = layout;
	fOrientation = orientation;
	/* round up fRowBytes to next uint32 boundry */
	fRowBytes = (((uint32)(imageSize.x) * (BitsPerPixel())/8 + 3)/4)*4;
	fLogical = true;
	fFrameNumber = 0;
	fTimestamp = 0;
	
	if (allocateBuffer)
	{
		fAllocateBuffer = true;
		buffer_size = fRowBytes * (uint32)imageSize.y;
		buffer_size	= ((buffer_size + B_PAGE_SIZE)/B_PAGE_SIZE) * B_PAGE_SIZE;
		fBufferAreaID = create_area(	"BVideoImage buffer",
										const_cast<void **>(&fBuffer),
										B_ANY_ADDRESS,
										buffer_size,
										B_FULL_LOCK,
										B_READ_AREA + B_WRITE_AREA);
										
		if (fBufferAreaID == B_ERROR || 
			fBufferAreaID == B_BAD_VALUE ||
			fBufferAreaID == B_NO_MEMORY )
			fIsValid = false;
		else
			fIsValid = true;
	}
	else 
	{
		fAllocateBuffer = false;
		fBuffer = 0;
		fIsValid = false;
	}
}

//-----------------------------------------------------------------

BVideoImage::~BVideoImage()
{
	if (fAllocateBuffer)
	{
		delete_area(fBufferAreaID);
	}
}

//-----------------------------------------------------------------

bool
BVideoImage::IsValid() const
{
	return fIsValid;
}

//-----------------------------------------------------------------

void
BVideoImage::SetImageSize(const BPoint size)
{
	fImageSize = size;
}

//-----------------------------------------------------------------

BPoint
BVideoImage::ImageSize() const
{
	return(fImageSize);
}

//-----------------------------------------------------------------

void
BVideoImage::SetColorSpace(const color_space color)
{
	fColorSpace = color;
}

//-----------------------------------------------------------------

color_space
BVideoImage::ColorSpace() const
{
	return fColorSpace;
}

//-----------------------------------------------------------------

void
BVideoImage::SetLayout(const video_layout layout)
{
	fLayout = layout;
}

//-----------------------------------------------------------------

video_layout
BVideoImage::Layout() const
{
	return(fLayout);
}

//-----------------------------------------------------------------

void
BVideoImage::SetOrientation(const buffer_orientation orientation)
{
	fOrientation = orientation;
}
		
//-----------------------------------------------------------------

buffer_orientation
BVideoImage::Orientation() const
{
	return fOrientation;
}

//-----------------------------------------------------------------

void
BVideoImage::SetBytesPerRow(const int32 rowbytes)
{
	fRowBytes = rowbytes;	
}

//-----------------------------------------------------------------

int32
BVideoImage::BytesPerRow() const
{
	return fRowBytes;
}

//-----------------------------------------------------------------

void
BVideoImage::SetFrameNumber(const uint32 framenumber)
{
	fFrameNumber= framenumber;
}

//-----------------------------------------------------------------

uint32
BVideoImage::FrameNumber() const
{
	return fFrameNumber;
}

//-----------------------------------------------------------------

void
BVideoImage::SetTimestamp(const bigtime_t timestamp)
{
	fTimestamp = timestamp;
}

//-----------------------------------------------------------------

bigtime_t
BVideoImage::Timestamp() const
{
	return fTimestamp;
}

//-----------------------------------------------------------------

void
BVideoImage::SetTimecode(const BTimecode timecode)
{
	fTimecode = timecode;
}

//-----------------------------------------------------------------

BTimecode
BVideoImage::Timecode() const
{
	return fTimecode;
}

//-----------------------------------------------------------------

void
BVideoImage::SetBuffer(const void *buffer, const bool logical_address)
{
	fBuffer = buffer;
	fLogical = logical_address;
	fIsValid = true;
}

//-----------------------------------------------------------------

void *
BVideoImage::Buffer() const
{
	return (void *)fBuffer;
}

//-----------------------------------------------------------------

void
BVideoImage::SetLogical(bool logicalAddress)
{
	fLogical = logicalAddress;
}

//-----------------------------------------------------------------

bool
BVideoImage::IsLogical() const
{
	return fLogical;
}

//-----------------------------------------------------------------

void
BVideoImage::SetStatus(status_t status)
{
	fStatus = status;
}

//-----------------------------------------------------------------

status_t
BVideoImage::Status() const
{
	return fStatus;
}

//-----------------------------------------------------------------

int32
BVideoImage::BitsPerPixel()
{
	switch(ColorSpace())
	{			
		case B_RGB32:
		case B_RGB32_BIG:
		case B_RGBA32:
		case B_RGBA32_BIG:
			return 32;
						
		case B_RGB24:
		case B_RGB24_BIG:
			return 24;	
					
		case B_RGB16:
		case B_RGB16_BIG:
		case B_RGB15:
		case B_RGB15_BIG:
		case B_YUV422:
		case B_YCbCr422:
			return 16;
						
		case B_YUV411:
		case B_YCbCr411:
			return 12;
			
		case B_CMAP8:
		case B_GRAY8:
			return 8;
						
		case B_NO_COLOR_SPACE:
		default:
			return 0;
	}
}

//-----------------------------------------------------------------

void
BVideoImage::_ReservedVideoImage1()
{

}

//-----------------------------------------------------------------

void
BVideoImage::_ReservedVideoImage2()
{

}

//-----------------------------------------------------------------

void
BVideoImage::_ReservedVideoImage3()
{

}


