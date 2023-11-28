#include <Application.h>
#include <Window.h>
#include <View.h>
#include <Screen.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

inline double
frand()
{
	return (float) rand() / (float) RAND_MAX;
}

inline BPoint 
Rotate(const BPoint &centerPoint, double rotation, double radius)
{
	return BPoint(centerPoint.x + cos(rotation) * radius, 
		centerPoint.y + sin(rotation) * radius);
}

const double revolutionIncrement = .01;

static int32 
DrawThread(void *castToView)
{
	BView *view = (BView*) castToView;
		
	while (true) {
		double camRevolutionPos	= 0.0;
		double camRotation 		= 0.0;
		double wiggle 			= 0.0;

		view->Window()->Lock();
		double halfWidth = view->Bounds().Width() / 2;
		double halfHeight = view->Bounds().Height() / 2;
		view->FillRect(view->Bounds(), B_SOLID_LOW);	// Erase
		view->Window()->Unlock();

		double maxRadius			= min_c(halfWidth, halfHeight) * .75;
		double camRadius 			= frand() * maxRadius;
		double ringRadius 			= frand() * maxRadius;
		bool camIsOutsideRing 		= rand() % 1;
		double slip 				= frand() / 5;
		double rotationalIncrement 	= (camRadius / ringRadius 
										* revolutionIncrement + slip) * 
										(camIsOutsideRing ? 1 : -1);
		double wiggleSpeed 			= frand();
		double wiggleRadius			= frand() * 50;
		int firstLine 				= true;

		double red 					= frand();
		double green 				= frand();
		double blue 					= frand();
		double destRed 				= frand();
		double destGreen 			= frand();
		double destBlue 				= frand();
	
		BPoint center(halfWidth, halfHeight);
		for (int numLines = 5000; numLines > 0; numLines--) {
			BPoint pen(Rotate(Rotate(Rotate(center, wiggle, wiggleRadius), 
				camRevolutionPos, camRadius), camRotation, ringRadius));

			camRevolutionPos += revolutionIncrement;
			camRotation += rotationalIncrement;
			wiggle += wiggleSpeed;
			red += (destRed - red) / numLines;
			green += (destGreen - green) / numLines;
			blue += (destBlue - blue) / numLines;

			if (!view->Window()->Lock()) 
				return 0;

			if (firstLine) {
				view->MovePenTo(pen);
				firstLine = false;
			}

			view->SetHighColor((int) (red * 255.0), (int) (green * 255.0), 
				(int) (blue * 255.0));

			view->StrokeLine(pen);	
			view->Window()->Unlock();

			snooze(200);
		}

		snooze(500000);
	}
			
	return 0;
}

class SpiralWindow : public BWindow {
public:

	SpiralWindow()
		: 	BWindow(BScreen().Frame().InsetByCopy(50, 50), "Spiral", 
				B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 0)
	{
		BView *view = new BView(Bounds(), "SpiralView", B_FOLLOW_ALL, 
			B_WILL_DRAW);
		view->SetViewColor(0, 0, 0);
		view->SetLowColor(0, 0, 0);
		view->SetHighColor(255, 255, 255);
		AddChild(view);
		fDrawThread = spawn_thread(DrawThread, "Draw Thread", 
			B_NORMAL_PRIORITY, view);
		resume_thread(fDrawThread);
	}

	virtual bool QuitRequested() {
		int32 dummy;
		kill_thread(fDrawThread);
		wait_for_thread(fDrawThread, &dummy);
		be_app->PostMessage(B_QUIT_REQUESTED);
		return true;
	}

private:

	thread_id fDrawThread;
};

int 
main( int argc, char **argv )
{
	BApplication app("application/x-vnd.Be.Spiral");
	srand(time(0));
	(new SpiralWindow)->Show();
	app.Run();
	return 0;
} 
