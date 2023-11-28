#include "VideoView.h"

VideoView::VideoView(BRect frame,
					 const char *name,
					 uint32 resizeMask,
					 uint32 flags,
					 BBitmap * bitmap):
	BView(frame,name,resizeMask,flags),
	m_bitmap(bitmap) 
{
}

void VideoView::Draw(BRect UpdateRect)
{
	if(m_bitmap)
	{
		DrawBitmap(m_bitmap,Bounds());
	}
}

void VideoView::SetBitmap(BBitmap * bitmap)
{
	m_bitmap = bitmap;
}


