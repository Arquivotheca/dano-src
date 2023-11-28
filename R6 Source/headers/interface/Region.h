/*******************************************************************************
/
/	File:			Region.h
/
/   Description:    BRegion represents an area that's composed of individual
/                   rectangles.
/
/	Copyright 1992-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef	_REGION_H
#define	_REGION_H

#include <BeBuild.h>
#include <Rect.h>

namespace BPrivate {
	class IRegion;
	class IKAccess;
}

class BDataIO;

/*----------------------------------------------------------------*/
/*----- BRegion class --------------------------------------------*/

class BRegion {

public:
				BRegion();
				BRegion(const BRegion &region);
				BRegion(const BRect rect);
virtual			~BRegion();	

		BRegion	&operator=(const BRegion &from);

		BRect	Frame() const;
clipping_rect	FrameInt() const;
		BRect	RectAt(int32 index) const;
clipping_rect	RectAtInt(int32 index) const;
		int32	CountRects() const;
		void	Set(BRect newBounds);
		void	Set(clipping_rect newBounds);
		bool	Intersects(BRect r) const;
		bool	Intersects(clipping_rect r) const;
		bool	Contains(BPoint pt) const;
		bool	Contains(int32 x, int32 y);
		void	PrintToStream() const;
		void	OffsetBy(int32 dh, int32 dv);
		void	MakeEmpty();
		void	Include(BRect r);
		void	Include(clipping_rect r);
		void	Include(const BRegion*);
		void	Exclude(BRect r);
		void	Exclude(clipping_rect r);
		void	Exclude(const BRegion*);
		void	IntersectWith(const BRegion*);

/*----- Private or reserved -----------------------------------------*/

				BRegion(BPrivate::IRegion* reg, bool takeOwnership=true);
		
private:

friend	class				BPrivate::IKAccess;

private:
		BPrivate::IRegion	*region;
		int32				ownsRegion;
		int32				_reserved[5];
};

BDataIO& operator<<(BDataIO& io, const BRegion& region);

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _REGION_H */
