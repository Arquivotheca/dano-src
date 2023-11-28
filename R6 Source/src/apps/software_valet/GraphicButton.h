#ifndef _GRAPHICBUTTON_H_
#define _GRAPHICBUTTON_H_

#include <PictureButton.h>
#include <Bitmap.h>
#include <View.h>
#include <Picture.h>

// GraphicButton.h

class BufPlay;

class GraphicButton : public BPictureButton
{
public:
				GraphicButton(BRect frame,
							  const char *name,
							  const char *label,
							  BMessage *msg,
							  BBitmap *bmapOff,
							  BBitmap *bmapPressed = NULL,
							  ulong resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP);
							  
virtual				~GraphicButton();

virtual void		AttachedToWindow();
virtual void		MouseDown(BPoint where);

static	void		Init(BView *);
		void		Free();

static	BPicture	*dButtonPicture;

inline		BBitmap		*OffBitmap();
inline		BBitmap		*OnBitmap();
inline		void		SetOffBitmap(BBitmap	*bmap);
inline		void		SetOnBitmap(BBitmap	*bmap);


protected:

	void			DrawBackground(BView *v);
	void			DrawGraphic(BView *v,BBitmap *bmapw);
	void			DrawText(BView *v);

private:
		void 		SetupPicture(BView *v, BBitmap *bitmap, BPicture **result);
	static bool		picsInited;

	BBitmap			*offBitmap;
	BBitmap			*onBitmap;
	
	bool			madeOnBitmap;
	
	BufPlay			*player;
	// BufPlay		*playerUp;
};


inline BBitmap	*GraphicButton::OffBitmap()
{
	return offBitmap;
}

inline BBitmap	*GraphicButton::OnBitmap()
{
	return onBitmap;
}

inline void		GraphicButton::SetOffBitmap(BBitmap	*bmap)
{
	offBitmap = bmap;
}

inline void		GraphicButton::SetOnBitmap(BBitmap *bmap)
{
	onBitmap = bmap;
}

#endif

