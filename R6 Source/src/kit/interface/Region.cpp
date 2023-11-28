//******************************************************************************
//
//	File:		Region.cpp 
//
//	Description:	implementation of the region class
//	
//	Written by:	Benoit Schillings
//			New Version on Sept 28 1993	BGS
//
//	Copyright 1992-93, Be Incorporated
//
//******************************************************************************/

#include <Region.h>

#ifndef _DEBUG_H
#include <Debug.h>
#endif
 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <new>

#include <StreamIO.h>

#include <interface_misc.h>
#include <IRegion.h>

/*----------------------------------------------------------------*/

clipping_rect to_clipping_rect(BRect fr)
{
	clipping_rect r;
	r.top   = (int32)floor(fr.top);
	r.left  = (int32)floor(fr.left);
	r.bottom = (int32)ceil(fr.bottom);
	r.right = (int32)ceil(fr.right);
	return r;
}

BRect to_BRect(clipping_rect r)
{
	BRect fr;
	fr.top   = r.top;
	fr.left  = r.left;
	fr.bottom = r.bottom;
	fr.right = r.right;
	return fr;
}

#define	MAX_OUT			0x0FFFFFFF
static const clipping_rect invalRect = {MAX_OUT,MAX_OUT,-MAX_OUT,-MAX_OUT};

/*----------------------------------------------------------------*/

BRegion::BRegion()
{
	region = new(std::nothrow) IRegion();
	ownsRegion = true;
}

/*----------------------------------------------------------------*/

BRegion::BRegion(const BRect rect)
{
	region = new(std::nothrow) IRegion(to_clipping_rect(rect));
	ownsRegion = true;
}


/*----------------------------------------------------------------*/

BRegion::BRegion(const BRegion &reg)
{
	region = new(std::nothrow) IRegion(*reg.region);
	ownsRegion = true;
}

/*----------------------------------------------------------------*/

BRegion::BRegion(BPrivate::IRegion* reg, bool takeOwnership)
{
	region = reg;
	ownsRegion = takeOwnership;
}

/*----------------------------------------------------------------*/

BRegion	&BRegion::operator=(const BRegion &from)
{
	*region = *from.region;
	return *this;
}

/*----------------------------------------------------------------*/

BRegion::~BRegion()
{
	if (ownsRegion) delete region;
}

/*----------------------------------------------------------------*/

BRect BRegion::Frame() const
{
	if (!region) return BRect();
	return(BRect(	region->Bounds().left,region->Bounds().top,
					region->Bounds().right,region->Bounds().bottom));
}

/*----------------------------------------------------------------*/

clipping_rect BRegion::FrameInt() const
{
	if (!region) return invalRect;
	return region->Bounds();
}

/*----------------------------------------------------------------*/

BRect	BRegion::RectAt(int32 index) const
{
	const clipping_rect* r;
	if ((region != NULL) && (index >= 0) && (index < region->CountRects()) &&
			((r=region->Rects()) != NULL)) {
		r += index;
		return BRect(r->left, r->top, r->right, r->bottom);
	}
	
	return BRect();
}

/*----------------------------------------------------------------*/

clipping_rect	BRegion::RectAtInt(int32 index) const
{
	const clipping_rect* r;
	if ((region != NULL) && (index >= 0) && (index < region->CountRects()) &&
			((r=region->Rects()) != NULL)) {
		return r[index];
	}
	
	return invalRect;
}

/*----------------------------------------------------------------*/

int32	BRegion::CountRects() const
{
	return region ? region->CountRects() : 0;
}

/*----------------------------------------------------------------*/

bool	BRegion::Intersects(BRect a_rect) const
{
	return Intersects(to_clipping_rect(a_rect));
}

/*----------------------------------------------------------------*/

bool BRegion::Intersects(clipping_rect r) const
{
	return region ? region->Intersects(r) : false;
}

/*----------------------------------------------------------------*/

bool BRegion::Contains(int32 h, int32 v)
{
	return region ? region->Contains(h, v) : false;
}

/*----------------------------------------------------------------*/

bool	BRegion::Contains(BPoint a_point) const
{
	return const_cast<BRegion*>(this)->Contains((int32)a_point.x,(int32)a_point.y);
}

/*----------------------------------------------------------------*/

void	BRegion::Include(const BRegion* region)
{
	if (this->region) this->region->OrSelf(*region->region);
}

/*----------------------------------------------------------------*/

void	BRegion::Exclude(const BRegion* region)
{
	if (this->region) this->region->SubSelf(*region->region);
}

/*----------------------------------------------------------------*/

void	BRegion::IntersectWith(const BRegion* region)
{
	if (this->region) this->region->AndSelf(*region->region);
}

/*----------------------------------------------------------------*/

void	BRegion::PrintToStream() const
{
#if SUPPORTS_STREAM_IO
	BOut << *this << endl;
#endif
}

/*----------------------------------------------------------------*/

void BRegion::Exclude(BRect fr)
{
	Exclude(to_clipping_rect(fr));
}

/*----------------------------------------------------------------*/

void BRegion::Exclude(clipping_rect r)
{
	if (!r.is_valid() || !region) return;

	IRegion tmp(r);
	region->SubSelf(tmp);
}

/*----------------------------------------------------------------*/

void BRegion::Include(BRect fr)
{
	Include(to_clipping_rect(fr));
}

/*----------------------------------------------------------------*/

void BRegion::Include(clipping_rect r)
{
	if (region) region->OrRect(r);
}

void BRegion::MakeEmpty()
{
	if (region) region->MakeEmpty();
}

//-------------------------------------------------

void BRegion::Set(clipping_rect initialbound)
{
	if (region) region->Set(initialbound);
}

void BRegion::Set(BRect initialbound)
{
	Set(to_clipping_rect(initialbound));
}

/*----------------------------------------------------------------*/

void	BRegion::OffsetBy(int32 dh, int32 dv)
{
	if (region) region->OffsetBy(dh, dv);
}

/*----------------------------------------------------------------*/

BDataIO& operator<<(BDataIO& io, const BRegion& region)
{
#if SUPPORTS_STREAM_IO
	const int32 N = region.CountRects();
	
	io << "BRegion(" << region.FrameInt() << ") {";
	
	if (N > 1) io << endl << "\t";
	else io << " ";
	for (int i = 0; i < N; i++) {
		io << region.RectAtInt(i);
		if (i < N-1) {
			if (N <= 1) io << ", ";
			else io << "," << endl << "\t";
		}
	}
	
	if (N > 1) io << "\n}";
	else io << " }";
#else
	(void)region;
#endif
	return io;
}

const IRegion* IKAccess::RegionData(const BRegion* region)
{
	return region->region;
}

IRegion* IKAccess::RegionData(BRegion* region)
{
	return region->region;
}

// --------- Const incorrect methods removed 5/2000 ---------

#if _R5_COMPATIBLE_
extern "C" {

	_EXPORT BRect
	#if __GNUC__
	RectAt__7BRegionl
	#elif __MWERKS__
	RectAt__7BRegionFl
	#endif
	(BRegion* This, int32 index)
	{
		return This->RectAt(index);
	}
	
	_EXPORT clipping_rect
	#if __GNUC__
	RectAtInt__7BRegionl
	#elif __MWERKS__
	RectAtInt__7BRegionFl
	#endif
	(BRegion* This, int32 index)
	{
		return This->RectAtInt(index);
	}
	
	_EXPORT int32
	#if __GNUC__
	CountRects__7BRegion
	#elif __MWERKS__
	CountRects__7BRegionF
	#endif
	(BRegion* This)
	{
		return This->CountRects();
	}
	
}
#endif
