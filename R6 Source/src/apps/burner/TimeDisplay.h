#ifndef __TIME_DISPLAY__
#define __TIME_DISPLAY__

#include <View.h>

class Bitmap;

class TimeDisplay : public BView {
public:
	TimeDisplay(BRect frame, const char *name, int16 largestSegment,
		int16 smallestSegment, bool displaySign, uint32 resizeMask = B_FOLLOW_LEFT | B_FOLLOW_TOP);
	virtual ~TimeDisplay();
	void SetValue(int32 value);

	enum {
		TIME_HOURS		= 4,
		TIME_MINUTES	= 3,
		TIME_SECONDS	= 2,
		TIME_FRAMES		= 1
	};

protected:
	virtual void Draw(BRect);
	float DrawDigit(float offset, uint32 digitIndex, BRect digitSize,
		const BBitmap *map);

	void SetHours(int32 newHours);
	void SetMinutes(int32 newMinutes);
	void SetSeconds(int32 newSeconds);
	void SetFrames(int32 newFrame);
	void SetSign(int32 value);
	void InvalidateDigit(int32 prevDigits, int32 prevColons);
private:
	
	int16 fLargestShown;
	int16 fSmallestShown;
	int32 fValue;			// number of frames in value to display
	int32 fHour;
	int32 fMinute;
	int32 fSecond;
	int32 fFrame;
	BBitmap *fSegments;
	bool fDisplaySign;
};

#endif // __TIME_DISPLAY__
