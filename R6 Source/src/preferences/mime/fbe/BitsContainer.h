#include <Debug.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Bitmap.h>
#include <View.h>

class TBitsContainer : public BView {
public:
			TBitsContainer(BRect frame, uchar *bits = NULL,
				color_space cspace = B_COLOR_8_BIT, bool makeChild = true);
			~TBitsContainer();
			
			bool		HasChild();
			color_space ColorSpace();
			
			BBitmap* 	Bitmap();
			uchar*	 	Bits();
			int32 		BitsLength();
			
			void 		NewBitmap(BBitmap *b);
			void		SetBits(uchar*, BRect);
			void		ResizeContainer(BRect);
			
			bool		Lock();
			void		Unlock();
			
			void		Invalidate();
private:
			bool		fHasChild;
			color_space	fColorSpace;
			BBitmap*	fBitmap;
};
