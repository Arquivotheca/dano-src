/******************************************************************************
/
/	File:			BitmapCollection.cpp
/
/	Copyright 1992-2000, Be Incorporated, All Rights Reserved.
/
*******************************************************************************/

#include <stdlib.h>

#include <OS.h>
#include <DataIO.h>
#include <Bitmap.h>
#include <BitmapCollection.h>
#include <session.h>
#include <message_util.h>
#include <messages.h>
#include <token.h>
#include <support/memadviser.h>

#include "DrawEngine.h"

namespace BPrivate
{
	struct bitmap_collection_p
	{
		status_t status;
		LBX_Container *lbx;
		int32 server_token;
		void *lbx_buffer;
		area_id shared_area;
		area_id local_area;
	};
} using namespace BPrivate;

#define m _m_rprivate

#define B_BITMAP_LBX_COMPRESSED	B_BITMAP_RESERVED_1

// -------------------------------------------------------------

BBitmapCollection::BBitmapCollection(BDataIO *lbx_data, size_t lbx_size)
	: 	_m_private(new bitmap_collection_p),
		_m_rprivate(*_m_private)
{
	m.lbx = NULL;
	m.status = B_NO_INIT;
	m.server_token = NO_TOKEN;

	if (!madv_reserve_memory(lbx_size, "BBitmapCollection::LBX Buffer")) {
		m.shared_area = B_ERROR;
	} else {
		// Ask the app_server to create a shared area for the LBX buffer
		_BAppServerLink_ link;
		link.session->swrite_l(GR_NEW_LBX);
		link.session->swrite_l(lbx_size);
		link.session->flush();
		link.session->sread(4, &m.server_token);
		link.session->sread(4, &m.shared_area);
	}

	madv_finished_allocating(lbx_size);

	// There was an error
	if (m.shared_area < B_OK)
		return;
	
	// Clone the area and initialize the LBX_Container
	m.local_area = clone_area("LBX", &m.lbx_buffer, B_ANY_ADDRESS, B_READ_AREA|B_WRITE_AREA, m.shared_area);
	if ((m.local_area > 0) && (m.lbx_buffer))
	{
		ssize_t s;
		if ((s = lbx_data->Read(m.lbx_buffer, lbx_size)) == (ssize_t)lbx_size)
		{
			m.lbx = LBX_BuildContainer((uint8 *)m.lbx_buffer);
			m.status = B_OK;
		}
	}
}

BBitmapCollection::~BBitmapCollection()
{
	// Get rid of our copy of the LBX area
	if (m.local_area > 0)
		delete_area(m.local_area);

	// Tell the app_server that this LBX will not be used anymore
	if (m.server_token != NO_TOKEN)
	{
		_BAppServerLink_ link;
		link.session->swrite_l(GR_DELETE_LBX);
		link.session->swrite_l(m.server_token);
		link.session->flush();
	}

	delete m.lbx;
	delete _m_private;
}

status_t BBitmapCollection::InitCheck() const
{
	return m.status;
}

// -------------------------------------------------------------
// #pragma mark -

int32 BBitmapCollection::CountBitmaps() const
{
	if (InitCheck() != B_OK)
		return 0;
	return m.lbx->BitmapCount();
}

status_t BBitmapCollection::GetBitmapInfo(const char *name, int32 *width, int32 *height) const
{
	if (InitCheck() != B_OK)
		return InitCheck();

	const int32 index = m.lbx->GetIndexFromName((char *)name);
	if ((index < 0) || (index >= (int32)m.lbx->BitmapCount()))
		return B_BAD_VALUE;

	*width = (int32)m.lbx->BitmapWidth(index);
	*height = (int32)m.lbx->BitmapHeight(index);
	return B_OK;
}

status_t BBitmapCollection::GetBitmapInfo(int32 index, BString& name, int32 *width, int32 *height) const
{
	if (InitCheck() != B_OK)
		return InitCheck();

	if ((index < 0) || (index >= (int32)m.lbx->BitmapCount()))
		return B_BAD_VALUE;

	char nameBuffer[256];
	m.lbx->GetBitmapName(index, nameBuffer);
	name = nameBuffer;

	if (width)	*width = (int32)m.lbx->BitmapWidth(index);
	if (height)	*height = (int32)m.lbx->BitmapHeight(index);
	return B_OK;
}

int32 BBitmapCollection::GetIndexForBitmap(const char *name) const
{
	if (InitCheck() != B_OK)
		return -1;
	return m.lbx->GetIndexFromName((char *)name);
}

BBitmap *BBitmapCollection::GetBitmap(const char *name)
{
	if (InitCheck() != B_OK)
		return NULL;
	return GetBitmap(m.lbx->GetIndexFromName((char *)name));
}

void BBitmapCollection::_ReservedBitmapCollection0() { }
void BBitmapCollection::_ReservedBitmapCollection1() { }
void BBitmapCollection::_ReservedBitmapCollection2() { }
void BBitmapCollection::_ReservedBitmapCollection3() { }

// -------------------------------------------------------------
// #pragma mark -

BBitmap *BBitmapCollection::GetBitmap(int32 index)
{
	if (InitCheck() != B_OK)
		return NULL;

	if ((index < 0) || (index >= (int32)m.lbx->BitmapCount()))
		return NULL;

	// Create an empty LBX BBitmap
	BRect r;
	BBitmap *bitmap = NULL;
	if (!m.lbx->IsRotated())
	{
		r.Set(0, 0, (float)(m.lbx->BitmapWidth(index)-1), (float)(m.lbx->BitmapHeight(index)-1));
		bitmap = new BBitmap(r, B_BITMAP_LBX_COMPRESSED, B_RGB32);
	}
#if ROTATE_DISPLAY
	else
	{ // We need to invert the bitmap's coordinates, because the user must see a non rotated bitmap
		// when calling Bounds()
		r.Set(0, 0, (float)(m.lbx->BitmapHeight(index)-1), (float)(m.lbx->BitmapWidth(index)-1));
		bitmap = new BBitmap(r, B_BITMAP_LBX_COMPRESSED | B_BITMAP_IS_ROTATED, B_RGB32);
	}
#endif

	if (!bitmap || (bitmap->InitCheck() != B_OK)) {
		delete bitmap;
		return NULL;
	}
	
	// Initialize it with the requested bitmap
	if (bitmap->SelectLbxBitmap(m.server_token, index) != B_OK) {
		delete bitmap;
		return NULL;
	}

	return bitmap;
}

