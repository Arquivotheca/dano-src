#include "BitsContainer.h"
#include "utils.h"

#include <experimental/BitmapTools.h>

#include <vector>

// ---------------------------- TBitsContainer ----------------------------

TBitsContainer::TBitsContainer(float width, float height,
							   color_space cspace,
							   bool makeChild)
	: BView(BRect(0, 0, floor(width-1+.5), floor(height-1+.5)),
			"bits", B_FOLLOW_NONE, 0),
	  fAccess(cspace),
	  fHasChild(makeChild),
	  fWidth(32), fHeight(32), fColorSpace(B_CMAP8),
	  fBitmap(0),
	  fBits(0), fBitsLength(0), fBytesPerRow(0)
{
	SetAttributes(width, height, cspace);
}


TBitsContainer::~TBitsContainer()
{
	if( fBitmap && fHasChild )
		fBitmap->RemoveChild(this);

	delete fBitmap;
	fBitmap = 0;
}

bool TBitsContainer::SetAttributes(float new_width, float new_height,
								   color_space new_space,
								   bool initialize, bool dither)
{
	bool changed = false;
	
	if( new_width > 0 && floor(new_width+.5) != fWidth ) {
		fWidth = floor(new_width+.5);
		changed = true;
	}
	if( new_height > 0 && floor(new_height+.5) != fHeight ) {
		fHeight = floor(new_height+.5);
		changed = true;
	}
	if( new_space != B_NO_COLOR_SPACE && new_space != fColorSpace ) {
		if( fAccess.set_to(new_space) == B_OK ) {
			fColorSpace = new_space;
			changed = true;
		}
	}
	
	if( changed || !fBitmap ) {
		if( fBitmap && fHasChild ) fBitmap->RemoveChild(this);
		
		BBitmap* new_bm = new BBitmap(BRect(0, 0, fWidth-1, fHeight-1),
									  fColorSpace, fHasChild);
		
		MoveTo(0, 0);
		ResizeTo(fWidth-1, fHeight-1);
		
		if( fHasChild ) new_bm->AddChild(this);
		
		if( initialize ) {
			// if color space has changed but not bounds, set old bitmap
			// into new so that dithering is performed; otherwise, draw it
			// in which will not allow us to dither.
			if( !fBitmap || new_bm->Bounds() != fBitmap->Bounds() ||
					set_bitmap(new_bm, fBitmap, dither) != B_OK ) {
				if( fHasChild && new_bm->Lock() ) {
					PushState();
					SetDrawingMode(B_OP_COPY);
					SetHighColor(B_TRANSPARENT_COLOR);
					FillRect(Bounds());
					if( fBitmap ) {
						if( new_bm->ColorSpace() == B_CMAP8 ) {
							// This is needed to retain transparency information
							// when copying into a B_CMAP8 color space.  The others
							// work better without it.
							SetDrawingMode(B_OP_ALPHA);
							SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
						}
						DrawBitmap(fBitmap, BPoint(0, 0));
					}
					PopState();
					Sync();
					new_bm->Unlock();
				}
			}
		}
		
		delete fBitmap;
		fBitmap = new_bm;
		fBits = (uint8*)fBitmap->Bits();
		fBitsLength = fBitmap->BitsLength();
		fBytesPerRow = fBitmap->BytesPerRow();
	}
	
	return changed;
}

bool
TBitsContainer::HasChild() const
{
	return fHasChild;
}

float
TBitsContainer::Width() const
{
	return fWidth;
}

float
TBitsContainer::Height() const
{
	return fHeight;
}

color_space
TBitsContainer::ColorSpace() const
{
	return fColorSpace;
}

const BBitmap*
TBitsContainer::Bitmap() const
{
	return fBitmap;
}

BBitmap*
TBitsContainer::Bitmap()
{
	return fBitmap;
}

const uint8*
TBitsContainer::Bits() const
{
	return fBits;
}

uint8*
TBitsContainer::Bits()
{
	return fBits;
}

int32 
TBitsContainer::BitsLength() const
{
	return fBitsLength;
}

bool 
TBitsContainer::Lock()
{
	return fBitmap->Lock();
}

void 
TBitsContainer::Unlock()
{
	fBitmap->Unlock();
}

void 
TBitsContainer::Invalidate()
{
	if (!HasChild())
		return;
	
	if (fBitmap->Lock()) {
		Invalidate();
		fBitmap->Unlock();
	}
}

// ---------------------------- Drawing Primitives ----------------------------

void
TBitsContainer::SetPixel(BPoint p, rgb_color color)
{
	fAccess.write(fBits + size_t(fBytesPerRow*p.y) + size_t(fAccess.bpp()*p.x),
				  color);
}

rgb_color
TBitsContainer::GetPixel(BPoint p) const
{
	return fAccess.read(fBits + size_t(fBytesPerRow*p.y) + size_t(fAccess.bpp()*p.x));
}

rgb_color
TBitsContainer::ConstrainColor(rgb_color orig) const
{
	uint8 buffer[8];
	const_cast<TBitsContainer*>(this)->fAccess.write(buffer, orig);
	return fAccess.read(buffer);
}

namespace BPrivate {

	struct fill_segment
	{
		fill_segment()
			: y(0), xl(0), xr(0), dy(0)
		{
		}
		
		fill_segment(float in_y, float in_xl, float in_xr, float in_dy)
			: y(in_y), xl(in_xl), xr(in_xr), dy(in_dy)
		{
		}
		
		float y;
		float xl, xr;
		float dy;
	};
	
	static inline void push(std::vector<fill_segment>& stack,
							const fill_segment& seg, const BRect& bounds)
	{
		if( seg.y+seg.dy >= bounds.top && seg.y+seg.dy <= bounds.bottom ) {
			stack.push_back(seg);
		}
	}
	
	static inline fill_segment pop(std::vector<fill_segment>& stack)
	{
		fill_segment seg = stack.back();
		stack.pop_back();
		seg.y += seg.dy;
		return seg;
	}
}	// namespace BPrivate

using namespace BPrivate;

BRect
TBitsContainer::DoFill(BPoint pt, rgb_color pen)
{
	BRect region;
	
	// Area fill algorithm based on seed fill algorithm by
	// Paul S Heckbert, found on ppg 275 of Graphics Gems
	// volume 1, ed. Andrew S. Glassner ISBN 0-12-286165-5
	
	std::vector<fill_segment> stack;
	const BRect bounds = Bounds();
	if( !bounds.Contains(pt) ) return region;
	
	rgb_color ov = GetPixel(pt);
	pen = ConstrainColor(pen);
	PRINT(("Filling: overpixel (r=%d,g=%d,b=%d,a=%d) with pen (r=%d,g=%d,b=%d,a=%d)\n",
			ov.red, ov.green, ov.blue, ov.alpha,
			pen.red, pen.green, pen.blue, pen.alpha));
	if( ov == pen ) return region;
	
	push(stack, fill_segment(pt.y, pt.x, pt.x, 1), bounds);
	push(stack, fill_segment(pt.y+1, pt.x, pt.x, -1), bounds);
	
	while( stack.size() > 0 ) {
		// pop segment off stack and fill a neighboring scan line.
		
		fill_segment segment = pop(stack);
		pt.x = segment.xl;
		pt.y = segment.y;
		
		// segment of scan line y-dy for segment.xr <= pt.x <= segment.xr
		// was previously filled, no explore adjacent pixels in scan line y.
		
		while( pt.x >= bounds.left && GetPixel(pt) == ov ) {
			SetPixel(pt, pen);
			if( !region.Contains(pt) ) {
				region = ExtendRectPoint(region, pt);
			}
			pt.x -= 1;
		}
		
		float start = 0;
		if( pt.x >= segment.xl ) goto skip;
		
		start = pt.x+1;
		if( start < segment.xl ) {
			// leak on left?
			push(stack, fill_segment(pt.y, start, segment.xl-1, -segment.dy), bounds);
		}
		pt.x = segment.xl + 1;
		
		do {
		
			while( pt.x <= bounds.right && GetPixel(pt) == ov ) {
				SetPixel(pt, pen);
				if( !region.Contains(pt) ) {
					region = ExtendRectPoint(region, pt);
				}
				pt.x += 1;
			}
			
			push(stack, fill_segment(pt.y, start, pt.x-1, segment.dy), bounds);
			
			if( pt.x > segment.xr+1 ) {
				// leak on right?
				push(stack, fill_segment(pt.y, segment.xr+1, pt.x-1, -segment.dy), bounds);
			}
			
		skip:
			pt.x += 1;
			while( pt.x <= segment.xr && GetPixel(pt) != ov ) {
				pt.x += 1;
			}
			start = pt.x;
			
		} while( pt.x <= segment.xr );
	}
	
	return region;
}
