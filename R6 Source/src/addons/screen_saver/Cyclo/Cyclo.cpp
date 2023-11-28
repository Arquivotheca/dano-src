/*

Cyclo - a "Spirograph (tm)" implementation as a BeOS screen saver
Written by Christopher Tate, February 1999

Math:
	ring = ring radius
	disc = disc radius
	offset = pen offset from center of disc (need not be less than disc radius)

	Requires: ring and disc both integral, ring > disc

	Math:
		x = (ring + disc)*cos(theta) - (disc + offset)*cos(phi) 
		y = (ring + disc)*sin(theta) - (disc + offset)*sin(phi)

	where phi = ((ring + disc) / disc) * theta
	and theta is parameterized over [0, 2*pi*disc / gcd(ring, disc)]

	(theta = angle of deflection of disc's center; phi = rotation of disc)

*/

#include <ScreenSaver.h>
#include <StringView.h>
#include <stdlib.h>
#include <math.h>

// Screen saver class declaration

class Cyclo : public BScreenSaver
{
public:
				Cyclo(BMessage *message, image_id id);
	void		StartConfig(BView *view);
	status_t	StartSaver(BView *v, bool preview);
	void		Draw(BView *v, int32 frame);

private:
	float mRing, mDisc, mOffset;
	float mHue, mSaturation, mValue, mHueIncr;
	float mDiff, mTheta, mPhiMul, mDelta, mLimit, mIncr;
	bool mDoneDrawing;
};

// Factory function

extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message, image_id image)
{
	return new Cyclo(message, image);
}

// Utility functions

inline float min(float a, float b) { return (a < b) ? a : b; }
inline float abs(float n) { return (n < 0) ? -n : n; }

// Euclid's greatest-common-divisor algorithm, recursively

static long gcd(long a, long b)
{
	long r = a % b;
	return (r == 0) ? b : gcd(b, r);
}

// HSV to RGB: courtesy of Foley, van Dam, Feiner, & Hughes
//
// In this implementation, HSV are all in [0, 1]

static rgb_color hsv2rgb(float h, float s, float v)
{
	float r, g, b;

	if (s == 0)			// zero saturation = greyscale
	{
		r = g = b = v;
	}
	else
	{
		float f, p, q, t;
		int i;

		if (h == 1) h = 0;	// h now in [0, 1)
		h *= 6;				// scale h to [0, 6)
		i = (int) h;
		f = h - i;			// fractional part of h
		p = v * (1.0 - s);
		q = v * (1.0 - (s*f));
		t = v * (1.0 - (s*(1.0-f)));
		switch (i)
		{
		case 0: r = v; g = t; b = p; break;
		case 1: r = q; g = v; b = p; break;
		case 2: r = p; g = v; b = t; break;
		case 3: r = p; g = q; b = v; break;
		case 4: r = t; g = p; b = v; break;
		case 5: r = v; g = p; b = q; break;
		default: r = g = b = 0; break;			// can't happen; eliminates warning
		}
	}

	rgb_color c;
	c.red = int(r * 255.0);
	c.green = int(g * 255.0);
	c.blue = int(b * 255.0);
	return c;
}

// Actual saver implementation

Cyclo::Cyclo(BMessage *message, image_id image)
	: BScreenSaver(message, image)
{
}

void Cyclo::StartConfig(BView *view)
{
	view->AddChild(new BStringView(BRect(10, 10, 200, 35), B_EMPTY_STRING, "Cyclo, by Christopher Tate"));
	view->AddChild(new BStringView(BRect(10, 40, 200, 65), B_EMPTY_STRING, "Remember the little plastic gears?"));
}

status_t Cyclo::StartSaver(BView *view, bool /* preview */)
{
	srand(system_time() & 0xFFFF);
	view->SetHighColor(255,255,255);
	view->SetLowColor(0, 0, 0);

	BRect bounds = view->Bounds();
	view->ScrollBy(-bounds.Width() / 2, -bounds.Height() / 2);
	mDoneDrawing = true;
	return B_OK;
}

void Cyclo::Draw(BView *view, int32 /* frameNumber */ )
{
	// handle new images, including first frame
	if (mDoneDrawing)
	{
		BRect bounds = view->Bounds();
		float d = min(bounds.Width() / 2, bounds.Height() / 2);		// smallest view dim.

		// ring & disc radii must be integral to take their GCD
		// offset can be anything; try to keep the figure on-screen
		float minRing = floor(d/4);
		mRing = floor( (float)rand() / ((float)RAND_MAX + 1) * (d - minRing) ) + minRing;
		mDisc = floor( (float)rand() / ((float)RAND_MAX + 1) * (d - mRing - 5) ) + 5;
		mDiff = mRing - mDisc;
		mOffset = (float)rand() / ((float)RAND_MAX + 1) * (d - abs(mDiff) - 10) + 10;
		float aGCD = gcd((long) mRing, (long) mDisc);

		mPhiMul = (mDisc - mRing) / mDisc;
		mLimit = 2.0 * M_PI * mDisc / aGCD;

		// choose an angle increment based on the view's size; this was
		// empirically determined to be fine enough to avoid almost all
		// "jaggies" at full-screen and in the preferences panel
		mIncr = mLimit / (30 * d);

		// cycle hues in HSV space, with some random fading & dimming
		mHue = (float)rand() / (float)(RAND_MAX + 1);		// [0, 1)
		mHueIncr = mIncr / mLimit;
		if (rand() > RAND_MAX/2)		// half the time, run the colors the other way
		{
			mHueIncr = -mHueIncr;
		}
		mValue = ((float) rand() / (float) (2.0*RAND_MAX)) + 0.5;			// [0.5, 1]
		mSaturation = ((float)rand() / (float) RAND_MAX) * 0.80 + 0.2;		// [0.2, 1]

		// erase to black
		view->FillRect(view->Bounds(), B_SOLID_LOW);

		// begin drawing at the origin
		mTheta = 0;
		view->MovePenTo(mDiff + mOffset, 0);

		// set the delay interval then fall through; we're done
		SetTickSize(100);
		mDoneDrawing = false;
	}

	if (mTheta <= mLimit)
	{
		float phi = mPhiMul * mTheta;
		float x = mDiff * cos(mTheta) + mOffset * cos(phi);
		float y = mDiff * sin(mTheta) + mOffset * sin(phi);
		view->SetHighColor(hsv2rgb(mHue, mSaturation, mValue));
		view->StrokeLine(BPoint(x, y), B_SOLID_HIGH);
		mTheta += mIncr;
		mHue += mHueIncr;
		if (mHue > 1) mHue -= 1;
		if (mHue < 0) mHue += 1;
	}
	else			// waiting for an appropriate interval, then choose new parameters
	{
		SetTickSize(3000000);
		mDoneDrawing = true;
	}
}
