#include <Debug.h>

#include <stdlib.h>
#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _LOOPER_H
#include <Looper.h>
#endif
#ifndef _MESSAGE_QUEUE_H
#include <MessageQueue.h>
#endif
#include <OS.h>
#include "KalView.h"
#include "KalDraw.h"
#include "Kaleid.h"

static BRect frame;
static KalView *v;
static long cx, cy, aspect;

KalDraw::KalDraw (KalView *view) : BLooper()
{
	pending_message_count = 0;
	v = view;
	ChangeSize();
}

void KalDraw::ChangeSize (void)
{
	if (!v)
		return;

	if (v->Window()->Lock()) {
		frame = v->Bounds();
		v->Window()->Unlock();

		cx = ((int)frame.right - (int)frame.left) >> 1;
		cy = ((int)frame.bottom - (int)frame.top) >> 1;
		aspect = (cx << 10) / cy;
	}
}

void KalDraw::Quit()
{
	if (v && v->Window()->Lock()) {
		v->Window()->Quit();
		v = NULL;
	}

	BLooper::Quit();
}

int KalDraw::Random (int min, int max)
{
	int n = rand();

	max -= min;
	if (n < 0)
		return((-n % max) + min);
	else
		return((n % max) + min);
}

void KalDraw::DrawLines (void)
{
	long xv1, xv2, yv1, yv2;
	long xa, xb, ya, yb;
	long x1, y1, x2, y2;
	int i, nr;
	BPoint sp, ep, bp;
	rgb_color	c;

	enum {maxSpread = 6, minCount = 2, maxCount = 12};

	if (!v)
		return;

	while (TRUE)
	{
		if (!v->Window()->Lock())
			goto done;

		v->SetLowColor(0, 0, 0);
		v->SetHighColor(0, 0, 0);
		v->FillRect(frame);
		v->Window()->Unlock();

		x1 = Random(2, cy);
		x2 = Random(2, cy);
		y1 = Random(1, x1);
		y2 = Random(1, x2);

		for (i = 0; i < 200  && (i < 20 || Random(0, 99)); ++i)
		{
			xv1 = Random(-maxSpread, maxSpread);
			xv2 = Random(-maxSpread, maxSpread);
			yv1 = Random(-maxSpread, maxSpread);
			yv2 = Random(-maxSpread, maxSpread);
	
			if (!v->Window()->Lock())
				goto done;
			v->SetHighColor(rand(), rand(), rand());
			c.red = rand();
			c.green = rand();
			c.blue = rand();
			for (nr = Random(minCount, maxCount); nr != 0; --nr)
			{
				xa = (x1 * aspect) >> 10;
				xb = (x2 * aspect) >> 10;
				ya = (y1 * aspect) >> 10;
				yb = (y2 * aspect) >> 10;

				v->BeginLineArray(10);
				sp.Set(cx+xa, cy-y1); ep.Set(cx+xb, cy-y2);
				v->AddLine(sp,ep,c);
				sp.Set(cx-ya, cy+x1); ep.Set(cx-yb, cy+x2);
				v->AddLine(sp,ep,c);
				sp.Set(cx-xa, cy-y1); ep.Set(cx-xb, cy-y2);
				v->AddLine(sp,ep,c);
				sp.Set(cx-ya, cy-x1); ep.Set(cx-yb, cy-x2);
				v->AddLine(sp,ep,c);
				sp.Set(cx-xa, cy+y1); ep.Set(cx-xb, cy+y2);
				v->AddLine(sp,ep,c);
				sp.Set(cx+ya, cy-x1); ep.Set(cx+yb, cy-x2);
				v->AddLine(sp,ep,c);
				sp.Set(cx+xa, cy+y1); ep.Set(cx+xb, cy+y2);
				v->AddLine(sp,ep,c);
				sp.Set(cx+ya, cy+x1); ep.Set(cx+yb, cy+x2);
				v->AddLine(sp,ep,c);

				v->EndLineArray();

// See bug #1992 regarding the following call to BView::Sync()
//				v->Sync();

				x1 = (x1 + xv1) % cy;
				y1 = (y1 + yv1) % cy;
				x2 = (x2 + xv2) % cy;
				y2 = (y2 + yv2) % cy;
			}
			v->Window()->Unlock();

			if (atomic_add(&pending_message_count, 0) > 0) {
				atomic_add(&pending_message_count, -1);
				return;
			}
		}
	}
done:
	return;
}

void KalDraw::MessageReceived (BMessage *msg)
{
	uint32 what = msg->what;

	switch (what)
	{
		case dStart:
			if (v) {
				if (v->Window()->Lock()) {
					v->Window()->Show();
					v->Window()->Unlock();
					snooze(500000);
					DrawLines();
				}
			}
			break;

		case dChangeSize:
			ChangeSize();
			DrawLines();
			break;
	}
}
