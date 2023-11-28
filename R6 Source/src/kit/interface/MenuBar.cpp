//*****************************************************************************
//
//	File:		MenuBar.cpp
//
//	Description:	BBox class.
//
//	Written by:	Peter Potrebic
//
//	Copyright 1994-97, Be Incorporated
//
//*****************************************************************************

#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _DEBUG_H
#include <Debug.h>
#endif
#ifndef _MENU_BAR_H
#include "MenuBar.h"
#endif
#ifndef _MENU_H
#include "Menu.h"
#endif
#ifndef _MENU_ITEM_H
#include "MenuItem.h"
#endif
#ifndef _LIST_H
#include <List.h>
#endif
#ifndef _MENU_WINDOW_H
#include "MenuWindow.h"
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef _MESSAGE_H
#include <Message.h>
#endif
#ifndef _MENU_PRIVATE_H
#include "MenuPrivate.h"
#endif
#ifndef _REGION_H
#include "Region.h"
#endif
#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _MESSAGE_UTIL_H
#include <message_util.h>
#endif
#ifndef _TOKEN_SPACE_H
#include <token.h>
#endif

#include <math.h>
#include <stdlib.h>

/* ---------------------------------------------------------------- */

BMenuBar::BMenuBar(	BRect frame,
					const char *title,
					uint32 resizeMask,
					menu_layout layout,
					bool resizeToFit)
	: BMenu(frame, title, resizeMask, B_WILL_DRAW | B_FRAME_EVENTS,
		layout, resizeToFit)
{
	InitData(layout);
}

void BMenuBar::InitData(menu_layout layout)
{
	fLastBounds = new BRect(Bounds());
	fBorder = B_BORDER_FRAME;
	if (layout != B_ITEMS_IN_MATRIX)
		SetItemMargins(menu_bar_pad.left, menu_bar_pad.top, 
					   menu_bar_pad.right, menu_bar_pad.bottom);
	fTracking = false;
	SetIgnoreHidden(true);
	fMenuSem = B_NO_MORE_SEMS;
	fPrevFocusToken = -1;
	fExtraRect = NULL;
}

/* ---------------------------------------------------------------- */

	// S_LAYOUT is also define by Menu.cpp
#define S_LAYOUT	"_layout"
#define S_BORDER	"_border"

BMenuBar::BMenuBar(BMessage *data)
	: BMenu(data)
{
	long	l;
	data->FindInt32(S_LAYOUT, &l);
	InitData((menu_layout) l);

	if (data->HasInt32(S_BORDER)) {
		data->FindInt32(S_BORDER, &l);
		SetBorder((menu_bar_border) l);
	}
}

/* ---------------------------------------------------------------- */

BArchivable *BMenuBar::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BMenuBar"))
		return NULL;
	return new BMenuBar(data);
}

/* ---------------------------------------------------------------- */

status_t BMenuBar::Archive(BMessage *data, bool deep) const
{
	BMenu::Archive(data, deep);
//+	data->AddString(B_CLASS_NAME_ENTRY, "BMenuBar");

	if (fBorder != B_BORDER_FRAME)
		data->AddInt32(S_BORDER, fBorder);
	return 0;
}

/* ---------------------------------------------------------------- */

BMenuBar::~BMenuBar()
{
//	PRINT(("D: Menu Bar\n"));
	if (fTracking) {
		/*
		oops. We're in the middle of tracking and the view is
		getting destroyed. Probably what happened is that the menubar
		was in "sticky" mode and the user closed the window via ALT-W
		*/

		status_t		err; 
		do {
			long dummy;
			err = wait_for_thread(fTrackingPID,&dummy);
		} while (err == B_INTERRUPTED);
	}
	delete fLastBounds;
}

/* ---------------------------------------------------------------- */

void BMenuBar::WindowActivated(bool state)
{
	BMenu::WindowActivated(state);
}

/* ---------------------------------------------------------------- */

void BMenuBar::MouseUp(BPoint where)
{
	BMenu::MouseUp(where);
}

/* ---------------------------------------------------------------- */

void BMenuBar::MouseDown(BPoint)
{
//+	PRINT(("MouseDown (tracking=%d) (class=%s)\n",
//+		fTracking, class_name(this)));

	if (fTracking) {
		return;
	}
	
	if (!Window()->IsActive() || !Window()->IsFront()) {
		/*
		If we're here then the window holding the menubar accepts
		first clicks. Need to Activate the window and do an update
		so that the window redraw once it comes to the front.
		*/
		Window()->Activate(true);
		Window()->UpdateIfNeeded();
	}
	
	SetMouseEventMask(0, B_LOCK_WINDOW_FOCUS|B_SUSPEND_VIEW_FOCUS|B_NO_POINTER_HISTORY);
	fKeyPressed = false;
	StartMenuBar(-1, false);
}

/* ---------------------------------------------------------------- */

void BMenuBar::Show()
{
	BView::Show();	// don't call BMenu::Show()
}

/* ---------------------------------------------------------------- */

void BMenuBar::Hide()
{
	BView::Hide();	// don't call BMenu::Hide()
}

/* ---------------------------------------------------------------- */

struct trackData {
	BMenuBar	*menuBar;
	long		startIndex;
	bool		sticky;
	bool		show_menu;
	bool		special_flag;
	BRect		special_rect;
};

void BMenuBar::StartMenuBar(int32 menuIndex, bool sticky, bool show_menu,
	BRect *special_rect)
{
	if (fTracking) {
		PRINT(("Can't have concurrent menu tracking sessions.\n"));
		return;
	}
	if (!Window()) {
		debugger("MenuBar must be added to a window before it can be used.");
	}

	if (Window()->Lock()) {
		fTracking = true;
		fPrevFocusToken = -1;

		trackData	d;

		d.menuBar = this;
		d.startIndex = menuIndex;
		d.sticky = sticky;
		d.show_menu = show_menu;
		if (special_rect) {
			d.special_flag = true;
			d.special_rect = *special_rect;
		} else
			d.special_flag = false;
		
		Window()->MenusBeginning();

		// Set up fMenuSem. This semaphore is used to prevent the owning
		// Window from closing while in menu tracking mode.
		fMenuSem = create_sem(0, "window close lock");
		_set_menu_sem_(Window(), fMenuSem);

		fTrackingPID = spawn_thread(BMenuBar::TrackTask, "_Track_",
			B_DISPLAY_PRIORITY, NULL);
		if (fTrackingPID >= 0) {
			send_data(fTrackingPID, 0, (char *) (&d), sizeof(trackData));
			resume_thread(fTrackingPID);
		} else {
			// if we didn't get a valid pid there's little we can do...

			// we can clear out the fMenuSem semaphore.
			_set_menu_sem_(Window(), B_NO_MORE_SEMS);
			delete_sem(fMenuSem);
			fMenuSem = B_NO_MORE_SEMS;
		}

		Window()->Unlock();
	}
}

/* ---------------------------------------------------------------- */
bigtime_t	_gtrack_start_time_;
bool		_gstartup_phase_;

long BMenuBar::TrackTask(void *)
{
	thread_id	pid;
	BMenuBar	*mb;
	trackData	data;
	int32		action;
	BWindow		*w;
	status_t	err;

	// get info from spawner
	receive_data(&pid, (char *) (&data), sizeof(data));

	_gtrack_start_time_ = system_time();
	_gstartup_phase_ = true;

	mb = (BMenuBar *) data.menuBar;
	w = mb->Window();
	mb->SetStickyMode(data.sticky);
	mb->fExtraRect = data.special_flag ? &data.special_rect : NULL;

	mb->Track(&action, data.startIndex, data.show_menu);

	mb->fTracking = false;
	mb->fChosenItem = NULL;

	err = w->PostMessage(_MENUS_DONE_);
	// clear out the fMenuSem. The owning Window is free to Close at will
	_set_menu_sem_(w, B_NO_MORE_SEMS);
	delete_sem(mb->fMenuSem);
	mb->fMenuSem = B_NO_MORE_SEMS;

	while (err == B_WOULD_BLOCK) {
		// If window port was full try resending.
		err = w->PostMessage(_MENUS_DONE_);
		snooze(50000);
	}
		

	return B_NO_ERROR;
}

/* ---------------------------------------------------------------- */

BMenuItem *BMenuBar::Track(int32 *returnAction, int32 startIndex, bool showMenu)
{
//+	PRINT(("BMenuBar::Track(sticky=%d)\n", IsStickyMode()));
	ASSERT((Window()));
	BWindow		*w = Window();

	BPoint		loc;
	ulong		buttons;
	BRect		bounds;
	BMenuItem	*item;
	BMenuItem	*executeItem = NULL;
	bool		startedSticky = false;
	bool		startedNotSticky = false;
	int			action;
	BPoint		initialLoc;
	BPoint		globalLoc;
	BPoint		prevLoc(-1, -1);
	long		snoozeTime;
	uint32		iworkspace = current_workspace();
	BRect		windowBounds = w->Bounds();
	bigtime_t	lock_timeout = 2000000;

	/*
	 All this LockWithTimeout() stuff is to attempt to deal with bug #11809
	 It helps in some cases, but not in all - example, when menu nav'ing
	 in sticky mode i've still hit a deadlock
	*/
	if (w->LockWithTimeout(lock_timeout) != B_OK)
		return NULL;

	bounds = Bounds();

	/*
	Intersect with window bounds to "clip" off any part of menubar outside
	the window.
	*/
//+	PRINT_OBJECT(Frame());
//+	PRINT_OBJECT(bounds);
//+	PRINT_OBJECT(windowBounds);
	windowBounds = ConvertFromParent(windowBounds);
//+	PRINT(("New ")); PRINT_OBJECT(windowBounds);
	bounds = bounds & windowBounds;
//+	PRINT(("New ")); PRINT_OBJECT(bounds);

	w->Unlock();

	fState = NOW_TRACKING;
	
	if (startIndex >= 0) {
		if (IsStickyMode()) {
			fKeyPressed = true;
			be_app->ObscureCursor();
		}
		if (w->LockWithTimeout(lock_timeout) != B_OK)
			return NULL;
		DrawItems(bounds);
		SelectItem(ItemAt(startIndex),
			showMenu ? SHOW_MENU : DONT_SHOW_MENU);
		w->Unlock();
	}
	
	bool		first_time = true;
	bigtime_t	click_speed;
	extern bigtime_t	_gtrack_start_time_;
	extern bool			_gstartup_phase_;
	get_click_speed(&click_speed);

//+	int ii = 0;

	for (; true; snooze(snoozeTime*1000)) {
		snoozeTime = 5;

//+		if ((++ii % 400) == 0)
//+			PRINT(("BMenuBar:Track(%d)\n", ii));

		if (w->LockWithTimeout(lock_timeout) != B_OK) {
			goto abort;
		}

		// Need to deal with current workspace changing out from under
		// this menu tracking session. If that happens then abort!
		uint32 cworkspace = current_workspace();
		if (cworkspace != iworkspace) {
//+			PRINT(("current=%x, initial=%x\n", (1<<cworkspace), iworkspace));
			item = NULL;
			break;
		}

		item = CurrentSelection();
		/*
		Check to see if the menu being shown has "completed". This
		means that some item was choosen - likely via the keyboard.
		If this case we need to exit the menu tracking code.
		*/
		if (item && item->Submenu() &&
			(action = item->Submenu()->State(&executeItem)) != 0) {
				if (action == EXIT_TRACKING) {
					break;
				} else if (action == SELECT_PREV || action == SELECT_NEXT) {
					bool r = SelectNextItem(item, (action == SELECT_NEXT));
					// in case there was nothing to select... This is a
					// bizzare/edge case. Reset the visible menu's state.
					if (!r)
						item->Submenu()->fState = NOW_TRACKING;
					goto bottom;
				}
		}

		if (fState == EXIT_TRACKING) {
			if (fChosenItem)
				executeItem = fChosenItem;
			break;
		}
		
// why was this here?
//		MakeFocus(true);

		GetMouse(&loc, &buttons);
		globalLoc = loc;
		ConvertToScreen(&globalLoc);

		if (first_time)
			initialLoc = globalLoc;

//+		PRINT(("mouse location:"));
//+		PRINT(("\t")); PRINT_OBJ(loc);
//+		PRINT(("\t")); PRINT_OBJ(globalLoc);
		
		// if we're in sticky mode and now some mouse buttons are pressed...
		if (IsStickyMode() && buttons) {
			if (bounds.Contains(loc)
//+				|| (fExtraRect && fExtraRect->Contains(loc))
				) {
				SetStickyMode(false);		// get out of sticky mode
				startedNotSticky = true;
				RedrawAfterSticky(bounds);
				SetMouseEventMask(0, B_LOCK_WINDOW_FOCUS|B_SUSPEND_VIEW_FOCUS|B_NO_POINTER_HISTORY);
			} else {
				break;						// if mouse is outside - exit
			}
		}

		// if we're not in sticky mode and NO buttons are down..
		if (!IsStickyMode() && !buttons) {
			// Determine where the mouse is now that no buttons are down
			if ((item = HitTestItems(loc)) != 0) {
				// over some item... if Sticky mode is allowed and
				// if that item has a menu then enter sticky mode.
				if (IsStickyPrefOn() && item->Submenu()) {
					SetStickyMode(true);
					startedSticky = true;
					RedrawAfterSticky(bounds);
				} else {
					// otherwise the user selected some item in the menubar
					executeItem = item;
					break;
				}
			} else if (fExtraRect && fExtraRect->Contains(loc) && IsStickyPrefOn()) {
				// mouse was release of the 'extra' area around the menubar.
				// this means we should go to sticky mode
				SetStickyMode(true);
				startedSticky = true;
				RedrawAfterSticky(bounds);
			} else {
				// not over any item so exit menu tracking
				break;
			}
		}

		if (fKeyPressed && pt_distance(globalLoc, prevLoc) <= 1)
			goto bottom;
		fKeyPressed = false;
		
		item = CurrentSelection();

		{
		// "Slop" for dragging over to a submenu... If the mouse

		BMenuItem	*nItem;
		BMenu		*sm;
		if (item && (sm = item->Submenu()) != 0 && (sm->Window()) && 
			(globalLoc != prevLoc) &&
			((nItem = HitTestItems(loc, BPoint(-1,0))) != item)) {

			BRect mr = sm->Window()->Frame();

			float cur = pt_distance(globalLoc, mr);
			float prev = pt_distance(prevLoc, mr);

//+			PRINT(("(mb) cur=%.2f, prev=%.2f\n", cur, prev));
			// If the mouse is moving in the correct direction, and "fast"
			// enough, then give it a chance to get there.
			if (cur < (prev)) {
				// mouse is going towards the menu
//+				PRINT(("	slop\n"));
				snoozeTime = 50;
				goto bottom;
			}
		}
		}
			
		if (item && OverSubmenu(item, globalLoc)) {
			ASSERT((item->Submenu()));
			// start tracking the menu. Break if some item was selected
			w->Unlock();
//+			PRINT(("going to submenu track\n"));
			executeItem = item->Submenu()->_track(&action);
			if (w->LockWithTimeout(lock_timeout) != B_OK) {
				goto abort;
			}
			if (action == EXIT_TRACKING) {
				bool	really_exit = true;
				if (IsStickyPrefOn() && fExtraRect) {
					BPoint	pt;
					ulong	bs;
					GetMouse(&pt, &bs);
					// If the Mouse is now over the 'extra' area then go into
					// sticky mode
					if (_gstartup_phase_ && fExtraRect->Contains(pt)) {
						really_exit = false;
						SetStickyMode(true);
						SelectItem(ItemAt(0), SHOW_MENU);
						RedrawAfterSticky(bounds);
					}
				}
				if (really_exit) {
					break;
				}
			} else if (action == SELECT_PREV || action == SELECT_NEXT) {
				SelectNextItem(item, (action == SELECT_NEXT));
			}
		} else if (bounds.Contains(loc)) {
			if (!IsStickyMode() || (IsStickyMode() && startedSticky)) {
					item = HitTestItems(loc);
//+					PRINT(("Selecting item (%x) %s\n",
//+						item, item ? item->Label() : "null"));
					if (startedNotSticky) {
						if (item == fSelected)
							break;
						startedNotSticky = false;
					}
					SelectItem(item, SHOW_MENU);
					if (IsStickyMode() && startedSticky) {
						startedSticky = false;
				}
			}
		} 

bottom:		

		if (_gstartup_phase_) {
			bigtime_t	cur_time = system_time();
			if ((cur_time > _gtrack_start_time_ + click_speed) ||
				(abs((int) (initialLoc.x - globalLoc.x)) +
				 abs((int) (initialLoc.y - globalLoc.y)) > 2)) {
//+				PRINT(("startup phase OVER\n"));
				_gstartup_phase_ = false;
			}
		}
		first_time = false;

		prevLoc = globalLoc;
		w->Unlock();
	}
	SelectItem(NULL);
	
	DeleteMenuWindow();
	
	if (IsStickyMode()) {
		SetStickyMode(false);			// get out of sticky mode
		RedrawAfterSticky(bounds);
	}

	RestoreFocus();

	if (executeItem) {
		executeItem->Invoke();
	}

	fState = IDLE_MENU;
	fKeyPressed = false;
	
	w->Unlock();

	// in case cursor was hidden because of key navigating menus.
	be_app->ShowCursor();

//	PRINT(("BMenuBar::Track() - DONE\n"));
	*returnAction = EXIT_TRACKING;
	return NULL;

abort:
//+	SelectItem(NULL);
	if (fSelected) {
		BMenu *m = fSelected->Submenu();
		if (m && m->Window()) {
			m->_hide();
		}
		fSelected = NULL;
	}
	DeleteMenuWindow();
	fState = IDLE_MENU;
	fStickyMode = false;
	*returnAction = EXIT_TRACKING;
	return NULL;
}

/* ---------------------------------------------------------------- */

void
BMenuBar::StealFocus()
{
	if (fPrevFocusToken != -1)
		return;

	BWindow *window = Window();
	
	if (window == NULL)
		return;

	if (!window->Lock())
		return;

	BView *focus = window->CurrentFocus();
	if (focus != NULL)
		fPrevFocusToken = _get_object_token_(focus);

	MakeFocusNoScroll(true);

	window->Unlock();	
}

/* ---------------------------------------------------------------- */

void BMenuBar::RestoreFocus()
{
//+	PRINT(("(%d) restoring focus (token=%d)\n", find_thread(NULL), fPrevFocusToken));
	BView *cf = Window()->CurrentFocus();
	if (fPrevFocusToken != -1) {
		BView *v = NULL;
		gDefaultTokens->GetToken(fPrevFocusToken, HANDLER_TOKEN_TYPE, (void **) &v);
		if (v && (v->Window() == this->Window())) {
			v->MakeFocusNoScroll(true);
		}
	} else {
		if (IsFocus())
			MakeFocusNoScroll(false);
	}
	fPrevFocusToken = -1;
	cf = Window()->CurrentFocus();
//+	PRINT(("(%d) done restoring focus\n", find_thread(NULL)));
}

/* ---------------------------------------------------------------- */

void BMenuBar::AttachedToWindow()
{
	Install(Window());

	Window()->SetKeyMenuBar(this);

	BMenu::AttachedToWindow();
}

/* ---------------------------------------------------------------- */

void BMenuBar::Draw(BRect updateRect)
{
	BMenuItem	*item;
	BRect		bounds;
	BRect		b;
	bool		enabled = IsEnabled();

	if (!fUseCachedMenuLayout) {
		LayoutItems(0);
		Sync();
		Invalidate();	//Do we really want to comment this out?  HL.  No.
		return;			//Do we really want to comment this out?  HL.  No.
	}

//+	PRINT(("Draw: "));	PRINT_OBJECT(Bounds());
//+	PRINT(("\t"));		PRINT_OBJECT(updateRect);

	bounds = Bounds();
	b = bounds;
	
	/*
	BMenuBar draws some sort of frame around the menubar. This frame
	takes 1 pixel on the top, left, and right sides and 2 pixels on the bottom.
	*/
	if( fBorder != B_BORDER_OFF ) {
		b.InsetBy(1,1);
		b.bottom -= 1;
	}
	const rgb_color background = ViewColor();
	const rgb_color low = LowColor();
	const rgb_color high = HighColor();
	SetHighColor(LowColor());
	FillRect(b & updateRect);
	SetHighColor(tint_color(background, cDARKEN_3));

	if (fBorder == B_BORDER_EACH_ITEM) {
		long	i;
		long	c = CountItems();
		long	last = c-1;
		BRect	b;
		bool	each = (fBorder & B_BORDER_EACH_ITEM) != 0;
		for (i=0; i<c; i++) {
			item = ItemAt(i);
			ASSERT((item));
			b = item->Frame();

			if (!b.Intersects(updateRect))
				continue;

			SetHighColor(tint_color(background, cLIGHTEN_2));
			if (each || (i == 0))
				StrokeLine(b.LeftBottom(), b.LeftTop());
			StrokeLine(b.LeftTop(), b.RightTop());

			SetHighColor(tint_color(background, cDARKEN_4));
			if (each || (i == last))
				StrokeLine(b.RightTop(), b.RightBottom());
			StrokeLine(b.RightBottom(), b.LeftBottom());
			
			b.bottom -= 1;
			b.right -= 1;

			SetHighColor(tint_color(background, cDARKEN_1));
			if (each || (i == last))
				StrokeLine(b.RightTop(), b.RightBottom());
			if (each)
				StrokeLine(b.RightBottom(), b.LeftBottom());
			else
				StrokeLine(b.RightBottom() + BPoint(0,0), b.LeftBottom());

			SetHighColor(sMenuInfo.background_color);
			if (each || (i == 0))
				StrokeLine(b.LeftBottom(), b.LeftBottom());
			if (each || (i == last))
				StrokeLine(b.RightTop(), b.RightTop());
		}
	} else if (fBorder == B_BORDER_CONTENTS) {
			BRect		b = bounds;
			rgb_color	c;

			BeginLineArray(6);
			c = tint_color(background, cLIGHTEN_2);
			AddLine(b.LeftBottom(), b.LeftTop(), c);
			AddLine(b.LeftTop(), b.RightTop(), c);

			if (enabled)
				c = tint_color(background, cDARKEN_4);
			else
				c = tint_color(background, cDARKEN_2);
			AddLine(b.RightTop(), b.RightBottom(), c);
			AddLine(b.RightBottom(), b.LeftBottom(), c);
			
			b.bottom -= 1;
			b.right -= 1;

			if (enabled)
				c = tint_color(background, cDARKEN_1);
			else
				c = ui_color(B_MENU_BACKGROUND_COLOR);
			AddLine(b.RightTop(), b.RightBottom(), c);
			AddLine(b.RightBottom(), b.LeftBottom() + BPoint(1,0), c);
			EndLineArray();

	} else if (fBorder == B_BORDER_FRAME) {
		/*
		one white line across the top and down the left side
		one gray line across the bottom and down the right side
		one lt_gray line across the bottom-1
		one lt_gray pixel at the LeftBottom point
		*/

//+		PRINT(("B_BORDER_FRAME (%s), mode=%d\n", Name(), DrawingMode()));
//+		PRINT_OBJECT(updateRect);
//+		PRINT_OBJECT(bounds);

		SetHighColor(tint_color(background, cLIGHTEN_2));
		StrokeLine(bounds.LeftBottom(), bounds.LeftTop());
		StrokeLine(bounds.LeftTop(), bounds.RightTop());

		SetHighColor(tint_color(background, cDARKEN_2));
		StrokeLine(bounds.RightTop(), bounds.RightBottom());
		StrokeLine(bounds.RightBottom(), bounds.LeftBottom());
		
		BRect b = bounds;
		b.bottom -= 1;
		b.right -= 1;

		SetHighColor(tint_color(background, cDARKEN_1));
		StrokeLine(b.RightBottom(), b.LeftBottom());

		SetHighColor(sMenuInfo.background_color);
		StrokeLine(b.LeftBottom(), b.LeftBottom());
	} else if (fBorder == B_BORDER_OFF) {
		// don't draw any border
	}

	SetHighColor(IsEnabled() ? high : high.disable_copy(LowColor()));
	DrawItems(updateRect);
	SetLowColor(low);
	SetHighColor(high);
}

/* ---------------------------------------------------------------- */

menu_bar_border BMenuBar::Border() const
{
	return fBorder;
}

/* ---------------------------------------------------------------- */

void BMenuBar::SetBorder(menu_bar_border border)
{
	fBorder = border;
}

/* ---------------------------------------------------------------- */

void BMenuBar::MessageReceived(BMessage *msg)
{
	BMenu::MessageReceived(msg);
}

/*---------------------------------------------------------------*/

BHandler *BMenuBar::ResolveSpecifier(BMessage *msg, int32 index,
	BMessage *spec, int32 form, const char *prop)
{
	return BMenu::ResolveSpecifier(msg, index, spec, form, prop);
}

/*---------------------------------------------------------------*/

status_t	BMenuBar::GetSupportedSuites(BMessage *data)
{
	return BMenu::GetSupportedSuites(data);
}

/*----------------------------------------------------------------*/

status_t BMenuBar::Perform(perform_code d, void *arg)
{
	return BMenu::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

void BMenuBar::DetachedFromWindow()
{
	BMenu::DetachedFromWindow();
}

/*---------------------------------------------------------------*/

void	BMenuBar::FrameMoved(BPoint new_position)
{
	BMenu::FrameMoved(new_position);
}

/*---------------------------------------------------------------*/

void	BMenuBar::FrameResized(float new_width, float new_height)
{
	const BRect bounds(Bounds());
	if( Window() ) {
		// Invalidate any currently visible parts of the frame
		// that need to be redraw to look correct at this new size.
		if( bounds.Width() < fLastBounds->Width() ) {
			Invalidate(BRect(bounds.right-1, bounds.top,
							 bounds.right, bounds.bottom));
		} else if( bounds.Width() > fLastBounds->Width() ) {
			Invalidate(BRect(fLastBounds->right-1, bounds.top,
							 fLastBounds->right, bounds.bottom));
		}
		if( bounds.Height() < fLastBounds->Height() ) {
			Invalidate(BRect(bounds.left, bounds.bottom-1,
							 bounds.right, bounds.bottom));
		} else if( bounds.Height() > fLastBounds->Height() ) {
			Invalidate(BRect(bounds.left, fLastBounds->bottom-1,
							 bounds.right, fLastBounds->bottom));
		}
	}
	*fLastBounds = bounds;
	BMenu::FrameResized(new_width, new_height);
}

/*---------------------------------------------------------------*/

void BMenuBar::ResizeToPreferred()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BMenu::ResizeToPreferred();
}

/*---------------------------------------------------------------*/

void BMenuBar::GetPreferredSize(float *width, float *height)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BMenu::GetPreferredSize(width, height);
}

/*---------------------------------------------------------------*/

void BMenuBar::MakeFocus(bool state)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BMenu::MakeFocus(state);
}

/*---------------------------------------------------------------*/

void BMenuBar::AllAttached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BMenu::AllAttached();
}

/*---------------------------------------------------------------*/

void BMenuBar::AllDetached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BMenu::AllDetached();
}

/* ---------------------------------------------------------------- */

BMenuBar &BMenuBar::operator=(const BMenuBar &) {return *this;}

/* ---------------------------------------------------------------- */

void BMenuBar::_ReservedMenuBar1() {}
void BMenuBar::_ReservedMenuBar2() {}
void BMenuBar::_ReservedMenuBar3() {}
void BMenuBar::_ReservedMenuBar4() {}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
