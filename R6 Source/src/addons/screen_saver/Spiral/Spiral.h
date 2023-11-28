#include <ScreenSaver.h>

class Spiral : public BScreenSaver
{
	double camRevolutionPos;
	double camRotation;
	double wiggle;

	double maxRadius;
	double camRadius;
	double ringRadius;
	bool camIsOutsideRing;
	double slip;
	double rotationalIncrement;
	double wiggleSpeed;
	double wiggleRadius;
	int firstLine;

	double red;
	double green;
	double blue;
	double destRed;
	double destGreen;
	double destBlue;

	BPoint	center;

public:
				Spiral(BMessage *message, image_id id);
	void		StartConfig(BView *view);
	status_t	StartSaver(BView *v, bool preview);
	void		Draw(BView *v, int32 frame);
};
