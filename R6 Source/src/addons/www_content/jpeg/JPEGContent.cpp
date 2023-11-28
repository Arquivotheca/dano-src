//
//	Browser interface
//
#include <Bitmap.h>
#include <View.h>
#include <Debug.h>
#include <stdio.h>
#include <Message.h>
#include <Screen.h>

#include <ResourceCache.h>
#include "Content.h"
#include "JPEG.h"

using namespace Wagner;

bigtime_t kMinUpdateInterval = 10000;

class BBitmap;

class JPEGContentInstance : public ContentInstance {
public:
	JPEGContentInstance(Content *content, GHandler *handler);
	virtual status_t Draw(BView *into, BRect exposed);
	virtual status_t GetSize(int32 *width, int32 *height, uint32 *flags);
};

class JPEGContent : public Content {
public:
	JPEGContent(void* handle);
	virtual ~JPEGContent();
	virtual size_t GetMemoryUsage();
	virtual	bool IsInitialized();
	virtual ssize_t Feed(const void *buffer, ssize_t bufferLen, bool done=false);
	BBitmap *GetBitmap();

private:
	static JPEGError DrawRowCallback(void *decoder);
	virtual status_t CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage&);

	BBitmap*	fBitmap;

	JPEGDecoder* mJPEG;
	BRect		mDrawRect;
	bool		mLookingForRect;
	int			mMaxHeight;
	int32		mRealWidth;
	int32		mRealHeight;
	bigtime_t 	mLastUpdate;
	BRect		mDirtyRect;
	bool        mTooBig;
	friend class JPEGContentInstance;
};

short MakeColorSlice(JPEGDecoder *j, BBitmap *pixels, short line, short width, short height);

// ----------------------- JPEGContentInstance -----------------------

JPEGContentInstance::JPEGContentInstance(Content *content, GHandler *handler)
	: ContentInstance(content, handler)
{
}

status_t JPEGContentInstance::GetSize(int32 *width, int32 *height, uint32 *outFlags)
{
	JPEGContent *content = (JPEGContent *)GetContent();
	BBitmap *bitmap = content->GetBitmap();
	if (bitmap) {
		*width = content->mRealWidth;
		*height = content->mRealHeight;
	} else {
		*width = 0;
		*height = 0;
	}

	*outFlags = 0;
	
	return B_OK;
}

status_t JPEGContentInstance::Draw(BView *into, BRect /* exposed */)
{
	BBitmap *bitmap = ((JPEGContent*)GetContent())->GetBitmap();
	if (bitmap) {
		BRect src(0, 0,
				  ((JPEGContent*)GetContent())->mRealWidth-1,
				  ((JPEGContent*)GetContent())->mRealHeight-1);
		
		into->SetDrawingMode(B_OP_COPY);		  
		into->DrawBitmapAsync(bitmap, src, FrameInParent());
	}	
	return B_OK;
}

// ----------------------- JPEGContent -----------------------
// #pragma mark -

JPEGContent::JPEGContent(void* handle)
	: Content(handle),
	  fBitmap(0), mLastUpdate(system_time()), mTooBig(false)
{
	mJPEG = 0;
	mMaxHeight = 0;
}

JPEGContent::~JPEGContent()
{
	delete fBitmap;
	if (mJPEG)
		DisposeJPEGDecoder(mJPEG);
}

bool JPEGContent::IsInitialized()
{
	return fBitmap;
}

size_t JPEGContent::GetMemoryUsage()
{
	return fBitmap ? fBitmap->BitsLength() : 0;
}

JPEGError JPEGContent::DrawRowCallback(void *_decoder)
{
	JPEGDecoder *decoder = (JPEGDecoder*) _decoder;
	JPEGContent *content = (JPEGContent*) decoder->DrawRefCon;
	if (content == 0)
		return kJPEGCancelled;

	short top = decoder->currentSlice * decoder->MCUVPixels;
	short bottom = decoder->MCUVPixels;
	short height = decoder->Height;
	short width = decoder->Width;
	short fullWidth = decoder->MCUHPixels * decoder->WidthMCU;
	short fullHeight = decoder->HeightMCU * decoder->MCUVPixels;

	if (decoder->thumbnail) {
		height = height >> 3;
		width = width >> 3;
	}
	if (top + bottom > height)
		bottom = height % decoder->MCUVPixels;

	if ((content->fBitmap == NULL) && !content->mTooBig) {
		BScreen screen;
		color_space colors;
		unsigned bytes_per_pixel;
		switch (screen.ColorSpace()) {
			case B_RGB16:
			case B_RGB15:
			case B_RGBA15:
			case B_RGB16_BIG:
			case B_RGB15_BIG:
			case B_RGBA15_BIG:
				colors = B_RGBA15;
				bytes_per_pixel = 2;
				break;
			case B_RGB32:
			case B_RGBA32:
			case B_RGB24:
			case B_RGB32_BIG:
			case B_RGBA32_BIG:
			case B_RGB24_BIG:
			case B_CMAP8:
			default:
				colors = B_RGBA32;
				bytes_per_pixel = 4;
				break;
		}
		content->fBitmap = new BBitmap(BRect(0, 0, fullWidth - 1, fullHeight - 1), 0, colors, fullWidth*bytes_per_pixel);
		if(!content->fBitmap || !content->fBitmap->IsValid()) {
			printf("JPEGContent::DrawRowCallback: BBitmap constructor failed\n");
			content->mTooBig= true;
			delete content->fBitmap;
			content->fBitmap= NULL;
		}

		content->mRealWidth = decoder->Width;
		content->mRealHeight = decoder->Height;
		PRINT(("Made bitmap (%dx%d) from (%dx%d) with colors=0x%lx\n",
				fullWidth, fullHeight, decoder->Width, decoder->Height, colors));
	}
	
	MakeColorSlice(decoder, content->fBitmap, top, fullWidth, decoder->MCUVPixels);
	if ((top+bottom) > content->mMaxHeight) content->mMaxHeight = top + bottom;
	
	BRect dirty(0, top, fullWidth, top+bottom-1);
	if( !content->mDirtyRect.IsValid() ) content->mDirtyRect = dirty;
	else content->mDirtyRect = content->mDirtyRect | dirty;

	PRINT(("Drawing JPEG slice %d to %d, dirty=", top, top+bottom-1));
	#if DEBUG
	content->mDirtyRect.PrintToStream();
	#endif
	
	return kNoError;
}

ssize_t JPEGContent::Feed(const void *d, ssize_t count, bool done)
{
	if(mTooBig) {
		return -1;
	}

	if (!mJPEG) {
		BRect r(0,0,32000,32000);	// Huge draw size to avoid clipping
		mJPEG = NewJPEGDecoder(&r,false /* mDrawAsThumbnail */,DrawRowCallback, NULL);
		if (!mJPEG)
			return -1;
		mJPEG->DrawRefCon = this;
	}
	
	ssize_t amount = 0;
	ssize_t retval;
	while( count > 0 && (retval=JPEGWrite(mJPEG,(uchar*)d,count)) > 0 ) {
		amount += retval;
		d = (uint8*)d + retval;
		if( retval > count ) count = 0;
		else count -= retval;
	}
	
	PRINT(("JPEGContent::Feed() wrote %ld bytes with %ld left\n", amount, count));
	if (done && count == 0) {
		if (mDirtyRect.IsValid()) {
			MarkAllDirty();
			//For now, be paranoid and redraw the entire picture.
			//MarkAllDirty(&mDirtyRect);
			mDirtyRect = BRect();
		};
		if (mJPEG) DisposeJPEGDecoder(mJPEG);
		mJPEG = NULL;
		// Freeze() is not supported by the app_server anymore
		// FYI: it crashes with BPicture.
		//if (fBitmap) fBitmap->Freeze();
	} else if( mDirtyRect.IsValid() ) {
		if (system_time() - mLastUpdate > kMinUpdateInterval) {
			PRINT(("Redrawing JPEG, old time=%.4fs, cur time=%.4fs\n",
					(float)mLastUpdate / 1000000,
					(float)system_time() / 1000000));
			mLastUpdate = system_time();
			MarkAllDirty(&mDirtyRect);
			mDirtyRect = BRect();
		};
	}

	return amount;
}

BBitmap *JPEGContent::GetBitmap()
{
	return fBitmap;
}

status_t JPEGContent::CreateInstance(ContentInstance **outInstance, GHandler *handler,
	const BMessage&)
{
	*outInstance = new JPEGContentInstance(this, handler);
	return B_OK;
}

// ----------------------- JPEGContentFactory -----------------------
// #pragma mark -

class JPEGContentFactory : public ContentFactory
{
public:
	virtual void GetIdentifiers(BMessage* into)
	{
		 /*
		 ** BE AWARE: Any changes you make to these identifiers should
		 ** also be made in the 'addattr' command in the makefile.
		 */
		into->AddString(S_CONTENT_MIME_TYPES, "image/jpeg");
		into->AddString(S_CONTENT_EXTENSIONS, "jpeg");
		into->AddString(S_CONTENT_EXTENSIONS, "jpg");
	}
	
	virtual Content* CreateContent(void* handle,
								   const char* mime,
								   const char* extension)
	{
		(void)mime;
		(void)extension;
		return new JPEGContent(handle);
	}
};

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id /*you*/, uint32 /*flags*/, ...)
{
	if( n == 0 ) return new JPEGContentFactory;
	return 0;
}



//====================================================================
uint32 jb_seed=0x12345678;

inline uint32 jb_rand()
{
	return (jb_seed=(jb_seed<<5|jb_seed>>27)+3141592654UL);
}

inline void ConvertYUVAndOutput(uchar *&data, short y, short u, short v, short p, color_space colors) {
	if (colors == B_RGBA32) {
		*data++ = (p = (y + u)) > 0 ? (p <= 255 ? p : 255) : 0;
		*data++ = (p = (y - (v/2 + u/6))) > 0 ? (p <= 255 ? p : 255) : 0;
		*data = (p = (y + v)) > 0 ? (p <= 255 ? p : 255) : 0;
		data += 2;
	} else {
		// Convert
		uint8 red = (p = (y + v)) > 0 ? (p <= 255 ? p : 255) : 0;
		uint8 green = (p = (y - (v/2 + u/6))) > 0 ? (p <= 255 ? p : 255) : 0;
		uint8 blue = (p = (y + u)) > 0 ? (p <= 255 ? p : 255) : 0;

		// Dither
		red=(red>=248)?248:((red&7)<(jb_rand()&7))?red&248:red+8&248;
		green=(green>=248)?248:((green&7)<(jb_rand()&7))?green&248:green+8&248;
		blue=(blue>=248)?248:((blue&7)<(jb_rand()&7))?blue&248:blue+8&248;

		// Pack
		*data++ = ((green & 0x38) << 2) + ((blue & 0xf8) >> 3);
		*data++ = 0x80 + ((red & 0xf8) >> 1) + ((green & 0xc0) >> 6);
	}
}

//	Make a slice of a picture from buffer
//	NOTE: width must be the full width of the component buffers
//	otherwise the image will skew

short MakeColorSlice(JPEGDecoder *j, BBitmap *pixels, short line, short width, short height)
{
	short	x,yy,y,u,v,p;
	uchar 	*ySrc,*ySrc2,*uSrc,*vSrc,*dst,*dst2;

	if (!pixels) {
		return 0;
	}

//	See we understand the color subsampling
//	CompInFrame is a better measure than components in scan

	color_space colors = pixels->ColorSpace();
	if (j->CompInFrame == 1) {
		j->sampling = 0x0000;			// Grayscale
	} else {
		CompSpec *y,*u,*v;
		
		y = &j->Comp[0];
		u = &j->Comp[1];
		v = &j->Comp[2];
		
		if (u->Hi + u->Vi + v->Hi + v->Vi != 4)  {
			printf("Can only draw 1:1 chroma <%d:%d> <%d:%d>",
			u->Hi , u->Vi , v->Hi , v->Vi);
			return kGenericError;
		}
		
		j->sampling = ( y->Hi << 4) | y->Vi;
		
		switch (j->sampling) {
			case	0x0011:
			case	0x0021:
			case	0x0022:
				break;
			default:
			if (j->sampling==j->sampling)
				printf("Can only draw 1:1, 2:1 or 2:2 <%d:%d>",y->Hi,y->Vi);
			return kGenericError;
		}
	}

//	Draw the image into gWorld

	dst = (uchar*) pixels->Bits() + line * pixels->BytesPerRow();
	ySrc = j->buffer[0];
	uSrc = j->buffer[1];
	vSrc = j->buffer[2];
	switch (j->sampling) {
		case	0x0000:								// Grayscale
			for (yy = 0; yy < height; yy++) {
				for (x = 0; x < width; x++) {
					y = (y = *ySrc++) > 0 ? (y <= 255 ? y : 255) : 0;
					if (colors == B_RGBA32) {
						*dst++ = y;
						*dst++ = y;
						*dst++ = y;
						dst++;
					} else {
						*dst++ = ((y & 0x38) << 2) + ((y & 0xf8) >> 3);
						*dst++ = 0x80 + ((y & 0xf8) >> 1) + ((y & 0xc0) >> 6);
					}
				}
				dst = (uchar*) pixels->Bits() + ++line * pixels->BytesPerRow();
			}
			break;
		case	0x0011:								// 1:1 RGB
			for (yy = 0; yy < height; yy++) {
				for (x = 0; x < width; x++) {
					y = *ySrc++;
					u = (*uSrc++ - 128) << 1;		// u = u*2;
					v = ((*vSrc++ - 128) << 4)/10;	// v = v*1.6
					ConvertYUVAndOutput(dst, y, u, v, p, colors);
				}
				dst = (uchar*) pixels->Bits() + ++line * pixels->BytesPerRow();
			}
			break;
		case	0x0021:								// 2:1 RGB
			for (yy = 0; yy < height; yy++) {
				for (x = 0; x < (width >> 1); x++) {
					u = (*uSrc++ - 128) << 1;		// u = u*2;
					v = ((*vSrc++ - 128) << 4)/10;	// v = v*1.6
					y = *ySrc++;
					ConvertYUVAndOutput(dst, y, u, v, p, colors);
					y = *ySrc++;
					ConvertYUVAndOutput(dst, y, u, v, p, colors);
				}

				dst = (uchar*) pixels->Bits() + ++line * pixels->BytesPerRow();
			}
			break;
		case	0x0022:								// 2:2 RGB
			dst2 = (uchar*) pixels->Bits() + (line + 1) * pixels->BytesPerRow();
			ySrc2 = j->buffer[0] + j->rowBytes[0];
			for (yy = 0; yy < (height >> 1); yy++) {
				for (x = 0; x < (width >> 1); x++) {
					u = (*uSrc++ - 128) << 1;		// u = u*2;
					v = ((*vSrc++ - 128) << 4)/10;	// v = v*1.6
					y = *ySrc++;
					ConvertYUVAndOutput(dst, y, u, v, p, colors);
					y = *ySrc++;
					ConvertYUVAndOutput(dst, y, u, v, p, colors);
					y = *ySrc2++;
					ConvertYUVAndOutput(dst2, y, u, v, p, colors);
					y = *ySrc2++;
					ConvertYUVAndOutput(dst2, y, u, v, p, colors);
				}
				ySrc += width;
				ySrc2 += width;
				line += 2;
				dst = (uchar*) pixels->Bits() + line * pixels->BytesPerRow();
				dst2 = (uchar*) pixels->Bits() + (line + 1) * pixels->BytesPerRow();
			}
			break;
	}
	return 0;
}
