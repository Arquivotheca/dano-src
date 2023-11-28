//******************************************************************************
//
//	File:			IRegion.h
//
//	Description:	Integer-based region class, derived from the app_server.
//	
//	Copyright 2001, Be Incorporated, All Rights Reserved.
//
//******************************************************************************

#ifndef _IREGION_H
#define _IREGION_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#ifndef _RECT_H
#include <Rect.h>
#endif

class BRegion;
struct DisplayRotater;

namespace BPrivate {

class IRegionCache;

class IRegion {
public:
						IRegion();
						IRegion(const clipping_rect& r);
						IRegion(const IRegion& o);
						~IRegion();

		IRegion&		operator=(const IRegion& o);
		bool			operator==(const IRegion& o) const;
inline	bool			operator!=(const IRegion& o) const		{ return !(*this == o); }
		
inline	bool			IsEmpty() const							{ return fData ? (fData->count == 0) : true; }
inline	bool			IsRect() const							{ return fData ? (fData->count == 1) : false; }

inline	const clipping_rect&	Bounds() const					{ return fBounds.rects[0]; }
inline	clipping_rect&			Bounds()						{ return fBounds.rects[0]; }
inline	int32					CountRects() const				{ return fData ? fData->count : 0; }
inline	const clipping_rect*	Rects() const					{ return fData ? fData->rects : NULL; }

		void			MakeEmpty();
		void			Set(const clipping_rect& r);
		
		void			AddRect(const clipping_rect& r);
		void			OrRect(const clipping_rect& r);
		
		void			And(const IRegion& r2, IRegion* dest) const;
		void			Or(const IRegion& r2, IRegion* dest) const;
		void			Sub(const IRegion& r2, IRegion* dest) const;
		void			XOr(const IRegion& r2, IRegion* dest) const;
		
inline	IRegion&		AndSelf(const IRegion& r2)			{ IRegion dest; And(r2, &dest); return (*this = dest); }
inline	IRegion&		OrSelf(const IRegion& r2)			{ IRegion dest; Or(r2, &dest); return (*this = dest); }
inline	IRegion&		SubSelf(const IRegion& r2)			{ IRegion dest; Sub(r2, &dest); return (*this = dest); }
inline	IRegion&		XOrSelf(const IRegion& r2)			{ IRegion dest; XOr(r2, &dest); return (*this = dest); }
		
inline	IRegion&		operator&=(const IRegion& r2)		{ return AndSelf(r2); }
inline	IRegion&		operator|=(const IRegion& r2)		{ return OrSelf(r2); }
inline	IRegion&		operator-=(const IRegion& r2)		{ return SubSelf(r2); }
inline	IRegion&		operator^=(const IRegion& r2)		{ return XOrSelf(r2); }

inline	IRegion			operator&(const IRegion& r2) const	{ IRegion dest; And(r2, &dest); return dest; }
inline	IRegion			operator|(const IRegion& r2) const	{ IRegion dest; Or(r2, &dest); return dest; }
inline	IRegion			operator-(const IRegion& r2) const	{ IRegion dest; Sub(r2, &dest); return dest; }
inline	IRegion			operator^(const IRegion& r2) const	{ IRegion dest; XOr(r2, &dest); return dest; }

		void			OffsetBy(int32 dh, int32 dv);
		void			ScaleBy(IRegion* dest,
								const clipping_rect& srcRect, const clipping_rect& dstRect,
								int32 offsX, int32 offsY, bool tileX, bool tileY) const;
		void			Rotate(const DisplayRotater& rotater, IRegion* dest) const;
		
		void			CompressSpans();
		void			CompressMemory();
		
		bool			Contains(int32 x, int32 y) const;
		bool			Intersects(const clipping_rect& r) const;
		
		int32			FindSpanBetween(int32 top, int32 bottom) const;
		
inline	int32			CountAvail() const						{ return fData ? fData->avail : 0; }
		clipping_rect*	EditRects(int32 needed, int32 coulduse);
		clipping_rect*	CreateRects(int32 needed, int32 coulduse);
		void			SetRectCount(int32 num);
		
private:

friend	class			IRegionCache;

		struct region {
					void	acquire() const;
					void	release() const;
					
			static	region*	create(const int32 init_avail = 8);
			
					region*	edit(const int32 needed, const int32 coulduse) const;
					region*	reuse(const int32 needed, const int32 coulduse) const;
					
			mutable	int32	refs;
					int32	count;
					int32	avail;
					void*	link;
					clipping_rect	rects[1];
		};
		
inline	region*			Grow(const int32 amount)
							{ return Edit(fData->count+amount, (fData->count+amount)*2); }
inline	region*			GrowAvail(const int32 amount)
							{ return Edit(fData->avail+amount, (fData->avail+amount)*2); }
		region*			Edit(const int32 needed, const int32 coulduse) const;
		region*			ReUse(const int32 needed, const int32 coulduse) const;
		
		region*			EditSlow(const int32 needed, const int32 coulduse) const;
		region*			ReUseSlow(const int32 needed, const int32 coulduse) const;
		
		bool			CheckIntegrity() const;
		
		region			fBounds;
		const region*	fData;
};

BDataIO& operator<<(BDataIO& io, const IRegion& region);

// -------------------------------------------------------------------

// Backwards compatibilty with the old region API in the app_server.
// Do not use with new code.

typedef IRegion region;

region 				*newregion(void);
void				kill_region(region* reg);

inline void			init_regions(void)
	{ }
//void		set_size(region *a_region, long new_size);
inline void			copy_region(const region *src_region, region *dst_region)
	{ *dst_region = *src_region; }
inline void   		and_region(const region* r1, const region* r2, region* dst)
	{ r1->And(*r2, dst); }
inline void   		or_region(const region* r1, const region* r2, region* dst)
	{ r1->Or(*r2, dst); }
inline void   		sub_region(const region* r1, const region* r2, region* dst)
	{ r1->Sub(*r2, dst); }
inline void   		xor_region(const region* r1, const region* r2, region* dst)
	{ r1->XOr(*r2, dst); }
inline void			clear_region(region* reg)
	{ reg->MakeEmpty(); }
inline void			set_region(region* reg, clipping_rect* r)
	{ reg->Set(*r); }
inline void   		offset_region(region* reg, long dh, long dv)
	{ reg->OffsetBy(dh, dv); }
inline char   		point_in_region(const region* reg, long h, long v)
	{ return reg->Contains(h, v); }
inline char			rect_in_region(const region* reg, const clipping_rect * r)
	{ return reg->Intersects(*r); }
inline void			scale_region(	const region *src, clipping_rect &srcRect,
									region *dst, clipping_rect &dstRect,
									int32 offsX, int32 offsY, bool tileX, bool tileY)
	{ src->ScaleBy(dst, srcRect, dstRect, offsX, offsY, tileX, tileY); }
inline int			equal_region(const region *r1, const region *r2)
	{ return *r1 == *r2; }
inline void		compress_spans(region *spans)
	{ spans->CompressSpans(); }
inline int32		find_span_between(const region *spans, int32 top, int32 bottom)
	{ return spans->FindSpanBetween(top, bottom); }
inline void			add_rect(region *a_region, clipping_rect a_rect)
	{ a_region->AddRect(a_rect); }
inline void			or_rect(region *a_region, clipping_rect a_rect)
	{ a_region->OrRect(a_rect); }

#define		zero_region				clear_region
#define 	empty_region(reg)		((reg)->IsEmpty())
#define		region_is_rect(reg)		((reg)->IsRect())

}
using namespace BPrivate;

#endif
