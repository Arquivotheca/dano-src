/*

Lissart - fun with Lissajous figures
Written by Christopher Tate, March 1999

*/

// the paths here mean we don't have to add to the system include paths
#include <add-ons/screen_saver/ScreenSaver.h>
#include <interface/StringView.h>
#include <interface/TextView.h>

#include <stdlib.h>
#include <math.h>

// Screen saver class declaration

class Lissart : public BScreenSaver
{
public:
	Lissart(BMessage *message, image_id id);
	void StartConfig(BView *view);
	status_t StartSaver(BView *v, bool preview);
	void Draw(BView *v, int32 frame);

private:
	double mFac1X, mFac1Y, mFac2X, mFac2Y;
	double mRad1, mRad2;
	double mTheta, mLimit, mIncr;
	bool mDoneDrawing;
};

// useful type
struct factor_pair { double fac1, fac2; };

// Factory function

extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message, image_id image)
{
	return new Lissart(message, image);
}

// generic min() function
inline double min(double a, double b) { return (a < b) ? a : b; }

// double-precision U[0,1) distribution
inline double drand() { return double(rand()) / double(RAND_MAX); }

// offset a BRect using a BPoint as the delta
inline BRect operator + (const BRect& rect, const BPoint& pt)
	{ return BRect(rect.left + pt.x, rect.top + pt.y, rect.right + pt.x, rect.bottom + pt.y); }

// swap two doubles in place
static void swap(double& a, double& b) { double tmp = a; a = b; b = tmp; }

// Euclid's greatest-common-divisor algorithm, recursively
static long gcd(long a, long b)
{
	if (!a) return b;
	if (!b) return a;
	long r = a % b;
	return (r == 0) ? b : gcd(b, r);
}

// HSV to RGB: courtesy of Foley, van Dam, Feiner, & Hughes
//
// In this implementation, HSV are all in [0, 1]  (unlike the implementation is Foley et alia)

static rgb_color hsv2rgb(double h, double s, double v)
{
	double r, g, b;

	if (s == 0)			// zero saturation = greyscale
	{
		r = g = b = v;
	}
	else
	{
		double f, p, q, t;
		int i;

		if (h == 1) h = 0;	// h now in [0, 1)
		h *= 6.0;				// scale h to [0, 6)
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
	c.alpha = 255;
	return c;
}

// Actual saver implementation

Lissart::Lissart(BMessage *message, image_id image)
	: BScreenSaver(message, image)
{
}

void Lissart::StartConfig(BView *view)
{
	BRect r(10, 10, view->Bounds().right - 10, view->Bounds().bottom - 10);
	BTextView* tv = new BTextView(r, "", r.operator+(BPoint(-r.left, -r.top)),
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	tv->MakeEditable(false);
	tv->MakeSelectable(false);
	tv->SetViewColor(view->ViewColor());
	tv->SetText("Lissart, by Christopher Tate\n\nOriginally written in BASIC, on an "
		"HP \"calculator,\" drawing on a plotter, in 1981.");
	view->AddChild(tv);
}

status_t Lissart::StartSaver(BView *view, bool /* preview */)
{
	BRect bounds = view->Bounds();
	view->ScrollBy(-bounds.Width() / 2, -bounds.Height() / 2);
	view->SetLowColor(0, 0, 0);
	mDoneDrawing = true;
	srand(system_time() & RAND_MAX);
	return B_OK;
}

void Lissart::Draw(BView *view, int32 /* frameNumber */ )
{
	// handle new images, including first frame
	if (mDoneDrawing)
	{
		static const factor_pair factors[] = {
			{1, 1}, {1, 1}, {2, 2}, {1, 2}, {2, 1}, {1, 2}, {3, 1}, {1, 3}, {3, 2}, {2, 3},
			{3, 1}, {1, 3}, {3, 2}, {2, 3}, {3, 3}, {4, 1}, {1, 4}, {4, 3}, {3, 4}, {4, 4},
			{5, 1}, {1, 5}, {5, 2}, {2, 5}, {5, 3}, {3, 5}, {5, 4}, {4, 5} };

		BRect bounds = view->Bounds();
		double min_dim = min(bounds.Width() / 2, bounds.Height() / 2);

		mRad1 = min_dim;
		mRad2 = drand() * (0.8 * min_dim) + (0.2 * min_dim);

		if (rand() < RAND_MAX / 2) mRad2 = -mRad2;
		if (rand() < RAND_MAX / 2) swap(mRad1, mRad2);

		int fig = int( drand() * sizeof(factors) / sizeof(factors[0]) );
		mFac1X = factors[fig].fac1;
		mFac1Y = factors[fig].fac2;

		fig = int( drand() * sizeof(factors) / sizeof(factors[0]) );
		mFac2X = factors[fig].fac1;
		mFac2Y = factors[fig].fac2;

		// use angles relatively prime to 360 to maximize line density; give some particularly interesting
		// ones a higher chance of being selected by duplicating their entries in this array
		static const long angles[] = { 1, 7, 11, 13, 17, 19, 19, 19, 23, 29, 31, 31, 37, 41, 41,
			43, 47, 49, 53, 59, 61, 61, 61, 67, 71, 73, 77, 79, 83, 89, 91, 97, 101, 103, 107,
			109, 113, 119, 121, 127, 131, 133, 137, 139, 143, 149 };
		int i = int( drand() * sizeof(angles) / sizeof(angles[0]) );
		mIncr = M_PI / 180.0 * angles[i];		// convert to radians

		long facCommon = gcd(gcd(long(mFac1X), long(mFac1Y)), gcd(long(mFac2X), long(mFac2Y)));
		mLimit = 2 * M_PI * angles[i] / facCommon + mIncr;
		view->FillRect(view->Bounds(), B_SOLID_LOW);

		// some negative factors spice things up (do this AFTER calculating the gcd, please!)
		if (rand() < RAND_MAX / 2) mFac1Y = -mFac1Y;
		if (rand() < RAND_MAX / 2) mFac2Y = -mFac2Y;

		// pick a random color (in HSV space, because it's more fun :)
		double hue = drand();									// [0, 1)
		double value = drand() * 0.6 + 0.4;			// [0.4, 1]
		double saturation = drand() * 0.7 + 0.3;	// [0.3, 1]

		view->SetHighColor(hsv2rgb(hue, saturation, value));

		mTheta = 0;
		view->MovePenTo(0, mRad1);
		SetTickSize(10000);
		mDoneDrawing = false;
	}

	if (mTheta < mLimit)
	{
		float x1 = float(mRad1 * cos(mFac1X * mTheta));
		float y1 = float(mRad2 * sin(mFac1Y * mTheta));
		float x2 = float(mRad2 * cos(mFac2X * mTheta));
		float y2 = float(mRad1 * sin(mFac2Y * mTheta));
		mTheta += mIncr;

		view->StrokeLine(BPoint(-y2, x2), B_SOLID_HIGH);		// rotate & invert for aesthetic effect
		view->StrokeLine(BPoint(-y1, x1), B_SOLID_HIGH);
	}
	else			// waiting for an appropriate interval, then choose new parameters
	{
		SetTickSize(3500000);
		mDoneDrawing = true;
	}
}
