// Foam
//
// Fractal bubbles screensaver

#include <ScreenSaver.h>
#include <StringView.h>
#include <TextView.h>

#include <stdlib.h>
#include <math.h>

class Foam : public BScreenSaver
{
public:
	Foam(BMessage *message, image_id id);

	void StartConfig(BView *view);
	status_t StartSaver(BView *v, bool preview);
	void StopSaver();
	void Draw(BView *v, int32 frame);

	struct Circle
	{
		double x, y;			// center of circle
		double rad, rad2;	// radius and radius^2 [speed optimization]
	};

private:
	Circle* mBubbles;
	size_t mBubblesDrawn;
	size_t mMaxBubbles;
	bool mDoneDrawing;
};

// double-precision helper functions
inline double min(double a, double b) { return (a < b) ? a : b; }
inline double abs(double n) { return (n < 0) ? -n : n; }

// double-precision U[0,1) distribution
inline double drand() { return double(rand()) / double(RAND_MAX); }

// square of distance between a point and a circle's center
static double distance2(double x, double y, const Foam::Circle& circle)
{
	return (x - circle.x)*(x - circle.x) + (y - circle.y)*(y - circle.y);
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

//#pragma mark -
// Factory function
extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message, image_id image)
{
	return new Foam(message, image);
}

// Saver implementation
Foam::Foam(BMessage *message, image_id id)
	: BScreenSaver(message, id)
{
}

void 
Foam::StartConfig(BView *view)
{
	BRect r(10, 10, view->Bounds().right - 10, view->Bounds().bottom - 10);
	BTextView* tv = new BTextView(r, "", r + BPoint(-r.left, -r.top),
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	tv->MakeEditable(false);
	tv->MakeSelectable(false);
	tv->SetViewColor(view->ViewColor());
	tv->SetText("Foam, by Christopher Tate\n\nFractal bubbles, just touching but never overlapping...");
	view->AddChild(tv);
}

status_t 
Foam::StartSaver(BView *v, bool /*preview*/)
{
	v->SetLowColor(0, 0, 0);
	mMaxBubbles = size_t( (v->Bounds().Width() * v->Bounds().Height()) / 40 );
	mBubbles = new Circle[mMaxBubbles];
	mDoneDrawing = true;
	srand(system_time() & RAND_MAX);
	return B_OK;
}

void 
Foam::StopSaver()
{
	delete [] mBubbles;
}

void 
Foam::Draw(BView *v, int32 /*frame*/)
{
	// first frame or restart with a new bubble set
	if (mDoneDrawing)
	{
		v->FillRect(v->Bounds(), B_SOLID_LOW);

		// pick a random color (in HSV space, because it's more fun :)
		double hue = drand();								// [0, 1]
		double value = drand() * 0.6 + 0.4;			// [0.4, 1]
		double saturation = drand() * 0.7 + 0.3;	// [0.3, 1]
		v->SetHighColor(hsv2rgb(hue, saturation, value));

		// start with a single bubble that touches the wall
		mBubbles[0].x = drand() * v->Bounds().Width();
		mBubbles[0].y = drand() * v->Bounds().Height();
		double dx = min(mBubbles[0].x, v->Bounds().right - mBubbles[0].x);
		double dy = min(mBubbles[0].y, v->Bounds().bottom - mBubbles[0].y);
		mBubbles[0].rad2 = (dx < dy) ? dx*dx : dy*dy;
		mBubbles[0].rad = sqrt(mBubbles[0].rad2);
		v->StrokeEllipse(BPoint(mBubbles[0].x, mBubbles[0].y), mBubbles[0].rad, mBubbles[0].rad, B_SOLID_HIGH);
		mBubblesDrawn = 1;

		SetTickSize(10000);
		mDoneDrawing = false;
	}

	// drawn all the bubbles yet?
	if (mBubblesDrawn < mMaxBubbles)
	{
		double x, y, rad = 0;
		BRect r = v->Bounds();

		// the maximum possible radius is half the smallest dimension of the view
		double nearestRad = min(r.Height(), r.Width()) / 2;
		bool isInside;
		do
		{
			// pick a point in the view
			x = drand() * r.Width();
			y = drand() * r.Height();

			// find the nearest edge, and make sure we clip to it
			double edgeDist = min ( min(r.right - x, x - r.left), min(r.bottom - y, y - r.top) );
			if (edgeDist < nearestRad) nearestRad = edgeDist;

			// check whether this point is outside all extant bubbles
			isInside = false;
			for (size_t i = 0; i < mBubblesDrawn; i++)
			{
				// actually rad^2 for now...
				rad = distance2(x, y, mBubbles[i]);

				// if it's inside this bubble, bail and start over
				if (rad < mBubbles[i].rad2)
				{
					nearestRad = min(r.Height(), r.Width()) / 2;
					isInside = true;
					break;		// break out of the for() loop over all bubbles
				}

				// otherwise, keep track of the distance to the nearest bubble
				// we encounter during the scan
				rad = sqrt(rad) - mBubbles[i].rad;
				if (rad < nearestRad)
				{
					nearestRad = rad;
				}
			}
		} while (isInside);

		// okay, we have a new bubble -- add & draw it
		mBubbles[mBubblesDrawn].x = x;
		mBubbles[mBubblesDrawn].y = y;
		mBubbles[mBubblesDrawn].rad = nearestRad;
		mBubbles[mBubblesDrawn].rad2 = nearestRad*nearestRad;

		v->StrokeEllipse(BPoint(x, y), mBubbles[mBubblesDrawn].rad, mBubbles[mBubblesDrawn].rad, B_SOLID_HIGH);
		mBubblesDrawn++;
	}
	else
	{
		SetTickSize(3500000);
		mDoneDrawing = true;
	}
}
