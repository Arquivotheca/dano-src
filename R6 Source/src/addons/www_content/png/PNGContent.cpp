#include <Bitmap.h>
#include <View.h>
#include <Debug.h>
#include <stdio.h>
#include <png.h>
#include <Content.h>
#include <ResourceCache.h>

#include <stdlib.h>
#include <string.h>

using namespace Wagner;

bigtime_t kMinUpdateInterval = 10000;
static const uint32 msgUpdateFlags = 'mupf';

class BBitmap;

class PNGContentInstance : public ContentInstance {
public:
	PNGContentInstance(Content *content, GHandler *handler);
	
	void UpdateTransparency();
	
	virtual status_t Draw(BView *into, BRect exposed);
	virtual status_t GetSize(int32 *width, int32 *height, uint32 *flags);
	virtual	status_t ContentNotification(BMessage *msg);
};

class PNGContent : public Content {
public:
	PNGContent(void* handle);
	virtual ~PNGContent();
	virtual ssize_t Feed(const void *buffer, ssize_t bufferLen, bool done=false);
	BBitmap *GetBitmap();
	
	void SetAlpha(bool hasIt);
	bool HasAlpha() const;
	
	virtual size_t GetMemoryUsage();
	virtual	bool	IsInitialized();

private:
	virtual status_t CreateInstance(ContentInstance **outInstance, GHandler *handler,
		const BMessage&);
	static void InfoCallback(png_structp png_ptr, png_infop info);
	static void PNGContent::ProcessRowCallback (png_structp png_ptr, png_bytep new_row,
		png_uint_32 row_num, int pass);

	BBitmap *fBitmap;
	bool fHasAlpha;
	int fDataOffset;
	size_t fDataSize;
	BRect fDirtyRect;
	bigtime_t fLastUpdate;
	png_structp png_ptr;
	png_infop info_ptr;

	bool fTooBig;

	friend class PNGContentInstance;
};

// ----------------------- PNGContentInstance -----------------------

PNGContentInstance::PNGContentInstance(Content *content, GHandler *handler)
	: ContentInstance(content, handler)
{
	UpdateTransparency();
}

void PNGContentInstance::UpdateTransparency()
{
	PNGContent *content = (PNGContent *)GetContent();
	if( content ) {
		// Update transparency state.
		SetFlags( (Flags()&~(cifHasTransparency|cifDoubleBuffer))
					| ( content->HasAlpha() ? (cifDoubleBuffer|cifHasTransparency) : 0 ) );
	}
}

status_t PNGContentInstance::GetSize(int32 *width, int32 *height, uint32 *flags)
{
	PNGContent *content = (PNGContent *)GetContent();
	BBitmap *bitmap = content->GetBitmap();
	if (bitmap) {
		BRect r = bitmap->Bounds();
		*width = (int32)(r.Width()+1);
		*height = (int32)(r.Height()+1);
	} else {
		*width = 0;
		*height = 0;
	}

	*flags = 0;
	return B_OK;
}

status_t PNGContentInstance::Draw(BView *into, BRect exposed)
{
	BBitmap *bitmap = ((PNGContent*)GetContent())->GetBitmap();
	bool hasAlpha = ((PNGContent*)GetContent())->HasAlpha();
	if (bitmap) {
		if( hasAlpha ) {
			into->SetDrawingMode(B_OP_ALPHA);
			into->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		} else {
			into->SetDrawingMode(B_OP_COPY);
		}
		into->DrawBitmapAsync(bitmap, bitmap->Bounds(), FrameInParent());
	}
	
	return B_OK;
}

status_t PNGContentInstance::ContentNotification(BMessage *msg)
{
	if( msg && msg->what == msgUpdateFlags ) {
		UpdateTransparency();
		return B_OK;
	}
	
	return ContentInstance::ContentNotification(msg);
}

// ----------------------- PNGContent -----------------------

PNGContent::PNGContent(void* handle)
	: Content(handle),
	  fBitmap(0), fHasAlpha(false),
	  fDataOffset(0), fDataSize(0),
	  fLastUpdate(system_time()),
	  png_ptr(0),
	  fTooBig(false)
{
}

PNGContent::~PNGContent()
{
	if (png_ptr != 0) {
		png_destroy_read_struct(&png_ptr, &info_ptr,(png_infopp) NULL);
	}
	delete fBitmap;
}

bool PNGContent::IsInitialized()
{
	return fBitmap;
}

size_t PNGContent::GetMemoryUsage()
{
	return fBitmap ? fBitmap->BitsLength() : 0;
}

ssize_t PNGContent::Feed(const void *buffer, ssize_t count, bool done)
{
	if (fTooBig) {
		return -1;
	}

	if (png_ptr == 0) {
		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if (!png_ptr)
			return -1;
	
		info_ptr = png_create_info_struct(png_ptr);
		if (!info_ptr) {
			png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp) NULL);
			return -1;
		}
		
		if (setjmp(png_ptr->jmpbuf)) {
			png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp) NULL);
			return -1;
		}
		
		png_set_progressive_read_fn(png_ptr, this, InfoCallback, ProcessRowCallback,
			NULL);
	}
	
    if (setjmp(png_ptr->jmpbuf)) {
        png_destroy_read_struct(&png_ptr, &info_ptr,
           (png_infopp) NULL);
        return -1;
    }

	PRINT(("Processing %ld bytes, done=%ld\n", count, (int32)done));
	
	fDataSize += count;
	png_process_data(png_ptr, info_ptr, (png_bytep) buffer, count);
	
	//PRINT(("Dirty: ")); fDirtyRect.PrintToStream();
	//if( fBitmap ) { PRINT(("Bounds: ")); fBitmap->Bounds().PrintToStream(); }
	if( fDirtyRect.IsValid() ) {
		if( done || (system_time()-fLastUpdate) > kMinUpdateInterval ) {
			PRINT(("Reporting dirty rect.\n"));
			fLastUpdate = system_time();
			MarkAllDirty(&fDirtyRect);
			fDirtyRect = BRect();
		};
	}

	if (done) {
		// Freeze() is not supported by the app_server anymore
		// FYI: it crashes with BPicture.
		//if (fBitmap) fBitmap->Freeze();
		png_destroy_read_struct(&png_ptr, &info_ptr,(png_infopp) NULL);
	}
	
#if DEBUG
	if( done ) {
		PRINT(("*** PNG: Read %.1fk, made %.1fk\n",
				(float)fDataSize / 1000, (float)GetMemoryUsage() / 1000));
	}
#endif
	
	return count;
}

BBitmap *PNGContent::GetBitmap()
{
	return fBitmap;
}

void PNGContent::SetAlpha(bool hasIt)
{
	if( hasIt != fHasAlpha ) {
		fHasAlpha = hasIt;
		BMessage update(msgUpdateFlags);
		NotifyInstances(&update);
	}
}

bool PNGContent::HasAlpha() const
{
	return fHasAlpha;
}

status_t PNGContent::CreateInstance(ContentInstance **outInstance, GHandler *handler, const BMessage&)
{
	*outInstance = new PNGContentInstance(this, handler);
	return B_OK;
}

void PNGContent::ProcessRowCallback(png_structp png_ptr, png_bytep new_row,
    png_uint_32 row_num, int pass)
{
	PNGContent *content = (PNGContent*)  png_get_progressive_ptr(png_ptr);

	if(!content->fBitmap) {
		return;
	}

	png_progressive_combine_row(content->png_ptr, (png_bytep) content->fBitmap->Bits()
		+ row_num * content->fBitmap->BytesPerRow(), new_row);
	
	BRect invalRect(content->fBitmap->Bounds().left, row_num,
					content->fBitmap->Bounds().right, row_num);
	if( !content->fDirtyRect.IsValid() ) content->fDirtyRect = invalRect;
	else content->fDirtyRect = content->fDirtyRect | invalRect;
}

void PNGContent::InfoCallback(png_structp png_ptr, png_infop info)
{
	PNGContent *content = (PNGContent*)  png_get_progressive_ptr(png_ptr);
	ASSERT(content != 0);

	// Set up color space
	
	png_byte color_type(png_get_color_type(png_ptr, info));
	png_byte bit_depth(png_get_bit_depth(png_ptr, info));

	if (bit_depth <= 8) {
		png_set_expand(png_ptr);
		png_set_packing(png_ptr);
	}

	if (png_get_valid(png_ptr, info, PNG_INFO_tRNS)) {
		content->SetAlpha(true);
		png_set_expand(png_ptr);
	}
	
	if (bit_depth > 8)
		png_set_strip_16(png_ptr);
	
	png_set_bgr(png_ptr);

	if (!(color_type & PNG_COLOR_MASK_COLOR))
		png_set_gray_to_rgb(png_ptr);

	if (!(color_type & PNG_COLOR_MASK_ALPHA))
		png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
	else
		content->SetAlpha(true);

	// Temporarily disable gamma correction for the design folks.
#if 0
	// Set up gamma correction.
	
	double LUT_exponent, CRT_exponent = 2.2, display_exponent, aGamma;
#if __POWERPC__
	LUT_exponent = 1.8 / 2.61;	// MAC gamma
#else
	// LUT_exponent = 1.0 / 1.7;	// SGI gamma
	// LUT_exponent = 1.0 / 2.2;	// NeXT cube gamma
	// LUT_exponent = 1.0;			// Typical Unix workstation gamma
	LUT_exponent = 1.0;				// Typical PC gamma
#endif

	// Allow screen gamma to be changed through an environmental variable.
	const char* gammavar = getenv("SCREEN_GAMMA");
	if( gammavar && *gammavar ) {
		double expon = atof(gammavar);
		if( expon != 0 ) CRT_exponent = expon;
	}
	
	display_exponent = LUT_exponent * CRT_exponent;
	
	// This is a little different than how the standard says it
	// should be done.  If there is no gamma chunk in the image, we
	// just don't do gamma correction.  This is so that we can
	// create PNG images that will exactly match other surrounding
	// colors by not including gamma information with them.
	if (png_get_gAMA(png_ptr, info, &aGamma)) {
		png_set_gamma(png_ptr, display_exponent, aGamma); 
	}
#endif

	// Finish up and update formats
	
	png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info);

	// Create bitmap
	
	content->fBitmap = new BBitmap(BRect(0, 0, png_get_image_width(png_ptr, info) - 1,
		png_get_image_height(png_ptr, info) - 1), B_RGB32);

	if(!content->fBitmap || !content->fBitmap->IsValid()) {
		content->fTooBig = true;
		delete content->fBitmap;
		content->fBitmap= NULL;
	}
	content->fDataOffset = 0;
}


// ----------------------- PNGContentFactory -----------------------

class PNGContentFactory : public ContentFactory
{
public:
	virtual void GetIdentifiers(BMessage* into)
	{
		 /*
		 ** BE AWARE: Any changes you make to these identifiers should
		 ** also be made in the 'addattr' command in the makefile.
		 */
		into->AddString(S_CONTENT_MIME_TYPES, "image/png");
		into->AddString(S_CONTENT_EXTENSIONS, "png");
	}
	
	virtual Content* CreateContent(void* handle,
								   const char* mime,
								   const char* extension)
	{
		(void)mime;
		(void)extension;
		return new PNGContent(handle);
	}
};

extern "C" _EXPORT ContentFactory* make_nth_content(int32 n, image_id you, uint32 flags, ...)
{
	if( n == 0 ) return new PNGContentFactory;
	return 0;
}
