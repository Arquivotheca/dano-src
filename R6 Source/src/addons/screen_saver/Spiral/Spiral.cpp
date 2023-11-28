#include "Spiral.h"
#include <StringView.h>
#include <stdlib.h>
#include <time.h>

const double revolutionIncrement = .01;

inline BPoint 
Rotate(const BPoint &centerPoint, double rotation, double radius)
{
	return BPoint(centerPoint.x + cos(rotation) * radius, 
		centerPoint.y + sin(rotation) * radius);
}

inline double
frand()
{
	return (float) rand() / (float) RAND_MAX;
}

// MAIN INSTANTIATION FUNCTION
extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message, image_id image)
{
	return new Spiral(message, image);
}

Spiral::Spiral(BMessage *message, image_id image)
 : BScreenSaver(message, image)
{
}

void Spiral::StartConfig(BView *view)
{
	view->AddChild(new BStringView(BRect(10, 10, 200, 35), B_EMPTY_STRING, 
		"Spiral, Jeff Bush & Ficus Kirkpatrick"));
}

status_t Spiral::StartSaver(BView *, bool /* preview */)
{
	SetTickSize(400);
	srand(time(0));
	SetLoop(5000, 2500);	/* 'frame' will cycle 5000 then idle 2500 */
	return B_OK;
}

void Spiral::Draw(BView *view, int32 frame)
{
	if(frame == 0)
	{
		double halfWidth 	= view->Bounds().Width() / 2;
		double halfHeight 	= view->Bounds().Height() / 2;
		view->FillRect(view->Bounds(), B_SOLID_LOW);	// Erase

		camRevolutionPos	= 0.0;
		camRotation 		= 0.0;
		wiggle 				= 0.0;

		maxRadius			= min_c(halfWidth, halfHeight) * .75;
		camRadius 			= frand() * maxRadius + 0.01;
		ringRadius 			= frand() * maxRadius + 0.01;
		camIsOutsideRing 	= rand() % 1;
		slip 				= frand() / 5;
		rotationalIncrement = (camRadius / ringRadius 
								* revolutionIncrement + slip) * 
								(camIsOutsideRing ? 1 : -1);
		wiggleSpeed 		= frand();
		wiggleRadius		= frand() * 50;
		firstLine 			= true;

		red 				= frand();
		green 				= frand();
		blue 				= frand();
		destRed 			= frand();
		destGreen 			= frand();
		destBlue 			= frand();

		center.Set(halfWidth, halfHeight);
	}

	BPoint pen(Rotate(Rotate(Rotate(center, wiggle, wiggleRadius), 
		camRevolutionPos, camRadius), camRotation, ringRadius));

	camRevolutionPos 	+= revolutionIncrement;
	camRotation 		+= rotationalIncrement;
	wiggle 				+= wiggleSpeed;
	red 				+= (destRed - red) / (5000 - frame);
	green 				+= (destGreen - green) / (5000 - frame);
	blue 				+= (destBlue - blue) / (5000 - frame);

	if (firstLine) {
		view->MovePenTo(pen);
		firstLine = false;
	}

	view->SetHighColor((int) (red * 255.0), (int) (green * 255.0), 
		(int) (blue * 255.0));

	view->StrokeLine(pen);	
}
