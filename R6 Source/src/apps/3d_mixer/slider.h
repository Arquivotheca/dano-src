#ifndef SLIDER_H
#define SLIDER_H

#ifndef _BITMAP_H
#include <Bitmap.h>
#endif
#ifndef _CHECK_BOX_H
#include <CheckBox.h>
#endif
#ifndef _VIEW_H
#include <View.h>
#endif

#define THUMB_WIDTH (24 - 1)
#define THUMB_HEIGHT (28 - 1)

#define	S_VS	140
#define	S_HS	40

class TSlider : public BView {
public:
  				TSlider(BRect r, char* name, float value);
  				~TSlider();

  		void	Draw(BRect);
  		void	SetValue (float);
  		void	xSetValue (float);
  		float	Value();
  		void	MouseDown(BPoint);
		void	DrawBack(BView *v);
		void	DoComposit();

  		BBitmap	*back;
  		BBitmap *composit;
		BView	*bv;

		float	max;
		float	val;
		float	dt;
};

#endif
