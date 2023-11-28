//******************************************************************************
//
//	File:		Bitmap.cpp
//
//	Description:	BBitmap class.
//
//	Written by:	Benoit Schillings
//
//	Copyright 1992-2001, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#include <string.h>

#ifndef _DEBUG_H
#include <Debug.h>
#endif
#ifndef _OS_H
#include <OS.h>
#endif

// ---- private includes ---
#include <message_util.h>
#include <messages.h>
#include <token.h>
#include <session.h>
#include <archive_defs.h>
#include <interface_misc.h>

// ---- end private includes ---

#include <Application.h>
#include <Screen.h>
#include <Window.h>
#include <InterfaceDefs.h>
#include <View.h>
#include <Bitmap.h>
#include <support/memadviser.h>


struct _offscreen_data_ {
	sem_id	lock;
	void *	bits;
	area_id shadow;
	int32	state;
};

#define	B_BITMAP_GEH_COMPRESSED		B_BITMAP_RESERVED_0
#define	B_BITMAP_LBX_COMPRESSED		B_BITMAP_RESERVED_1

/*---------------------------------------------------------------*/

BBitmap::BBitmap(BRect frame, color_space depth, bool accepts_views,
												 bool need_contiguous)
{
	uint32 flags = B_BITMAP_CLEAR_TO_WHITE;
	if (accepts_views) flags |= B_BITMAP_ACCEPTS_VIEWS;
	if (need_contiguous) flags |= B_BITMAP_IS_CONTIGUOUS;
	InitObject(frame, depth, flags, -1, B_MAIN_SCREEN_ID);
}

/*---------------------------------------------------------------*/

BBitmap::BBitmap(
	BRect bounds,
	uint32 flags,
	color_space depth,
	int32 bytesPerRow,
	screen_id screenID)
{
	InitObject(bounds, depth, flags, bytesPerRow, screenID);
}

BBitmap::BBitmap(const BBitmap& source)
	: BArchivable()
{
	InitObject(source.fBound, source.fType, 0, source.fRowBytes, B_MAIN_SCREEN_ID);
	if ((Bits()!=NULL)&&(source.fSize==fSize)) memcpy(Bits(), source.Bits(), fSize);
}

BBitmap::BBitmap(
	const BBitmap& source,
	uint32 flags)
	: BArchivable()
{
	InitObject(source.fBound, source.fType, flags, source.fRowBytes, B_MAIN_SCREEN_ID);
	if ((Bits()!=NULL)&&(source.fSize==fSize)) memcpy(Bits(), source.Bits(), fSize);
}

/*---------------------------------------------------------------*/

BBitmap::BBitmap(const BBitmap* source, bool accepts_view, bool need_contiguous)
{
	uint32 flags = 0;
	if (accepts_view) flags |= B_BITMAP_ACCEPTS_VIEWS;
	if (need_contiguous) flags |= B_BITMAP_IS_CONTIGUOUS;
	InitObject(source->fBound, source->fType, flags, source->fRowBytes, B_MAIN_SCREEN_ID);
	if ((Bits()!=NULL)&&(source->fSize==fSize)) memcpy(Bits(), source->Bits(), fSize);
}

/*---------------------------------------------------------------*/

void BBitmap::InitObject(
	BRect frame,
	color_space depth,
	uint32 flags,
	int32 bytesPerRow,
	screen_id )
{
	uint8		data_type;
	uint32		data;

	fRowBytes = bytesPerRow;
	fType = depth;
	fBasePtr = NULL;
	fOrigArea = fArea = B_ERROR;
	fFlags = flags;
	fInitError = B_OK;

	if ((flags & (B_BITMAP_GEH_COMPRESSED | B_BITMAP_LBX_COMPRESSED)) && (flags & B_BITMAP_ACCEPTS_VIEWS)) {
		fInitError = B_UNSUPPORTED;
		return;
	}

/*	if ((flags & B_BITMAP_IS_OFFSCREEN) ||
		(flags & B_BITMAP_WILL_OVERLAY)) {
		fInitError = B_UNSUPPORTED;
		return;
	}
*/
// exit if frame too large (or infinite or NaN)
	if ((!(frame.left>=-16777216.))
	||(!(frame.top>=-16777216.))
	||(!(frame.right<=16777216.))
	||(!(frame.bottom<=16777216.))) {
		fSize=0;
		fBound=BRect();
		fWindow=NULL;
		fServerToken=NO_TOKEN;
		fToken=gDefaultTokens->NewToken(BITMAP_TOKEN_TYPE, this);
		fInitError = B_BAD_VALUE;
		return;
	}

// constrain the frame to integral values
	frame.left=floor(frame.left);
	frame.top=floor(frame.top);
	frame.right=floor(frame.right);
	frame.bottom=floor(frame.bottom);

// from here, frame has integral coordinates.
	fBound = frame;

// check with memory adviser
	size_t totalMemory = 0;
	if ((flags & (B_BITMAP_GEH_COMPRESSED | B_BITMAP_LBX_COMPRESSED)))
	{
	 // don't call the memory advisor. Or do someting better.
	}
	else
	{
		size_t pixel_chunk;
		size_t bytes_per_pixel;
		size_t pixels_per_chunk;
		if (get_pixel_size_for(depth, &pixel_chunk, &bytes_per_pixel, &pixels_per_chunk) != B_OK) {
			bytes_per_pixel = 4;
		}

		if (fRowBytes> 0) {
			totalMemory= size_t(fRowBytes*(fBound.bottom-fBound.top+1));
		} else {
			double aprox_row_stride = (fBound.right-fBound.left+1)*bytes_per_pixel;
			totalMemory= size_t(aprox_row_stride*(fBound.bottom-fBound.top+1));
		}
		if(!madv_reserve_memory(totalMemory, "BBitmap::init")) {
			data_type = 0;
			data= 0xffffffff;
			fWindow = NULL;
			fRowBytes = 0;
			goto no_mem;
		}
	}

	if (flags & B_BITMAP_ACCEPTS_VIEWS) {
		fToken = NO_TOKEN;
		fWindow = new BWindow(frame, depth,fFlags,fRowBytes);

		if (fWindow->server_token != NO_TOKEN) {
			fWindow->a_session->swrite_l(GR_GET_BASE_POINTER);
			fWindow->a_session->flush();
			fWindow->a_session->sread(1, &data_type);
			fWindow->a_session->sread(4, &data);
			fWindow->a_session->sread(4, &fRowBytes);
		}
		else {
			data_type = 0;
			data = 0xffffffff;
			fRowBytes = 0;
		}
	} else {
		fWindow = NULL;

		_BAppServerLink_ link;
		fToken = gDefaultTokens->NewToken(BITMAP_TOKEN_TYPE, this);
		
		link.session->swrite_l(GR_NEW_BITMAP);
		link.session->swrite_rect(&fBound);
		link.session->swrite_l(fType);
		link.session->swrite_l(flags);
		link.session->swrite_l(fRowBytes);

		link.session->flush();

		link.session->sread(4, &fServerToken);
		link.session->sread(1, &data_type);
		link.session->sread(4, &data);
		link.session->sread(4, &fRowBytes);
	}

no_mem:
	if (data == 0xFFFFFFFF) {
		fInitError = B_UNSUPPORTED;
		fArea = B_NO_MEMORY;
		fBasePtr = NULL; //malloc(fRowBytes * (int32)(fBound.bottom-fBound.top+1));
	} else {
		if (fFlags & (B_BITMAP_GEH_COMPRESSED | B_BITMAP_LBX_COMPRESSED)) {
			fBasePtr = NULL;
			fInitError = B_OK;
		} else if (data_type == 2) {
			fBasePtr = (void*)data;
			fInitError = fBasePtr?B_OK:B_NO_MEMORY;
		} else if (data_type == 1) {
			fOrigArea = data;
			fInitError = (fOrigArea>0)?B_OK:B_NO_MEMORY;
		} else {
			fBasePtr = be_app->rw_offs_to_ptr(data);
			fInitError = (fBasePtr!=NULL)?B_OK:B_NO_MEMORY;
		};
	};

	fSize = int32(fBound.bottom - fBound.top + 1) * fRowBytes;
}
	
/*-------------------------------------------------------------*/

bool BBitmap::IsValid() const
{
	return (fInitError == B_OK);
}
	
/*-------------------------------------------------------------*/

status_t BBitmap::InitCheck() const
{
	return fInitError;
}
	
/*-------------------------------------------------------------*/

status_t BBitmap::Upload(BBitmap */*from*/, BRect /*fromRect*/, BPoint /*toPoint*/)
{
	return B_UNSUPPORTED;
}

/*-------------------------------------------------------------*/

status_t BBitmap::Freeze()
{
	return B_UNSUPPORTED;
}

/*-------------------------------------------------------------*/

status_t BBitmap::Thaw()
{
	return B_UNSUPPORTED;
}
	
/*-------------------------------------------------------------*/

status_t BBitmap::SelectLbxBitmap(int32 lbx_token, int32 index)
{
	if (!(fFlags & B_BITMAP_LBX_COMPRESSED))
		return B_UNSUPPORTED;
	int32 err;
	_BAppServerLink_ link;
	link.session->swrite_l(GR_SELECT_LBX_BITMAP);
	link.session->swrite_l(fServerToken);
	link.session->swrite_l(lbx_token);
	link.session->swrite_l(index);
	link.session->flush();
	link.session->sread(sizeof(int32), &err);
	return (status_t)err;
}

/*-------------------------------------------------------------*/

status_t BBitmap::GetOverlayRestrictions(overlay_restrictions *restrict) const
{
	if (!(fFlags & B_BITMAP_WILL_OVERLAY)) return B_ERROR;
	
	struct {
		uint32 opCode;
		int32 token;
	} protocol;
	int32 err;
	
	_BAppServerLink_ a;
	
	protocol.opCode = GR_GET_OVERLAY_RESTRICTIONS;
	protocol.token = fServerToken;
	a.session->swrite(sizeof(protocol),&protocol);
	a.session->flush();
	a.session->sread(sizeof(int32),&err);
	a.session->sread(sizeof(overlay_restrictions),restrict);
	return err;
}

/*-------------------------------------------------------------*/

BBitmap::~BBitmap()
{
	if (fArea != B_BAD_VALUE && fArea != B_NO_MEMORY && fArea != B_ERROR)
		delete_area(fArea);

	if (fWindow)
		fWindow->BitmapClose();
	else {
		if (fServerToken!=NO_TOKEN) {
			_BAppServerLink_	link(false);
	
			if (link.session) {
				link.session->swrite_l(GR_DELETE_BITMAP);
				link.session->swrite_l(fServerToken);
				link.session->flush();
			}
		}
		if (fToken != NO_TOKEN && gDefaultTokens)
			gDefaultTokens->RemoveToken(fToken);
		if ((fArea == B_NO_MEMORY) && fBasePtr) free(fBasePtr);
	}
	
	memset(this,0,sizeof(BBitmap));
	fToken = NO_TOKEN;
	fServerToken = NO_TOKEN;
	fBasePtr = (void*)0x08675309;  // Gotcha, Jenny!
}

/*------------------------------------------------------------*/

BBitmap::BBitmap(BMessage *data)
	: BArchivable(data)
{
	char *ptr;
	int32 size;
	bool	accepts_views;
	bool	need_contiguous;
	int32	flags;
	int32	rowBytes;
	screen_id screenID;
	BRect	r;
	long	cspace;

	if (data->FindInt32(S_BITMAP_FLAGS, &flags)) {
		flags = B_BITMAP_CLEAR_TO_WHITE;
		if (!data->FindBool(S_ACCEPTS_VIEWS, &accepts_views))
			flags |= accepts_views?B_BITMAP_ACCEPTS_VIEWS:0;
		if (!data->FindBool(S_NEED_CONTIGUOUS, &need_contiguous))
			flags |= need_contiguous?B_BITMAP_IS_CONTIGUOUS:0;
	};
	
	data->FindRect(S_FRAME, &r);
	data->FindInt32(S_COLOR_SPACE, &cspace);
	if (data->FindInt32(S_ROW_BYTES, &rowBytes)) rowBytes = -1;
	data->FindInt32(S_SCREEN_ID, &screenID.id);

	InitObject(r, (color_space) cspace, flags, rowBytes, B_MAIN_SCREEN_ID);
	AssertPtr();

	if (flags & B_BITMAP_ACCEPTS_VIEWS) fWindow->UnarchiveChildren(data);
	if (data->FindData(S_DATA, B_RAW_TYPE, (const void **)&ptr, &size) == B_OK)
		memcpy(get_shared_pointer(), ptr, size);
}

/*------------------------------------------------------------*/

BArchivable *BBitmap::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BBitmap"))
		return NULL;
	return new BBitmap(data);
}

/*------------------------------------------------------------*/

status_t BBitmap::Archive(BMessage *data, bool deep) const
{
	BArchivable::Archive(data, deep);

	int32 flags = fFlags;
	if (!deep) flags &= ~B_BITMAP_ACCEPTS_VIEWS;

	data->AddRect(S_FRAME, fBound);
	data->AddInt32(S_COLOR_SPACE, fType);
	data->AddInt32(S_BITMAP_FLAGS, flags);
	data->AddInt32(S_ROW_BYTES, fRowBytes);
	if (flags & B_BITMAP_ACCEPTS_VIEWS) fWindow->ArchiveChildren(data, deep);
	if (get_shared_pointer()) data->AddData(S_DATA, B_RAW_TYPE, get_shared_pointer(), fSize);
	
	return 0;
}

/*-------------------------------------------------------------*/


static bool bitmaps_are_big_endian( void )
{
	system_info si;

	return 0;
	get_system_info( &si );
	return si.platform_type == B_MAC_PLATFORM;
}

/*	This is just totally lame.  We need to deprecate this and
	provide a real way to easily import pixels. --geh */
void	BBitmap::SetBits(const void* data, int32 length, int32 offset,
			      color_space depth)
{
	if (fType == B_MONOCHROME_1_BIT) {
		switch (depth) {
			case B_MONOCHROME_1_BIT:
				set_bits(offset, (char *)data, length);
				break;

			case B_GRAYSCALE_8_BIT	:
			case B_COLOR_8_BIT		:
			case B_RGB_32_BIT		:
			default:
				debugger("BBitmap::SetBits - depth not supported yet\n");
				break;
			
		}
		return;
	}

	if (fType == B_COLOR_8_BIT)
	switch (depth) {
		case B_COLOR_8_BIT		:
		case B_MONOCHROME_1_BIT	:
			set_bits(offset, (char *)data, length);
			break;

		case B_RGB_32_BIT		:
			set_bits_24(offset, (char *)data, length);
			break;

		case B_GRAYSCALE_8_BIT	:
			debugger("BBitmap::SetBits - depth not supported yet\n");
			break;

		default:
			break;
	}
	if (fType == B_RGB_32_BIT) {
		bool swab = bitmaps_are_big_endian();
		
		switch (depth) {
			case B_COLOR_8_BIT:
				set_bits_8_24(offset, (char *)data, length, swab);
				break;
	
			case B_RGB_32_BIT:
				set_bits_24_24(offset, (char *)data, length, swab);
				break;
	
			case B_GRAYSCALE_8_BIT:
				set_bits_gray_24(offset, (char *)data, length, swab);
				break;
			
			default:
				break;
		}
	}
}

/*---------------------------------------------------------------*/

void	BBitmap::set_bits_gray_24(long offset, char *data, long length,
							   bool big_endian_dst )
{
	char		*bits = (char *)Bits();
	uchar		index;
	//rgb_color	c;

	if (bits) {
		bits += offset;
		if (big_endian_dst) {
			while(length > 0) {
				index = *((uchar *)data);
				data++;
				length--;
				*bits++ = 0;
				*bits++ = index;
				*bits++ = index;
				*bits++ = index;
			}
		}
		else {
			while(length > 0) {
				index = *((uchar *)data);
				data++;
				length--;
				*bits++ = index;
				*bits++ = index;
				*bits++ = index;
				*bits++ = 0;
			}
		}
	}
}


/*---------------------------------------------------------------*/

void	BBitmap::set_bits_8_24(long offset, char *data, long length,
							   bool big_endian_dst )
{
	char		*bits = (char *)Bits();
	uchar		index;
	rgb_color	c;
	
	// This prevents the screen and its color map from disappearing
	// for the duration of this function call.
  	BScreen screen( B_MAIN_SCREEN_ID );
	const color_map *map = screen.ColorMap();
	if( map == NULL ) {
		return;
	}

	if (bits) {
		bits += offset;
		if (big_endian_dst) {
			while(length > 0) {
				index = *((uchar *)data);
				data++;
				c = map->color_list[index];
				length--;
				*bits++ = 255;
				*bits++ = c.red;
				*bits++ = c.green;
				*bits++ = c.blue;
			}
		}
		else {
			while(length > 0) {
				index = *((uchar *)data);
				data++;
				c = map->color_list[index];
				length--;
				*bits++ = c.blue;
				*bits++ = c.green;
				*bits++ = c.red;
				*bits++ = 255;
			}
		}
	}
}

/*---------------------------------------------------------------*/

void	BBitmap::set_bits_24_24(long offset, char *data, long length,
							   bool big_endian_dst )
{
	char	*bits = (char *)Bits();
	char	p1,p2,p3;

	if (bits) {
		bits += offset;
		if (big_endian_dst) {
			while(length > 0) {
				p1 = *data++;
				p2 = *data++;
				p3 = *data++;
				*bits++ = 0;
				*bits++ = p1;
				*bits++ = p2;
				*bits++ = p3;
				length -= 3;
			}
		}
		else {
			while(length > 0) {
				p1 = *data++;
				p2 = *data++;
				p3 = *data++;
				*bits++ = p3;
				*bits++ = p2;
				*bits++ = p1;
				*bits++ = 0;
				length -= 3;
			}
		}
	}
}

/*---------------------------------------------------------------*/

void	BBitmap::set_bits_24(long offset, char *data, long length)
{
	char		*bits;
	long		bid;
	long		buf[4];

	bits = (char *)Bits();

	if (bits) {
		switch(fType) {
			case B_COLOR_8_BIT:
				set_bits_24_local_256(offset, (uchar *)data, length);
				return;
			case B_GRAYSCALE_8_BIT:
				set_bits_24_local_gray(offset, data, length);
				return;
			default:
				break;
		}
	}	
	

// Do i need to put a warning in that case ??

	if ((offset + length) > fSize) {
		length = fSize - offset;
	}

	if (bits) {
		memcpy(bits + offset, data, length);
		return;
	}

	_BAppServerLink_	link;

	buf[0] = GR_SET_BITS_24;
	buf[1] = get_server_token();
	buf[2] = offset;
	buf[3] = length;

	link.session->swrite(16, buf);
	link.session->swrite(length, data);
	link.session->flush();
	link.session->sread(4, &bid);
}

/*-------------------------------------------------------------*/

char	*BBitmap::get_shared_pointer() const
{
	return((char *)fBasePtr);
}

/*-------------------------------------------------------------*/

void	BBitmap::set_bits(long offset, char *data, long length)
{
	char		*shared_pointer;
	long		bid;
	long		buf[4];

	shared_pointer = (char*)Bits();	//get_shared_pointer();

// Do i need to put a warning in that case ??

	if ((offset + length) > fSize) {
		length = fSize - offset;
	}

	if (shared_pointer) {
		memcpy(shared_pointer + offset, data, length);
		return;
	}

//fixme:	this will fail miserably
	_BAppServerLink_	link;

	buf[0] = GR_SET_BITS;
	buf[1] = get_server_token();
	buf[2] = offset;
	buf[3] = length;

	link.session->swrite(16, buf);
	link.session->swrite(length, data);
	link.session->flush();
	link.session->sread(4, &bid);
}

/*---------------------------------------------------------------*/

void	BBitmap::set_bits_24_local_gray(long offset, char *data, long length)
{
	char		*base;
	long		i;
	ulong		a_byte;
	long		result;
	long		error = 0;
	long		c;

	base = get_shared_pointer() + offset;

	length /= 3;
	for (i = 0; i < length; i++) {

		c  = (*data++);
		c += (*data++);
		c += (*data++);
		c /= 3;
		
		error = c - error;

		if (error > 255)
			error = 255;
		else
		if (error < 0)
			error = 0;
		
// here is a code that assume that the first 32 clut entries are a gray ramp !!

		*base++ = a_byte = (error >> 3);
		result = a_byte << 3;
		
		
		error = result - error;
	}
}

/*---------------------------------------------------------------*/

void	BBitmap::set_bits_24_local_256(long offset, uchar *data, long length)
{
	char		*base;
	long		i;
	uchar		a_byte;
	rgb_color	a_color;
	rgb_color	result_color;
	long		error_r = 0;
	long		error_g = 0;
	long		error_b = 0;
	
	BScreen screen( B_MAIN_SCREEN_ID );
	const color_map *map = screen.ColorMap();
	if( map == NULL ) {
		return;
	}

	base = get_shared_pointer() + offset;

	length /= 3;
	for (i = 0; i < length; i++) {

		error_r = (long)*data++ - error_r;

		if (error_r > 255)
			error_r = 255;
		else
			if (error_r < 0)
				error_r = 0;
		
		error_g = (long)*data++ - error_g;
		
		if (error_g > 255)
			error_g = 255;
		else
			if (error_g < 0)
				error_g = 0;

		error_b = (long)*data++ - error_b;
		
		if (error_b > 255)
			error_b = 255;
		else
			if (error_b < 0)
				error_b = 0;


		a_color.red = error_r;
		a_color.green = error_g;
		a_color.blue = error_b;

		*base++ = a_byte = screen.IndexForColor(a_color);
		result_color = map->color_list[a_byte];
		
		
		error_r = (long)result_color.red - (long)a_color.red;
		error_g = (long)result_color.green - (long)a_color.green;
		error_b = (long)result_color.blue - (long)a_color.blue;
	}
}

/*-------------------------------------------------------------*/

int32	BBitmap::get_server_token() const
{
	if (fWindow)
		return fWindow->server_token;
	else
		return fServerToken;
}

/*-------------------------------------------------------------*/

void	BBitmap::AddChild(BView *view)
{
	if (!fWindow) {
		debugger("This BBitmap doesn't accept children views\n");
		return;
	}
	fWindow->AddChild(view);
}

/*-------------------------------------------------------------*/

bool	BBitmap::RemoveChild(BView *view)
{
	if (!fWindow) {
		debugger("This BBitmap doesn't accept children views\n");
		return false;
	}
	return fWindow->RemoveChild(view);
}

/*-------------------------------------------------------------*/

long	BBitmap::CountChildren() const
{
	if (!fWindow) {
		debugger("This BBitmap doesn't accept children views\n");
		return 0;
	}
	return fWindow->CountChildren();
}

/*-------------------------------------------------------------*/

BView	*BBitmap::ChildAt(long index) const
{
	if (!fWindow) {
		debugger("This BBitmap doesn't accept children views\n");
		return NULL;
	}
	return fWindow->ChildAt(index);
}

/*-------------------------------------------------------------*/

BView	*BBitmap::FindView(const char *name) const
{
	if (!fWindow) {
		debugger("This BBitmap doesn't accept children views\n");
		return NULL;
	}
	return fWindow->FindView(name);
}

/*-------------------------------------------------------------*/

BView	*BBitmap::FindView(BPoint pt) const
{
	if (!fWindow) {
		debugger("This BBitmap doesn't accept children views\n");
		return NULL;
	}
	return fWindow->FindView(pt);
}

/*-------------------------------------------------------------*/

bool	BBitmap::Lock()
{
	if (fWindow) {
		return fWindow->Lock();
	}
	return false;
}

/*-------------------------------------------------------------*/

void	BBitmap::Unlock()
{
	if (fWindow) {
		fWindow->Unlock();
	}
}

/*-------------------------------------------------------------*/

bool	BBitmap::IsLocked() const
{
	if (fWindow) {
		return fWindow->IsLocked();
	} else {
		return false;
	}
}

/*-------------------------------------------------------------*/

int32 BBitmap::BytesPerRow() const
	{ return(fRowBytes); }

/*-------------------------------------------------------------*/

color_space BBitmap::ColorSpace() const
	{ return(fType); }

/*-------------------------------------------------------------*/

BRect BBitmap::Bounds() const
	{ return(fBound); }

/*-------------------------------------------------------------*/

area_id BBitmap::Area() const
	{ return (fOrigArea>0)?fOrigArea:B_BAD_VALUE; }

/*-------------------------------------------------------------*/

void BBitmap::AssertPtr()
{
	if (fBasePtr) return;
	if ((fOrigArea > 0) && (fArea == B_ERROR)) {
		if (fFlags & B_BITMAP_IS_OFFSCREEN) {
			fArea = clone_area("overlay shared data", &fBasePtr, B_ANY_ADDRESS,
								B_READ_AREA, fOrigArea);
			if (fArea<0) {
				fArea = B_BAD_VALUE;
				fBasePtr=NULL;
			}
		} else {
			fArea = clone_area("BBitmap", &fBasePtr, B_ANY_ADDRESS,
								B_READ_AREA + B_WRITE_AREA, fOrigArea);
			if (fArea<0) {
				fArea = B_BAD_VALUE;
				fBasePtr=NULL;
			}
		};
	}
};

status_t BBitmap::LockBits(uint32 *state)
{
	status_t err = B_ERROR;
	AssertPtr();
	if (fBasePtr && (fFlags & B_BITMAP_IS_OFFSCREEN)) {
		while ((err=acquire_sem(((_offscreen_data_*)fBasePtr)->lock)) == B_INTERRUPTED)
			;
	};
	if (state) *state = 0;
	return B_OK;
}

void BBitmap::UnlockBits()
{
	if (fBasePtr && (fFlags & B_BITMAP_IS_OFFSCREEN))
		release_sem(((_offscreen_data_*)fBasePtr)->lock);
}

void *BBitmap::Bits() const
{
	BBitmap *b = const_cast<BBitmap*>(this);
	b->AssertPtr();

	if (fBasePtr && (fFlags & B_BITMAP_IS_OFFSCREEN))
		return ((_offscreen_data_*)fBasePtr)->bits;

	return fBasePtr;
}

/*-------------------------------------------------------------*/

int32 BBitmap::BitsLength() const
	{ return(fSize); }

/*-------------------------------------------------------------*/

int32 BPrivate::IKAccess::BitmapToken(const BBitmap* bitmap)
{
	return bitmap->get_server_token();
}

/*-------------------------------------------------------------*/

status_t BBitmap::Perform(perform_code code, void *arg)
{
	return BArchivable::Perform(code, arg);
}

/*-------------------------------------------------------------*/

BBitmap &BBitmap::operator=(const BBitmap &) { return *this; }

/* ---------------------------------------------------------------- */

void BBitmap::_ReservedBitmap1() {}
void BBitmap::_ReservedBitmap2() {}
void BBitmap::_ReservedBitmap3() {}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
