#include <Debug.h>

#include "KalView.h"
#include "KalDraw.h"
#include "Kaleid.h"
#include "KalWindow.h"

extern KalDraw *drawThread;

KalWindow::KalWindow (BRect rect)
		  :BWindow(rect, "Kaleidoscope", B_TITLED_WINDOW, 0)
{
	fDrawThread = NULL;
	Run();
}

void KalWindow::FrameResized (float width, float height)
{
	fDrawThread->PostMessage(dChangeSize);
	atomic_add(&fDrawThread->pending_message_count, 1);
}

void KalWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
//		case dQuit:
//			be_app->PostMessage(B_QUIT_REQUESTED);
//			Quit();
//			break;
		default:
			BWindow::MessageReceived(msg);
			break;
		}
}

bool KalWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(FALSE);
}
