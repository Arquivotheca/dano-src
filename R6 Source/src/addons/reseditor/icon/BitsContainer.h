#ifndef BITS_CONTAINER_H
#define BITS_CONTAINER_H

#include <Debug.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Bitmap.h>
#include <View.h>

#include <experimental/BitmapTools.h>

class TBitsContainer : public BView {
public:
			TBitsContainer(float width, float height,
						   color_space cspace = B_CMAP8,
						   bool makeChild = true);
			~TBitsContainer();
			
			bool			SetAttributes(float new_width=-1, float new_height=-1,
										  color_space new_space=B_NO_COLOR_SPACE,
										  bool initialize=true,
										  bool dither=true);
			
			bool			HasChild() const;
			
			float			Width() const;
			float			Height() const;
			color_space 	ColorSpace() const;
			
			const BBitmap* 	Bitmap() const;
			BBitmap* 		Bitmap();
			
			const uint8* 	Bits() const;
			uint8*	 		Bits();
			int32 			BitsLength() const;
			
			bool			Lock();
			void			Unlock();
			
			void			Invalidate();
			
			// Drawing primitives.  Operate directly on the bitmap, so you
			// don't need an attached view.
			void			SetPixel(BPoint p, rgb_color color);
			rgb_color		GetPixel(BPoint p) const;
			rgb_color		ConstrainColor(rgb_color orig) const;
			BRect			DoFill(BPoint pt, rgb_color pen);

private:
			pixel_access fAccess;
			
			bool		fHasChild;
			float		fWidth, fHeight;
			color_space	fColorSpace;
			BBitmap*	fBitmap;
			uint8*		fBits;
			int32		fBitsLength;
			uint32		fBytesPerRow;
};

#endif
