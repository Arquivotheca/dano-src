#include "PPAddOn.h"
#include <File.h>
#include <Resources.h>
#include <Entry.h>
#include <stdio.h>
#include <PopUpMenu.h>
#include "PrefsAppExport.h"

/////////////////////////////////////////////////////
// the add-on class. contains all that's needed by the application
//

class MouseAddOn : public PPAddOn {
public:
	MouseAddOn(image_id i,PPWindow*w);

	BView* MakeView();
	bool UseAddOn();

	BBitmap* Icon();
	BBitmap* SmallIcon();
	char* Name();
	char* InternalName();

	void PanelActivated(bool);
};

//////////////////////////////////////////////////////
// the top view. all the logic goes there.
//

class MouseView : public BView {
public:
	MouseView(BMessage*);

	void AllAttached();
	void AllDetached();
	void MessageReceived(BMessage*);

	void DoubleClickSpeed(BMessage*);
	void MouseSpeed(BMessage*);
	void MouseAcceleration(BMessage*);
	void FFM(BMessage*);
	void Orientation(BMessage*);
	void NumberButtons(BMessage*);
	void ButtonMapping(BMessage*);
	void Default(BMessage*);
	void Revert(BMessage*);

	void PanelActivated();

	void EnableRevert();

	static _EXPORT BArchivable* Instantiate(BMessage*);

	int32 nbuttons;
	mouse_map mmap;

	int32 initial_nbuttons;
	int32 initial_speed;
	int32 initial_accel;
	int32 initial_clkspeed;
	mouse_map initial_mmap;
	int32 initial_mmode;
	int32 initial_orientation;
};

///////////////////////////////////////////////////
// the view that displays the mouse pointer orientations.
//

class MousePointerView : public BView {
public:
	MousePointerView(BMessage*);
	~MousePointerView();
	void Draw(BRect);

	BBitmap*bmp[2];
	int current_bitmap;

	static _EXPORT BArchivable* Instantiate(BMessage*);
};

///////////////////////////////////////////////////////
// the 'mouse preview' view.
//

class MouseButtonsView : public BView {
public:
	MouseButtonsView(BMessage*);
	~MouseButtonsView();
	void AttachedToWindow();
	void Draw(BRect);

	void MouseDown(BPoint);
	void MouseUp(BPoint);
	void MouseMoved(BPoint,uint32,const BMessage*);

	static _EXPORT BArchivable*Instantiate(BMessage*);

	uint32 buttons;
	BBitmap*bmp;
	BPopUpMenu* menu[3];
	int32 xmin,xmax,ymin,ymax;
};
