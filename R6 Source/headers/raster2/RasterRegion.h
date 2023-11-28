//******************************************************************************
//
//	File:			BRasterRegion.h
//
//	Description:	Integer-based region class, derived from the app_server.
//	
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#ifndef _RASTER2_REGION_H_
#define _RASTER2_REGION_H_

#include <support2/Flattenable.h>
#include <support2/Value.h>
#include <raster2/RasterDefs.h>
#include <raster2/RasterRect.h>

namespace B { namespace Private { class BRasterRegionCache; } }// This should be done with statics!

namespace B {
namespace Raster2 {


using namespace Support2;

class BRasterRegion : public BFlattenable
{
	public:
									BRasterRegion();
									BRasterRegion(const BRasterRect& r);
									BRasterRegion(const BRasterRegion& o);
									BRasterRegion(const BValue& o, status_t* result=NULL);
									BRasterRegion(const value_ref& o, status_t* result=NULL);
		virtual						~BRasterRegion();
		
				BRasterRegion&		operator=(const BRasterRegion& o);
				bool				operator==(const BRasterRegion& o) const;
		inline	bool				operator!=(const BRasterRegion& o) const		{ return !(*this == o); }
				
				void				Swap(BRasterRegion& with);
				
				BValue				AsValue() const;
		inline						operator BValue() const			{ return AsValue(); }
		
		inline	bool				IsEmpty() const					{ return fData ? (fData->count == 0) : true; }
		inline	bool				IsRect() const					{ return fData ? (fData->count == 1) : false; }
		
		inline	const BRasterRect &	Bounds() const					{ return fBounds.rects[0]; }
		inline	BRasterRect &		Bounds()						{ return fBounds.rects[0]; }
		inline	int32				CountRects() const				{ return fData ? fData->count : 0; }
		inline	const BRasterRect *	Rects() const					{ return fData ? fData->rects : NULL; }
		
				void				MakeEmpty();
				void				Set(const BRasterRect& r);
					
				void				AddRect(const BRasterRect& r);
				void				OrRect(const BRasterRect& r);
				
				void				And(const BRasterRegion& r2, BRasterRegion* dest) const;
				void				Or(const BRasterRegion& r2, BRasterRegion* dest) const;
				void				Sub(const BRasterRegion& r2, BRasterRegion* dest) const;
				void				XOr(const BRasterRegion& r2, BRasterRegion* dest) const;
				
		inline	BRasterRegion&		AndSelf(const BRasterRegion& r2)			{ BRasterRegion dest; And(r2, &dest); return (*this = dest); }
		inline	BRasterRegion&		OrSelf(const BRasterRegion& r2)				{ BRasterRegion dest; Or(r2, &dest); return (*this = dest); }
		inline	BRasterRegion&		SubSelf(const BRasterRegion& r2)			{ BRasterRegion dest; Sub(r2, &dest); return (*this = dest); }
		inline	BRasterRegion&		XOrSelf(const BRasterRegion& r2)			{ BRasterRegion dest; XOr(r2, &dest); return (*this = dest); }
				
		inline	BRasterRegion&		operator&=(const BRasterRegion& r2)		{ return AndSelf(r2); }
		inline	BRasterRegion&		operator|=(const BRasterRegion& r2)		{ return OrSelf(r2); }
		inline	BRasterRegion&		operator-=(const BRasterRegion& r2)		{ return SubSelf(r2); }
		inline	BRasterRegion&		operator^=(const BRasterRegion& r2)		{ return XOrSelf(r2); }
		
		inline	BRasterRegion		operator&(const BRasterRegion& r2) const	{ BRasterRegion dest; And(r2, &dest); return dest; }
		inline	BRasterRegion		operator|(const BRasterRegion& r2) const	{ BRasterRegion dest; Or(r2, &dest); return dest; }
		inline	BRasterRegion		operator-(const BRasterRegion& r2) const	{ BRasterRegion dest; Sub(r2, &dest); return dest; }
		inline	BRasterRegion		operator^(const BRasterRegion& r2) const	{ BRasterRegion dest; XOr(r2, &dest); return dest; }
		
				void				OffsetBy(int32 dh, int32 dv);
				void				ScaleBy(BRasterRegion* dest,
											const BRasterRect& srcRect, const BRasterRect& dstRect,
											int32 offsX, int32 offsY, bool tileX, bool tileY) const;
				
				void				CompressSpans();
				void				CompressMemory();
				
				bool				Contains(int32 x, int32 y) const;
				bool				Intersects(const BRasterRect& r) const;
				
				int32				FindSpanAt(int32 y) const;
				int32				FindSpanBetween(int32 top, int32 bottom) const;
				
		inline	int32				CountAvail() const						{ return fData ? fData->avail : 0; }
				BRasterRect *		EditRects(int32 needed, int32 coulduse);
				BRasterRect *		CreateRects(int32 needed, int32 coulduse);
				void				SetRectCount(int32 num);
		
				void				PrintToStream(ITextOutput::arg io, uint32 flags=0) const;
		static	status_t			printer(ITextOutput::arg io, const value_ref& val, uint32 flags);

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
							BRasterRect	rects[1];
				};
				
				friend	class	B::Private::BRasterRegionCache;
				
				void			Construct(const value_ref& ref, status_t* result);
		inline	region *		Grow(const int32 amount)
									{ return Edit(fData->count+amount, (fData->count+amount)*2); }
		inline	region *		GrowAvail(const int32 amount)
									{ return Edit(fData->avail+amount, (fData->avail+amount)*2); }
				region *		Edit(const int32 needed, const int32 coulduse) const;
				region *		ReUse(const int32 needed, const int32 coulduse) const;
				
				region *		EditSlow(const int32 needed, const int32 coulduse) const;
				region *		ReUseSlow(const int32 needed, const int32 coulduse) const;
				
				bool			CheckIntegrity() const;
				
				region			fBounds;
				const region*	fData;
};

/*----- Type and STL utilities --------------------------------------*/
void			BMoveBefore(BRasterRegion* to, BRasterRegion* from, size_t count = 1);
void			BMoveAfter(BRasterRegion* to, BRasterRegion* from, size_t count = 1);
void			BSwap(BRasterRegion& t1, BRasterRegion& t2);
void			swap(BRasterRegion& x, BRasterRegion& y);

ITextOutput::arg operator<<(ITextOutput::arg io, const BRasterRegion& region);

/*-------------------------------------------------------------*/
/*---- No user serviceable parts after this -------------------*/

inline void BSwap(BRasterRegion& v1, BRasterRegion& v2)
{
	v1.Swap(v2);
}

inline void swap(BRasterRegion& x, BRasterRegion& y)
{
	x.Swap(y);
}

} } // namespace B::Raster2

#endif	/* _RASTER2_REGION_H_ */
