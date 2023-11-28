#include <Debug.h>

#include "KalWindow.h"
#include "KalView.h"
#include "KalDraw.h"
#include "Kaleid.h"
#include <Screen.h>

KalWindow *window;
KalView *view;
KalDraw *drawThread;

Kaleidoscope::Kaleidoscope ()
		  	 :BApplication ("application/x-vnd.Be-KALI")
{
}

void Kaleidoscope::ReadyToRun ()
{
	BRect r;

	r = BScreen(B_MAIN_SCREEN_ID).Frame();
	r.InsetBy(8.0, 8.0);
	r.left -= 6.0;
	r.top += 28;
	window = new KalWindow(r);

	r = window->Bounds();
	view = new KalView(r, "Kaleidoscope");
	window->AddChild(view);

	drawThread = new KalDraw(view);
	window->fDrawThread = drawThread;
	drawThread->Run();
	drawThread->PostMessage((ulong)dStart);
}

bool Kaleidoscope::QuitRequested ()
{
//	drawThread->Quit();
	drawThread->PostMessage(B_QUIT_REQUESTED);

	return(TRUE);
}

int main ()
{
	Kaleidoscope *kaleidoscope;

	kaleidoscope = new Kaleidoscope();
	kaleidoscope->Run();

	delete(kaleidoscope);
	return 0;
}
