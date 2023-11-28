#ifndef _PROGRESSBAR_H_
#define _PROGRESSBAR_H_

#include <View.h>
#include <Bitmap.h>
#include <Rect.h>

// Draw swanky progress bars

class ProgressBar {
public:
					ProgressBar();
	
	virtual			~ProgressBar();
			void	Draw(BView *view, BRect rect, float fillFraction);
			BRect	ChangedRect(BRect theRect, float oldV, float newV);
			
private:
	BBitmap					*fStartBmap;
	BBitmap					*fMidBmap;
	BBitmap					*fEndBmap;
	
	static const int		DATA_HEIGHT;
	static const int		DATA_WIDTH;
	static const int		END_WIDTH;
	
	static const unsigned char start[];
	static const unsigned char middle[];
	static const unsigned char end[];	
};

#endif
