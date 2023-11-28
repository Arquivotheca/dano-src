#include "MultiScrollBar.h"
#include "MyDebug.h"

MultiScrollBar::MultiScrollBar(	BRect frame,
						const char *name,
						BView *target,
						long min,
						long max,
						orientation direction)
	:	BScrollBar(frame,name,target,min,max,direction),
		oldValue(0)
{
	extraTargets = new RList<BView *>;
}

MultiScrollBar::~MultiScrollBar()
{
	delete extraTargets;
}

void MultiScrollBar::AddExtraTarget(BView *v)
{
	extraTargets->AddItem(v);
}

void MultiScrollBar::ValueChanged(float n)
{
	long delta = (long)(oldValue - n);

	oldValue = (long)n;
	PRINT(("multiscroll delta %d\n",delta));
	
	BScrollBar::ValueChanged(n);
	
	orientation o = Orientation();
	for (long i = extraTargets->CountItems()-1; i >= 0; i--) {
		BView *t = extraTargets->ItemAt(i);
		if (o == B_HORIZONTAL) {
			t->ScrollBy(-delta,0);
		}
		else if (o == B_VERTICAL) {
			t->ScrollBy(0,-delta);
		}
	}
}
