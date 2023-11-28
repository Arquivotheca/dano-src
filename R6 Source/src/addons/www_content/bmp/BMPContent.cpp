/*********************************************************************
 *   Author: Myron W. Walker
 *
 */

#include "BMPContent.h"

/*********************************************************************
 *   BMPContentInstance
 *
 */
BMPContentInstance::BMPContentInstance(Content *content, GHandler *handler)
	: ContentInstance(content, handler)
{
}

status_t BMPContentInstance::GetSize(int32 *width, int32 *height, uint32 *outFlags)
{
	BMPContent *content = (BMPContent *)GetContent();
	BBitmap *bitmap = content->GetBitmap();
	if (bitmap) {
		*width = content->mBitmapInfo.biWidth;
		*height = content->mBitmapInfo.biHeight;
	} else {
		*width = 0;
		*height = 0;
	}

	*outFlags = 0;
	
	return B_OK;
}

status_t BMPContentInstance::Draw(BView *into, BRect /* exposed */)
{
	BBitmap *bitmap = ((BMPContent*)GetContent())->GetBitmap();
	if (bitmap) {
		into->DrawBitmapAsync(bitmap, bitmap->Bounds(), FrameInParent());
	}
	
	return B_OK;
}

status_t BMPContentInstance::ContentNotification(BMessage *msg)
{
	/**** Content Notification Flags ******
	 *	enum {
	 *		bmsgContentDirty		= 'drty',
	 *		bmsgLayoutCompletion	= 'layc',
	 *		bmsgContentDisplayable	= 'cdsp',
	 *		bmsgContentLoaded		= 'cldd'
	 *	};
	 **************************************/
	status_t rtn_value = B_OK;
	
	if(msg)
	{
		switch(msg->what)
		{
			case bmsgLayoutCompletion:
				{
					((BMPContent*)GetContent())->MarkAllDirty();
				}
				break;
			default:
				rtn_value = ContentInstance::ContentNotification(msg);
		}
	}
	
	return rtn_value;
}


/*********************************************************************
 *   BMPContent
 *
 */
 
BMPContent::BMPContent(void* handle)
	: Content(handle),
	  mBitmap(NULL)
{
	mBitmap = NULL;
	mLastUpdate = system_time();
	mBuffer = NULL;
	mHeaderSize = 0;
	mState = BMP_CONTENT_INIT;
	mLastRow = 0;
	mBufferUsedPrev = 0;
	mDecodeFunction = NULL;
	
	//Setup Target ColorSpace
	BScreen cs;
	color_space scrn_clrspace = cs.ColorSpace();
	switch(scrn_clrspace)
	{
		case B_NO_COLOR_SPACE: 
			mColorSpace = B_NO_COLOR_SPACE;
			break;      
        case B_RGB16:
        case B_RGB15:
        case B_RGBA15:
        case B_CMAP8:
        case B_GRAY8:
        case B_GRAY1:
        case B_RGB16_BIG:
        case B_RGB15_BIG:
        case B_RGBA15_BIG:
        	mColorSpace = B_RGB16;
        	break;        
        case B_RGB32:
        case B_RGBA32: 
        case B_RGB24:
        case B_RGB32_BIG:
        case B_RGBA32_BIG:
        case B_RGB24_BIG:
		// Dont Know what these are /////
		case B_YCbCr422:
        case B_YCbCr411:
        case B_YCbCr444:
        case B_YCbCr420:
        case B_YUV422:
        case B_YUV411:
        case B_YUV444:
        case B_YUV420:
        case B_YUV9:
        case B_YUV12:
        //////////////////////////////////
        case B_UVL24:
        case B_UVL32:
        case B_UVLA32:
        case B_LAB24:
        case B_LAB32:
        case B_LABA32:
        case B_HSI24:
        case B_HSI32:
        case B_HSIA32:
        case B_HSV24:
        case B_HSV32:
        case B_HSVA32:
        case B_HLS24:
        case B_HLS32:
        case B_HLSA32:
        case B_CMY24:
        case B_CMY32:
        case B_CMYA32:
        case B_CMYK32:
       	default:
			mColorSpace = B_RGBA32;
	}
}

BMPContent::~BMPContent()
{
	if(mBitmap != NULL) delete mBitmap;
}

bool BMPContent::IsInitialized()
{
	return mBitmap;
}

size_t BMPContent::GetMemoryUsage()
{
	size_t rtnsize = sizeof(BMPContent);
		
	if(mBitmap != NULL)	rtnsize += mBitmap->BitsLength();
	
	return rtnsize;
}

ssize_t BMPContent::Feed(const void *d, ssize_t count, bool done)
{
	mBuffer = d;
	mBufferUsed = 0;
	
	if(mState != BMP_DATA_REC)
	{

		if(((mBufferUsedPrev + count) > 14) && (mState == BMP_CONTENT_INIT))
		{
			SetupFileHeader((uchar *)mBuffer);
			mState = BMP_FILEHEADER_REC;
			mBufferUsed = 14;
		}
		
		if(((mBufferUsedPrev + count) > 54) && (mState == BMP_FILEHEADER_REC))
		{
			unsigned long nextpos = (((unsigned long)mBuffer) + mBufferUsed);
			
			if(mHeaderSize == 0)
				mHeaderSize = B_LENDIAN_TO_HOST_INT32(*((unsigned long *)nextpos));
			
			if((mBufferUsedPrev + count) > (14 + mHeaderSize))
			{
				SetupBitmapInfo((uchar *)nextpos);
				mState = BMP_COLORMAP_REC;
				mBufferUsed += mHeaderSize;
			} 
		}
		
		if(mState == BMP_COLORMAP_REC)
		{
			unsigned long nextpos = (((unsigned long)mBuffer) + mBufferUsed);
			
			if((count + mBufferUsedPrev) > mFileHeader.bfOffsetBits)
			{
				SetupColorArray((uchar *)nextpos);
				mBufferUsed = (mFileHeader.bfOffsetBits - mBufferUsedPrev);  	
				mState = BMP_DATA_REC;
			}
		}
	}
		
	if(mState == BMP_DATA_REC) //Start Receiving Pixel Data		
	{
		mPixelData = (void *)(((unsigned long)mBuffer) + mBufferUsed);
		mPixelDataSize = (count - mBufferUsed);
		
		if(mDecodeFunction == NULL) SetupDecode();
		
		mDecodeFunction(this);
	
		if ((mLastRow % 10) == 0) MarkAllDirty();
	}
	
	if(done)
	{
		mState = BMP_FINALSTATE;
		mBufferUsed = count;
		MarkAllDirty();
	}
		
	mBufferUsedPrev += mBufferUsed;
	
	return mBufferUsed;
}

BBitmap *BMPContent::GetBitmap()
{
	return mBitmap;
}

status_t BMPContent::CreateInstance(ContentInstance **outInstance, GHandler *handler,
	const BMessage&)
{
	*outInstance = new BMPContentInstance(this, handler);
	return B_OK;
}

void BMPContent::SetupFileHeader(uchar *fheaderptr)
{
	uchar *base = fheaderptr;
	
	mFileHeader.bfType = B_LENDIAN_TO_HOST_INT16(*((unsigned short *)base));
	mFileHeader.bfSize = B_LENDIAN_TO_HOST_INT32(*((unsigned long *)(base + 2)));
	mFileHeader.bfReserved1 = B_LENDIAN_TO_HOST_INT16(*((unsigned short *)(base + 6)));
	mFileHeader.bfReserved2 = B_LENDIAN_TO_HOST_INT16(*((unsigned short *)(base + 8)));
	mFileHeader.bfOffsetBits = B_LENDIAN_TO_HOST_INT32(*((unsigned long *)(base + 10)));
}

void BMPContent::SetupBitmapInfo(uchar *bmpinfo)
{
	bool initsuccess = false;
	{
		register unsigned long base = ((unsigned long)bmpinfo);			
			
		initsuccess = true;
		switch(mHeaderSize)
		{
			case 40:
				mVersion = BMP_WIN3X;
				break;
			case 88:
				mVersion = BMP_PV4;
				break;
			case 104:
				mVersion = BMP_PV5;
				break;
			default:
				initsuccess = false;		
		}
	}

	if(initsuccess)
	{
		register unsigned long base = ((unsigned long)bmpinfo);			
		
		switch(mVersion)
		{
			case BMP_PV5:
			{
				mBitmapInfo.biIntent = B_LENDIAN_TO_HOST_INT32(*((unsigned long *)(base + 84)));
				mBitmapInfo.biProfileData = B_LENDIAN_TO_HOST_INT32(*((unsigned long *)(base + 88)));
				mBitmapInfo.biProfileSize = B_LENDIAN_TO_HOST_INT32(*((unsigned long *)(base + 92)));
				mBitmapInfo.biReserved = B_LENDIAN_TO_HOST_INT32(*((unsigned long *)(base + 96)));
			}
			case BMP_PV4:
			{
				mBitmapInfo.biRedMask = B_LENDIAN_TO_HOST_INT32(*((unsigned long *)(base + 40)));
				mBitmapInfo.biGreenMask = B_LENDIAN_TO_HOST_INT32(*((unsigned long *)(base + 44)));
				mBitmapInfo.biBlueMask = B_LENDIAN_TO_HOST_INT32(*((unsigned long *)(base + 48)));
				mBitmapInfo.biAlphaMask = B_LENDIAN_TO_HOST_INT32(*((unsigned long *)(base + 52)));
				mBitmapInfo.biCSType = B_LENDIAN_TO_HOST_INT32(*((unsigned long *)(base + 56)));	
				mBitmapInfo.biEndPoints.red = *((unsigned long *)(base + 60));
				mBitmapInfo.biEndPoints.green = *((unsigned long *)(base + 64));
				mBitmapInfo.biEndPoints.blue = *((unsigned long *)(base + 68));				
				mBitmapInfo.biGammaRed = B_LENDIAN_TO_HOST_INT32((unsigned long *)(base + 72));
				mBitmapInfo.biGammaGreen = B_LENDIAN_TO_HOST_INT32((unsigned long *)(base + 76));
				mBitmapInfo.biGammaBlue = B_LENDIAN_TO_HOST_INT32((unsigned long *)(base + 80));
			}
			case BMP_WIN3X:
			{
				mBitmapInfo.biInfoHeaderSize = B_LENDIAN_TO_HOST_INT32(*((unsigned long *)base));
				mBitmapInfo.biWidth = B_LENDIAN_TO_HOST_INT32(*((long *)(base + 4)));
				mBitmapInfo.biHeight = B_LENDIAN_TO_HOST_INT32(*((long *)(base + 8)));
				mBitmapInfo.biPlanes = B_LENDIAN_TO_HOST_INT16(*((unsigned short *)(base + 12)));
				mBitmapInfo.biBitCount = B_LENDIAN_TO_HOST_INT16(*((unsigned short *)(base + 14)));
				mBitmapInfo.biCompression = B_LENDIAN_TO_HOST_INT32(*((unsigned long *)(base + 16)));
				mBitmapInfo.biSizeImage = B_LENDIAN_TO_HOST_INT32(*((unsigned long *)(base + 20)));
				mBitmapInfo.biXPelsPerMeter = B_LENDIAN_TO_HOST_INT32(*((long *)(base + 24)));
				mBitmapInfo.biYPelsPerMeter = B_LENDIAN_TO_HOST_INT32(*((long *)(base + 28)));
				mBitmapInfo.biClrUsed = B_LENDIAN_TO_HOST_INT32(*((unsigned long *)(base + 32)));
				mBitmapInfo.biClrImportant = B_LENDIAN_TO_HOST_INT32(*((unsigned long *)(base + 36)));
			}
		}
	}
}

void BMPContent::SetupColorArray(uchar *colorarray)
{
	int colors = 0;
	
	switch(mBitmapInfo.biBitCount)
	{
		case 0:
		
			break;
		case 1:
			
			break;
		case 4:
			colors = 16;
			break;
		case 8:
			colors = 256;		
			break;
		case 16:
		
			break;
		case 24:
			colors = 0;
			break;
		case 32:
			break;
	}	

	mColorArray = new RGB_Color[colors];
	
	for(int iA = 0; iA < colors; iA++)
	{
		mColorArray[iA].blue = colorarray[(iA * 4)];
		mColorArray[iA].green = colorarray[(iA * 4) + 1];
		mColorArray[iA].red = colorarray[(iA * 4) + 2];
		mColorArray[iA].reserved = 0xff;
	}
}

void BMPContent::SetupDecode()
{
	
	//Find Decode Function >>> This is really discusting but it only happens once...
	switch(mColorSpace)
	{
		//Target a B_RGB16 Bitmap
		case B_RGB16:
			switch(mVersion)
			{
				case BMP_WIN3X:
				case BMP_PV4:
				case BMP_PV5:
					switch(mBitmapInfo.biBitCount)
					{
						case 0:
							mDecodeFunction = DecodeNotSupported;
							break;
						case 1:
							mDecodeFunction = DecodeNotSupported;
							break;
						case 4:
							mDecodeFunction = Decode4Bit_Tar16Bit;
							break;
						case 8:
							mDecodeFunction = Decode8Bit_Tar16Bit;		
							break;
						case 16:
							mDecodeFunction = DecodeNotSupported;
							break;
						case 24:
							mDecodeFunction = Decode24Bit_Tar16Bit;
							break;
						case 32:
							mDecodeFunction = DecodeNotSupported;
							break;
						default:
							mDecodeFunction = DecodeNotSupported;
					}
					break;
				default:
					mDecodeFunction = DecodeNotSupported;
			}
			break;
		//Target a B_RGBA32 Bitmap
		case B_RGBA32:
			switch(mVersion)
			{
				case BMP_WIN3X:
				case BMP_PV4:
				case BMP_PV5:
					switch(mBitmapInfo.biBitCount)
					{
						case 0:
							mDecodeFunction = DecodeNotSupported;
							break;
						case 1:
							mDecodeFunction = DecodeNotSupported;
							break;
						case 4:
							mDecodeFunction = Decode4Bit_Tar24Bit;
							break;
						case 8:
							mDecodeFunction = Decode8Bit_Tar24Bit;		
							break;
						case 16:
							mDecodeFunction = DecodeNotSupported;
							break;
						case 24:
							mDecodeFunction = Decode24Bit_Tar24Bit;
							break;
						case 32:
							mDecodeFunction = DecodeNotSupported;
							break;
						default:
							mDecodeFunction = DecodeNotSupported;
					}
					break;
				default:
					mDecodeFunction = DecodeNotSupported;
			}
			break;
		default:
			mDecodeFunction = DecodeNotSupported;
	}	
}

void BMPContent::Decode4Bit_Tar16Bit(BMPContent *tc)
{
	//Setup Source Info
	int32 swidth = tc->mBitmapInfo.biWidth;
	int32 sheight = tc->mBitmapInfo.biHeight;
	int32 bytewidth = ((swidth + (swidth % 2)) / 2);
	int32 sbprmod = (bytewidth % 4);
	int32 sbpr, bytesused = 0, iR, iC;
	if(sbprmod > 0) sbpr = bytewidth + (4 - sbprmod);
	else sbpr = bytewidth;
		
	uchar* source = (uchar *)tc->mPixelData; 
	
	if(tc->mBitmap == NULL)
	{
		tc->mBitmap = new BBitmap(BRect(0, 0, (swidth - 1),	(sheight - 1)), B_RGB16);
		if(sheight > 0)	tc->mLastRow = (sheight - 1);
		else tc->mLastRow = 0;
	}
	
	//Setup Target Info
	char *tbits = (char *) tc->mBitmap->Bits();
	int32 tbpr = tc->mBitmap->BytesPerRow();
	
	int32 curpixelval;
	int32 curhalf;
	int32 curnibble = 0;
	RGB_Color *curcolor;
	
	if(sheight > 0)//Bottom Up Bitmap
	{
		iR = tc->mLastRow;
		while((iR >= 0) && ((tc->mPixelDataSize - bytesused) >= sbpr))
		{
			curhalf = 1;
			curpixelval = source[bytesused];
			for(iC = 0; iC < swidth; iC++)
			{	
				curnibble = (curpixelval >> (curhalf * 4)) & 0xf;
				curcolor = (RGB_Color *)&tc->mColorArray[curnibble];
				
				uint16 wb = (((uint16)curcolor->red) << 8) & 0xf800;
				wb += (((uint16)curcolor->green) << 3) & 0x07e0;
				wb += (((uint16)curcolor->blue) >> 3) & 0x001f;
				
				int32 tpos = (tbpr * iR) + (iC * 2);
				*((uint16 *)(&tbits[tpos])) = wb;
				
				if(curhalf == 0)
				{
					curhalf = 1;
					bytesused++;
					curpixelval = source[bytesused];
				}
				else curhalf--;
			}
			iR--;
			bytesused = (tc->mLastRow - iR) * sbpr;
		}
	}
	else //Top Down Bitmap
	{
		iR = tc->mLastRow;
		while((iR < sheight) && ((tc->mPixelDataSize - bytesused) >= sbpr))
		{
			curhalf = 1;
			curpixelval = source[bytesused];
			for(iC = 0; iC < swidth; iC++)
			{	
				curnibble = (curpixelval >> (curhalf * 4)) & 0xf;
				curcolor = (RGB_Color *)&tc->mColorArray[curnibble];
				
				uint16 wb = (((uint16)curcolor->red) << 8) & 0xf800;
				wb += (((uint16)curcolor->green) << 3) & 0x07e0;
				wb += (((uint16)curcolor->blue) >> 3) & 0x001f;
				
				int32 tpos = (tbpr * iR) + (iC * 2);
				*((uint16 *)(&tbits[tpos])) = wb;
				
				if(curhalf == 0)
				{
					curhalf = 1;
					bytesused++;
					curpixelval = source[bytesused];
				}
				else curhalf--;
			}
			iR++;
			bytesused = (iR - tc->mLastRow) * sbpr;
		}
	}

	tc->mLastRow = iR;
	tc->mBufferUsed += bytesused;
}

void BMPContent::Decode8Bit_Tar16Bit(BMPContent *tc)
{
	//Setup Source Info
	int32 swidth = tc->mBitmapInfo.biWidth;
	int32 sheight = tc->mBitmapInfo.biHeight;
	int32 sbprmod = swidth % 4;
	int32 sbpr, bytesused = 0, iR, iC;
	if(sbprmod > 0) sbpr = swidth + (4 - sbprmod);
	else sbpr = swidth;
		
	uchar* source = (uchar *)tc->mPixelData; 
	
	if(tc->mBitmap == NULL)
	{
		tc->mBitmap = new BBitmap(BRect(0, 0, (swidth - 1),	(sheight - 1)), B_RGB16);
		if(sheight > 0)	tc->mLastRow = (sheight - 1);
		else tc->mLastRow = 0;
	}
	
	//Setup Target Info
	char *tbits = (char *) tc->mBitmap->Bits();
	int32 tbpr = tc->mBitmap->BytesPerRow();
	
	int32 curpixelval;
	RGB_Color *curcolor;
		
	if(sheight > 0)//Bottom Up Bitmap
	{
		iR = tc->mLastRow;
		while((iR >= 0) && ((tc->mPixelDataSize - bytesused) >= sbpr))
		{
			for(iC = 0; iC < swidth; iC++)
			{	
				curpixelval = source[bytesused];
				curcolor = (RGB_Color *)&tc->mColorArray[curpixelval];
				
				uint16 wb = (((uint16)curcolor->red) << 8) & 0xf800;
				wb += (((uint16)curcolor->green) << 3) & 0x07e0;
				wb += (((uint16)curcolor->blue) >> 3) & 0x001f;
				
				int32 tpos = (tbpr * iR) + (iC * 2);
				*((uint16 *)(&tbits[tpos])) = wb;
			
				bytesused++;
			}
			iR--;
			bytesused = (tc->mLastRow - iR) * sbpr;
		}
	}
	else //Top Down Bitmap
	{
		iR = tc->mLastRow;
		while((iR < sheight) && ((tc->mPixelDataSize - bytesused) >= sbpr))
		{
			for(iC = 0; iC < swidth; iC++)
			{	
				curpixelval = source[bytesused];
				curcolor = (RGB_Color *)&tc->mColorArray[curpixelval];
				
				uint16 wb = (((uint16)curcolor->red) << 8) & 0xf800;
				wb += (((uint16)curcolor->green) << 3) & 0x07e0;
				wb += (((uint16)curcolor->blue) >> 3) & 0x001f;
				
				int32 tpos = (tbpr * iR) + (iC * 2);
				*((uint16 *)(&tbits[tpos])) = wb;
			
				bytesused++;
			}
			iR++;
			bytesused = (iR - tc->mLastRow) * sbpr;
		}
	}
	
	tc->mLastRow = iR;
	tc->mBufferUsed += bytesused;
}

void BMPContent::Decode24Bit_Tar16Bit(BMPContent *tc)
{
	//Setup Source Info
	int32 swidth = tc->mBitmapInfo.biWidth;
	int32 sheight = tc->mBitmapInfo.biHeight;
	int32 sbprmod = (swidth * 3) % 4;
	int32 sbpr, bytesused = 0, iR, iC;
	if(sbprmod > 0) sbpr = (swidth * 3) + (4 - sbprmod);
	else sbpr = (swidth * 3);
		
	uchar* source = (uchar *)tc->mPixelData; 
	
	if(tc->mBitmap == NULL)
	{
		tc->mBitmap = new BBitmap(BRect(0, 0, (swidth - 1),	(sheight - 1)), B_RGB16);
		if(sheight > 0)	tc->mLastRow = (sheight - 1);
		else tc->mLastRow = 0;
	}
	
	//Setup Target Info
	char *tbits = (char *) tc->mBitmap->Bits();
	int32 tbpr = tc->mBitmap->BytesPerRow();
	
		
	if(sheight > 0)//Bottom Up Bitmap
	{
		iR = tc->mLastRow;
		while((iR >= 0) && ((tc->mPixelDataSize - bytesused) >= sbpr))
		{
			for(iC = 0; iC < swidth; iC++)
			{
				uint16 wb = (((uint16)source[bytesused + 2]) << 8) & 0xf800;
				wb += (((uint16)source[bytesused + 1]) << 3) & 0x07e0;
				wb += (((uint16)source[bytesused]) >> 3) & 0x001f;
				
				int32 tpos = (tbpr * iR) + (iC * 2);
				*((uint16 *)(&tbits[tpos])) = wb;
				
				bytesused += 3;
			}
			iR--;
			bytesused = (tc->mLastRow - iR) * sbpr;
		}
	}
	else //Top Down Bitmap
	{
		iR = tc->mLastRow;
		while((iR < sheight) && ((tc->mPixelDataSize - bytesused) >= sbpr))
		{
			for(iC = 0; iC < swidth; iC++)
			{
				uint16 wb = (((uint16)source[bytesused + 2]) << 8) & 0xf800;
				wb += (((uint16)source[bytesused + 1]) << 3) & 0x07e0;
				wb += (((uint16)source[bytesused]) >> 3) & 0x001f;
				
				int32 tpos = (tbpr * iR) + (iC * 2);
				*((uint16 *)(&tbits[tpos])) = wb;
			
				bytesused += 3;
			}
			iR++;
			bytesused = (iR - tc->mLastRow) * sbpr;
		}	
	}

	tc->mLastRow = iR;
	tc->mBufferUsed += bytesused;
}

void BMPContent::Decode4Bit_Tar24Bit(BMPContent *tc)
{
	//Setup Source Info
	int32 swidth = tc->mBitmapInfo.biWidth;
	int32 sheight = tc->mBitmapInfo.biHeight;
	int32 bytewidth = ((swidth + (swidth % 2)) / 2);
	int32 sbprmod = (bytewidth % 4);
	int32 sbpr, bytesused = 0, iR, iC;
	if(sbprmod > 0) sbpr = bytewidth + (4 - sbprmod);
	else sbpr = bytewidth;
		
	uchar* source = (uchar *)tc->mPixelData; 
	
	if(tc->mBitmap == NULL)
	{
		tc->mBitmap = new BBitmap(BRect(0, 0, (swidth - 1),	(sheight - 1)), B_RGBA32);
		if(sheight > 0)	tc->mLastRow = (sheight - 1);
		else tc->mLastRow = 0;
	}
	
	//Setup Target Info
	char *tbits = (char *) tc->mBitmap->Bits();
	int32 tbpr = tc->mBitmap->BytesPerRow();
	
	int32 curpixelval;
	int32 curhalf;
	int32 curnibble = 0;
	RGB_Color *curcolor;
	
	if(sheight > 0)//Bottom Up Bitmap
	{
		iR = tc->mLastRow;
		while((iR >= 0) && ((tc->mPixelDataSize - bytesused) >= sbpr))
		{
			curhalf = 1;
			curpixelval = source[bytesused];
			for(iC = 0; iC < swidth; iC++)
			{	
				curnibble = (curpixelval >> (curhalf * 4)) & 0xf;
				curcolor = (RGB_Color *)&tc->mColorArray[curnibble];
				
				int32 tpos = (tbpr * iR) + (iC * 4);
				tbits[tpos] = curcolor->blue;
				tbits[tpos + 1] = curcolor->green;
				tbits[tpos + 2] = curcolor->red;
				tbits[tpos + 3] = 0xff;
				
				if(curhalf == 0)
				{
					curhalf = 1;
					bytesused++;
					curpixelval = source[bytesused];
				}
				else curhalf--;
			}
			iR--;
			bytesused = (tc->mLastRow - iR) * sbpr;
		}
	}
	else //Top Down Bitmap
	{
		iR = tc->mLastRow;
		while((iR < sheight) && ((tc->mPixelDataSize - bytesused) >= sbpr))
		{
			curhalf = 1;
			curpixelval = source[bytesused];
			for(iC = 0; iC < swidth; iC++)
			{	
				curnibble = (curpixelval >> (curhalf * 4)) & 0xf;
				curcolor = (RGB_Color *)&tc->mColorArray[curnibble];
				
				int32 tpos = (tbpr * iR) + (iC * 4);
				tbits[tpos] = curcolor->blue;
				tbits[tpos + 1] = curcolor->green;
				tbits[tpos + 2] = curcolor->red;
				tbits[tpos + 3] = 0xff;
				
				if(curhalf == 0)
				{
					curhalf = 1;
					bytesused++;
					curpixelval = source[bytesused];
				}
				else curhalf--;
			}
			iR++;
			bytesused = (iR - tc->mLastRow) * sbpr;
		}
	}

	tc->mLastRow = iR;
	tc->mBufferUsed += bytesused;
}

void BMPContent::Decode8Bit_Tar24Bit(BMPContent *tc)
{
	//Setup Source Info
	int32 swidth = tc->mBitmapInfo.biWidth;
	int32 sheight = tc->mBitmapInfo.biHeight;
	int32 sbprmod = swidth % 4;
	int32 sbpr, bytesused = 0, iR, iC;
	if(sbprmod > 0) sbpr = swidth + (4 - sbprmod);
	else sbpr = swidth;
		
	uchar* source = (uchar *)tc->mPixelData; 
	
	if(tc->mBitmap == NULL)
	{
		tc->mBitmap = new BBitmap(BRect(0, 0, (swidth - 1),	(sheight - 1)), B_RGBA32);
		if(sheight > 0)	tc->mLastRow = (sheight - 1);
		else tc->mLastRow = 0;
	}
	
	//Setup Target Info
	char *tbits = (char *) tc->mBitmap->Bits();
	int32 tbpr = tc->mBitmap->BytesPerRow();
	
	int32 curpixelval;
	RGB_Color *curcolor;
		
	if(sheight > 0)//Bottom Up Bitmap
	{
		iR = tc->mLastRow;
		while((iR >= 0) && ((tc->mPixelDataSize - bytesused) >= sbpr))
		{
			for(iC = 0; iC < swidth; iC++)
			{	
				curpixelval = source[bytesused];
				curcolor = (RGB_Color *)&tc->mColorArray[curpixelval];
				
				int32 tpos = (tbpr * iR) + (iC * 4);
				tbits[tpos] = curcolor->blue;
				tbits[tpos + 1] = curcolor->green;
				tbits[tpos + 2] = curcolor->red;
				tbits[tpos + 3] = 0xff;
			
				bytesused++;
			}
			iR--;
			bytesused = (tc->mLastRow - iR) * sbpr;
		}
	}
	else //Top Down Bitmap
	{
		iR = tc->mLastRow;
		while((iR < sheight) && ((tc->mPixelDataSize - bytesused) >= sbpr))
		{
			for(iC = 0; iC < swidth; iC++)
			{	
				curpixelval = source[bytesused];
				curcolor = (RGB_Color *)&tc->mColorArray[curpixelval];
				
				int32 tpos = (tbpr * iR) + (iC * 4);
				tbits[tpos] = curcolor->blue;
				tbits[tpos + 1] = curcolor->green;
				tbits[tpos + 2] = curcolor->red;
				tbits[tpos + 3] = 0xff;
			
				bytesused++;
			}
			iR++;
			bytesused = (iR - tc->mLastRow) * sbpr;
		}
	}
	
	tc->mLastRow = iR;
	tc->mBufferUsed += bytesused;
}

void BMPContent::Decode24Bit_Tar24Bit(BMPContent *tc)
{
	//Setup Source Info
	int32 swidth = tc->mBitmapInfo.biWidth;
	int32 sheight = tc->mBitmapInfo.biHeight;
	int32 sbprmod = (swidth * 3) % 4;
	int32 sbpr, bytesused = 0, iR, iC;
	if(sbprmod > 0) sbpr = (swidth * 3) + (4 - sbprmod);
	else sbpr = (swidth * 3);
		
	uchar* source = (uchar *)tc->mPixelData; 
	
	if(tc->mBitmap == NULL)
	{
		tc->mBitmap = new BBitmap(BRect(0, 0, (swidth - 1),	(sheight - 1)), B_RGBA32);
		if(sheight > 0)	tc->mLastRow = (sheight - 1);
		else tc->mLastRow = 0;
	}
	
	//Setup Target Info
	char *tbits = (char *) tc->mBitmap->Bits();
	int32 tbpr = tc->mBitmap->BytesPerRow();
	
		
	if(sheight > 0)//Bottom Up Bitmap
	{
		iR = tc->mLastRow;
		while((iR >= 0) && ((tc->mPixelDataSize - bytesused) >= sbpr))
		{
			for(iC = 0; iC < swidth; iC++)
			{
				int32 tpos = (tbpr * iR) + (iC * 4);
				
				tbits[tpos] = source[bytesused];
				tbits[tpos + 1] = source[bytesused + 1];
				tbits[tpos + 2] = source[bytesused + 2];
				tbits[tpos + 3] = 0xff;
				
				bytesused += 3;
			}
			iR--;
			bytesused = (tc->mLastRow - iR) * sbpr;
		}
	}
	else //Top Down Bitmap
	{
		iR = tc->mLastRow;
		while((iR < sheight) && ((tc->mPixelDataSize - bytesused) >= sbpr))
		{
			for(iC = 0; iC < swidth; iC++)
			{
				int32 tpos = (tbpr * iR) + (iC * 4);
				
				tbits[tpos] = source[bytesused];
				tbits[tpos + 1] = source[bytesused + 1];
				tbits[tpos + 2] = source[bytesused + 2];
				tbits[tpos + 3] = 0xff;
			
				bytesused += 3;
			}
			iR++;
			bytesused = (iR - tc->mLastRow) * sbpr;
		}	
	}

	tc->mLastRow = iR;
	tc->mBufferUsed += bytesused;
}

void BMPContent::DecodeNotSupported(BMPContent *tc)
{
	if(tc->mBitmap == NULL)
	{
		tc->mBitmap = new BBitmap(BRect(0, 0, 31, 31), B_RGBA32);
	
		//TODO: Add Bitmap Generation Code Here
	}
}
	

