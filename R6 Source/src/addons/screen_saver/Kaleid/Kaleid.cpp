// kaleid.cpp
// by Craig Dickson
//
// History:
// 08/30/93  crd  New After Dark module.
// 01/13/94  crd  Clear screen every 15 minutes or so.
// 04/25/94  rab  Cleanups, more comments, etc.
// 05/05/94  crd  Happy Cinco de Mayo!
// 01/20/99  dw   ported to BeOS

#include "Kaleid.h"
#include <StringView.h>
#include <Slider.h>
#include <stdlib.h>
#include <time.h>


#ifndef max
#define max(a,b)    ((a) > (b)? (a): (b))
#endif

#ifndef min
#define min(a,b)    ((a) < (b)? (a): (b))
#endif


class SetupView : public BView
{
	int32	*pix;
	BSlider *sl;
public:
	SetupView(BRect frame, const char *name, int32 *pixelsize)
	 : BView(frame, name, 0, B_FOLLOW_ALL), pix(pixelsize)
	{
	}

	void AttachedToWindow()
	{
		SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		AddChild(new BStringView(BRect(10, 10, 200, 35), B_EMPTY_STRING, "Kaleid: port of an AfterDark module"));
		sl = new BSlider(BRect(10, 40, 200, 100), "slider", "Pixel size", new BMessage('pixl'), 1, 30);
		sl->SetValue(*pix);
		sl->SetLimitLabels("1", "30");
		sl->SetHashMarks(B_HASH_MARKS_BOTTOM);
		sl->SetTarget(this);
		AddChild(sl);
	}

	void MessageReceived(BMessage *msg)
	{
		switch(msg->what)
		{
			case 'pixl' :
				*pix = sl->Value();
				break;

			default :
				BView::MessageReceived(msg);
				break;
		}
	}
};

// MAIN INSTANTIATION FUNCTION
extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message, image_id image)
{
	return new Kaleid(message, image);
}

Kaleid::Kaleid(BMessage *message, image_id image)
 : BScreenSaver(message, image)
{
	if(message->FindInt32("pixels", &nPixels) != B_OK)
		nPixels = 1;
}

status_t Kaleid::SaveState(BMessage *into) const
{
	into->AddInt32("pixels", nPixels);
	return B_OK;
}

void Kaleid::StartConfig(BView *view)
{
	setup = new SetupView(view->Bounds(), "setup", &nPixels);
	view->AddChild(setup);
}

status_t Kaleid::StartSaver(BView *v, bool preview)
{
	SetTickSize(1000);
	srand(time(0));

	previewmode = preview;

	// for preview mode:
	int n;
	if(previewmode)
		n = 1;
	else
		n = nPixels;

	bForce = true;
	colorref.red = 0;
	colorref.green = 0;
	colorref.blue = 0;
	colorref.alpha = 255;
	nPosX = 0;
	nPosY = 0;

    int nDiv = 2 * n;     // Temporary variable to save on divisions

    // Calculate max. distance from center that we will draw to.
    nLimitRange = min(v->Bounds().IntegerWidth(), v->Bounds().IntegerHeight()) / nDiv;

    // Calculate center of logical display.
    nWindowMidX = v->Bounds().IntegerWidth() / nDiv;
    nWindowMidY = v->Bounds().IntegerHeight() / nDiv;

    // Initialize screen-clearing timer.
    LastCleared = system_time();

	return B_OK;
}

void Kaleid::Draw(BView *view, int32 frame)
{
	if(frame == 0 || system_time() > LastCleared + 15 * 60 * 1000000LL)  // blank every 15 minutes
	{
		LastCleared = system_time();
		view->FillRect(view->Bounds(), B_SOLID_LOW);
	}

	if(bForce || (rand() % 0x1FF) == 0x137)
    {
        bForce = false;
        colorref.red = rand() % 255;
        colorref.green = rand() % 255;
        colorref.blue = rand() % 255;
        colorref.alpha = 255;
    }

    nPosX += (rand() % 3) - 1;
    if (nPosX > nLimitRange)
        nPosX = 0;
    else if (nPosX < 0)
        nPosX = nLimitRange;

    nPosY += (rand() % 3) - 1;
    if (nPosY > nLimitRange)
        nPosY = 0;
    else if (nPosY < 0)
        nPosY = nLimitRange;

    int nXMinusX = nWindowMidX - nPosX;
    int nXMinusY = nWindowMidX - nPosY;
    int nXPlusX  = nWindowMidX + nPosX;
    int nXPlusY  = nWindowMidX + nPosY;
    int nYMinusX = nWindowMidY - nPosX;
    int nYMinusY = nWindowMidY - nPosY;
    int nYPlusX  = nWindowMidY + nPosX;
    int nYPlusY  = nWindowMidY + nPosY;

#if 0
    if (nPixels == 1)
    {
        SetPixel(hDC, nXPlusX, nYPlusY, colorref);
        SetPixel(hDC, nXPlusX, nYMinusY, colorref);
        SetPixel(hDC, nXMinusX, nYPlusY, colorref);
        SetPixel(hDC, nXMinusX, nYMinusY, colorref);
        SetPixel(hDC, nXPlusY, nYPlusX, colorref);
        SetPixel(hDC, nXPlusY, nYMinusX, colorref);
        SetPixel(hDC, nXMinusY, nYPlusX, colorref);
        SetPixel(hDC, nXMinusY, nYMinusX, colorref);
    }
    else
    {
#endif
		// for preview mode:
		int n;
		if(previewmode)
			n = 1;
		else
			n = nPixels;

        nXMinusX *= n;
        nXMinusY *= n;
        nXPlusX  *= n;
        nXPlusY  *= n;
        nYMinusX *= n;
        nYMinusY *= n;
        nYPlusX  *= n;
        nYPlusY  *= n;

		view->SetHighColor(colorref);

		view->FillRect(BRect(nXPlusX, nYPlusY, nXPlusX + n, nYPlusY + n), B_SOLID_HIGH);
		view->FillRect(BRect(nXPlusX, nYMinusY, nXPlusX + n, nYMinusY + n), B_SOLID_HIGH);
		view->FillRect(BRect(nXMinusX, nYPlusY, nXMinusX + n, nYPlusY + n), B_SOLID_HIGH);
		view->FillRect(BRect(nXMinusX, nYMinusY, nXMinusX + n, nYMinusY + n), B_SOLID_HIGH);
		view->FillRect(BRect(nXPlusY, nYPlusX, nXPlusY + n, nYPlusX + n), B_SOLID_HIGH);
		view->FillRect(BRect(nXPlusY, nYMinusX, nXPlusY + n, nYMinusX + n), B_SOLID_HIGH);
		view->FillRect(BRect(nXMinusY, nYPlusX, nXMinusY + n, nYPlusX + n), B_SOLID_HIGH);
		view->FillRect(BRect(nXMinusY, nYMinusX, nXMinusY + n, nYMinusX + n), B_SOLID_HIGH);
#if 0
    }
#endif
}
