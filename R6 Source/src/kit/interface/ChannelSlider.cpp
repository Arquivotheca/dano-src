#include <ChannelSlider.h>

#include <Bitmap.h>
#include <Region.h>
#include <Screen.h>
#include <Window.h>
#include <PropertyInfo.h>
#include <Debug.h>

#include <archive_defs.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//	the size of the border around the slide area
#define XFRAME 3
#define YFRAME 3

static rgb_color
scale(
	rgb_color value,
	uchar target_source,
	uchar target_destination)
{
	int red = value.red * target_destination / target_source;
	int green = value.green * target_destination / target_source;
	int blue = value.blue * target_destination / target_source;
	rgb_color ret = {
		red < 0 ? 0 : red > 255 ? 255 : red , 
		green < 0 ? 0 : green > 255 ? 255 : green , 
		blue < 0 ? 0 : blue > 255 ? 255 : blue ,
		255
	};
	return ret;
}

#if 0
static uchar _s_slider_bits[] = {	/* 9x9 plus padding */
	0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 
	0x00, 0x1f, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x16, 0x00, 0xff, 0xff, 0xff,
	0x00, 0x1c, 0x16, 0x16, 0x16, 0x16, 0x16, 0x0c, 0x00, 0xff, 0xff, 0xff,
	0x00, 0x1c, 0x16, 0x16, 0x16, 0x16, 0x16, 0x0c, 0x00, 0xff, 0xff, 0xff,
	0x00, 0x1c, 0x16, 0x16, 0x16, 0x16, 0x16, 0x0c, 0x00, 0xff, 0xff, 0xff,
	0x00, 0x1c, 0x16, 0x16, 0x16, 0x16, 0x16, 0x0c, 0x00, 0xff, 0xff, 0xff,
	0x00, 0x1c, 0x16, 0x16, 0x16, 0x16, 0x16, 0x0c, 0x00, 0xff, 0xff, 0xff,
	0x00, 0x16, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x00, 0xff, 0xff, 0xff,
	0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 
};
#endif
static const unsigned char s_vertical_knob_12_15 [] = {
		0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,
        0xff,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x00,0xff,
        0xff,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x00,0xff,
        0xff,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x00,0x12,
        0xff,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x00,0x12,
        0xff,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x00,0x12,
        0xff,0x00,0x3f,0x3f,0xcb,0xcb,0xcb,0xcb,0x3f,0x3f,0x00,0x12,
        0xff,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x00,0x12,
        0xff,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x00,0x12,
        0xff,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x00,0x12,
        0xff,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x00,0x12,
        0xff,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x00,0x12,
        0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0c,0x12,
		0xff,0xff,0xff,0xff,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0xff,
};
static const unsigned char s_horizontal_knob_16_11 [] = {
	0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,
	0xff,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x00,0xff,0xff,
	0xff,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x00,0xff,0xff,
	0xff,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0xcb,0x3f,0x3f,0x3f,0x3f,0x3f,0x00,0x12,0xff,
	0xff,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0xcb,0x3f,0x3f,0x3f,0x3f,0x3f,0x00,0x12,0xff,
	0xff,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0xcb,0x3f,0x3f,0x3f,0x3f,0x3f,0x00,0x12,0xff,
	0xff,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0xcb,0x3f,0x3f,0x3f,0x3f,0x3f,0x00,0x12,0xff,
	0xff,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x00,0x12,0xff,
	0xff,0x00,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x00,0x12,0xff,
	0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0c,0x12,0xff,
	0xff,0xff,0xff,0xff,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0x12,0xff,0xff,
};


/*-------------------------------------------------------------*/
	/*
	 BChannelSlider supports the following:
	 	GET/SET		"Orientation"	DIRECT form only
	*/

static property_info	prop_list[] = {
	{"Orientation",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		NULL, 0,
		{B_INT32_TYPE},
		{},
		{}
	},
	{NULL,
		{},
		{},
		NULL, 0,
		{},
		{},
		{}
	}
};

enum {
	kOrientation = 0
};

/*-------------------------------------------------------------*/

BChannelSlider::BChannelSlider(
	BRect area,
	const char * name,
	const char * label,
	BMessage * model,
	int32 channels,
	uint32 resize,
	uint32 flags) :
	BChannelControl(area, name, label, model, channels, resize,
					flags | B_WILL_DRAW | B_FRAME_EVENTS )
{
	InitData();
}

BChannelSlider::BChannelSlider(
	BRect area,
	const char * name,
	const char * label,
	BMessage * model,
	orientation o,
	int32 channels,
	uint32 resize,
	uint32 flags) :
	BChannelControl(area, name, label, model, channels, resize,
					flags | B_WILL_DRAW | B_FRAME_EVENTS )
{
	InitData();
	SetOrientation(o);
}

BChannelSlider::BChannelSlider(
	BMessage * from) :
	BChannelControl(from)
{
	PRINT(("Created from message.\n"));
	InitData();
	orientation msgOrientation;
	if ( from->FindInt32(S_ORIENTATION, (int32 *)&msgOrientation) == B_OK) {
		SetOrientation(msgOrientation);
	}
}

void
BChannelSlider::InitData()
{
	_m_baseline = _m_linefeed = -1;
	_m_left_knob = NULL;
	_m_mid_knob = NULL;
	_m_right_knob = NULL;
	_m_vertical = Bounds().Width() < Bounds().Height();
	SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	SetHighUIColor(B_UI_PANEL_TEXT_COLOR);
	SetDoubleBuffering(B_UPDATE_INVALIDATED | B_UPDATE_RESIZED);
	SetInvalidate(true);
	_m_click_delta = B_ORIGIN;
	fInitialValues = NULL;
	fFocusChannel = -1;
	PRINT(("Initialized data.\n"));
}

BChannelSlider::~BChannelSlider()
{
	delete fInitialValues;
	delete _m_left_knob;
	delete _m_mid_knob;
	delete _m_right_knob;
}

BArchivable * BChannelSlider::Instantiate(
	BMessage * from)
{
	return new BChannelSlider(from);
}

status_t BChannelSlider::Archive(
	BMessage * into,
	bool deep) const
{
	status_t err = inherited::Archive(into, deep);
	if( err == B_OK ) {
		into->AddInt32(S_ORIENTATION, Orientation());
	}
	return err;
}


orientation
BChannelSlider::Orientation() const
{
	if (_m_vertical) return B_VERTICAL;
	return B_HORIZONTAL;
}

void
BChannelSlider::SetOrientation(orientation o)
{
	bool v = (o == B_VERTICAL) ? true : false;
	if( v != _m_vertical ) {
		_m_vertical = v;
		Invalidate(Bounds());
	}
}

int32 BChannelSlider::MaxChannelCount() const
{
	return 32;
}

bool 
BChannelSlider::SupportsIndividualLimits() const
{
	return false;
}


void
BChannelSlider::AttachedToWindow()
{
	SetColorsFromParent();
	inherited::AttachedToWindow();
}

void
BChannelSlider::AllAttached()
{
	inherited::AllAttached();
}

void
BChannelSlider::DetachedFromWindow()
{
	inherited::DetachedFromWindow();
}

void
BChannelSlider::AllDetached()
{
	inherited::AllDetached();
}

void
BChannelSlider::MessageReceived(BMessage* msg)
{
	bool handled = false;
	BMessage	reply(B_REPLY);
	status_t	err;
	
	switch(msg->what) {
		case B_GET_PROPERTY:
		case B_SET_PROPERTY:
		{
			BMessage	specifier;
			int32		form;
			const char	*prop;
			int32		cur;
			int32		i;
			err = msg->GetCurrentSpecifier(&cur, &specifier, &form, &prop);
			if( !err ) {
				BPropertyInfo	pi(prop_list);
				i = pi.FindMatch(msg, 0, &specifier, form, prop);
			}
			if( err ) break;
			switch (i) {
				case kOrientation: {
					if (msg->what == B_GET_PROPERTY) {
						reply.AddInt32("result", (int32)Orientation());
						handled = true;
					} else {
						int32 orient;
						err = msg->FindInt32("data", &orient);
						if (!err) {
							SetOrientation((orientation)orient);
							reply.AddInt32("error", B_OK);
							handled = true;
						}
					}
				} break;
			}
		}
	}
	
	if (handled)
		msg->SendReply(&reply);
	else
		inherited::MessageReceived(msg);
}

/* this is a fairly minimal way to draw a slider */
void BChannelSlider::Draw(
	BRect)
{
	PRINT(("Drawing...\n"));
	BRect b(Bounds());
	
	PRINT(("Drawing thumbs...\n"));
	PushState();
	DrawThumbs();
	PopState();
	
	if( !IsEnabled() ) SetHighColor(HighColor().disable(ViewColor()));
	SetLowColor(ViewColor());
	
	const char * l = Label();
	if (l != NULL) {
		PRINT(("Drawing label: %s\n", l));
		float x = floor((b.Width()-StringWidth(l))/2);
		DrawString(l, BPoint(x, _m_baseline));
	}
	const char * mi = MinLimitLabel();
	const char * ma = MaxLimitLabel();
	if (mi || ma) {
		if (Vertical()) {
			PRINT(("Drawing vertical labels.\n"));
			if (ma) {
				float x = ((b.Width()-StringWidth(ma))/2);
				DrawString(ma, BPoint(x, _m_baseline+_m_linefeed));
			}
			if (mi) {
				float x = ((b.Width()-StringWidth(mi))/2);
				DrawString(mi, BPoint(x, b.bottom-_m_linefeed+_m_baseline));
			}
		}
		else {
			PRINT(("Drawing horizontal labels.\n"));
			if (mi) DrawString(mi, BPoint(_m_linefeed, b.bottom-_m_linefeed+_m_baseline));
			if (ma) DrawString(ma, BPoint(b.right-_m_linefeed-StringWidth(ma), b.bottom-_m_linefeed+_m_baseline));
		}
	}
	
	if (IsFocus() && IsEnabled() && Window()->IsActive() && !IsInvalidatePending()) {
		InvalidateAtTime(NextFocusTime());
	}
}

void
BChannelSlider::MouseMovedCommon(BPoint into, BPoint old)
{
	if (into != old) {
		float trackPos;
		if (Vertical())
			trackPos = into.y-fMinpoint;
		else
			trackPos = fMinpoint-into.x;
		
		ValueList()[fCurrentChannel] = (int)(MaxLimitList()[fCurrentChannel] -
			trackPos*(MaxLimitList()[fCurrentChannel] - MinLimitList()[fCurrentChannel])/
			ThumbRangeFor(fCurrentChannel));

		if (fAllChannels) {
			for (int ix=0; ix<CountChannels(); ix++) {
				ValueList()[ix] = ValueList()[fCurrentChannel];
			}
		}

		if (fAllChannels) {
			int32 * val = new int32[CountChannels()];
#if _SUPPORTS_EXCEPTION_HANDLING
			try {
#endif
				memcpy(val, ValueList(), sizeof(int32)*CountChannels());
				SetValue(0, CountChannels(), val);
				InvokeNotifyChannel(ModificationMessage(), B_CONTROL_MODIFIED);
#if _SUPPORTS_EXCEPTION_HANDLING
			}
			catch (...) {
				delete[] val;
				throw;
			}
#endif
			delete[] val;
		} else {
			int32 v = ValueList()[fCurrentChannel];
			SetValueFor(fCurrentChannel, v);
			InvokeNotifyChannel(ModificationMessage(), B_CONTROL_MODIFIED,
								fCurrentChannel, 1);
		}
	}
}

void
BChannelSlider::MouseDown(BPoint where)
{
//	BRect b(Bounds());
	BPoint into = where;
	int ix;
	
	// If not enabled, user can't interact with control.
	if( !IsEnabled() ) {
		inherited::MouseDown(where);
		return;
	}
	
	if ((Flags() & B_NAVIGABLE) != 0 && !IsFocus())
		MakeFocus(true);
	
	SetExplicitFocus();
	
	fMinpoint = 0;
	fCurrentChannel = -1;
	
	//	find the channel that the click was in
	//	save the minpoint and the channel in case
	//	this is an asynchronous control
	//	these will be used in mousemoved
	for (ix=0; ix<CountChannels(); ix++) {
		BRect r = ThumbFrameFor(ix);
		r.OffsetBy(_m_click_delta);
		if (Vertical()) {
			fMinpoint = floor((r.bottom+r.top)/2);
			r.bottom += ThumbRangeFor(ix);
		} else {
			fMinpoint = floor((r.left+r.right)/2) + ThumbRangeFor(ix);
			r.right += ThumbRangeFor(ix);
		}
		if (r.Contains(where)) {
			fCurrentChannel = ix;
			break;
		}
	}
	if (fCurrentChannel < 0) {
		fMinpoint = 0;
		return;	/* no hit */
	}
	//	set the current channel to the one determined as hit
	SetCurrentChannel(fCurrentChannel);

	fAllChannels = false;
	
	//	remember the initial channel values
	if( fInitialValues ) {
		delete[] fInitialValues;
		fInitialValues = NULL;
	}
	fInitialValues = new int32[CountChannels()];
	for (ix=0; ix<CountChannels(); ix++) {
		fInitialValues[ix] = ValueFor(ix);
	}
	
	uint32 flags = 0;
	if (Window()->CurrentMessage()) {
		Window()->CurrentMessage()->FindInt32("buttons", (int32*)&flags);
	}
	BPoint old(into);
	old.x -= 1;
	old.y -= 1;
	
	while (true) {
		if ((Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) == 0) {
			old = into;
			GetMouse(&into, &flags);
		}
		if (!(flags & (B_PRIMARY_MOUSE_BUTTON | B_SECONDARY_MOUSE_BUTTON))) {
			break;
		}
		if (!(flags & B_SECONDARY_MOUSE_BUTTON)) {
			fAllChannels = true;
		}
		if( !IsTracking() ) {
			SetTracking(true);
			Redraw();
		}
		
		MouseMovedCommon(into, old);

		if ((Window()->Flags() & B_ASYNCHRONOUS_CONTROLS) != 0) {
			SetTracking(true);
			SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY | B_LOCK_WINDOW_FOCUS);
			//
			//	instead of returning here
			//	pass through the while loop once to
			//	handle single clicks
			//
			return;
		}
		if (into == old)
			snooze(20000);

	}	// end while
	
	FinishChange();
}

void
BChannelSlider::FinishChange()
{
	if( !fInitialValues ) return;
	
	if (fAllChannels) {
		bool changed = false;
		for( int ix=0; !changed && ix<CountChannels(); ix++ ) {
			if( fInitialValues[ix] != ValueFor(ix) ) changed = true;
		}
		if( changed ) {
			int32 * val = new int32[CountChannels()];
#if _SUPPORTS_EXCEPTION_HANDLING
			try {
#endif
				memcpy(val, ValueList(), sizeof(int32)*CountChannels());
				SetValue(0, CountChannels(), val);
				InvokeChannel();
#if _SUPPORTS_EXCEPTION_HANDLING
			}
			catch (...) {
				delete[] val;
				throw;
			}
#endif
			delete[] val;
		}
	} else {
		int32 v = ValueList()[fCurrentChannel];
		if( v != fInitialValues[fCurrentChannel] ) {
			SetValueFor(fCurrentChannel, v);
			InvokeChannel(Message(), fCurrentChannel, 1);
		}
	}
	
	SetTracking(false);
	Redraw();
}

void
BChannelSlider::MouseUp(BPoint pt)
{
	// If not enabled, user can't interact with control.
	if( !IsEnabled() ) {
		inherited::MouseUp(pt);
		return;
	}
	
	if (IsTracking()) {
		FinishChange();
		fAllChannels = false;
		fCurrentChannel = -1;
		fMinpoint = 0;
		SetTracking(false);
	} else
		inherited::MouseUp(pt);
}

void
BChannelSlider::MouseMoved(BPoint pt, uint32 code, const BMessage *message)
{
	// If not enabled, user can't interact with control.
	if( !IsEnabled() || !IsTracking()) 
		inherited::MouseMoved(pt, code, message);
	else
		MouseMovedCommon(pt, BPoint(-1, -1));
}

void
BChannelSlider::WindowActivated(bool state)
{
	inherited::WindowActivated(state);
}

enum key_action {
	kNoAction,
	kNextChannel,
	kPrevChannel,
	kAllChannels,
	kBumpUp,
	kBumpDown,
	kJumpUp,
	kJumpDown,
	kMax,
	kMin
};

struct key_action_map {
	char key;
	key_action action;
};

static key_action_map vertical_keys[] = {
	{ B_DOWN_ARROW, kBumpDown },
	{ B_UP_ARROW, kBumpUp },
	{ B_LEFT_ARROW, kPrevChannel },
	{ B_RIGHT_ARROW, kNextChannel },
	{ B_PAGE_UP, kJumpUp },
	{ B_PAGE_DOWN, kJumpDown },
	{ B_HOME, kMin },
	{ B_END, kMax },
	{ B_ESCAPE, kAllChannels },
	{ B_ENTER, kNoAction },
	{ B_SPACE, kNoAction },
	{ 0, kNoAction }
};

static key_action_map horizontal_keys[] = {
	{ B_DOWN_ARROW, kNextChannel },
	{ B_UP_ARROW, kPrevChannel },
	{ B_LEFT_ARROW, kBumpDown },
	{ B_RIGHT_ARROW, kBumpUp },
	{ B_PAGE_UP, kJumpUp },
	{ B_PAGE_DOWN, kJumpDown },
	{ B_HOME, kMin },
	{ B_END, kMax },
	{ B_ESCAPE, kAllChannels },
	{ B_ENTER, kNoAction },
	{ B_SPACE, kNoAction },
	{ 0, kNoAction }
};

static key_action find_action(const char* key, key_action_map* map)
{
	while( map && map->key ) {
		if( *key == map->key ) return map->action;
		map++;
	}
	return kNoAction;
}

void BChannelSlider::KeyDown(
	const char * bytes,
	int32 size)
{
	if( !IsEnabled() || IsHidden() )
		inherited::KeyDown(bytes, size);
	
	int32* new_values = 0;
	
	const key_action action =
		find_action(bytes, Vertical() ? vertical_keys:horizontal_keys);
		
	switch (action) {
		case kNextChannel:
			if( fFocusChannel < 0 ) fFocusChannel = CountChannels()-1;
			else fFocusChannel++;
			if( fFocusChannel >= CountChannels() ) fFocusChannel = 0;
			Redraw();
			break;
		case kPrevChannel:
			if( fFocusChannel < 0 ) fFocusChannel = 0;
			else fFocusChannel--;
			if( fFocusChannel < 0 ) fFocusChannel = CountChannels()-1;
			Redraw();
			break;
		case kAllChannels:
			fFocusChannel = -1;
			Redraw();
			break;
		case kBumpUp:
		case kBumpDown:
		case kJumpUp:
		case kJumpDown: {
			new_values = new int32[CountChannels()];
			for( int32 i=0; i<CountChannels(); i++ ) {
				new_values[i] = ValueList()[i];
				if( fFocusChannel < 0 || i == fFocusChannel ) {
					float span = ThumbRangeFor(i);
					if( span < 1 ) span = 1;
					int32 off = int((MaxLimitList()[i]-MinLimitList()[i])/span+.5);
					if( off <= 0 ) off = 1;
					if( action == kJumpUp || action == kJumpDown ) off *= 5;
					if( action == kBumpUp || action == kJumpUp ) new_values[i] += off;
					else new_values[i] -= off;
				}
			}
		} break;
		case kMax: {
			new_values = new int32[CountChannels()];
			for( int32 i=0; i<CountChannels(); i++ ) {
				new_values[i] = ValueList()[i];
				if( fFocusChannel < 0 || i == fFocusChannel ) {
					new_values[i] = MaxLimitList()[i];
				}
			}
		} break;
		case kMin: {
			new_values = new int32[CountChannels()];
			for( int32 i=0; i<CountChannels(); i++ ) {
				new_values[i] = ValueList()[i];
				if( fFocusChannel < 0 || i == fFocusChannel ) {
					new_values[i] = MinLimitList()[i];
				}
			}
		} break;
		default:
			inherited::KeyDown(bytes, size);
			break;
	}
	
	if( new_values ) {
		if( fFocusChannel >= 0 ) {
			SetValueFor(fFocusChannel, new_values[fFocusChannel]);
			InvokeChannel(Message(), fFocusChannel, 1);
		} else {
			SetValue(0, CountChannels(), new_values);
			InvokeChannel();
		}
		delete[] new_values;
		Redraw();
	}
}

void BChannelSlider::KeyUp(
	const char * bytes,
	int32 size)
{
	inherited::KeyUp(bytes, size);
}

void BChannelSlider::FrameResized(
	float width,
	float height)
{
	PRINT(("Frame resized.\n"));
	inherited::FrameResized(width, height);
	Invalidate(Bounds());
	//Redraw();
}

void BChannelSlider::SetFont(const BFont *font, uint32 mask)
{
	PRINT(("Setting font...\n"));
	inherited::SetFont(font, mask);
	_m_baseline = -1;
}

void BChannelSlider::MakeFocus(
	bool focusState)
{
	if( focusState && !IsFocus() ) {
		fFocusChannel = -1;
	}
	inherited::MakeFocus(focusState);
}

void BChannelSlider::SetEnabled(
	bool on)
{
	inherited::SetEnabled(on);
}

void BChannelSlider::GetPreferredSize(
	float * width,
	float * height)
{
	PRINT(("Getting preferred size...\n"));
	UpdateFontDimens();
	
	if (Vertical()) {
		float w = CountChannels() * 11 + 2 * XFRAME;
		// The preferred height -really- shouldn't be this large!!
		// However, the media kit seems to depend on it...
		*height = 150 + _m_linefeed*4;
		if (Label()) {
			float w2 = StringWidth(Label());
			if (w2 > w) {
				w = w2;
			}
		}
		if (MinLimitLabel()) {
			float w2 = StringWidth(MinLimitLabel());
			if (w2 > w) {
				w = w2;
			}
		}
		if (MaxLimitLabel()) {
			float w2 = StringWidth(MaxLimitLabel());
			if (w2 > w) {
				w = w2;
			}
		}
		*width = w;
	}
	else {
		float w = 40 + _m_linefeed*2;
		*height = CountChannels()*10+_m_linefeed*3+2*YFRAME;
		if (Label()) {
			float w2 = StringWidth(Label());
			if (w2 > w) {
				w = w2;
			}
		}
		float w2 = 0;
		{
			if (MinLimitLabel()) {
				w2 += StringWidth(MinLimitLabel());
			}
			if (MaxLimitLabel()) {
				w2 += StringWidth(MaxLimitLabel());
			}
			if (w2 > w) {
				w = w2;
			}
		}
		*width = w;
	}
}

BHandler*
BChannelSlider::ResolveSpecifier(BMessage *msg,
								 int32 index,
								 BMessage *specifier,
								 int32 form,
								 const char *property)
{
	BHandler	*target = NULL;
	BPropertyInfo	pi(prop_list);
	int32			i;

	if ((i = pi.FindMatch(msg, index, specifier, form, property)) >= 0) {
		target = this;
	} else {
		target = inherited::ResolveSpecifier(msg, index, specifier, form, property);
	}

	return target;
}

status_t
BChannelSlider::GetSupportedSuites(BMessage *data)
{
	data->AddString("suites", "suite/vnd.Be-channel-slider");
	BPropertyInfo	pi(prop_list);
	data->AddFlat("messages", &pi);
	return inherited::GetSupportedSuites(data);
}

void BChannelSlider::DrawChannel(
	BView* into,
	int32 channel,
	BRect area,
	bool pressed)
{
	BPoint p1(floor((area.left+area.right)/2),
			  floor((area.top+area.bottom)/2));
	BPoint p2(p1);
	
	if (Orientation() == B_VERTICAL) {
		p2.y += ThumbRangeFor(channel);
	} else {
		p2.x += ThumbRangeFor(channel);
	}
	
	DrawGroove(into, channel, p1, p2);
	
	if (Orientation() == B_VERTICAL) {
		p1.y += ThumbDeltaFor(channel);
	}
	else {
		p1.x += ThumbDeltaFor(channel);
	}
	
	DrawThumb(into, channel, p1, pressed);
}

void BChannelSlider::DrawGroove(
	BView* into,
	int32 /*channel*/,
	BPoint tl,
	BPoint br)
{
	// This data defines the various lines needed to make a groove
	// for a single channel in the control.
	enum which_point {
		kTL,	// The top/left point
		kBR		// The bottom/right point
	};
	struct groove_line {
		which_point p1;
		float offx1;
		float offy1;
		which_point p2;
		float offx2;
		float offy2;
		float tint;
	};
	static const groove_line grooves[] = {
		{ kTL, -2,  0, kBR, -2,  0, B_DARKEN_1_TINT },
		{ kTL, -1,  0, kBR, -1,  0, B_DARKEN_2_TINT },
		{ kTL,  0,  0, kBR,  0,  0, B_DARKEN_MAX_TINT },
		{ kTL,  1,  0, kBR,  1,  0, B_DARKEN_MAX_TINT },
		{ kTL,  2,  0, kBR,  2,  0, B_LIGHTEN_1_TINT },
		{ kTL,  3,  0, kBR,  3,  0, B_LIGHTEN_MAX_TINT },
		{ kTL, -2,  0, kTL,  3,  0, B_DARKEN_1_TINT },
		{ kTL, -1,  1, kTL,  2,  1, B_DARKEN_2_TINT },
		{ kTL,  0,  2, kTL,  1,  2, B_DARKEN_MAX_TINT },
		{ kBR, -2,  0, kBR,  3,  0, B_LIGHTEN_1_TINT },
		{ kBR, -2, -1, kBR,  2, -1, B_LIGHTEN_MAX_TINT },
		{ kBR, -1, -2, kBR,  1, -2, B_LIGHTEN_2_TINT }
	};
	
	const rgb_color base_col(ViewColor());
	const size_t N = sizeof(grooves)/sizeof(groove_line);
	const orientation o = Orientation();
	
	into->BeginLineArray(N);
	for( size_t i=0; i<N; i++ ) {
		const groove_line& g = grooves[i];
		BPoint l1(g.p1 == kTL ? tl : br);
		BPoint l2(g.p2 == kTL ? tl : br);
		if( o == B_VERTICAL ) {
			l1.x += g.offx1;
			l1.y += g.offy1;
			l2.x += g.offx2;
			l2.y += g.offy2;
		} else {
			l1.x += g.offy1;
			l1.y += g.offx1;
			l2.x += g.offy2;
			l2.y += g.offx2;
		}
		rgb_color col = tint_color(base_col,g.tint);
		if( !IsEnabled() ) col.disable(ViewColor());
		into->AddLine(l1, l2, col);
	}
	into->EndLineArray();
}

void BChannelSlider::DrawThumb(
	BView* into,
	int32 channel,
	BPoint where,
	bool pressed)
{
	into->SetDrawingMode(B_OP_ALPHA);
	into->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	const BBitmap* thumb = ThumbFor(channel, pressed);
	where.x -= thumb->Bounds().right/2;
	where.y -= thumb->Bounds().bottom/2;
	into->DrawBitmapAsync(thumb, where);
	
	BRect trect(where,
				 BPoint(where.x+thumb->Bounds().right,
						where.y+thumb->Bounds().bottom));
	
	if( pressed ) {
		into->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		rgb_color black_mix = { 0, 0, 0, 255 - (uint8)(255*2/3) };
		into->SetHighColor(black_mix);
		into->FillRect(trect);
	}
	if( IsFocus() && IsEnabled() && Window()->IsActive() ) {
		if( fFocusChannel == channel || fFocusChannel < 0 ) {
			// Draw focus indicator on current thumb.
			BRect frect(trect);
			frect.InsetBy(2,2);
			into->SetHighColor(255, 255, 255);
			into->StrokeRect(frect.OffsetByCopy(1,1));
			into->SetHighColor(NextFocusColor());
			into->StrokeRect(frect);
		}
	}
	if( !IsEnabled() ) {
		// Note that this isn't right for thumbnail bitmaps that
		// contain alpha information...  but it won't be -too-
		// bad, either.
		into->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		rgb_color col = ViewColor();
		col.disable(B_TRANSPARENT_COLOR);
		col.alpha = 255-col.alpha;
		into->SetHighColor(col);
		into->FillRect(trect);
	}
	into->SetDrawingMode(B_OP_COPY);
}

const BBitmap * BChannelSlider::ThumbFor(
	int32 channel,
	bool /*pressed*/)
{
#if 0
	if (channel == 0) {
		if (!_m_left_knob) {
			_m_left_knob = new BBitmap(BRect(0,0,8,8), B_COLOR_8_BIT);
			memcpy(_m_left_knob->Bits(), _s_slider_bits, 12*9);
		}
		return _m_left_knob;
	}
	if (channel == CountChannels()-1) {
		if (!_m_right_knob) {
			_m_right_knob = new BBitmap(BRect(0,0,8,8), B_COLOR_8_BIT);
			memcpy(_m_right_knob->Bits(), _s_slider_bits, 12*9);
		}
		return _m_right_knob;
	}
	if (!_m_mid_knob) {
		_m_mid_knob = new BBitmap(BRect(0,0,8,8), B_COLOR_8_BIT);
		memcpy(_m_mid_knob->Bits(), _s_slider_bits, 12*9);
	}
#else
	if (channel == 0) {
		if (!_m_left_knob) {
			if (Vertical()) {
				_m_left_knob = new BBitmap(BRect(0,0,11,14), B_COLOR_8_BIT);
				memcpy(_m_left_knob->Bits(), s_vertical_knob_12_15, 12*15);
			}
			else {
				_m_left_knob = new BBitmap(BRect(0,0,14,10), B_COLOR_8_BIT);
				memcpy(_m_left_knob->Bits(), s_horizontal_knob_16_11, 16*11);
			}
		}
		return _m_left_knob;
	}
	if (channel == CountChannels()-1) {
		if (!_m_right_knob) {
			if (Vertical()) {
				_m_right_knob = new BBitmap(BRect(0,0,11,14), B_COLOR_8_BIT);
				memcpy(_m_right_knob->Bits(), s_vertical_knob_12_15, 12*15);
			}
			else {
				_m_right_knob = new BBitmap(BRect(0,0,14,10), B_COLOR_8_BIT);
				memcpy(_m_right_knob->Bits(), s_horizontal_knob_16_11, 16*11);
			}
		}
		return _m_right_knob;
	}
	if (!_m_mid_knob) {
		if (Vertical()) {
			_m_mid_knob = new BBitmap(BRect(0,0,11,14), B_COLOR_8_BIT);
			memcpy(_m_mid_knob->Bits(), s_vertical_knob_12_15, 12*15);
		}
		else {
			_m_mid_knob = new BBitmap(BRect(0,0,14,10), B_COLOR_8_BIT);
			memcpy(_m_mid_knob->Bits(), s_horizontal_knob_16_11, 16*11);
		}
	}
#endif
	return _m_mid_knob;
}

BRect BChannelSlider::ThumbFrameFor(
	int32 channel)
{
	UpdateFontDimens();
	
	const BBitmap * bm = ThumbFor(channel, false);
	if (!bm) return BRect(0,0,0,0);
	
	BRect r(bm->Bounds());
	if (Vertical()) {
		r.OffsetBy(channel*11.0, 0);
		r.OffsetBy(0, _m_linefeed*2);
	}
	else {
		r.OffsetBy(0, channel*11.0);
		r.OffsetBy(_m_linefeed, _m_linefeed);
	}
	return r;
}

float BChannelSlider::ThumbDeltaFor(
	int32 channel)
{
	if (channel < 0 || channel >= CountChannels()) {
		return 0.0;
	}
	float span = ThumbRangeFor(channel);
	if (Vertical()) {
		return span - span*(ValueList()[channel]-MinLimitList()[channel])/
			(MaxLimitList()[channel]-MinLimitList()[channel]);
	}
	return span*(ValueList()[channel]-MinLimitList()[channel])/
		(MaxLimitList()[channel]-MinLimitList()[channel]);
}

float BChannelSlider::ThumbRangeFor(
	int32 channel)
{
	UpdateFontDimens();
	
	BRect b(Bounds());
	float span = 1;
	BRect r(ThumbFrameFor(channel));
	if (Vertical()) {
		span = b.Height() - 4*_m_linefeed - r.Height();
	}
	else {
		span = b.Width() - 2*_m_linefeed - r.Width();
	}
	return span;
}


bool BChannelSlider::Vertical()
{
	return _m_vertical;
}

void BChannelSlider::Redraw()
{
	if (Window()) {
		Invalidate();
		Window()->UpdateIfNeeded();
	}
}

status_t BChannelSlider::_Reserved_BChannelSlider_0(void *, ...)
{
	return B_ERROR;
}

status_t BChannelSlider::_Reserved_BChannelSlider_1(void *, ...)
{
	return B_ERROR;
}

status_t BChannelSlider::_Reserved_BChannelSlider_2(void *, ...)
{
	return B_ERROR;
}

status_t BChannelSlider::_Reserved_BChannelSlider_3(void *, ...)
{
	return B_ERROR;
}

status_t BChannelSlider::_Reserved_BChannelSlider_4(void *, ...)
{
	return B_ERROR;
}

status_t BChannelSlider::_Reserved_BChannelSlider_5(void *, ...)
{
	return B_ERROR;
}

status_t BChannelSlider::_Reserved_BChannelSlider_6(void *, ...)
{
	return B_ERROR;
}

status_t BChannelSlider::_Reserved_BChannelSlider_7(void *, ...)
{
	return B_ERROR;
}

void
BChannelSlider::UpdateFontDimens()
{
	if( _m_baseline < 0 ) {
		PRINT(("Updating font dimens...\n"));
		font_height fh;
		be_plain_font->GetHeight(&fh);
		_m_baseline = fh.ascent+fh.leading;
		_m_linefeed = fh.ascent+fh.leading+fh.descent;
	}
}

void
BChannelSlider::DrawThumbs()
{
	UpdateFontDimens();
	
	BRect a(ThumbFrameFor(0));
	BRect b(ThumbFrameFor(CountChannels()-1));
	if (Vertical())
	{
		a.right = b.right;
		a.bottom += ThumbRangeFor(0);
	}
	else
	{
		a.bottom = b.bottom;
		a.right += ThumbRangeFor(0);
	}
	a.right = floor(a.right+.5);
	a.bottom = floor(a.bottom+.5);
	
	_m_click_delta = B_ORIGIN;
	if (Vertical()) {
		_m_click_delta.x = floor((Bounds().Width()-a.Width())/2);
		_m_click_delta.y = floor((Bounds().Height()-a.Height()-_m_linefeed*3)/2);
	}
	else {
		_m_click_delta.x = floor((Bounds().Width()-a.Width()-_m_linefeed*2)/2);
		_m_click_delta.y = floor((Bounds().Height()-_m_linefeed*2-a.Height())/2);
	}
	
	a.InsetBy(-XFRAME, -YFRAME);
	
	a.OffsetBy(_m_click_delta);
	
#if 0
	DrawThumbFrame(_m_backing_view, a);
#else
	SetHighColor(ViewColor());
	FillRect(a, B_SOLID_HIGH);
	SetHighColor(HighColor());
//+	_m_backing_view->StrokeRect(a, B_SOLID_HIGH);
#endif
	a.InsetBy(XFRAME, YFRAME);
	
	for (int ix=0; ix<CountChannels(); ix++) {
		PRINT(("Drawing channel: %d\n", ix));
		BRect r(ThumbFrameFor(ix));
		r.OffsetBy(_m_click_delta);
		DrawChannel(this, ix, r,
					IsTracking() && (fAllChannels || fCurrentChannel==ix));
	}
}


void
BChannelSlider::DrawThumbFrame(
	BView * where,
	const BRect & area)
{
	rgb_color h = where->HighColor();
	rgb_color vc = where->ViewColor();
//	if (vc.red < 240) vc.red += 16; else vc.red = 255;
//	if (vc.green < 240) vc.green += 16; else vc.green = 255;
//	if (vc.blue < 240) vc.blue += 16; else vc.blue = 255;
	where->SetHighColor(scale(vc, 216, 232));
	BRect a(area.left+1, area.top+1, area.right-1, area.bottom-1);
	where->FillRect(a, B_SOLID_HIGH);
	where->SetHighColor(h);
	rgb_color hi1 = scale(vc, 216, 255);
	rgb_color lo1 = scale(vc, 216, 96);
	rgb_color hi2 = scale(vc, 216, 240);
	rgb_color lo2 = scale(vc, 216, 144);
	rgb_color hi3 = scale(vc, 216, 224);
	rgb_color lo3 = scale(vc, 216, 192);
	BRect r(area);
	where->BeginLineArray(12);
	where->AddLine(r.LeftBottom(), r.RightBottom(), hi3);
	where->AddLine(r.RightBottom(), r.RightTop(), hi3);
	where->AddLine(r.RightTop(), r.LeftTop(), lo3);
	where->AddLine(r.LeftTop(), r.LeftBottom(), lo3);
	r.InsetBy(1,1);
	where->AddLine(r.LeftBottom(), r.RightBottom(), hi2);
	where->AddLine(r.RightBottom(), r.RightTop(), hi2);
	where->AddLine(r.RightTop(), r.LeftTop(), lo2);
	where->AddLine(r.LeftTop(), r.LeftBottom(), lo2);
	r.InsetBy(1,1);
	where->AddLine(r.LeftBottom(), r.RightBottom(), hi1);
	where->AddLine(r.RightBottom(), r.RightTop(), hi1);
	where->AddLine(r.RightTop(), r.LeftTop(), lo1);
	where->AddLine(r.LeftTop(), r.LeftBottom(), lo1);
	where->EndLineArray();
}
