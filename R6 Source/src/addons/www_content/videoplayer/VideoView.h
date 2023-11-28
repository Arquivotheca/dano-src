#ifndef _videoview_H_
#define _videoview_H_

#include <View.h>
#include <Bitmap.h>

class VideoView : public BView
{
	public:
						VideoView(BRect frame,
					 			  const char *name,
								  uint32 resizeMask,
								  uint32 flags,
								  BBitmap * bitmap);
		void 			Draw(BRect UpdateRect);
		void 			SetBitmap(BBitmap * bitmap);
	private:
		BBitmap *		m_bitmap;
};

#endif
