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

#ifndef	_RENDER2_REGION_H_
#define	_RENDER2_REGION_H_

#include <render2/RenderDefs.h>
#include <render2/Rect.h>
#include <raster2/RasterRegion.h>
#include <support2/Flattenable.h>
#include <support2/Value.h>

namespace B {
namespace Render2 {

using namespace Support2;
using namespace Raster2;

class B2dTransform;
class BRegionCache;

/*----------------------------------------------------------------*/
/*----- BRegion class --------------------------------------------*/

class BRegion : public BFlattenable
{
	public:

		static const 	BRegion	empty;
		static const	BRegion	full;

						BRegion();
						BRegion(BRect rect);
						BRegion(const BRegion &region);
						BRegion(const BRasterRegion &region, const B2dTransform &xform);
						BRegion(const BValue& o, status_t* result=NULL);
						BRegion(const value_ref& o, status_t* result=NULL);
		virtual			~BRegion();
		
				BRegion	&operator=(const BRegion &from);
				bool	operator==(const BRegion& o) const;
		inline	bool	operator!=(const BRegion& o) const { return !(*this == o); }

				void	Swap(BRegion& with);
				
				BValue	AsValue() const;
		inline			operator BValue() const		{ return AsValue(); }
		
				BRect	Bounds() const;
		inline	bool	IsRect() const { return fData ? (fData->count == 1) : false; }
				bool	IsEmpty() const;
				bool	IsFull() const;
				void	MakeEmpty();
				void	Set(BRect newBounds);
				bool	Intersects(BRect r) const;
				bool	Contains(BPoint pt) const;
				void	OffsetBy(BPoint pt);
				void	OffsetBy(coord dx, coord dy) { OffsetBy(BPoint(dx,dy)); }

//				IRender::ptr	StartDrawing();
//				void			EndDrawing(const IRender::ptr &r);
					
				void		And(const BRegion& r2, BRegion* dest) const;
				void		Or(const BRegion& r2, BRegion* dest) const;
				void		Sub(const BRegion& r2, BRegion* dest) const;
				void		XOr(const BRegion& r2, BRegion* dest) const;
				
		inline	BRegion&	AndSelf(const BRegion& r2)			{ BRegion dest; And(r2, &dest); return (*this = dest); }
		inline	BRegion&	OrSelf(const BRegion& r2)			{ BRegion dest; Or(r2, &dest); return (*this = dest); }
		inline	BRegion&	SubSelf(const BRegion& r2)			{ BRegion dest; Sub(r2, &dest); return (*this = dest); }
		inline	BRegion&	XOrSelf(const BRegion& r2)			{ BRegion dest; XOr(r2, &dest); return (*this = dest); }
				
		inline	BRegion&	operator&=(const BRegion& r2)		{ return AndSelf(r2); }
		inline	BRegion&	operator|=(const BRegion& r2)		{ return OrSelf(r2); }
		inline	BRegion&	operator-=(const BRegion& r2)		{ return SubSelf(r2); }
		inline	BRegion&	operator^=(const BRegion& r2)		{ return XOrSelf(r2); }
		
		inline	BRegion		operator&(const BRegion& r2) const	{ BRegion dest; And(r2, &dest); return dest; }
		inline	BRegion		operator|(const BRegion& r2) const	{ BRegion dest; Or(r2, &dest); return dest; }
		inline	BRegion		operator-(const BRegion& r2) const	{ BRegion dest; Sub(r2, &dest); return dest; }
		inline	BRegion		operator^(const BRegion& r2) const	{ BRegion dest; XOr(r2, &dest); return dest; }

				void	Include(BRect r);
				void	Include(const BRegion &);
				void	Exclude(BRect r);
				void	Exclude(const BRegion &);
				void	IntersectWith(const BRegion &);
		
				void	Rasterize(BRasterRegion* target) const;
				void	Rasterize(BRasterRegion* target, const B2dTransform& t) const;
				
				void	Transform(const B2dTransform& t);
				
				void	PrintToStream(ITextOutput::arg io, uint32 flags = 0) const;
static	status_t		printer(ITextOutput::arg io, const value_ref& val, uint32 flags);

			// Implement BFlattenable API.
			virtual	bool		IsFixedSize() const;
			virtual	type_code	TypeCode() const;
			virtual	ssize_t		FlattenedSize() const;
			virtual	status_t	Flatten(void *buffer, ssize_t size) const;
			virtual	bool		AllowsTypeCode(type_code code) const;
			virtual	status_t	Unflatten(type_code c, const void *buf, ssize_t size);

	private:

			struct region {
						void	acquire() const;
						void	release() const;
						
				static	region*	create(const int32 init_avail = 8);
				
						region*	edit(const int32 needed, const int32 coulduse) const;
						region*	reuse(const int32 needed, const int32 coulduse) const;
						
				mutable	int32		refs;
						int32		count;
						int32		avail;
						void*		link;
						BRect		rects[1];
			};
			
			friend	class	BRegionCache;
			
	inline	region *		Grow(const int32 amount)
								{ return Edit(fData->count+amount, (fData->count+amount)*2); }
	inline	region *		GrowAvail(const int32 amount)
								{ return Edit(fData->avail+amount, (fData->avail+amount)*2); }
			region *		Edit(const int32 needed, const int32 coulduse) const;
			region *		ReUse(const int32 needed, const int32 coulduse) const;
			
			region *		EditSlow(const int32 needed, const int32 coulduse) const;
			region *		ReUseSlow(const int32 needed, const int32 coulduse) const;

			BRect *			CreateRects(int32 needed, int32 coulduse);
			void			SetRectCount(int32 num);
	inline	int32			CountRects() const
								{ return fData ? fData->count : 0; }
	inline	int32			CountAvail() const
								{ return fData ? fData->avail : 0; }
	inline	const BRect *	Rects() const
								{ return fData ? fData->rects : NULL; }
			
	inline	BRect &			BoundsRef()
								{ return fBounds.rects[0]; }
	inline	const BRect &	BoundsRef() const
								{ return fBounds.rects[0]; }
	
			int32			FindSpanAt(coord y) const;
			int32			FindSpanBetween(coord top, coord bottom) const;
			void			ScaleBy(BRegion* dest,
									const BRect& srcRect, const BRect& dstRect,
									BPoint offset) const;

	inline	void			UnsafeCopyFrom(const BRegion &src);
	inline	void			ConstructCommon();
	inline	void			Construct(const BRect &rect);
			void			Construct(const value_ref& ref, status_t* result);
	
			bool			CheckIntegrity() const;
			
			region			fBounds;
			const region*	fData;
			
	static	BRegionCache	cache;
};

/*----- Type and STL utilities --------------------------------------*/
void			BMoveBefore(BRegion* to, BRegion* from, size_t count = 1);
void			BMoveAfter(BRegion* to, BRegion* from, size_t count = 1);
void			BSwap(BRegion& t1, BRegion& t2);
void			swap(BRegion& x, BRegion& y);

ITextOutput::arg	operator<<(ITextOutput::arg io, const BRegion& region);

/*-------------------------------------------------------------*/
/*---- No user serviceable parts after this -------------------*/

inline void BSwap(BRegion& v1, BRegion& v2)
{
	v1.Swap(v2);
}

inline void swap(BRegion& x, BRegion& y)
{
	x.Swap(y);
}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

} } // namespace B::Render2

#endif /* _RENDER2_REGION_H_ */
