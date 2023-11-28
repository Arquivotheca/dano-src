#ifndef _SPLASHWINDOW_H_
#define _SPLASHWINDOW_H_

#include <Window.h>
#include <View.h>
#include <Point.h>
#include <Bitmap.h>

class PackWindow;

class SplashWindow : public BWindow
{
public:
	SplashWindow(BBitmap *);			
};



class SplashView : public BView
{
public:
					SplashView(BRect b,BBitmap *);
	
	virtual			~SplashView();
	virtual void	Draw(BRect up);
	virtual void	MouseDown(BPoint);
//	virtual void	Pulse();
private:
	BBitmap 		*splashBitmap;
	bigtime_t		startTime;
};

#endif
