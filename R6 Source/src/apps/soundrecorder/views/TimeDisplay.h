#ifndef __TIME_DISPLAY__
#define __TIME_DISPLAY__

#include <View.h>

class Bitmap;

class TimeDisplay : public BView {
public:
	TimeDisplay(BRect frame, const char *name, bool hours, bool minutes,
		bool seconds, bool doFrame,
		uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP);
	
	void SetHours(int32);
	void SetMinutes(int32);
	void SetSeconds(int32);
	void SetFrame(int32);

protected:
	virtual void Draw(BRect);
	float DrawDigit(float offset, uint32 digitIndex, BRect digitSize,
		const BBitmap *map);

private:

	bool doHours;
	bool doMinutes;
	bool doSeconds;
	bool doFrame;

	int32 hours;
	int32 minutes;
	int32 seconds;
	int32 frame;

	BBitmap *segments;
};

#endif
