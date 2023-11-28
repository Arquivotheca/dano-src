#ifndef _VIEW_H
#include <View.h>
#endif
#include <Bitmap.h>

class TWaveView : public BView {
public:
 	 				TWaveView (BPoint where, BView *parent);
  					~TWaveView();
virtual  	void	Draw(BRect r);
virtual		void	Pulse();
			void	SetSample(long val);

private:
			float	k;					//debuging stuff
			short	values[128];
			BBitmap	*bitview;
			void	clear_bitmap();
			void	init_wave(float v);
			void	ImageWave();
			void	UpdateWave();

};
