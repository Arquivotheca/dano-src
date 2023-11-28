//******************************************************************************
//
//	File:		Polygon.cpp
//
//	Description:	Implementation of client polygon class.
//	
//	Written by:	Eric Knight
//  Modified by: Jean-Baptiste Queru
//
//	Copyright 1992-93,1998, Be Incorporated, All Rights Reserved.
//
//******************************************************************************/

#include <stdlib.h>
#include <string.h>

#ifndef _DEBUG_H
#include "Debug.h"
#endif

#ifndef _POLYGON_H
#include "Polygon.h"
#endif

#ifndef _STREAM_IO_H
#include <StreamIO.h>
#endif

/*----------------------------------------------------------------*/

	BPolygon::BPolygon(const BPoint *ptArray, int32 numPts)
{	
	fBounds.Set(0, 0, -1, -1);
	fCount = 0;
	fPts = NULL;

	// NULL pointer and numPts<0 handled in AddPoints
	AddPoints(ptArray, numPts);
}

/*----------------------------------------------------------------*/

	BPolygon::BPolygon()
{
	fBounds.Set(0, 0, -1, -1);
	fCount = 0;
	fPts = NULL;
}

/*----------------------------------------------------------------*/

	BPolygon::BPolygon(const BPolygon* poly)
{
	if (poly == NULL) {
		fBounds.Set(0, 0, -1, -1);
		fCount = 0;
		fPts = NULL;
	} else {
		fBounds = poly->fBounds;
		fCount = poly->fCount;
		
		if (fCount) {
			fPts = (BPoint *)malloc(sizeof(BPoint) * fCount);
			memcpy(fPts, poly->fPts, sizeof(BPoint) * fCount);
		} else
			fPts = NULL;
	}
}

/*----------------------------------------------------------------*/

BPolygon::~BPolygon()
{
	if (fPts)
		free(fPts);
}

/*----------------------------------------------------------------*/

BPolygon	&BPolygon::operator=(const BPolygon &from)
{
	if (this != &from) {
		if (fPts != NULL)
			free (fPts);
		fBounds = from.fBounds;
		fCount = from.fCount;
		if (fCount) {
			fPts = (BPoint *)malloc(sizeof(BPoint) * fCount);
			memcpy(fPts, from.fPts, sizeof(BPoint) * fCount);
		} else
			fPts = NULL;
	}
	return *this;
}

/*----------------------------------------------------------------*/

void	BPolygon::PrintToStream() const
{
#if SUPPORTS_STREAM_IO
	BOut << *this << endl;
#endif
}

/*----------------------------------------------------------------*/

void	BPolygon::AddPoints(const BPoint *ptArray, int32 numPts)
{
	BPoint* pt_walker;

	if ((numPts <= 0)||(ptArray == NULL))
		return;

	// Handle case that these are this polygon's first pts
	if (fCount == 0) {
		pt_walker = fPts = (BPoint*)malloc(sizeof(BPoint) * numPts);
	}
	// Else, add space for new points
	else {
		fPts = (BPoint*)realloc(fPts, sizeof(BPoint) * (numPts + fCount));
		pt_walker = fPts + fCount;
	}

	// Add each new point in pt_list
	for(int32 i = 0; i < numPts; i++) {
		*pt_walker++ = *ptArray++;
	}

	fCount += numPts;
	compute_bounds();		
}

/*----------------------------------------------------------------*/

void	BPolygon::compute_bounds()
{
	if (fCount) {
		float 	top, left, bottom, right;
		BPoint*	pt_walker = fPts;
		
		if (pt_walker == NULL) {
			DEBUGGER("polygon fCount, fPts out of sync!\n");
			return;
		}
			
		left = right = pt_walker->x;
		top = bottom = pt_walker->y;
		
		for (int32 i = 1; i < fCount; i++) {
			pt_walker++;
			
			float h = pt_walker->x;
			float v = pt_walker->y;
			
			if (v < top)
				top = v;
			else if (v > bottom)
				bottom = v;
			if (h < left)
				left = h;
			else if (h > right)
				right = h;
		}

		fBounds.Set(left, top, right, bottom);
	}
}

/*----------------------------------------------------------------*/

void	BPolygon::MapTo(BRect src_rect, BRect dst_rect)
{
	if ((!(src_rect.left<src_rect.right)) // using !(a<b) instead of (a>=b) detects NaNs
	   //||(src_rect.left==(-1.0/0.0))
	   //||(src_rect.right==(1.0/0.0))
	   ||(!(src_rect.top<src_rect.bottom))
	   //||(src_rect.top==(-1.0/0.0))
	   //||(src_rect.bottom==(1.0/0.0))
	   ||(!(dst_rect.left<=dst_rect.right))
	   //||(dst_rect.left==(-1.0/0.0))
	   //||(dst_rect.right==(1.0/0.0))
	   ||(!(dst_rect.top<=dst_rect.bottom))
	   //||(dst_rect.top==(-1.0/0.0))
	   //||(dst_rect.bottom==(1.0/0.0))
	   )
		return;
	if (!(src_rect == dst_rect)) {
		map_rect(&fBounds, src_rect, dst_rect);
	
		BPoint*	pt_walker = fPts;
		for (int32 i = 0; i < fCount; i++) {
			map_pt(pt_walker, src_rect, dst_rect);
			pt_walker++;
		}
	}
}

/*----------------------------------------------------------------*/

void	BPolygon::map_pt(BPoint* pt, BRect src_rect, BRect dst_rect)
{
	//if (empty_rect(src_rect) || empty_rect(dst_rect))
	//	return;

	float	from_top = src_rect.left;
	float	from = src_rect.right - from_top;
	float	to_top = dst_rect.left;
	float	to = dst_rect.right - to_top;
	float*	val_ptr = &pt->x;
	
	for (short i = 0; i < 2; ++i) {
		float	pt_val, val_temp;

		pt_val = *val_ptr - from_top;

		if (from != to) {
			val_temp = pt_val;
			if (val_temp <= 0)
				pt_val = -pt_val;
			pt_val = ((pt_val * to)) / from;
			if (val_temp <= 0)
				pt_val = -pt_val;
		}
		
		*val_ptr = pt_val + to_top;
		
		from_top = src_rect.top;
		from = src_rect.bottom - from_top;
		to_top = dst_rect.top;
		to = dst_rect.bottom - to_top;
		val_ptr = &pt->y;
	}	
}

/*----------------------------------------------------------------*/

void	BPolygon::map_rect(BRect* r, BRect src_rect, BRect dst_rect)
{
	map_pt((BPoint*)(&(r->left)), src_rect, dst_rect);
	map_pt((BPoint*)(&(r->right)), src_rect, dst_rect);
}

/*----------------------------------------------------------------*/

BRect BPolygon::Frame() const
			{ return(fBounds); }

/*----------------------------------------------------------------*/

int32 BPolygon::CountPoints() const
			{ return(fCount); }

/*----------------------------------------------------------------*/

BPoint BPolygon::PointAt(int32 index) const
{
	if (index < 0 || index >= fCount) return BPoint();
	return fPts[index];
}

/*----------------------------------------------------------------*/

BDataIO& operator<<(BDataIO& io, const BPolygon& poly)
{
#if SUPPORTS_STREAM_IO
	const int32 N = poly.CountPoints();
	
	io << "BPolygon {";
	if (N > 3) io << endl << "\t";
	else io << " ";
	for (int i = 0; i < N; i++) {
		io << poly.PointAt(i);
		if (i < N-1) {
			if (N <= 3) io << ", ";
			else io << "," << endl << "\t";
		}
	}
	
	if (N > 3) io << endl << "}";
	else io << " }";
#else
	(void)poly;
#endif
	return io;
}
