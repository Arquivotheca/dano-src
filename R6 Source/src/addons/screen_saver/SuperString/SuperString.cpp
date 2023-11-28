//	SaverModule.cp
//	©1996 Jon Watte
// Ported to the BScreenSaver API Jan 1999 by Duncan Wilcox


#include "SuperString.h"
#include <StringView.h>
#include <CheckBox.h>
#include <stdlib.h>
#include <OS.h>
#include <stdlib.h>


//	features

#define FIXCOLOR	0
#define FADECOLOR	1
#define DRAWDOWN	0

//	constants

#define MAXNLINE	100
#define	PI360		(2.0*M_PI)
#define	PI180		(M_PI)
#define	PI120		(2.0/3.0*M_PI)
#define SPIN		0.28

#if FADECOLOR
bool gFadeColor;
#endif


//	utility
static float
fl_rand(
	float base,
	float top)
{
	float r = (rand()&0x7fff)/32767.0*(top-base)+base;
	return r;
}


rgb_color colors[MAXNLINE];
BRect steps[MAXNLINE];
BPoint posA, posB;
float dirA, spdA, spinA;
float dirB, spdB, spinB;
float color;
BRect bounds;
int NLINE;
float color_delta;

//	step the model

static void
step()
{
//	A
	float stepAX = sin(dirA)*spdA;
	float stepAY = -cos(dirA)*spdA;
	posA.x += stepAX;
	posA.y += stepAY;
	if ((stepAX < 0) && (posA.x < 10)) {
		dirA = PI360-dirA;
		spinA = -spinA;
	}
	if ((stepAX > 0) && (posA.x > bounds.Width()-9)) {
		dirA = PI360-dirA;
		spinA = -spinA;
	}
	if ((stepAY < 0) && (posA.y < 10)) {
		dirA = PI180-dirA;
		if (dirA < 0)
			dirA += PI360;
		spinA = -spinA;
	}
	if ((stepAY > 0) && (posA.y > bounds.Height()-9)) {
		dirA = PI180-dirA;
		if (dirA < 0)
			dirA += PI360;
		spinA = -spinA;
	}
	dirA += spinA;
	if (dirA < 0)
		dirA += PI360;
	if (dirA > PI360)
		dirA -= PI360;
//	B
	float stepBX = sin(dirB)*spdB;
	float stepBY = -cos(dirB)*spdB;
	posB.x += stepBX;
	posB.y += stepBY;
	if ((stepBX < 0) && (posB.x < 10)) {
		dirB = PI360-dirB;
		spinB = -spinB;
	}
	if ((stepBX > 0) && (posB.x > bounds.Width()-9)) {
		dirB = PI360-dirB;
		spinB = -spinB;
	}
	if ((stepBY < 0) && (posB.y < 10)) {
		dirB = PI180-dirB;
		if (dirB < 0)
			dirB += PI360;
		spinB = -spinB;
	}
	if ((stepBY > 0) && (posB.y > bounds.Height()-9)) {
		dirB = PI180-dirB;
		if (dirB < 0)
			dirB += PI360;
		spinB = -spinB;
	}
	dirB += spinB;
	if (dirB < 0)
		dirB += PI360;
	if (dirB > PI360)
		dirB -= PI360;
//	color
	color += color_delta;
	if (color > PI360)
		color -= PI360;
}

//	put the current model in a line

static void
putline(
	int ix)
{
	steps[ix] = BRect(posA, posB);
}

static void
putcolor(
	int ix)
{
	rgb_color rgb = {0, 0, 0, 0};
	rgb.red = (uint8)(sin(color)*127+128);
	rgb.green = (uint8)(sin(color+PI120)*127+128);
	rgb.blue = (uint8)(sin(color-PI120)*127+128);
	colors[ix] = rgb;
}

#if FADECOLOR
static void
fadecolor(
	int ix)
{
	rgb_color rgb = colors[ix];
	if (rgb.red) rgb.red--;
	if (rgb.green) rgb.green--;
	if (rgb.blue) rgb.blue--;
	colors[ix] = rgb;
}

static void
fadecolor(
	int ix,
	int delta)
{
	rgb_color rgb = colors[ix];
	if (rgb.red>delta) rgb.red-=delta; else rgb.red=0;
	if (rgb.green>delta) rgb.green-=delta; else rgb.green=0;
	if (rgb.blue>delta) rgb.blue-=delta; else rgb.blue=0;
	colors[ix] = rgb;
}
#endif

//	set-up for animation

static void
setup(
	BView *view)
{
//	set up the model parameters

	srand(find_thread(NULL));

	NLINE = (int)fl_rand(MAXNLINE/3, MAXNLINE);
	bounds = view->Bounds();
	posA.x = fl_rand(0, bounds.Width()+1);
	posA.y = fl_rand(0, bounds.Height()+1);
	posB.x = fl_rand(0, bounds.Width()+1);
	posB.y = fl_rand(0, bounds.Height()+1);
	dirA = fl_rand(0, PI360);
	spdA = fl_rand(3, 6);
	spinA = fl_rand(-SPIN, SPIN);
	spinA = spinA*fabs(spinA)*fabs(spinA);
	dirB = fl_rand(0, PI360);
	spdB = fl_rand(2, 6);
	spinB = fl_rand(-SPIN, SPIN);
	spinB = spinB*fabs(spinB)*fabs(spinB);
	color = fl_rand(0, PI360);
	color_delta = fl_rand(0.03,15.0/(NLINE+100.0));

//	fill the buffer with lines

	for (int ix=0; ix<NLINE; ix++) {
		putline(ix);
		putcolor(ix);
#if FADECOLOR
		if (gFadeColor)
			fadecolor(ix, (NLINE-ix-1)*2);
#endif
		step();
	}

//	last line always erases

	rgb_color black = { 0, 0, 0, 0 };
	colors[0] = black;
}

//	move the line buffer one notch

static void
flush()
{
	for (int ix=0; ix<NLINE-1; ix++) {
		steps[ix] = steps[ix+1];
#if FIXCOLOR
		if (ix > 0)
			colors[ix] = colors[ix+1];
#if FADECOLOR
		if (gFadeColor)
			fadecolor(ix);
#endif
#endif
	}
}

//	draw the buffer

static void
draw(
	BView *view)
{
	view->BeginLineArray(NLINE);
#if DRAWDOWN
	for (int ix=NLINE-1; ix>=0; ix--) {
#else
	for (int ix=0; ix<NLINE; ix++) {
#endif
		view->AddLine(
			steps[ix].LeftTop(),
			steps[ix].RightBottom(),
			colors[ix]);
	}
	view->EndLineArray();
}



/*	Use a custom view as target for messages from
 *	controls that change.
 */

#define FADE_COLOR	'Fade'

class MView :
	public BView
{
public:
								MView(
									BRect area,
									const char *name,
									ulong resize,
									ulong flags);

		void					MessageReceived(
									BMessage *message);

};

MView::MView(
	BRect area,
	const char *name,
	ulong resize,
	ulong flags) :
	BView(area, name, resize, flags)
{
}

void
MView::MessageReceived(
	BMessage *message)
{
	if (message->what != FADE_COLOR) {
		BView::MessageReceived(message);
	}
#if FADECOLOR
	BCheckBox *box = (BCheckBox *)FindView("fade");
	if (box) {
		gFadeColor = box->Value();
	}
#endif
}


// MAIN INSTANTIATION FUNCTION
extern "C" _EXPORT BScreenSaver *instantiate_screen_saver(BMessage *message, image_id image)
{
	return new SuperString(message, image);
}

SuperString::SuperString(BMessage *message, image_id image)
 : BScreenSaver(message, image)
{
#ifdef FADECOLOR
	if(message->FindBool("fade", &gFadeColor) != B_OK)
		gFadeColor = false;
#endif
}

void SuperString::StartConfig(BView *view)
{
	BView *mView = new MView(view->Bounds(), "mView",
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP_BOTTOM,
		B_WILL_DRAW);
	view->AddChild(mView);
	mView->SetViewColor(view->ViewColor());

	BRect area(20,20, 320, 35);
	BStringView *text = new BStringView(area, "whom", 
		"SuperString, by Jon Watte");
	mView->AddChild(text);
	text->SetViewColor(view->ViewColor());

#if FADECOLOR
	area.OffsetBy(0, 30);
	BCheckBox *box = new BCheckBox(area, "fade", "Fade Colors",
		new BMessage(FADE_COLOR));
	mView->AddChild(box);
	box->SetViewColor(view->ViewColor());
	box->SetTarget(mView);
	box->SetValue(gFadeColor);
#endif

//		area = mView->Bounds();
//		area.top = area.bottom-15;
//		area.OffsetBy(20, -20);
//		BStringView *comment = new BStringView(area, "comment",
//			"Requires accelerated screen card");
//		mView->AddChild(comment);
//		comment->SetViewColor(view->ViewColor());

}

status_t SuperString::StartSaver(BView *, bool /* preview */)
{
	SetTickSize(10000);
	return B_OK;
}

void SuperString::Draw(BView *view, int32 frame)
{
	if(frame == 0)
	{
		setup(view);
		view->FillRect(view->Bounds(), B_SOLID_LOW);	// Erase
	}

	flush();			//	move lines in buffer one step
	putline(NLINE-1);	//	put the current model
#if FIXCOLOR
	putcolor(NLINE-1);
#endif
	step();				//	prepare next frame

	draw(view);
}

status_t SuperString::SaveState(BMessage *into) const
{
	into->AddBool("fade", gFadeColor);
	return B_OK;
}
