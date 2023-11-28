// SplashView.cpp
#include <Messenger.h>

#include "SplashWindow.h"

#include "Util.h"
#include "MyDebug.h"

int32	timeout_thread(void *);
int32	timeout_thread(void *d)
{
	BMessenger	*msngr = (BMessenger *)d;
	snooze(3*1000*1000);
	msngr->SendMessage(B_QUIT_REQUESTED);
	delete msngr;
	return 0;
}

SplashWindow::SplashWindow(BBitmap *bmap)
	:	BWindow(BRect(0,0,100,100),
				B_EMPTY_STRING,
				B_MODAL_WINDOW,
				B_NOT_RESIZABLE | B_NOT_MOVABLE)
{
	Lock();

	if (bmap) {
		ResizeTo(bmap->Bounds().Width(),bmap->Bounds().Height());
		PositionWindow(this,0.5,0.4);
	}
	AddChild(new SplashView(Bounds(),bmap));
	Show();
	thread_id id = spawn_thread(timeout_thread,B_EMPTY_STRING,B_NORMAL_PRIORITY,new BMessenger(this));
	resume_thread(id);
	Unlock();
}


/*********************************************************/
void 	SwapComponents(ulong *data, long size);

SplashView::SplashView(BRect r,BBitmap *_splash)
	:	BView(r,B_EMPTY_STRING,B_FOLLOW_ALL,B_WILL_DRAW),
		splashBitmap(_splash)
{
	//if (splashBitmap)
	//	SwapComponents((ulong *)splashBitmap->Bits(),splashBitmap->BitsLength());	
	startTime = system_time();
	PRINT(("got system time\n"));
}

SplashView::~SplashView()
{
}

void SplashView::Draw(BRect up)
{
	if (splashBitmap) {
		DrawBitmap(splashBitmap,up,up);	
	}
}

void SplashView::MouseDown(BPoint where)
{
	BPoint loc = where;
	ulong buttons;
	do {
		snooze(20*1000);
		GetMouse(&loc, &buttons);
	} while(buttons);
	if (Bounds().Contains(loc)) {
		Looper()->PostMessage(B_QUIT_REQUESTED);
	}
}

void 	SwapComponents(ulong *data, long size)
{
	ulong	*p, *max;
	ulong	pixel;	
	long	siz = size/sizeof(pixel);
	for (p = data, max = data+siz; p < max; ++p) {
		pixel = *p;
		pixel = ((0x00FFFFFF & pixel) << 8) |
				((0xFF000000 & pixel) >> 24);
		*p = pixel;		
	}
}
