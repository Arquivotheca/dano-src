//*****************************************************************************
//
//	File:		Menu.cpp
//
//	Description:	BMenu class.
//
//	Written by:	Peter Potrebic
//
//	Copyright 1994-99, Be Incorporated
//
//*****************************************************************************


#include <Application.h>
#include <Autolock.h>
#include <Debug.h>
#include <OS.h>
#include <PropertyInfo.h>
#include <Region.h>
#include <Screen.h>
#include <StopWatch.h>
#include <View.h>
#include <Window.h>

#include <algorithm>
#include <archive_defs.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "Menu.h"
#include "MenuBar.h"
#include "MenuItem.h"
#include "MenuPrivate.h"
#include "MenuWindow.h"
#include "PopUpMenu.h"

#include "interface_misc.h"


bool BMenu::sSwapped = false;

// For clipping.
static const rgb_color kDummyColor = {0, 0, 0, 255};

/* ---------------------------------------------------------------- */

struct _ExtraMenuData_ {
	menu_tracking_hook trackingHook;
	void *trackingHookState;
};

/* ---------------------------------------------------------------- */

BMenu::BMenu(const char *title, menu_layout layout)
	: BView(BRect(0,0,0,0), title, B_FOLLOW_NONE, B_WILL_DRAW),
	  fItems(10)
{
	InitData();

	fLayout = layout;
	fResizeToFit = true;
	
	fPad = standard_pad;
}

BMenu::BMenu(const char *title, float width, float height)
	: BView(BRect(0, 0, width, height), title, B_FOLLOW_NONE,
		B_WILL_DRAW),
	  fItems(10)
{
	InitData();

	fLayout = B_ITEMS_IN_MATRIX;
	fResizeToFit = false;

	fInitMatrixSize = new BPoint(width, height);
	
	fPad.Set(0,0,0,0);
}

/* ---------------------------------------------------------------- */

BMenu::BMenu(BRect frame, const char *viewName, uint32 resizeMask,
	uint32 flags, menu_layout layout, bool resizeToFit)
	: BView(frame, viewName, resizeMask, flags),
	  fItems(10)
{
	InitData();
	fLayout = layout;
	fResizeToFit = resizeToFit;

	if (layout == B_ITEMS_IN_MATRIX) {
		// always turn off the resizing flag in this case
		fResizeToFit = false;
		fPad.Set(0,0,0,0);
	} else
		fPad = standard_pad;
}

/* ---------------------------------------------------------------- */

BMenu::BMenu(BMessage *data)
	: BView(data)
{
	long	l;
	bool	b;

	InitData(data);

	data->FindInt32(S_LAYOUT, &l);
	fLayout = (menu_layout) l;
	fResizeToFit = data->FindBool(S_RESIZE_TO_FIT);

	if (fLayout == B_ITEMS_IN_MATRIX)
		fPad.Set(0,0,0,0);
	else
		fPad = standard_pad;

	data->FindBool(S_DISABLED, &b);
	SetEnabled(!b);

	data->FindBool(S_DISABLE_TRIGGERS, &b);
	SetTriggersEnabled(!b);

	data->FindBool(S_RADIO_MODE, &b);
	SetRadioMode(b);

	data->FindBool(S_LABEL_FROM_MARKED, &b);
	SetLabelFromMarked(b);

	fMaxContentWidth = 0;
	data->FindFloat("_maxwidth", &fMaxContentWidth);

	BPoint	pt;
	if ((fLayout == B_ITEMS_IN_MATRIX) &&
		(data->FindPoint("_be:init_size", &pt) == B_OK)) {
			fInitMatrixSize = new BPoint(pt);
	}

	BMessage archive;
	for (int32 i = 0; data->FindMessage(S_MENU_ITEMS, i, &archive) == B_OK; i++) {
		BArchivable *obj = instantiate_object(&archive);
		if (!obj) {
			PRINT(("failed to instantiate menu item\n"));
#if DEBUG
			archive.PrintToStream();
#endif
			continue;
		}
		BMenuItem *item = dynamic_cast<BMenuItem *>(obj);
		if (!item)
			continue;
		if (fLayout == B_ITEMS_IN_MATRIX) {
			BRect rect;
			data->FindRect(S_ITEM_FRAMES, i, &rect);
			AddItem(item, rect);
		} else 
			AddItem(item);
	}
}

/* ---------------------------------------------------------------- */

BArchivable *BMenu::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BMenu"))
		return NULL;
	return new BMenu(data);
}

/* ---------------------------------------------------------------- */

status_t BMenu::Archive(BMessage *data, bool deep) const
{
	BView::Archive(data, deep);

	if (((long) fLayout) != 0)
		data->AddInt32(S_LAYOUT, fLayout);
	if (fResizeToFit)
		data->AddBool(S_RESIZE_TO_FIT, true);
	if (!fEnabled)
		data->AddBool(S_DISABLED, true);
	if (fRadioMode)
		data->AddBool(S_RADIO_MODE, true);
	if (!fTriggerEnabled)
		data->AddBool(S_DISABLE_TRIGGERS, true);
	if (fDynamicName)
		data->AddBool(S_LABEL_FROM_MARKED, true);
	if (fMaxContentWidth > 0)
		data->AddFloat("_maxwidth", fMaxContentWidth);

	if ((fLayout == B_ITEMS_IN_MATRIX) && fInitMatrixSize)
		data->AddPoint("_be:init_size", *fInitMatrixSize);

	// now archive all the menu items
	if (deep) {
		BMenuItem	*item;
		long		i = 0;
		long		err;
		while ((item = ItemAt(i++)) != 0) {
			BMessage	archive;
			err = item->Archive(&archive, true);
			if (!err) {
				data->AddMessage(S_MENU_ITEMS, &archive);
				if (fLayout == B_ITEMS_IN_MATRIX)
					// must also save away the location of the item
					data->AddRect(S_ITEM_FRAMES, item->Frame());
			}
		}
	}
	return 0;
}

/* ---------------------------------------------------------------- */

BMenu::p_menu_info	BMenu::sMenuInfo;

// This is -only- here for the Deskbar, for it to implement its
// spring-loaded menus.  Yuck.
menu_info *_menu_info_ptr_ = NULL;

void BMenu::InitData(BMessage *data)
{
	/*
	Each BMenu object potentially needs it own popup window.
	For BMenuBars it is obvious - for the menus. But BMenu's
	themselves need a window for heirarchical menus.
	*/
	fCachedMenuWindow = NULL;

	/*
	The layout of items within the MenuBase are calculated once
	and then reused each time the object is shown (i.e. each time
	a menu is displayed). When something happens that changes the
	way a menu would drawn the location cache is invalidated.
	*/
	fUseCachedMenuLayout = false;
	
	fEnabled = true;

	fSelected = NULL;
	fSuper = NULL;
	fSuperitem = NULL;

	fAscent = -1.0;
	fDescent = -1.0;
	fFontHeight = -1.0;
	fMaxContentWidth = 0;
	fInitMatrixSize = NULL;

	fChosenItem = NULL;
	fState = IDLE_MENU;
	fDynamicName = false;
	fRadioMode = false;
	fTrigger = 0;
	fStickyMode = false;
	fIgnoreHidden = false;
	fTriggerEnabled = true;
	fRedrawAfterSticky = false;
	fExtraRect = NULL;
	fAttachAborted = false;
	fKeyPressed = false;
	fSkipSnakeDraw = false;
	
	BFont aFont(*be_plain_font);
	
	uint32 mask = B_FONT_SPACING;
	//aFont.SetSpacing(B_STRING_SPACING);

	if (!data || !data->HasString(S_FONT_FAMILY_STYLE)) {
		aFont.SetFamilyAndStyle(sMenuInfo.f_family, sMenuInfo.f_style);
		mask |= B_FONT_FAMILY_AND_STYLE;
	}
	if (!data || !data->HasFloat(S_FONT_FLOATS)) {
		aFont.SetSize(sMenuInfo.font_size);
		mask |= B_FONT_SIZE;
	}

	BView::SetFont(&aFont, mask);

	if (!data || !data->HasInt32(S_COLORS)) {
		SetLowUIColor(B_UI_MENU_BACKGROUND_COLOR);
		SetViewUIColor(B_UI_MENU_BACKGROUND_COLOR);
		SetHighUIColor(B_UI_MENU_ITEM_TEXT_COLOR);
	}
	
	fExtraMenuData = NULL;
}

/* ---------------------------------------------------------------- */

BMenu::~BMenu()
{
	if (fInitMatrixSize) {
		delete fInitMatrixSize;
		fInitMatrixSize = NULL;
	}
	
	DeleteMenuWindow();

	int i;
	int	c = fItems.CountItems();
	
	// delete all the items in the list
	for (i=0; i<c; i++)
		delete((BMenuItem *) fItems.ItemAt(i));
		
	delete fExtraMenuData;
}

/* ---------------------------------------------------------------- */

void BMenu::Install(BWindow *target)
{
	// Loop through and Install all the items..
	int32 c = CountItems();
	for (int32 i = 0; i < c; i++) 
		ItemAt(i)->Install(target);
}

/* ---------------------------------------------------------------- */

void BMenu::Uninstall()
{
	// Loop through and Uninstall all the items..
	int32 c = CountItems();
	for (int32 i = 0; i < c; i++) 
		ItemAt(i)->Uninstall();
}

/* ---------------------------------------------------------------- */

status_t BMenu::SetTargetForItems(BHandler *target)
{
	int32 c = CountItems();
	for (int32 i = 0; i < c; i++) {
		status_t result = ItemAt(i)->SetTarget(target);
		if (result != B_OK)
			return result;
	}
	return B_OK;
}

/* ---------------------------------------------------------------- */

status_t BMenu::SetTargetForItems(BMessenger messenger)
{
	int32 c = CountItems();
	for (int32 i = 0; i < c; i++) {
		status_t result = ItemAt(i)->SetTarget(messenger);
		if (result != B_OK)
			return result;
	}
	return B_OK;
}

/* ---------------------------------------------------------------- */

void BMenu::SelectItem(BMenuItem *item, uint32 showMenu, bool selectFirstItem)
{

	if (fSelected == item) {
		BMenu *submenu = NULL;
		if (item)
			submenu = item->Submenu();

		if (submenu) {
			/*
			 we're re-selecting the selected menu. So make sure it is in
			 the proper state.
			*/
			submenu->fState = IDLE_MENU;

			if (showMenu == SHOW_MENU && !submenu->Window()
				&& submenu->_show(selectFirstItem)) 
				submenu->Window()->Activate(true);	
			
			if (showMenu == DONT_SHOW_MENU && submenu->Window()) {
				submenu->_hide();

				// hiroshi, 11/4/97
				BWindow *thisWin = Window();
				if (thisWin != NULL && dynamic_cast<BMenuWindow *>(thisWin) != NULL) 
					thisWin->Activate(true);
			}
		}
		return;
	}
	
	bool prevVisible = false;
	BMenuItem *oldItem = fSelected;
	fSelected = item;
	fDrawItemClipWindow = true;
		// snake drawing - 
		// make sure that the next time an item is drawn the window gets clipped

	
	if (oldItem) {
		BMenu *submenu = oldItem->Submenu();
		if (submenu  && submenu->Window()) {
			prevVisible = true;
			submenu->_hide();
		}
		oldItem->Select(false);
		
		if (oldItem->IsEnabled())
			// snake drawing - 
			// no need to clip window during the next draw in
			// fSelected->Select(..., we already took care of it
			fDrawItemClipWindow = false;
	}
	if (fSelected) {
		ScrollIntoView(fSelected);
		fSelected->Select(true);
		BMenu *submenu = fSelected->Submenu();
		if (submenu && (showMenu == SHOW_MENU
				|| (showMenu == KEEP_SAME && prevVisible))
			&& submenu->_show(selectFirstItem)) 
			submenu->Window()->Activate(true);
	}

	BMenuWindow *menuWindow = dynamic_cast<BMenuWindow *>(Window());
	if (menuWindow)
		menuWindow->UpdateScrollers();
}

/* ---------------------------------------------------------------- */

void BMenu::ScrollIntoView(BMenuItem *item)
{
	BRect itemFrame(item->Frame());
	BRect bounds(Bounds());
	float amount = 0;

	if (bounds.Contains(itemFrame))
		return;
	
	// the given item isn't 100% visible, so scroll it into view.
	if (itemFrame.top < bounds.top)						// top is off the top of item
		amount = itemFrame.top - bounds.top;			// scroll up
	else if (itemFrame.bottom > bounds.bottom)			// bottom is off the bottom
		amount = itemFrame.bottom - bounds.bottom;		// scroll down
	
	ScrollBy(0, amount);

	fTrackNewBounds = true;
}

/* ---------------------------------------------------------------- */

void BMenu::InvalidateLayout()
{
	fUseCachedMenuLayout = false;
	fAscent = -1.0;
}

/* ---------------------------------------------------------------- */

BMenuItem *BMenu::CurrentSelection() const
{
	return fSelected;
}


/* ---------------------------------------------------------------- */

bool BMenu::AddItem(BMenuItem *item)
{
	return AddItem(item, CountItems());
}

/* ---------------------------------------------------------------- */

bool BMenu::AddList(BList *list, int32 index)
{
	if (fLayout == B_ITEMS_IN_MATRIX) {
		debugger("Error - have to manually give location for items in custom menus");
		return false;
	}

	bool dirty = false;
	bool window_locked = LockLooper();

	int32 count = list->CountItems();
	for (int32 i = 0; i < count; i++) {
		BMenuItem *item = (BMenuItem *)list->ItemAt(i);
		if (item && _AddItem(item, index++))
			dirty = true;
	}
	
	BWindow *w = Window();
	if (dirty && window_locked && !w->IsHidden()) {
		LayoutItems(index);
		UpdateWindowViewSize();
		Invalidate();
	}

	if (window_locked)
		w->Unlock();

	return dirty;
}

/* ---------------------------------------------------------------- */

bool BMenu::_AddItem(BMenuItem *item, int32 index)
{
	bool success = false;
	BMenuItem *super;
	
	// simply return if the index is out of range
	if ((index < 0) || (index > fItems.CountItems()))
		goto done;
	
	/*
	If adding a "marked" item then inform the menu. It might unmark
	the previous marked item. Must do these before actually adding the
	item to the BList, that's so that the ItemMarked method can properly
	unmark the previous;y marked item.
	*/
	if (item->IsMarked())
		ItemMarked(item);

	// once a item is added to the menubar the menubar is responsible
	// for the object (i.e. it will takes care of disposal)
	if (!fItems.AddItem(item, index))
		goto done;

	// since new item is being added invalidate the cached layout.
	InvalidateLayout();

	BWindow *target;
	super = Superitem();

	if (super)
		target = super->fWindow;
	else
		target = Window();

	if (target)
		item->Install(target);
	
	item->SetSuper(this);
	success = true;

done:
	return success;
}

/* ---------------------------------------------------------------- */

bool BMenu::AddItem(BMenuItem *item, int32 index)
{
	if (fLayout == B_ITEMS_IN_MATRIX) {
		debugger("Error - have to manually give location for items in custom menus");
		return false;
	}

	bool window_locked = LockLooper();

	bool success = _AddItem(item, index);
		
	BWindow *w = Window();
	if (success && window_locked && !w->IsHidden()) {
//+		PRINT(("menu=%s, item=%s\n", Name(), item->Label()));
		LayoutItems(index);
		UpdateWindowViewSize();
		Invalidate();
	}

	if (window_locked)
		w->Unlock();

	return success;
}

/* ---------------------------------------------------------------- */

bool BMenu::AddItem(BMenuItem *item, BRect frame)
{
	if (fLayout != B_ITEMS_IN_MATRIX) {
		debugger("Error - can't specify location for items in a non-MATRIX menu");
		return false;
	}
	
	bool window_locked = LockLooper();

	BWindow *w = Window();

	/*
	If adding a "marked" item then inform the menu. It might unmark
	the previous marked item.
	*/
	if (item->IsMarked())
		ItemMarked(item);

	// once a item is added to the menubar the menubar is responsible
	// for the object (i.e. it will takes care of disposal)
	fItems.AddItem(item);
	
	BWindow *target;
	BMenuItem *super = Superitem();

	if (super)
		target = super->fWindow;
	else
		target = Window();
	
	if (target)
		item->Install(target);

	item->SetSuper(this);
	
	item->fBounds = frame;

	if (window_locked) {
		Invalidate(frame);
		w->Unlock();
	}

	return true;
}

/* ---------------------------------------------------------------- */

bool BMenu::AddSeparatorItem()
{
	return AddItem(new BSeparatorItem());
}

/* ---------------------------------------------------------------- */

bool BMenu::AddItem(BMenu *menu)
{
	return AddItem(new BMenuItem(menu), CountItems());
}

/* ---------------------------------------------------------------- */

bool BMenu::AddItem(BMenu *menu, int32 index)
{
	return AddItem(new BMenuItem(menu), index);
}

/* ---------------------------------------------------------------- */

bool BMenu::AddItem(BMenu *menu, BRect frame)
{
	return AddItem(new BMenuItem(menu), frame);
}

/* ---------------------------------------------------------------- */

bool BMenu::RemoveItem(BMenuItem *item)
{
	long index = IndexOf(item);

	if (index >= 0) {
		RemoveItems(index, 1, item);
		return true;
	}
	return false;
}

/* ---------------------------------------------------------------- */

BMenuItem *BMenu::RemoveItem(int32 index)
{
	BMenuItem *item = ItemAt(index);
	if (item)
		RemoveItems(index, 1, item);
	
	return item;
}

/* ---------------------------------------------------------------- */

bool BMenu::RemoveItems(int32 index, int32 count, bool del)
{
	if (count == 0)
		return false;

	BMenuItem *item = NULL;
	if (count == 1) {
		item = ItemAt(index);
		if (!item)
			return false;
	}
	return RemoveItems(index, count, item, del);
}

/* ---------------------------------------------------------------- */
bool BMenu::RemoveItems(int32 index, int32 count, BMenuItem *item, bool del)
{
	/*
	private remove function used by the public versions.
	*/
	
	bool dirty = false;
	bool window_locked = LockLooper();

	BWindow *w = Window();

	ASSERT(((count == 1) && item) || ((count > 1) && !item));

	for (int32 c = 0; c < count; c++) {
		if (count > 1) 
			item = ItemAt(index);
		
		if (!item)
			break;

		// if the item happens to be selected...
		if (fSelected == item && window_locked) 
			SelectItem(NULL);

		dirty = true;
		item->Uninstall();
		item->SetSuper(NULL);

		fItems.RemoveItem(index);

		if (del)
			delete item;
	}

	if (dirty)
		InvalidateLayout();

	if (window_locked) {
		if (dirty) {
			LayoutItems(index);
			UpdateWindowViewSize();
			Invalidate();
		}
		w->Unlock();
	}
	return dirty;
}

/* ---------------------------------------------------------------- */

bool BMenu::RemoveItem(BMenu *menu)
{
	return RemoveItem(IndexOf(menu)) != NULL;
}

/* ---------------------------------------------------------------- */

int32 BMenu::CountItems() const
{
	return fItems.CountItems();
}

/* ---------------------------------------------------------------- */

BMenuItem *BMenu::FindItem(ulong command) const
{
	/*
	This recursively walks through the menus looking for an item
	that posts the specified command.
	*/
	
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		BMenuItem *item = ItemAt(i);
		
		// ??? If we ever allow hierarchical menu items to also
		// post commands then we need to change this code here a
		// little - move check for command equality before Submenu
		// check.
		if (item->Submenu()) {
			item = item->Submenu()->FindItem(command);
			if (item)
				return item;
		} else if (item->Command() == command)
			return item;
	}

	return NULL;
}

/* ---------------------------------------------------------------- */

BMenuItem *BMenu::FindItem(const char *name) const
{
	// Recursively walk through the menus looking for an item
	// with the specified name.
	
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		BMenuItem *item = ItemAt(i);

		if (!strcmp(name, item->Label()))
			return item;

		if (item->Submenu()) {
			item = item->Submenu()->FindItem(name);
			if (item)
				return item;
		} 
	}

	return NULL;
}

/* ---------------------------------------------------------------- */

BMenuItem *BMenu::ItemAt(int32 index) const
{
	return (BMenuItem *) fItems.ItemAt(index);
}

/* ---------------------------------------------------------------- */

BMenu *BMenu::SubmenuAt(int32 index) const
{
	BMenuItem *item = ItemAt(index);
	
	if (!item)
		return NULL;
	
	return item->Submenu();
}

/* ---------------------------------------------------------------- */

int32 BMenu::IndexOf(BMenuItem *item) const
{
	return fItems.IndexOf(item);
}

/* ---------------------------------------------------------------- */

int32 BMenu::IndexOf(BMenu *menu) const
{
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		BMenu *m = ItemAt(i)->Submenu();
		if (m && m == menu)
			return i;
	}
		
	return -1;
}
/* ---------------------------------------------------------------- */

BMenuItem *BMenu::FindMarked()
{
	// Finds the first "marked" item in the menu.
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		BMenuItem *item = ItemAt(i);
		if (item->IsMarked())
			return item;
	}

	return NULL;
}

/* ---------------------------------------------------------------- */

#define NUM_MENU_KEYS 11

static char index_to_key[NUM_MENU_KEYS + 1] = {
	B_LEFT_ARROW,
	B_RIGHT_ARROW,
	B_UP_ARROW,
	B_DOWN_ARROW,
	B_RETURN,
	B_ESCAPE,
	B_SPACE,
	B_PAGE_DOWN,
	B_PAGE_UP,
	B_HOME,
	B_END,
	0		/* default - nothing happens */
};

enum {
	pITEM,
	nITEM,
	pMENU,
	nMENU,
	popOUT,
	popIN,
	firstI,
	lastI,
	pageD,
	pageU,
	INVK,
	NONE
};

static char key_to_action [3] [12] = {
		/*  left 	right	up		down	return	escape	space	pgDown	pgUp	Home	End		<other> */
/* ROW */	{pITEM,	nITEM,	popIN,	popIN,	INVK,	popOUT,	INVK,	NONE,	NONE,	firstI,	lastI,	NONE	},
/* COL */	{pMENU,	nMENU,	pITEM,	nITEM,	INVK,	popOUT,	INVK,	pageD,	pageU,	firstI,	lastI,	NONE	},
/* MRX */	{pITEM,	nITEM,	popIN,	popIN,	INVK,	popOUT,	INVK,	NONE,	NONE,	firstI,	lastI,	NONE	},
};

/* ---------------------------------------------------------------- */

static int IndexOfKey(char key)
{
	for (uint i = 0; i < sizeof(index_to_key); i++) {
		if (index_to_key[i] == key)
			return i;
	}
	return NUM_MENU_KEYS;
}

/* ---------------------------------------------------------------- */

void BMenu::KeyDown(const char *bytes, int32)
{
	uchar key = bytes[0];
	
	if (dynamic_cast<BMenuBar *>(this) && fState == IDLE_MENU) 
		// If a menubar is IDLE then it isn't tracking so you better
		// not do any keyboard navigation.
		return;


	if (fState > NOW_TRACKING) 
		return;

	
	BMenuItem *item = CurrentSelection();
	char action = key_to_action[fLayout][IndexOfKey(key)];
	
	switch (action) {
		case pITEM:
		case nITEM:
		case pMENU:
		case nMENU:
		case popOUT:
		case popIN:
		case firstI:
		case lastI:
		case pageD:
		case pageU:
			fKeyPressed = true;
			be_app->ObscureCursor();
			break;
	}

	switch (action) {
		case firstI:
			SelectNextItem(NULL, true);
			break;
			
		case lastI:
			SelectNextItem(NULL, false);
			break;
			
		case pageD:
			{
				BPoint pt(Bounds().LeftBottom());
				float h = Bounds().Height() - (2 * SCRL_HEIGHT) - 10;
				pt.y += h;
				pt.x += 2;
				item = HitTestItems(pt);
				if (item)
					SelectNextItem(item, true);
				else {
					item = ItemAt(CountItems() - 1);
					SelectItem(item, DONT_SHOW_MENU);
				}
				break;
			}
			
		case pageU:
			{
				BPoint	pt(Bounds().LeftTop());
				float h = Bounds().Height() - (2 * SCRL_HEIGHT) - 10;
				pt.y -= h;
				pt.x += 2;
				item = HitTestItems(pt);
				if (item) 
					SelectNextItem(item, false);
				else {
					item = ItemAt(0);
					SelectItem(item, DONT_SHOW_MENU);
				}
				break;
			}
			
		case pITEM:
			SelectNextItem(item, false);
			break;
			
		case nITEM:
			SelectNextItem(item, true);
			break;
			
		case pMENU:
			if (fSuper) {
				BWindow *w = fSuper->Window();
				if (w) {
					if (dynamic_cast<BMenuWindow *>(w)) {
						// Want to close myself
						if (fState == NOW_TRACKING)  {
							fState = POP_TRACKING;
						} else {
							BMenu *super = Supermenu();
							BMenuItem *superItem = Superitem();
							super->fKeyPressed = true;
							super->SelectItem(superItem, DONT_SHOW_MENU);
						}
					} else
						fState = SELECT_PREV;
				}
			}
			break;
			
		case nMENU:
			if (item && item->Submenu()) 
				SelectItem(item, SHOW_MENU, true);
			else if (dynamic_cast<BMenuBar *>(fSuper)) 
				fState = SELECT_NEXT;
			break;
			
		case INVK:
			if (item) {
				InvokeItem(item);
				fState = EXIT_TRACKING;
			}
			break;
			
		case popOUT:
// HL 5/31/99 Can't do the following fix because it breaks Gobe...they are relying on this bug!!!!
//			if ((key == B_ESCAPE) && (fState == NOW_TRACKING) && (!IsStickyMode()))
// 				don't popOUT if the menu is still being tracked (using the mouse)
//				break;

			if (item && item->Submenu() && item->Submenu()->Window()) 
				SelectItem(item, DONT_SHOW_MENU);
			else {
				BMenu *super = Supermenu();
				if (super && (fState == IDLE_MENU)) {
					// Want to close myself. Its safe here because this
					// menu isn't tracking.
					BMenuItem	*superItem = Superitem();
					super->fKeyPressed = true;
					super->SelectItem(superItem, DONT_SHOW_MENU);
				} else if (super) {
					// this menu is tracking so I can't close here. Set the
					// fState flag and menu will close itself in ::_track()
					fState = POP_TRACKING;
				} else 
					fState = EXIT_TRACKING;
			}
			break;
			
		case popIN:
			if (item && item->Submenu())
				SelectItem(item, SHOW_MENU, true);
			break;
			
		default:
			if (!IsStickyMode())
				break;

			/*
			iterate through all the items in list. See if there
			is a matching selector char.
			*/
			char ch = tolower(key);

			int32 count = CountItems();
			for (int32 index = 0; index < count; index++) {
				BMenuItem *item = ItemAt(index);
				if (ch == item->fUserTrigger || ch == item->fSysTrigger) {
					// skip the item if its hidden and we care about that
					if (fIgnoreHidden && !IsItemVisible(item))
						break;

					if (item->Submenu()) {
						fKeyPressed = true;
						be_app->ObscureCursor();
						SelectItem(item, SHOW_MENU, true);
					} else if (item->IsEnabled()) {
						fKeyPressed = true;
						be_app->ObscureCursor();
						SelectItem(item);
						InvokeItem(item);
						fState = EXIT_TRACKING;
					}
					break;				
				}
			}

			break;
	}
}

/* ---------------------------------------------------------------- */

void BMenu::DetachedFromWindow()
{
	BView::DetachedFromWindow();
}

/* ---------------------------------------------------------------- */

bool BMenu::AddDynamicItem(add_state)
{
	return false;
}

/* ---------------------------------------------------------------- */

#include <fbc.h>
#if _PR2_COMPATIBLE_

extern "C" bool _ReservedMenu1__5BMenuFv(BMenu *const THIS, BMenu::add_state s);

extern "C" bool _ReservedMenu1__5BMenuFv(BMenu *const THIS, BMenu::add_state s)
{
	// explicit call to the new function! Don't make another virtual call
	// or we'll potentially end up calling client code again.
	return THIS->BMenu::AddDynamicItem(s);
}

extern "C" void _ReservedMenu2__5BMenuFv(BMenu *const THIS, BRect update);

extern "C" void _ReservedMenu2__5BMenuFv(BMenu *const THIS, BRect update)
{
	// explicit call to the new function! Don't make another virtual call
	// or we'll potentially end up calling client code again.
	THIS->BMenu::DrawBackground(update);
}

#endif

/* ---------------------------------------------------------------- */

void
BMenu::SetTrackingHook(menu_tracking_hook hook, void *state)
{
	if (!fExtraMenuData)
		fExtraMenuData = new _ExtraMenuData_;
	fExtraMenuData->trackingHook = hook;
	fExtraMenuData->trackingHookState = state;
}

/* ---------------------------------------------------------------- */

void
BMenu::AdjustSubmenuLocation(BPoint* /*inout_location*/, BMenu* /*submenu*/)
{
}

// Backwards compatibility -- this function to the _ReservedMenu3 slot.

#if _R5_COMPATIBLE_
extern "C" {

	_EXPORT void
	#if __GNUC__
	_ReservedMenu3__5BMenu
	#elif __MWERKS__
	_ReservedMenu3__5BMenuFv
	#endif
	(BMenu* This, BPoint* inout_location, BMenu* submenu)
	{
		This->BMenu::AdjustSubmenuLocation(inout_location, submenu);
	}
}
#endif

/* ---------------------------------------------------------------- */

bool BMenu::OkToProceed(BMenuItem *parent_item)
{
	BPoint		pt;
	BPoint		globalpt;
	uint32		buttons;
	bool		proceed = true;
	BMenuItem	*citem;
	bool		is_menubar = dynamic_cast<BMenuBar*>(this) != NULL;

	GetMouse(&pt, &buttons, false);
	globalpt = ConvertToScreen(pt);

	if (be_app->IsCursorHidden())
		citem = parent_item;
	else
		citem = HitTestItems(pt, BPoint(-1,0));

	if (is_menubar) {
		if (!IsStickyMode() && !buttons) {
			if (citem != parent_item) {
				proceed = false;
			}
		} else {
			if (citem && (citem != parent_item)) {
				proceed = false;
			}
		}
	} else {
		if (!IsStickyMode() && !buttons) {
			proceed = false;
		} else if (IsStickyMode() && buttons) {
			proceed = false;
		} else {
			if (citem != parent_item) {
				proceed = false;
			}
		}
	}

	return proceed;
}

/* ---------------------------------------------------------------- */

void BMenu::AttachedToWindow()
{
	BView::AttachedToWindow();

	key_map	*map;
	char	*buf;
	get_key_map(&map, &buf);
	sSwapped = (map) && (map->left_control_key==0x5d) && (map->left_command_key==0x5c);
	free(map);
	free(buf);

	BMenuItem	*parent_item = Superitem();
	BMenu		*parent_menu = Supermenu();
	add_state	state = B_INITIAL_ADD;

	{
	while (AddDynamicItem(state)) {
		state = B_PROCESSING;
		if (parent_menu && (parent_menu->OkToProceed(parent_item) == false)) {
			fAttachAborted = true;
			AddDynamicItem(B_ABORT);
			break;
		}
	}
	}

	if (!fAttachAborted) {
		/*
		Now that we are in a window can go through all menus and
		determine their real position.
		*/
		LayoutItems(0);
		UpdateWindowViewSize(false);
	}
}

/* ---------------------------------------------------------------- */

void BMenu::GetItemMargins(float *left, float *top, float *right,
	float *bottom) const
{
	*left = fPad.left;
	*right = fPad.right;
	*top = fPad.top;
	*bottom = fPad.bottom;
}

/* ---------------------------------------------------------------- */

void BMenu::SetItemMargins(float left, float top, float right, float bottom)
{
	fPad.Set(left, top, right, bottom);
}

/* ---------------------------------------------------------------- */

void BMenu::GetFrameMargins(float *left, float *top, float *right,
	float *bottom) const
{
	*left = kHorizontalMenuGap-kSnakeGap;
	*right = kHorizontalMenuGap-kSnakeGap;
	*top = kVerticalMenuGap-kSnakeGap;
	*bottom = kVerticalMenuGap-kSnakeGap;
}

/* ---------------------------------------------------------------- */

BMenuItem *BMenu::HitTestItems(BPoint where, BPoint slop) const
{
	int32 count = fItems.CountItems();	
	for (int32 index = 0; index < count; index++) {
		BMenuItem *item = (BMenuItem *)fItems.ItemAt(index);
		BRect bounds(item->Frame());
		bounds.InsetBy(slop);
		if (bounds.Contains(where))
			return item;
	}
	
	return NULL;
}

/* ---------------------------------------------------------------- */

const char *BMenu::ChooseTrigger(const char *data, BList *chars)
{
	const char	*p = data;
	char		ch;

	// Only allow alpha/numeric characters to be selected by this function

	// first check the starting letters of words (if they are upper case)
	while (*p) {

		// eat up leading spaces
		while (isspace(*p))
			p++;

		// are we at the start of a word?
		if (*p) {
			ch = tolower(*p);
			if ((isupper(*p) || isdigit(*p)) && !chars->HasItem((void *) ch)) {
				chars->AddItem((void *) ch);
				return p;
			}

		// skip to end of "word"
		while (*p && !isspace(*p))
			p++;
		}
	}

	// we failed to find a "char" at the start of a word.

	p = data;

	// now look for any upper case letter
	while (*p) {
		ch = tolower(*p);
		if (isupper(ch) && !chars->HasItem((void *) ch)) {
			chars->AddItem((void *) ch);
			return p;
		}
		p++;
	}

	p = data;

	// now look for any character
	while (*p) {
		ch = tolower(*p);
		if (isalnum(ch) && !chars->HasItem((void *) ch)) {
			chars->AddItem((void *) ch);
			return p;
		}
		p++;
	}

	// if we couldn't find a unique char then return 0
	return NULL;
}

/* ---------------------------------------------------------------- */
static const char *firstMismatch(const char *s1, const char *s2)
{
	ASSERT(s1);
	ASSERT(s2);

	while (1) {
		if (*s1 == '\0') {
			s1 = NULL;		// didn't find a mismatch!
			break;
		}
		if (*s2 == '\0') {
			// return s1
			break;
		}
		if (*s1 != *s2)
			break;
		s1++;
		s2++;
	}
	
	return s1;
}

/* ---------------------------------------------------------------- */

void BMenu::CalcTriggers()
{
	long		i;
	long		numItems;
	BList		chars(26);
	char		ch;
	BMenuItem	*item = NULL;
	BMenuItem	*nextItem;
	const char	*title = NULL;
	const char	*nextTitle = NULL;
	const char	*prevTitle = NULL;
	const char	*data1;
	const char	*data2;

	numItems = fItems.CountItems();

	// first go through items adding all the hard-coded/user-defined chars
	for (i = 0; i < numItems; i++) {
		item = (BMenuItem *) fItems.ItemAt(i);
		
		ch = item->Trigger();
		if (ch)
			chars.AddItem((void *) tolower(ch));
		else {
			// clear out the prev system selected char 
			item->SetSysTrigger(0);
			item->fTriggerIndex = -1;
		}
	}

#if 1
	// also add any menu items that have keyboard shortcuts - use same char
	// if that char occurs in the Menu title
	// unless it was already taken or it is a 'navigation' character
	for (i = 0; i < numItems; i++) {
		item = (BMenuItem *) fItems.ItemAt(i);
		ch = item->Shortcut();
		if (ch && !((ch == B_UP_ARROW) || (ch == B_DOWN_ARROW) ||
				(ch == B_LEFT_ARROW) || (ch == B_RIGHT_ARROW))) {

			char lch;
			lch = tolower(ch);
			if (!chars.HasItem((void *) lch)) {
				bool match;
				const char	*title = item->Label();
				const char	*c;
				if (lch == ch) {
					match = ((c = strchr(title, ch)) != NULL);
				} else {
					match = (((c = strchr(title, ch)) != NULL) ||
							((c = strchr(title, lch)) != NULL));
				}
				if (match) {
					item->SetSysTrigger(lch);
					item->fTriggerIndex = (c - title);
					chars.AddItem((void *) lch);
				}
			}
		}
	}
#endif

	// now go back through list picking selectors for the rest of the items.

	/*
	idea: skip over the characters in the title that match with the
	next os prev item in the list.
	In this way, for a group of items like: item1, item2, item3
	the char's '1', '2', and '3' will automatically get selected.
	Also, give pref to letters at the start of a word or upper case letters
	*/

	if (numItems) {
		item = ItemAt(0);
		title = item->Label();
	}

	for (i = 1; i <= numItems; i++) {

		nextItem = (BMenuItem *) fItems.ItemAt(i);
		nextTitle = nextItem ? nextItem->Label() : NULL;

		if (!(item->Trigger() || item->fSysTrigger)) {
			data1 = prevTitle ? firstMismatch(title, prevTitle) : NULL;
			data2 = nextTitle ? firstMismatch(title, nextTitle) : NULL;
			
			if (!data1 && !data2)
				data1 = title;
			else
				data1 = (data1 > data2) ? data1 : data2;
			
			const char *cp = ChooseTrigger(data1, &chars);

			// if not char found and we weren't using the whole title...
			if (!cp && (data1 != title))
				// try using the entire title...
				cp  = ChooseTrigger(title, &chars);

			if (cp) {
				ch = tolower(*cp);
				item->SetSysTrigger(ch);
				item->fTriggerIndex = (cp - title);
			}
		}
		
		prevTitle = title;
		item = nextItem;
		title = nextTitle;
	}
}

/* ---------------------------------------------------------------- */

void BMenu::UpdateWindowViewSize(bool updateWindow)
{
	if (dynamic_cast<BMenuBar *>(this))
		return;

	// if Layout called while menu is in the middle of tracking.
	fTrackNewBounds = true;	

	if (fResizeToFit) {
		/*
		Determine the max size of the menu. Then resize the view to
		that max. (It might have been sized smaller by a previous
		instance of menu tracking. Also reset the view to 0,0 in
		the window.
		*/
		BPoint extent(B_ORIGIN);
		if (fLayout != B_ITEMS_IN_MATRIX) {
			int32 count = CountItems();
			if (count) 
				extent = ItemAt(count - 1)->Frame().RightBottom();
		} else 
			extent = Frame().RightBottom();
	
		ResizeTo(extent.x, extent.y);
	} else if ((fLayout == B_ITEMS_IN_MATRIX) && fInitMatrixSize) 
		ResizeTo(fInitMatrixSize->x, fInitMatrixSize->y);


	if (DrawNewLook())
		MoveTo(kHorizontalMenuGap, kVerticalMenuGap);
	else
		MoveTo(1, 1);

	if (updateWindow) {
		bool scrollOn;
		BWindow	*window = Window();
		BPoint where(window->Frame().LeftTop());
		BRect bounds(CalcFrame(where, &scrollOn));
		
		window->MoveTo(bounds.LeftTop());

		float h = bounds.Height();
		float w = bounds.Width();
		if (fItems.CountItems() == 0) {
			CacheFontInfo();
			h = fFontHeight + 5;
			w = StringWidth(kEmptyMenuString) + 25;
			if (DrawNewLook()) {
				h += 2 *kHorizontalMenuGap;
				w += 2 *kVerticalMenuGap;
			}
		}
		window->ResizeTo(w, h);
	}
}

/* ---------------------------------------------------------------- */

const int32 kItemOffset = 1;

void BMenu::LayoutItems(int32 index)
{
	// determine the position of the items starting with item at "index"
	
	ASSERT((Window()));
	
	if (fUseCachedMenuLayout)
		return;
	
	if (fLayout == B_ITEMS_IN_MATRIX) {
		fUseCachedMenuLayout = true;
		return;
	}

	if (index > fItems.CountItems())
		return;
	
	fUseCachedMenuLayout = true;
	
	CalcTriggers();

	float width=0, height=0;
	ComputeLayout(index, fResizeToFit, true, &width, &height);

	if (fResizeToFit) {
		ResizeTo(width, height);
	}
}

/* ---------------------------------------------------------------- */

void BMenu::ComputeLayout(int32 index, bool bestFit, bool moveItems,
						  float* width, float* height)
{
	// determine the space needed for items starting with item at "index"
	// if moveItems is true, also position all of these items
	
	long		i;
	long		numItems;
	BPoint		extent;
	int			extra_for_keys = 0;
	
	// Get this window we are in.  This is used to determine how the item
	// frames are computed -- if we are not in a MenuWindow, the menu padding
	// should not extend item frames to the view border.
	ASSERT(Window() != 0 || !moveItems);
	BMenuWindow* menuWindow = dynamic_cast<BMenuWindow*>(Window());
	
	// By default, the computed width and height is just the current size.
	*width = Bounds().Width()+1;
	*height = Bounds().Height()+1;
	
	if (fLayout == B_ITEMS_IN_MATRIX) {
		// can't compute a size for this layout.
		return;
	}

	if ((numItems = fItems.CountItems()) == 0) {
		// if no items, just compute a minimal size.
		CacheFontInfo();
		*width = 10;
		*height = fFontHeight + menu_bar_pad.top + menu_bar_pad.bottom;
		return;
	}
	if (index > numItems) {
		// if past the last item, size is pretty tiny.
		*width = 1;
		*height = 1;
		return;
	}
	
	if (bestFit || !moveItems) {
		// if not moving items, we are just computing the needed
		// size -- which is only useful if all items are included.
		index = 0;
	}

	int32 modifierExtra = 0;
	float maxWidth = 0;
	float maxHeight = 0;
	BRect itemFrame;
	
	if (index == 0) 
		itemFrame = BRect(kItemOffset, kItemOffset, kItemOffset, kItemOffset);
	else 
		itemFrame = ItemAt(index-1)->Frame();

	for (i=index; i<numItems; i++) {
		float w, h;
		
		BMenuItem *item = ItemAt(i);
		item->GetContentSize(&w, &h);	

		if (fMaxContentWidth > 0 && w > fMaxContentWidth)
			extent.Set(fMaxContentWidth, h);
		else
			extent.Set(w,h);

//		When a menu has a shortcut up to 4 little icons get drawn
//		next the the shortcut letter (depending on the modifier keys).
//		Need to determine the max number of icons for this menu and
//		make the menu wide enough to accomadate those icons.

		uint32 modifiers;
		if (item->Shortcut(&modifiers)) {
			modifierExtra = _BM_ALT_W_ + _BM_SPACING_;
			if (modifiers & B_SHIFT_KEY)
				modifierExtra += _BM_SHIFT_W_ + _BM_SPACING_;
			if (modifiers & B_CONTROL_KEY)
				modifierExtra += _BM_CONTROL_W_ + _BM_SPACING_;
			if (modifiers & B_OPTION_KEY)
				modifierExtra += _BM_OPTION_W_ + _BM_SPACING_;
			if (modifierExtra > extra_for_keys)
				extra_for_keys = modifierExtra;
		}
		
		if (bestFit) {
			// allow auto-resizing in the corresponding direction.
			if (fLayout == B_ITEMS_IN_COLUMN)
				maxWidth = std::max(maxWidth, extent.x);
			else 
				maxHeight = std::max(maxHeight, extent.y);
		}

		itemFrame = Bump(itemFrame, extent, i);
		
		if (!menuWindow) {
			// for the first/last item in an embedded menu, leave
			// a bit of space at to keep the selection highlight
			// away from the edge.
			if (i == 0) {
				if (fLayout == B_ITEMS_IN_COLUMN) {
					if (fPad.top > 1) itemFrame.top += 2;
					else if (fPad.top > 0) itemFrame.top += 1;
				} else {
					if (fPad.left > 1) itemFrame.left += 2;
					else if (fPad.left > 0) itemFrame.left += 1;
				}
			} else if (i == numItems-1) {
				if (fLayout == B_ITEMS_IN_COLUMN) {
					if (fPad.bottom > 1) itemFrame.bottom -= 2;
					else if (fPad.bottom > 0) itemFrame.bottom -= 1;
				} else {
					if (fPad.right > 1) itemFrame.right -= 2;
					else if (fPad.right > 0) itemFrame.right -= 1;
				}
			}
		}
		
		if( moveItems ) {
			item->fBounds = itemFrame;
		}
	}

	if (bestFit) {
		BFont font;
		GetFont(&font);

		if (fLayout == B_ITEMS_IN_COLUMN) {
			maxWidth += fPad.left + fPad.right +
//+				(numKeys ? (8 + aFont.Size() + 9 +(numKeys*11)) : 0);
				(extra_for_keys ? (8 + font.Size() + 9 + extra_for_keys) : 0);
		} else {
			maxHeight += fPad.top + fPad.bottom;
		}

		if( moveItems ) {
			for (i=index; i<numItems; i++) {
				BMenuItem* item = ItemAt(i);
				BRect newFrame(item->Frame());
				
				if (fLayout == B_ITEMS_IN_COLUMN) {
					newFrame.right = newFrame.left + maxWidth;
					if (!menuWindow) {
						if( fPad.left > 1 ) newFrame.left += 1;
						else if( fPad.left > 0 ) newFrame.left += 1;
						if( fPad.right > 1 ) newFrame.right -= 1;
						else if( fPad.right > 0 ) newFrame.right -= 1;
					}
				} else {
					newFrame.bottom = newFrame.top + maxHeight;
					if (!menuWindow) {
						if( fPad.top > 1 ) newFrame.top += 1;
						else if( fPad.top > 0 ) newFrame.top += 1;
						if( fPad.bottom > 1 ) newFrame.bottom -= 1;
						else if( fPad.bottom > 0 ) newFrame.bottom -= 1;
					}
				}
				
				item->fBounds = newFrame;
			}
		}
	
		uint32 rmode = ResizingMode();
		BRect parentBounds;
		if (Parent())
			parentBounds = Parent()->Bounds();
		else if (Window())
			parentBounds = Window()->Bounds();
		else
			parentBounds = Bounds();

		if (fLayout == B_ITEMS_IN_COLUMN) {
			*width = maxWidth + 2;
			if ((rmode & _rule_(0xf, 0, 0xf, 0)) == B_FOLLOW_TOP_BOTTOM) {
				*height = parentBounds.bottom+1;
			} else {
				*height = itemFrame.bottom;
			}
		} else {
			*height = maxHeight;
			if ((rmode & _rule_(0, 0xf, 0, 0xf)) == B_FOLLOW_LEFT_RIGHT) {
				*width = parentBounds.right+1;
			} else {
				*width = itemFrame.right;
			}
		}
	}
}

/* ---------------------------------------------------------------- */

BRect BMenu::Bump(BRect bounds, BPoint extent, int32 index) const
{
	
	if (index) {
		if (fLayout == B_ITEMS_IN_ROW) {
			bounds.left = bounds.right + 1;
			bounds.right = bounds.left + fPad.left + extent.x + fPad.right;
		} else {
			bounds.top = bounds.bottom + 1;
			bounds.bottom = bounds.top + fPad.bottom + extent.y + fPad.top;
		}
	} else {
		// index == 0 so were just starting things off.
		bounds.Set(0, 0, fPad.left + extent.x + fPad.right,
						 fPad.bottom + extent.y + fPad.top);
		if (fLayout == B_ITEMS_IN_ROW)
			bounds.bottom = Bounds().Height();
		else
			bounds.right = Bounds().Width();
	}
	
	return bounds;
}

/* ---------------------------------------------------------------- */

BRect BMenu::CalcFrame(BPoint where, bool *scrollingOn)
{
	/*
	where - should be in screen coords.
	
	The Menu has already been added to the menuWindow. This process
	will have calculated the size of the menu. We
	now need to resize things so that it fits on screen.
	
	Note: The menu view should be inset by one pixel on all sides of
	the window. This gives the menu window the space to draw the
	menu frame.
	*/
	BRect superBounds;
	bool scrolling = false;
	bool windowPadBottom = true;
	bool windowPadRight = true;
	
	BScreen current_screen( Window() );
	BRect screenFrame(current_screen.Frame());

	BRect bounds(Bounds());
	bounds.OffsetBy(1,1);		// bump - remember that the menu view
								// is positioned at (1,1) in window.
	bounds.OffsetTo(where);		// view bounds now in screen coords.


	int32 horizontalSnakeAdjustment = 0;
	int32 horizontalGapAdjustment = 0;
	int32 verticalSnakeAdjustment = 0;
	int32 verticalGapAdjustment = 0;
	if (DrawNewLook()) {
		horizontalGapAdjustment = 2 * kHorizontalMenuGap;
		verticalGapAdjustment = 2 * kVerticalMenuGap;
		bounds.right += horizontalGapAdjustment;
		bounds.bottom += verticalGapAdjustment;
		horizontalSnakeAdjustment = -kSnakeGap;
		verticalSnakeAdjustment = kSnakeGap;
	}

	BMenu *base = Supermenu();
	if (base) 
		superBounds = Superbounds();
		
	/*
	Left & right boundary condition is simple. Simply shift the menu
	over. Don't worry about a menu so wide that it doesn't fit on the
	screen. Need to treat vertical menus and horizontal menus (most
	menubars) differently. If the parent (base) is vertical then we
	must move the submenu all the way over to other side so that
	it doesn't cover the parent.
	*/
	if (bounds.right > screenFrame.right) {
		if (base && base->fLayout == B_ITEMS_IN_COLUMN) {
			/*
			We're bringing up a new menu and we want to avoid too
			much overlap with the super menu. So we have to shift
			it all the way over to the over side of the super menu.
			This means shifting it left by the sum of its width plus
			the supers width.
			*/
			bounds.OffsetTo(superBounds.left, bounds.top);
			bounds.OffsetBy(-(bounds.Width() + horizontalSnakeAdjustment), 0);

			windowPadRight = false;
		} else {
			bounds.OffsetBy(screenFrame.right - bounds.right, 0);
		}
	}
	if (bounds.left < screenFrame.left) {
		bounds.OffsetBy(screenFrame.left - bounds.left, 0);
	}
	
	if ((base && (base->fLayout == B_ITEMS_IN_COLUMN))
		|| dynamic_cast<BPopUpMenu *>(this)) {
		/*
		We have a popup menu or a menu with a vertically oriented parent.
		These cases are simple.
		Just move the menu up/down as needed. Don't need to
		worry above covering up a horizontal menu (e.g. menubar), since
		there isn't one.
		*/
		
		if ((bounds.Height()-(verticalSnakeAdjustment*2)) > screenFrame.Height()) {
			// menu is too large for screen. Shift to the top and
			// enable scrolling.
			bounds.top = screenFrame.top - verticalSnakeAdjustment;

			BMenuItem *item = HitTestItems(BPoint(0,
				(screenFrame.bottom - bounds.top) - (SCRL_HEIGHT*2)));
			ASSERT((item));
			bounds.bottom = item->Frame().top;

			scrolling = true;
		} else if (bounds.bottom > (screenFrame.bottom - 4)) {
			// menu is going off the bottom. Shift whole thing up
			bounds.top -= (bounds.bottom - (screenFrame.bottom+verticalSnakeAdjustment) + 4);
			bounds.bottom = (screenFrame.bottom+verticalSnakeAdjustment) - 4;
		} else if (bounds.top < (screenFrame.top + 4)) {
			// menu is drawing off the top of screen.
			bounds.bottom += (screenFrame.top - bounds.top + 4);
			bounds.top = screenFrame.top + 4;
		}

	} else {
		bool moveAbove = false;
		
		if (bounds.bottom > screenFrame.bottom) {
			/*
			If the bottom of menu goes off the screen.
			*/
			
			// Let's find the menuitem that will get chopped off if we
			// need to enable scrolling (need to account for the
			// scroller). Also remember that entire menu might be below the screen.
			// pt is in local coords.
			BPoint pt(0, (screenFrame.bottom - bounds.top) - (SCRL_HEIGHT*2));
			if (DrawNewLook())
				pt.y -= kVerticalMenuGap;
				
			BMenuItem *item = HitTestItems(pt);
			if (item) {
				/*
				Let's make the "cut" in window size just above the top of this
				item. In this way we'll keep the window showing "whole" items.
				*/
				pt = item->Frame().LeftTop();
			}
	
			/*
			If less than 3 vertical items are left visible then
			move the menu above its super (aka the menubar).
			*/
			BPoint tmpPoint(0,0);
			for (int32 count = 0; count < 5; count++) {
				if (tmpPoint.y >= pt.y) {
					moveAbove = true;
					break;
				}
				BMenuItem *item = HitTestItems(tmpPoint);
				if (!item)
					break;
				tmpPoint.y = item->Frame().bottom + 1;
			}
			if (moveAbove) {
				// move the menu completely above the menubar.
				float h = bounds.Height();
				bounds.top = superBounds.top - h + verticalSnakeAdjustment;
				bounds.bottom = bounds.top + h;
				windowPadBottom = false;
			} else {
				// not moving above so resize and turn on scrolling
				bounds.bottom = bounds.top + pt.y;
				scrolling = true;
			}
		}
	
		if ((bounds.top+verticalSnakeAdjustment) < screenFrame.top) {
			/*
			This top of the menu is above the top of the screen.  If
			this menu is to the far left or right of its supermenu,
			though, we still might be able to move it so everything fits
			on the screen.  Try that now.
			*/
			if (!base || (bounds.right < (superBounds.left+horizontalSnakeAdjustment))
					|| (bounds.left > (superBounds.right-horizontalSnakeAdjustment))) {
				if ((bounds.Height()-(verticalSnakeAdjustment*2)) <= screenFrame.Height()) {
					bounds.top = -verticalSnakeAdjustment;
				}
			}
		}
		
		if ((bounds.top+verticalSnakeAdjustment) < screenFrame.top) {
			/*
			The top of menu is above the top of the screen. We'll move
			the top down and scroll the contents up, thus leaving the
			same menu items visible.
			*/
			
			if (moveAbove) {
				// When we moved the menu above its parent, we had to
				// offset it to take into account the space for
				// the snake.  Now that we are scrolling it, we have
				// to remove the padding.
				bounds.top -= verticalSnakeAdjustment;
				bounds.bottom -= verticalSnakeAdjustment;
			}
			
			// Let's find the menuitem that is getting chopped off
			BPoint pt(0, (screenFrame.top - bounds.top) - 1);
			BMenuItem *item = HitTestItems(pt);
			
			/*
			Let's make the "cut" in window size just past the bottom of
			this item. In this way we'll keep the window showing "whole"
			items. (We add "1" to pt.y to get just PAST the item).
			*/
			pt = item->Frame().LeftBottom();
			pt.y += 1;
	
			ScrollBy(0, pt.y);
			bounds.top += pt.y;
			scrolling = true;
		}
	}

	if (scrolling) {
		if (DrawNewLook())
			bounds.bottom -= kSnakeGap;
		// Set the bounds of the menu view to appropriate size
		bounds.bottom -= (SCRL_HEIGHT * 2);
		ResizeTo(bounds.Width()-horizontalGapAdjustment,
				 bounds.Height());

		/*
		Now add extra space for the scroll up and down indicators.
		This amounts to SCRL_HEIGHT pixels at top and bottom of
		window
		*/
		bounds.bottom += (SCRL_HEIGHT * 2) + verticalGapAdjustment;
		
		/*
		Shift the menu view down by SCRL_HEIGHT pixels. That will
		leave SCRL_HEIGHT pixels above and below the menu view for
		the indicators.
		*/
		MoveBy(0, SCRL_HEIGHT);

		*scrollingOn = true;
	} else
		*scrollingOn = false;
	
	if (!DrawNewLook()) {
		if (windowPadRight)
			bounds.right += 2;
		else
			bounds.left -= 2;
		if (windowPadBottom)
			bounds.bottom += 2;
		else
			bounds.top -= 2;
	}

	return bounds;
}

/* ---------------------------------------------------------------- */

BPoint BMenu::ScreenLocation()
{
	/*
	Returns, in screen coords, the top-left coord of where the menu should
	get displayed.
	*/

	BMenuItem *item = Superitem();
	BMenu *base = Supermenu();

	if (!base || !item) {
		debugger("BMenu can't determine where to draw. "
			"Override BMenu::ScreenLocation method to determine location.");
		return B_ORIGIN;
	}

	BRect itemBounds(item->Frame());
	BPoint	where;
	
	/*
	"where" is in global/screen coord space. This positions
	the window containing popup.
	*/
	menu_layout layout = base->Layout();
	if (layout == B_ITEMS_IN_ROW) {
		where = itemBounds.LeftBottom();
		where.x += 1 + kSnakeGap;
		//where.y -= 1;
	} else if (layout == B_ITEMS_IN_COLUMN) {
		where = itemBounds.RightTop();
		where.x += 2;
	} else 
		// ??? need a way for the custom menus to specify menu location
		where = itemBounds.LeftBottom();

	where.y += 1;
	base->ConvertToScreen(&where);	

	return where;
}

/* ---------------------------------------------------------------- */

void BMenu::Show()
{
	// overrides BView::Show
	Show(false);
}

/* ---------------------------------------------------------------- */

void BMenu::Show(bool selectFirstItem)
{
	Install(NULL);
	_show(selectFirstItem);
}

/* ---------------------------------------------------------------- */

bool BMenu::_show(bool selectFirstItem)
{
	
	BMenuWindow	*window;

	if (fLayout == B_ITEMS_IN_MATRIX) {
		if (fInitMatrixSize == NULL)
			fInitMatrixSize = new BPoint();

		BRect bounds = Bounds();
		fInitMatrixSize->Set(bounds.Width(), bounds.Height());
	}	

	/*
	Need to get the bounds of the menu containing the item.
	This rect is needed for mouse tracking. Get this in global
	screen coord space.
	*/
	if (fSuper) {
		BRect baseBounds(fSuper->Bounds());
		fSuper->ConvertToScreen(&baseBounds);
		fSuperbounds = baseBounds;
	}
	
	fSelected = NULL;

	// If there's no parent then the menu must create it's own window to use
#define USE_SUPERMENU_WINDOW 0

#if USE_SUPERMENU_WINDOW
	if (Supermenu())
		window = Supermenu()->MenuWindow();
	else
#endif
		window = new BMenuWindow(Name(), DrawNewLook());

	BAutolock lock(window);

	fChosenItem = NULL;
	fState = IDLE_MENU;
	
	BView *menuFrame = window->ChildAt(0);

	fAttachAborted = false;
	menuFrame->AddChild(this);

	if (fAttachAborted) {
		menuFrame->RemoveChild(this);
		ResizeTo(Bounds().Width()-1, Bounds().Height()-1);
		fAttachAborted = false;
		return false;
	}

	window->SetMenu(this);

	MakeFocus(true);


	/*
	After adding the view to a window it now "knows" how big the 
	menu has to be. Get that info and set the window size.
	*/
	bool scrollOn;
	BPoint where(ScreenLocation());
	if (Supermenu()) Supermenu()->AdjustSubmenuLocation(&where, this);
	
	/*
	Offset position of menu to make room for the snake.
	*/
	if (DrawNewLook() && Supermenu()) {
		if (Supermenu()->Layout() == B_ITEMS_IN_ROW) {
			where.y -= kSnakeGap + 1;
			where.x -= kSnakeGap;
		} else if (Supermenu()->Layout() == B_ITEMS_IN_COLUMN) {
			where.y -= 6 + kSnakeGap;
			where.x -= kSnakeGap + 2;
		}
	} else if (!Supermenu()) {
		// This is a raw popup menu -- offset appropriately so that the
		// created menu will appear exactly at the returned point.
		where.x -= kSnakeGap;
		where.y -= kSnakeGap;
	}
	
	/*
	Calculate final size and position.
	*/
	BRect bounds(CalcFrame(where, &scrollOn));

	window->MoveTo(bounds.LeftTop());
	float height = bounds.Height();
	float width = bounds.Width();
	if (CountItems() == 0) {
		CacheFontInfo();
		height = fFontHeight + 4;
		width = StringWidth(kEmptyMenuString) + 25;
		if (DrawNewLook()) {
			height += 2 *kHorizontalMenuGap;
			width += 2 *kVerticalMenuGap;
		}
 	}
	window->ResizeTo(width, height);

	if (scrollOn)
		window->AddScrollerViews();

	if (Superitem() && Supermenu() && DrawNewLook()) {
		// stash the frame of the superitem, converted to this view's coordinates
		// so that we can decide on the shape of the snake
		BRect frame(Superitem()->Frame());
		Supermenu()->ConvertToScreen(&frame);
		ConvertFromScreen(&frame);
		BMenuFrame *menuFrame = dynamic_cast<BMenuFrame *>(Window()->ChildAt(0));
		// it would be more convenient to store fCachedSuperitemRect directly in
		// this object but we are running out of reserved fields where the menu
		// frame can be expanded as much as we like
		frame.right -= 1;
		frame.bottom -= 1;
		menuFrame->fCachedSuperitemRect = frame;
	}

	window->Show();

	if (selectFirstItem) 
		SelectItem(ItemAt(0), DONT_SHOW_MENU);

	
	return true;
}

/* ---------------------------------------------------------------- */

void BMenu::Hide()
{
	_hide();
	Uninstall();
}

/* ---------------------------------------------------------------- */

void BMenu::_hide()
{
	if (!LockLooper()) {
		// someone else is in the process of closing the menu...
		PRINT(("race condition averted 3\n"));
		return;
	}
	BWindow *window = Window();
	
	ASSERT(dynamic_cast<BMenuWindow *>(window));

	SelectItem(NULL);
		
	window->Hide();
	ScrollTo(B_ORIGIN);
	BView *pp = window->ChildAt(0);
	pp->RemoveChild(this);
	((BMenuWindow *) window)->RemoveScrollerViews();
	((BMenuWindow *) window)->SetMenu(NULL);

	DeleteMenuWindow();
	
	fState = IDLE_MENU;

	window->Unlock();

	// If there's no super then the menu is responsible for deleting window
	if (!USE_SUPERMENU_WINDOW || !Supermenu()) {
		if (window->Lock())
			window->Quit();
	}

	if ((fLayout == B_ITEMS_IN_MATRIX) && (fInitMatrixSize != NULL))
		ResizeTo(fInitMatrixSize->x, fInitMatrixSize->y);
}

/* ---------------------------------------------------------------- */

BMenuItem *BMenu::Track(bool is_sticky, BRect *sticky)
{
	int action = NOW_TRACKING;

	if (!IsStickyPrefOn())
		is_sticky = false;

	SetStickyMode(is_sticky);
	if (is_sticky && LockLooper()) {
		fKeyPressed = true;
		RedrawAfterSticky(Bounds());
		UnlockLooper();
	}

	fExtraRect = sticky;
	if (fExtraRect && LockLooper()) {
		ConvertFromScreen(fExtraRect);
		UnlockLooper();
	}
	BMenuItem *item = _track(&action, -1);
	SetStickyMode(false);
	fExtraRect = NULL;
	return item;
}

/* ---------------------------------------------------------------- */
static bool in_scroll_area(BRect window_frame, BPoint mloc)
{
	window_frame.InsetBy(-20, -20);
	return window_frame.Contains(mloc);
}

/* ---------------------------------------------------------------- */

uint32 _pjp_basic_snooze_ = 15;
uint32 _pjp_slop_snooze_ = 50;

extern uint32 _pjp_delay_;
uint32 _pjp_delay_ = 4;

BMenuItem *BMenu::_track(int *action, long)
{
	BWindow		*window = Window();
	ASSERT((Window()));

	BPoint		loc;
	BPoint		prevLoc(-1,-1);
	BPoint		initialLoc;
	BPoint		globalLoc;
	ulong		buttons;
	BRect		bounds;
	BRect		wFrame;
	BMenuItem	*item = NULL;
	long		snoozeTime;
	bool		tmpBool;
	ulong		iter;
	ulong		scroll_iter = 0;
	long		scroll_count;
	ulong		show_delay_iter = 0;
	uint32		show_delay_cycles = _pjp_delay_;
	uint32		iworkspace = current_workspace();

	// used in a couple places when the Menu is scrolled. After scrolling set
	// this flag so that tracking code will get a new bounds rect
	fTrackNewBounds = true;

	tmpBool = window->Lock();
	ASSERT((tmpBool));

	if (fState == EXIT_TRACKING) {
		TRACE();
		*action = EXIT_TRACKING;
		window->Unlock();
		return NULL;
	}

	// Make myself the active window (so that window receives
	// keyboard input)
	window->Activate(true);

	fState = NOW_TRACKING;

	window->Unlock();

	scroll_iter = 0;
	scroll_count = 0;

	bool		first_time = true;
	bigtime_t	click_speed;
	extern bigtime_t	_gtrack_start_time_;
	extern bool			_gstartup_phase_;
	get_click_speed(&click_speed);

	for(iter = 0; true; snooze(snoozeTime*1000), iter++) {

		snoozeTime = _pjp_basic_snooze_;

		if (show_delay_iter && (iter > (show_delay_iter + show_delay_cycles))) {
			show_delay_iter = 0;
		}

		if (!LockLooper()) {
			fState = EXIT_TRACKING;
			item = NULL;
			window = NULL;
			goto exit_loop;
		}
		window = Window();
		ASSERT(window);

		// Need to deal with current workspace changing out from under
		// this menu tracking session. If that happens then abort!
		uint32 cworkspace = current_workspace();
		if (cworkspace != iworkspace) {
			fState = EXIT_TRACKING;
			item = NULL;
			goto exit_loop;
		}

		switch (fState) {
			case POP_TRACKING:
			case SELECT_NEXT:
			case SELECT_PREV:
			case EXIT_TRACKING:
				item = NULL;
				goto exit_loop;
		}

		if (fExtraMenuData) {
			menu_tracking_hook hookfunction = fExtraMenuData->trackingHook;
			if (	hookfunction
				&& 	fExtraMenuData->trackingHookState
				&&	(*hookfunction)(this, fExtraMenuData->trackingHookState)) {
				item = NULL;
				fState = EXIT_TRACKING;
				goto exit_loop;
			}
		}
		
		int act;
		item = CurrentSelection();
		if (item && item->Submenu() &&
			((act = item->Submenu()->State(&fChosenItem)) > POP_TRACKING)) {
				fState = act;
				item = NULL;
				goto exit_loop;
		}
		
		if (fTrackNewBounds) {
			wFrame = window->Frame();
			ConvertFromScreen(&wFrame);
			bounds = Bounds();
			
			// to account for extra space on left & right of view
			bounds.InsetBy(-1,0);
			fTrackNewBounds = false;
		}
		
		BMenuWindow *menu_window = dynamic_cast<BMenuWindow *>(window);
		ASSERT(menu_window);
		menu_window->UpdateScrollers();

		GetMouse(&loc, &buttons, false);

		// need global coord here because I'm comparing this against prevLoc
		// to determine if the mouse is moving. Since the Menu might scroll
		// at any time converting to global coords isolates any scrolling.
		globalLoc = loc;
		ConvertToScreen(&globalLoc);

		if (first_time)
			initialLoc = globalLoc;

		// if we're in sticky mode, but now some buttons are pressed...
		if (IsStickyMode() && buttons) {
			if (bounds.Contains(loc)
				|| (fExtraRect && fExtraRect->Contains(loc))) {
				SetStickyMode(false);		// get out of sticky mode
				RedrawAfterSticky(bounds);
				SetMouseEventMask(0, B_LOCK_WINDOW_FOCUS|B_SUSPEND_VIEW_FOCUS|B_NO_POINTER_HISTORY);
			} else {
				item = NULL;				// if mouse is outside - exit
				fState = EXIT_TRACKING;
				goto exit_loop;
			}
		}
		
		if (!IsStickyMode() && !buttons) {
			// we're outta here. See what was selected and break.
			bool	c = bounds.Contains(loc);
			if (!_gstartup_phase_ && c) {
				item = HitTestItems(loc, BPoint(-1, 0));
				fState = EXIT_TRACKING;
				goto exit_loop;
			} else if (IsStickyPrefOn() &&
				(c || (fExtraRect && fExtraRect->Contains(loc)))) {
					// mouse was released over initially drawing menu or
					// it was release of the 'extra' area around the menubar.
					// this means we should go to sticky mode
					SetStickyMode(true);
					RedrawAfterSticky(bounds);
			} else {
				item = NULL;
				fState = EXIT_TRACKING;
				goto exit_loop;
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
			((nItem = HitTestItems(loc, BPoint(-1,0))) != item)) {

			BRect mr = sm->Window()->Frame();

			float cur = pt_distance(globalLoc, mr);
			float prev = pt_distance(prevLoc, mr);

			// If the mouse is moving in the correct direction, and "fast"
			// enough, then give it a chance to get there.
			if (cur < (prev)) {
				snoozeTime = _pjp_slop_snooze_;
				goto bottom;
			}
		}
		}
			
		if (item && OverSubmenu(item, globalLoc)) {
			/*
			A hierarchical menu has been shown. If the mouse moves
			over the menu itself then start tracking in that menu.
			Unlock the window (aka "menu") we're currently in so that
			it is free to handle updates.
			*/
			int state;
			window->Unlock();
			fChosenItem = item->Submenu()->_track(&state);
			if (!LockLooper()) {
				fState = EXIT_TRACKING;
				item = NULL;
				window = NULL;
				goto exit_loop;
			}
			window = Window();
			if (state == EXIT_TRACKING) {
				bool	really_exit = true;
				if (fExtraRect) {
					BPoint	pt;
					ulong	bs;
					GetMouse(&pt, &bs);
					// If the Mouse is now over the 'extra' area then go into
					// sticky mode
					if (_gstartup_phase_ && IsStickyPrefOn() && fExtraRect->Contains(pt)) {
						really_exit = false;
						SetStickyMode(true);
						SelectItem(ItemAt(0), SHOW_MENU);
						RedrawAfterSticky(bounds);
					}
				}
				if (really_exit) {
					fState = state;
					item = NULL;
					goto exit_loop;
				}
				
			} else if (state > POP_TRACKING) {
				fState = state;
				item = NULL;
				goto exit_loop;
			}
			// now that submenu is gone reactivate myself (for keyboard input)
			Window()->Activate(true);
		} else if (bounds.Contains(loc)) {
			BMenuItem	*newItem = HitTestItems(loc, BPoint(-1,0));
			BMenu		*sub;
			/*
			 We might be selecting a new item here. If that item has
			 a submenu don't initially display that menu. Wait until the
			 mouse stops moving quickly. This allows folks to drag down menus
			 without having every submenu popup open.
			*/
			if (newItem != item) {
				SelectItem(newItem, DONT_SHOW_MENU);
			} else if (newItem && (sub=newItem->Submenu()) != 0 && !sub->Window()) {
				if ((abs((int) (globalLoc.x - prevLoc.x)) < 2) &&
					(abs((int) (globalLoc.y - prevLoc.y)) < 2)) {

					/*
					 Using a 'delay' to prevent submenus from being displayed 
					 too easilty. This allows for 2 things:
					 	o user can drag past items w/ submenus w/o showing
						  those menus
						o Prevent submenus getting drawn when all user wants to
						  do it to select the parent menu item.
					 
					 The idea is that the mouse should remain relatively
					 still for a short while.

					 For small submenus (between 1 and 19 items) the delay
					 is shortened since those will draw faster. Submenus
					 with 0 items are assumed to be special. Their contents
					 are probably calcualted on the fly so we guess that they
					 will take awhile so the delay is long.
					*/

					if ((show_delay_cycles > 0) && (show_delay_iter == 0)) {
						show_delay_iter = iter;
						int32 ci = sub->CountItems();
						if ((ci > 0) && (ci < 20)) {
							show_delay_iter -= (show_delay_cycles / 2);
						}
					} else if (iter >= (show_delay_iter + show_delay_cycles)) {
						SelectItem(newItem, SHOW_MENU);
					}
				}
			}
		} else if (!wFrame.Contains(loc) && OverSuper(loc)) {
			/*
			Somewhere else over a "parent" so close this menu. The
			chain of menus will "unwind" until we get to the menu that
			contains the mouse.
			*/
			fState = IDLE_MENU;
			item = NULL;
			goto exit_loop;
		} else if (in_scroll_area(wFrame, loc) && menu_window->UpScroller()) {
			if (!be_app->IsCursorHidden()) {
				// auto scrolling
				SelectItem(NULL);
				
				bool fast;
				if (ScrollMenu(bounds, loc, &fast)) {
					fTrackNewBounds = true;
					snoozeTime = fast ? 30 : 140;
				}
			}
		} else {
			if (item && (prevLoc != globalLoc)) {
				SelectItem(NULL);
			}
		}

bottom:

		if (_gstartup_phase_) {
			bigtime_t	cur_time = system_time();
			if ((cur_time > _gtrack_start_time_ + click_speed) ||
				(abs((int) (initialLoc.x - globalLoc.x)) +
				 abs((int) (initialLoc.y - globalLoc.y)) > 2)) {
				_gstartup_phase_ = false;
			}
		}
		first_time = false;

		prevLoc = globalLoc;
		window->Unlock();
	}

exit_loop:

	SelectItem(item, DONT_SHOW_MENU);
	
	if (item) {
		InvokeItem(item);
		SelectItem(NULL);
		fState = EXIT_TRACKING;
	}
	
	fKeyPressed = false;
	
	if (window)
		window->Unlock();

	if (fState == POP_TRACKING) {
		// want to close ourselves
		BMenu		*super = Supermenu();
		BMenuItem	*superItem = Superitem();
		ASSERT((super));
		super->SelectItem(superItem, DONT_SHOW_MENU);
		fState = IDLE_MENU;
	}

	*action = fState;
	return fChosenItem;
}

/* ---------------------------------------------------------------- */

bool BMenu::ScrollMenu(BRect bounds, BPoint loc, bool *fast)
{
	BMenuItem		*tmp;
	BPoint			pt;
	bool			result = false;
	BRect			b;
	float			scrollBy = 0;
	BMenuScroller	*scroller;
	BMenuWindow		*w = dynamic_cast<BMenuWindow *>(Window());
	BPoint			globalPt;

	globalPt = loc;
	ConvertToScreen(&globalPt);

	ASSERT(w);

	if (loc.y < bounds.top) {
		// scrolling up

		// Find the item at top of the menu view.
		pt.x = 0;
		pt.y = bounds.top - 1;
		tmp = HitTestItems(pt);
		if (tmp) {
			b = tmp->Frame();
			scrollBy = -(bounds.top - b.top);

			scroller = w->UpScroller();
			b = scroller->Bounds();
			scroller->ConvertToScreen(&b);
			*fast = (globalPt.y < ((b.top + b.bottom)/2.0));

			result = true;
		}
	} else if (loc.y > bounds.bottom) {
		// scrolling down
		pt.x = 0;
		pt.y = bounds.bottom + 1;
		tmp = HitTestItems(pt);
		if (tmp) {
			b = tmp->Frame();
			scrollBy = b.bottom - bounds.bottom;

			scroller = w->DownScroller();
			b = scroller->Bounds();
			scroller->ConvertToScreen(&b);
			*fast = (globalPt.y > ((b.top + b.bottom)/2.0));

			result = true;
		}
	}

	if (result) {
		ScrollBy(0, scrollBy);
	}

	return result;
}

/* ---------------------------------------------------------------- */

void BMenu::InvokeItem(BMenuItem *item, bool now)
{
	if (!item->IsEnabled())
		return;
		
	if (!item->Submenu() && LockLooper()) {
		/*
		Don't do this flashing stuff for hierarchical menus.
		Also make sure that the menu is open (not using key shortcut).
		*/
		snooze(50000); item->Select(true); Sync();
		snooze(50000); item->Select(false); Sync();
		snooze(50000); item->Select(true); Sync();
		snooze(50000); item->Select(false); Sync();
		UnlockLooper();
	}
	if (now)
		item->Invoke();
		
	fChosenItem = item;
}

/* ---------------------------------------------------------------- */

void BMenu::ItemMarked(BMenuItem *item)
{
	if ((fLayout != B_ITEMS_IN_MATRIX) && (fPad.left != standard_pad.left)) {
		fPad.left = standard_pad.left;
		InvalidateLayout();
	}

	if (IsRadioMode()) {
		BMenuItem *prev = FindMarked();
		if (prev)
			prev->SetMarked(false);
		if (IsLabelFromMarked()) {
			if (Superitem()) {
				Superitem()->SetLabel(item->Label());
			}
		}
	}
}

/* ---------------------------------------------------------------- */

void BMenu::DrawBackground(BRect updateRect)
{
	BRect bounds(Bounds());
	const rgb_color c = HighColor();
	SetHighColor(ViewColor());
	FillRect(bounds & updateRect);
	SetHighColor(c);
}

/*---------------------------------------------------------------*/

void BMenu::SetFont(const BFont *font, uint32 mask)
{
	/*
	 This function/override did not exist in R5. So if adding functionality
	 here must consider the implications of that fact.
	*/
	BView::SetFont(font, mask&~B_FONT_SPACING);

// 1/17/2001 - mathias: I don't think we should call InvalidateLayout() here
// because it makes the whole menu flicker if called from BMenuItem::DrawItem() for eg.
// (which is legal and used by BMenuItem itelf to draw the arrows).
//	InvalidateLayout();
}

/* ---------------------------------------------------------------- */

void BMenu::GetPreferredSize(float *width, float *height)
{
	ComputeLayout(0, true, false, width, height);
}

void BMenu::ResizeToPreferred()
{
	BView::ResizeToPreferred();
}

bool BMenu::DrawSnakeSelection() const
{
	return BMenu::sMenuInfo.zsnake;
}

bool BMenu::DrawNewLook() const
{
	return true;
}

void BMenu::Draw(BRect updateRect)
{

	if (!fUseCachedMenuLayout) {
		LayoutItems(0);
		UpdateWindowViewSize();
		Sync();
		Invalidate();	//Do we really want to comment this out?  HL.  No.
		return;			//Do we really want to comment this out?  HL.  No.
	}

	DrawBackground(updateRect);
	DrawItems(updateRect);

	if (!fSkipSnakeDraw && DrawSnakeSelection()) {
		BMenuFrame *menuFrame = dynamic_cast<BMenuFrame *>(Window()->ChildAt(0));
		if (menuFrame)
			DrawBackgroundSnake(menuFrame);
	}
}

#if DEBUG
static void
DebugShowHideClipRegion(BView *view)
{
	view->InvertRect(view->Bounds());
	view->Sync();
}

static void
DebugShowHideClipRegion(BView *view, BRect rect)
{
	view->InvertRect(rect);
	view->Sync();
}

#else
inline void DebugShowHideClipRegion(BView *) {}
inline void DebugShowHideClipRegion(BView *, BRect) {}
#endif

void BMenu::DrawItemDirect(BRect frame)
{
	BMenuFrame *menuFrame = NULL;

	bool drawSnakeFirst = false;
	bool drawSnakeLast = true;
	
	BRect snakeDrawRect;

	// called by selection change 
	if (DrawSnakeSelection()) {

		frame.InsetBy(-10, -2);	
		menuFrame = dynamic_cast<BMenuFrame *>(Window()->ChildAt(0));
		if (menuFrame) {
					
			BPoint scrollOffset(Frame().LeftTop() - Bounds().LeftTop());
			BPoint inverseScrollOffset(-scrollOffset.x, -scrollOffset.y);
							
			snakeDrawRect = frame;
			snakeDrawRect.OffsetBy(scrollOffset);

			bool snakeOn = fSelected && (fSelected->IsEnabled() || fSelected->Submenu())
				&& Superitem();

			if (fDrawItemClipWindow
				&& (snakeOn ||  menuFrame->fLastSnakeSelectionRect.Width() != 0)) {

				drawSnakeFirst = true;

				// check if we need to undraw a snake or part of it when
				// selection moves towards the superitem or goes away altogether
				
				BRect clipRect(menuFrame->fLastSnakeSelectionRect);
				clipRect.right += 1;
				clipRect.bottom += 1;
				clipRect.OffsetBy(inverseScrollOffset);
				BRect selectedItemRect(clipRect);
				if (fSelected && (fSelected->IsEnabled() || fSelected->Submenu()))
					// drawing a snake to the next selected item
					selectedItemRect = fSelected->Frame();

				if (clipRect.Width() && clipRect.top <= selectedItemRect.top
					&& clipRect.bottom >= selectedItemRect.bottom) {
					// at this point there is either no new selected item or
					// the selection snake grew shorter; we need to undraw the part
					// of the snake that is gone
					
					selectedItemRect = selectedItemRect | frame;
					
					BRect offsetClipRect(selectedItemRect & clipRect);

					offsetClipRect.OffsetBy(scrollOffset);
					drawSnakeLast = false;
					snakeDrawRect = offsetClipRect;
				} else if (menuFrame->fLastSnakeSelectionRect.Width()) {
					// snake grows - 
					// we only need to draw the new snake part right next to
					// the item
					snakeDrawRect.left = menuFrame->fLastSnakeSelectionRect.left;
					snakeDrawRect.right = menuFrame->fLastSnakeSelectionRect.right;
				}
			}
		}
	}
	
	if (drawSnakeFirst) {
DebugShowHideClipRegion(menuFrame, snakeDrawRect);
DebugShowHideClipRegion(menuFrame, snakeDrawRect);
		ASSERT(menuFrame);
		menuFrame->PushState();
		menuFrame->SetLowColor(menuFrame->ViewColor());
		menuFrame->FillRect(snakeDrawRect, B_SOLID_LOW);
		menuFrame->Draw(snakeDrawRect, true, fDrawItemClipWindow);
		menuFrame->PopState();
#if DEBUG
menuFrame->Sync();
#endif
	}		

	fSkipSnakeDraw = !drawSnakeLast;
	Invalidate(frame);
	Window()->UpdateIfNeeded();
	fSkipSnakeDraw = false;
}

namespace BPrivate {

struct snake_colors {
	snake_colors(BView* source, bool clipOnly = false)
		:	background(clipOnly ? kDummyColor : ui_color(B_MENU_SELECTED_BACKGROUND_COLOR)),
			outline(ui_color(B_MENU_SELECTED_BORDER_COLOR)),
			shine(source->ViewColor().blend(ui_color(B_SHINE_COLOR), 186)),
			parent(ui_color(B_MENU_BACKGROUND_COLOR))
	{
	}
	
	rgb_color background;
	rgb_color outline;
	rgb_color shine;
	rgb_color parent;
};

}
using namespace BPrivate;

void BMenu::DrawItems(BRect updateRect)
{
	ASSERT((Window()));

	const snake_colors colors(this, false);
	
	int32 count = CountItems();
	for (int32 index = 0; index < count; index++) {
		BMenuItem *item = ItemAt(index);
		if (!item)
			break;
		if (updateRect.Intersects(item->Frame())) {
			bool drawSelection = item->IsSelected() && (item->IsEnabled() || item->Submenu());
			bool snakeSelection = DrawNewLook() && drawSelection;
			
			PushState();
			SetDrawingMode(B_OP_COPY);
				// - if I don't do the above, Tracker menus draw wrong
				// I shouldn't need this though - check with George
				
			if (snakeSelection)
				SetLowColor(colors.background);
				
			item->Draw();
			
			if (snakeSelection) {
				// the item is already drawn on a selection-color background,
				// just outline it
				SetHighColor(colors.outline);
				BRect rect(item->Frame());
				rect.right -= 1;
				rect.bottom -= 1;
				StrokeRoundRect(rect, kItemSelectionRadius, kItemSelectionRadius);
				rect.InsetBy(-1, -1);
				BeginLineArray(3);
				// add a drop shadow to the outline
				AddLine(BPoint(rect.left + kItemSelectionRadius, rect.bottom),
					BPoint(rect.right - kItemSelectionRadius, rect.bottom), colors.shine);
				AddLine(BPoint(rect.right, rect.top + kItemSelectionRadius),
					BPoint(rect.right, rect.bottom - kItemSelectionRadius), colors.shine);
				AddLine(BPoint(rect.right, rect.bottom - kItemSelectionRadius),
					BPoint(rect.right - kItemSelectionRadius, rect.bottom), colors.shine);
				EndLineArray();
			}
			PopState();
		}
	}
}

void 
BMenu::ClipBackgroundSnake(BView *menuFrameView)
{
	if (DrawSnakeSelection())
		ClipDrawSnakeCommon(menuFrameView, true);
}

void 
BMenu::DrawBackgroundSnake(BView *menuFrameView)
{
	if (DrawSnakeSelection())
		ClipDrawSnakeCommon(menuFrameView, false);
}

inline void
_DrawOrClipZCommon(BView *target, bool clipOnly, BRect startRect, BRect middleRect,
	BRect endRect, bool down, bool right, const snake_colors& colors)
{
	target->BeginLineArray(clipOnly ? 5 : 12);
	
	if (down) {
		// drawing or clipping, we need this to complete the outline of the selection
		target->AddLine(BPoint(middleRect.left - 2, startRect.bottom),
			BPoint(middleRect.left, startRect.bottom + 2), colors.outline);
		target->AddLine(BPoint(middleRect.right, endRect.top - 2),
			BPoint(middleRect.right + 2, endRect.top), colors.outline);
		target->AddLine(endRect.LeftBottom(),
			endRect.LeftBottom() + BPoint(-2, -2), colors.outline);
		target->AddLine(startRect.RightTop(),
			startRect.RightTop() + BPoint(2, 2), colors.outline);
		
		// touch up single pixel to look like it is showing from supermenu
		if (right)
			target->AddLine(startRect.RightTop(),
				startRect.RightTop() + BPoint(1, 0), colors.parent);	
		else
			target->AddLine(endRect.LeftBottom(),
				endRect.LeftBottom() + BPoint(-1, 0), colors.parent);	
	} else {
		target->AddLine(BPoint(middleRect.left - 2, startRect.top),
			BPoint(middleRect.left, startRect.top - 2), colors.outline);
		target->AddLine(BPoint(middleRect.right, endRect.bottom + 2),
			BPoint(middleRect.right + 2, endRect.bottom), colors.outline);
		target->AddLine(endRect.LeftTop(),
			endRect.LeftTop() + BPoint(-2, 2), colors.outline);
		target->AddLine(startRect.RightBottom(),
			startRect.RightBottom() + BPoint(1, -2), colors.outline);

		// touch up single pixel to look like it is showing from supermenu
		if (right)
			target->AddLine(startRect.RightBottom(),
				startRect.RightBottom() + BPoint(1, 0), colors.parent);	
		else
			target->AddLine(endRect.LeftTop(),
				endRect.LeftTop() + BPoint(-1, 0), colors.parent);	
	}
	
	if (!clipOnly) {
		// if we are actually drawing, we now need to stroke the outline black line
		if (down) {

			target->AddLine(startRect.LeftTop(), startRect.RightTop(), colors.outline);
			target->AddLine(startRect.LeftBottom(), startRect.RightBottom()
				+ BPoint(-4, 0), colors.outline);			
			target->AddLine(endRect.RightTop() + BPoint(-2, 0),
				endRect.RightTop(), colors.outline);
			target->AddLine(endRect.LeftBottom(), endRect.RightBottom(), colors.outline);
			target->AddLine(BPoint(middleRect.left, startRect.bottom + 2),
				middleRect.LeftBottom(), colors.outline);
			target->AddLine(middleRect.RightTop(),
				BPoint(middleRect.right, endRect.top - 2), colors.outline);

			target->AddLine(BPoint(endRect.left, endRect.bottom+1),
							BPoint(endRect.right, endRect.bottom+1), colors.shine);
			
		} else {
			target->AddLine(startRect.LeftTop(), startRect.RightTop()
				+ BPoint(-4, 0), colors.outline);
			target->AddLine(startRect.LeftBottom(), startRect.RightBottom(), colors.outline);			
			target->AddLine(endRect.LeftTop(), endRect.RightTop(), colors.outline);
			target->AddLine(endRect.LeftBottom() + BPoint(4, 0),
				endRect.RightBottom(), colors.outline);
			target->AddLine(BPoint(middleRect.left, startRect.top - 2),
				middleRect.LeftTop(), colors.outline);
			target->AddLine(BPoint(middleRect.right, endRect.bottom + 2),
				middleRect.RightBottom(), colors.outline);
		}
	}
	target->EndLineArray(); 
}

void
BMenu::ClipDrawSnakeCommon(BView *target, bool clipOnly)
{
	BMenuFrame *menuFrame = dynamic_cast<BMenuFrame *>(target);

	if (!fSelected || (!fSelected->IsEnabled() && !fSelected->Submenu())
		|| !Superitem()) {
		menuFrame->fLastSnakeSelectionRect = BRect(0, 0, 0, 0);
		return;
	}

	target->PushState();

	const snake_colors colors(this, clipOnly);
	
	target->SetHighColor(colors.background);

	BMenuWindow *window = dynamic_cast<BMenuWindow *>(Window());
	BRect superitemRect(menuFrame->fCachedSuperitemRect);

	BRect selectedFrame(fSelected->Frame());
	selectedFrame.right-=1;
	selectedFrame.bottom-=1;
	
	float scrollOffset = Frame().top - Bounds().top;
	if (menuFrame->fWindow->UpScroller())
		scrollOffset -= SCRL_HEIGHT;

	selectedFrame.OffsetBy(0, scrollOffset);

	if (selectedFrame.top == superitemRect.top + kVerticalMenuGap) {
		// trivial case, just connect the two item selections with a straight band
		BRect rect(Bounds());
		rect.top = superitemRect.top + kVerticalMenuGap;
		rect.bottom = superitemRect.bottom + kVerticalMenuGap;

		if (selectedFrame.right > superitemRect.right) {
			rect.right = selectedFrame.left + kHorizontalMenuGap + 1;
			rect.left += 2;
		} else {
			rect.left = rect.right;
			rect.right += kHorizontalMenuGap + 4;
		}
		
		if (window && window->UpScroller())
			rect.OffsetBy(0, SCRL_HEIGHT);

		target->FillRect(rect);
		target->BeginLineArray(clipOnly ? 1 : 3);
		if (!clipOnly) {
			target->AddLine(rect.LeftTop(), rect.RightTop(), colors.outline);
			target->AddLine(rect.LeftBottom(), rect.RightBottom(), colors.outline);
		}
		rect.bottom += 1;
		target->AddLine(rect.LeftBottom(), rect.RightBottom(), colors.shine);
		target->EndLineArray(); 
		
		// remember the snake size so we know how much to update during next change
		menuFrame->fLastSnakeSelectionRect = rect;
		
	} else {

		// draw a Z-shaped selection snake, either reaching up or down from a submenu to the
		// selected item
	
		// compose the shape from two rectangles butted to the respective ends of the menu items
		// connected by a narrow third rectangle, then smooth off the edges with a few lines
		
		const int32 kStartRectWidth = 5;
		const int32 kEndRectWidth = 7;
	
		BRect startRect;
		BRect middleRect;
		BRect endRect;
	
		if (selectedFrame.right > superitemRect.right) {
			startRect = BRect(0, superitemRect.top + kVerticalMenuGap,
				kStartRectWidth - 1, superitemRect.bottom + kVerticalMenuGap);
			endRect = BRect(startRect.right,
				selectedFrame.top,
				startRect.right + kEndRectWidth - 1,
				selectedFrame.bottom);
		} else {
			startRect = BRect(
				selectedFrame.right - kEndRectWidth + 3 + kHorizontalMenuGap + kSnakeBendRadius,
				selectedFrame.top,
				selectedFrame.right + 3 + kHorizontalMenuGap + kSnakeBendRadius,
				selectedFrame.bottom);
			endRect = BRect(startRect.right,
				superitemRect.top + kVerticalMenuGap,
				startRect.right + kStartRectWidth,
				superitemRect.bottom + kVerticalMenuGap);
		}

#ifdef GEORGE_THIS_IS_THE_BUG_THAT_TRIGGERS_THE_INVERSE_RECT_FILL_CRASH
		if (startRect.top > endRect.top)
#else
		if (startRect.top < endRect.top)
			// this if is correct the above is wrong, for debugging only
#endif
			middleRect = BRect(startRect.right - (kSnakeBendRadius - 1),
				startRect.top + (kSnakeBendRadius - 1),
				endRect.left + (kSnakeBendRadius - 1),
				endRect.bottom - (kSnakeBendRadius - 1));
		else 
			middleRect = BRect(startRect.right - (kSnakeBendRadius - 1),
				endRect.top + (kSnakeBendRadius - 1),
				endRect.left + (kSnakeBendRadius - 1),
				startRect.bottom - (kSnakeBendRadius - 1));

		if (window && window->UpScroller()) {
			startRect.OffsetBy(0, SCRL_HEIGHT);
			endRect.OffsetBy(0, SCRL_HEIGHT);
			middleRect.OffsetBy(0, SCRL_HEIGHT);
		}


#if 0
DebugShowHideClipRegion(target, startRect);
DebugShowHideClipRegion(target, startRect);
DebugShowHideClipRegion(target, middleRect);
DebugShowHideClipRegion(target, middleRect);
DebugShowHideClipRegion(target, endRect);
DebugShowHideClipRegion(target, endRect);
#endif

		target->FillRect(startRect);
		target->FillRect(middleRect);
		target->FillRect(endRect);

		if (selectedFrame.right > superitemRect.right)
			_DrawOrClipZCommon(target, clipOnly, startRect, middleRect, endRect,
				selectedFrame.top > superitemRect.top, true, colors);
		else 
			_DrawOrClipZCommon(target, clipOnly, startRect, middleRect, endRect,
				selectedFrame.top < superitemRect.top, false, colors);

		menuFrame->fLastSnakeSelectionRect = startRect | endRect;
	}
	
	target->PopState();
}

/* ---------------------------------------------------------------- */

int BMenu::State(BMenuItem **chosenItem) const
{	
	if (fState > NOW_TRACKING) {
		if (chosenItem)
			*chosenItem = fChosenItem;
		return fState;
	}
	
	BMenuItem *item = CurrentSelection();

	if (item && item->Submenu()) {
		return item->Submenu()->State(chosenItem);
	}
	
	return fState;
}

/* ---------------------------------------------------------------- */

bool BMenu::OverSubmenu(BMenuItem *item, BPoint loc)
{
	BMenu		*m = item->Submenu();

	if (m && m->Window()) {
		BRect mRect = m->Window()->Frame();
		// Only want to test with main body of the menu.
		mRect.InsetBy(kSnakeGap, kSnakeGap);
		if (mRect.Contains(loc))
			return true;
			
		BMenuItem *si = m->CurrentSelection();
		if (si)
			return m->OverSubmenu(si, loc);
	}
	return false;
}

/* ---------------------------------------------------------------- */

bool BMenu::OverSuper(BPoint loc)
{
	BMenu	*base = this;
	BMenu	*super;
	BRect	bounds;

	ConvertToScreen(&loc);				// convert to match bounds
	
	while ((super = base->Supermenu()) != 0) {
		bounds = base->Superbounds();	// these are in global screen
		if (bounds.Contains(loc))
			return true;
		base = super;	
	}
	
	return false;
}


/* ---------------------------------------------------------------- */

void BMenu::SetEnabled(bool state)
{
	BMenuItem	*item = Superitem();

	fEnabled = state;

	// if this menu is visible then force a redraw
	if (LockLooper()) {
		Invalidate();
		UnlockLooper();
	}

	if (!item)
		return;
	
	if (item && item->fEnabled != state)
		item->SetEnabled(state);
	
	// redraw the item that controls this menu 
	BMenu	*base = Supermenu();
	if (base && base->LockLooper()) {
		BRect	b = item->Frame();
		base->Invalidate(b);
		base->UnlockLooper();
	}
}

/* ---------------------------------------------------------------- */

bool BMenu::IsEnabled() const
{
	BMenu	*super = Supermenu();

	return fEnabled && (!super || super->IsEnabled());
}

/* ---------------------------------------------------------------- */

void BMenu::SetLabelFromMarked(bool on)
{
	fDynamicName = on;
	if (on)
		SetRadioMode(true);
}

/* ---------------------------------------------------------------- */

bool BMenu::IsLabelFromMarked()
{
	return fDynamicName;
}

/* ---------------------------------------------------------------- */

void BMenu::SetRadioMode(bool on)
{
	fRadioMode = on;
	if (!on)
		SetLabelFromMarked(false);
}

/* ---------------------------------------------------------------- */

bool BMenu::IsRadioMode() const
{
	return fRadioMode;
}

/* ---------------------------------------------------------------- */

BRect BMenu::Superbounds() const
{
	ASSERT((fSuper));
	return fSuperbounds;
}

/* ---------------------------------------------------------------- */

BMenu *BMenu::Supermenu() const
{
	return fSuper;
}

/* ---------------------------------------------------------------- */

BMenuItem *BMenu::Superitem() const
{
	return fSuperitem;
}

/* ---------------------------------------------------------------- */

bool BMenu::SelectNextItem(BMenuItem *start, bool forward)
{
	BMenuItem	*item;
	
	item = NextItem(start, forward);
	if (item)
		SelectItem(item, KEEP_SAME, true);

	return (item != NULL);
}

/* ---------------------------------------------------------------- */

BMenuItem *BMenu::NextItem(BMenuItem *start, bool forward) const
{
	long		oldIndex;
	long		maxIndex;
	long		newIndex;
	long		count;
	BMenuItem	*item;
	
	maxIndex = CountItems() - 1;
	if (maxIndex == -1)
		return NULL;

	if (start)
		oldIndex = IndexOf(start);
	else
		oldIndex = maxIndex + 1;

	newIndex = oldIndex;
	count = 0;

	/*
	Go into a loop looking for the next menu item to select.
	Need to make sure we don't go into infinite loop if the
	menu doesn't have any enabled menu items.
	*/
	do {
		if (forward)
			newIndex = (newIndex >= maxIndex) ? 0 : newIndex + 1;
		else
			newIndex = (newIndex == 0) ? maxIndex : newIndex - 1;
		item = ItemAt(newIndex);
		if (item->IsEnabled() || item->Submenu()) {
			// if we don't care about "visibility" then we're done.
			// otherwise make sure the selected item is visible.

			if (!fIgnoreHidden || IsItemVisible(item))
				break;
		}
		item = NULL;
	} while (++count <= maxIndex);
	
	return item;
}

/* ---------------------------------------------------------------- */

bool BMenu::IsItemVisible(BMenuItem *item) const
{
	BRect b = item->Frame();
	BRect w = Window()->Frame();
	BScreen screen( Window() );

	w = w & screen.Frame();
	ConvertToScreen(&b);

	return w.Intersects(b);
}

/* ---------------------------------------------------------------- */

void BMenu::SetIgnoreHidden(bool state)
{
	fIgnoreHidden = state;
}

/* ---------------------------------------------------------------- */

BMenuWindow *BMenu::MenuWindow()
{
	if (!fCachedMenuWindow) {
		char buf[100];
		sprintf(buf, "cached[%s]", Name());
		fCachedMenuWindow = new BMenuWindow(buf, DrawNewLook());
	}
	
	return fCachedMenuWindow;
}

/* ---------------------------------------------------------------- */

void BMenu::DeleteMenuWindow()
{
	if (fCachedMenuWindow) {
		if (fCachedMenuWindow->Lock())
			fCachedMenuWindow->Close();
		fCachedMenuWindow = NULL;
	}
}

/* ---------------------------------------------------------------- */

menu_layout BMenu::Layout() const
{
	return fLayout;
}

/* ---------------------------------------------------------------- */

bool BMenu::IsRedrawAfterSticky() const
{
	return fRedrawAfterSticky;
}

/* ---------------------------------------------------------------- */

bool BMenu::IsStickyMode() const
{
	if (fSuper)
		return fSuper->IsStickyMode();
	else
		return fStickyMode;
}

/* ---------------------------------------------------------------- */

void BMenu::SetStickyMode(bool on)
{
	if (fSuper)
		fSuper->SetStickyMode(on);
	else {
		fStickyMode = on;

		// BMenu::SetStickyMode() isn't virtual...  
		// Poor Man's C++ (or something like that)
		if (fStickyMode) {
			BMenuBar *mbar = dynamic_cast<BMenuBar *>(this);
			if (mbar != NULL) {
				if (LockLooper()) {
					mbar->StealFocus();
					mbar->UnlockLooper(); 
				}
			}
		}
	}
}

/* ---------------------------------------------------------------- */

void BMenu::SetMaxContentWidth(float width)
{
	fMaxContentWidth = width;
	fUseCachedMenuLayout = false;
}

/* ---------------------------------------------------------------- */

float BMenu::MaxContentWidth() const
{
	return fMaxContentWidth;
}

/* ---------------------------------------------------------------- */

void BMenu::SetTriggersEnabled(bool on)
{
	fTriggerEnabled = on;
}

/* ---------------------------------------------------------------- */

bool BMenu::AreTriggersEnabled() const
{
	return (fTriggerEnabled);
}

/* ---------------------------------------------------------------- */

status_t get_menu_info(menu_info *info)
{
	memcpy(info, &BMenu::sMenuInfo, sizeof(menu_info));
	return B_NO_ERROR;
}

/* ---------------------------------------------------------------- */

status_t set_menu_info(menu_info *info)
{
	BMessage update;
	
	if (info->font_size != BMenu::sMenuInfo.font_size ||
			strcmp(info->f_family, BMenu::sMenuInfo.f_family) != 0 ||
			strcmp(info->f_style, BMenu::sMenuInfo.f_style) != 0) {
		BFont f;
		f.SetFamilyAndStyle(info->f_family, info->f_style);
		f.SetSize(info->font_size);
		update.AddFlat(B_UI_MENU_ITEM_TEXT_FONT, &f);
	}
	if (info->background_color != BMenu::sMenuInfo.background_color)
		update.AddRGBColor(B_UI_MENU_BACKGROUND_COLOR, info->background_color);
	if (info->separator != BMenu::sMenuInfo.separator)
		update.AddInt32(B_UI_MENU_SEPARATOR, info->separator);
	if (info->triggers_always_shown != BMenu::sMenuInfo.triggers_always_shown)
		update.AddBool(B_UI_MENU_SHOW_TRIGGERS, info->triggers_always_shown);
	
	if (!update.IsEmpty()) {
		_b_cache_menu_info(update);
		return update_ui_settings(update);
	}
	
	return B_NO_ERROR;
}

/* ---------------------------------------------------------------- */

void _b_cache_menu_info(const BMessage& src)
{
	rgb_color c;
	BFont f;
	int32 i;
	bool b;
	
	// This has to be initialized somewhere, so might was well do it here.
	_menu_info_ptr_ = &BMenu::sMenuInfo;

	if (src.FindFlat(B_UI_MENU_ITEM_TEXT_FONT, &f) == B_OK) {
		BMenu::sMenuInfo.font_size = f.Size();
		f.GetFamilyAndStyle(&BMenu::sMenuInfo.f_family, &BMenu::sMenuInfo.f_style);
		BMenu::sMenuInfo.item_font = f;
	}
	if (src.FindRGBColor(B_UI_MENU_BACKGROUND_COLOR, &c) == B_OK) {
		BMenu::sMenuInfo.background_color = c;
		// oh gack.  it looks like the stupid menu
		// preferences can create colors with a 0 alpha!!
		BMenu::sMenuInfo.background_color.alpha = 255;
	}
	if (src.FindInt32(B_UI_MENU_SEPARATOR, &i) == B_OK)
		BMenu::sMenuInfo.separator = i;
	if (src.FindBool(B_UI_MENU_SHOW_TRIGGERS, &b) == B_OK)
		BMenu::sMenuInfo.triggers_always_shown = b;
	if (src.FindBool(B_UI_MENU_ZSNAKE, &b) == B_OK)
		BMenu::sMenuInfo.zsnake = b;
	
	// There are issues with things like right-drag popup menus when
	// turning the sticky menu option off, so for now make it always
	// on.
	BMenu::sMenuInfo.click_to_open = true;
}

/* ---------------------------------------------------------------- */

void BMenu::CacheFontInfo()
{
	if (fAscent == -1.0) {
		font_height	finfo;
		GetFontHeight(&finfo);
		fAscent = finfo.ascent;
		fDescent = finfo.descent;
		fFontHeight = ceil(finfo.ascent + finfo.descent + finfo.leading);
	}
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

float pt_distance(BPoint p1, BPoint p2)
{
	return sqrt(((p1.x - p2.x) * (p1.x - p2.x)) +
		((p1.y - p2.y) * (p1.y - p2.y)));
}

float	pt_distance(BPoint curLoc, BRect frame)
{
	/*

				  |       |
			  1   |   2   |   3
				  |       |
			----- ._______. ------
				  |       |
			  4   |       |   5
				  |       |
			----- ._______. ------

				  |       |
			  6   |   7   |   8
				  |       |

	*/

	if (frame.Contains(curLoc)) {
		return 0;
	}

	float d;

	if (curLoc.y < frame.top) {
		// --- quadrant 1, 2, or 3 ---

		if (curLoc.x < frame.left)
			// --- quadrant 1 ---
			d = pt_distance(curLoc, frame.LeftTop());

		else if (curLoc.x <= frame.right)
			// --- quadrant 2 ---
			d = frame.top - curLoc.y;

		else
			// --- quadrant 3 ---
			d = pt_distance(curLoc, frame.RightTop());

	} else if (curLoc.y > frame.bottom) {

		// --- quadrant 6, 7, or 8 ---

		if (curLoc.x < frame.left)
			// --- quadrant 6 ---
			d = pt_distance(curLoc, frame.LeftBottom());

		else if (curLoc.x <= frame.right)
			// --- quadrant 7 ---
			d = curLoc.y - frame.bottom;

		else
			// --- quadrant 8 ---
			d = pt_distance(curLoc, frame.RightBottom());

	} else {

		// --- quadrant 4 or 5 ---

		if (curLoc.x < frame.left)
			// --- quadrant 4 ---
			d = frame.left - curLoc.x;

		else
			// --- quadrant 5 ---
			d = curLoc.x - frame.right; 
	}

	return d;
}

/* ---------------------------------------------------------------- */

bool BMenu::IsStickyPrefOn()
{
	return sMenuInfo.click_to_open;
}

/* ---------------------------------------------------------------- */

void BMenu::RedrawAfterSticky(BRect bounds)
{
	BMenuItem	*item;

	item = CurrentSelection();
	if (fTriggerEnabled) {
		fRedrawAfterSticky = true;
		DrawItems(bounds);
		fRedrawAfterSticky = false;
		Sync();
	}
	BMenu *m;
	if (item && (m = item->Submenu()) != 0) {
		if (m->LockLooper()) {
			if (item->Submenu()->fTriggerEnabled) {
				item->Submenu()->SelectItem(NULL);
				item->Submenu()->fRedrawAfterSticky = true;
				item->Submenu()->DrawItems(item->Submenu()->Bounds());
				item->Submenu()->fRedrawAfterSticky = false;
			}
			m->UnlockLooper();
		}
	}

	// now redraw going up the menu chain
	BMenu *parent = this;
	while ((parent = parent->Supermenu()) != 0) {
		if (parent->LockLooper()) {
			if (parent->fTriggerEnabled) {
				parent->fRedrawAfterSticky = true;
				parent->DrawItems(parent->Bounds());
				parent->fRedrawAfterSticky = false;
				parent->Sync();
			}
			parent->UnlockLooper();
		}
	}
}

/*----------------------------------------------------------------*/

status_t BMenu::Perform(perform_code d, void *arg)
{
	return BView::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

status_t BMenu::DoEnabledMsg(BMenuItem *_SCRIPTING_ONLY(target_item), BMenu *_SCRIPTING_ONLY(target_menu),
	BMessage *_SCRIPTING_ONLY(msg), BMessage *_SCRIPTING_ONLY(reply)) const
{
#if _SUPPORTS_FEATURE_SCRIPTING
	status_t err = B_OK;

	if (msg->what == B_GET_PROPERTY) {
		if (target_item)
			err = reply->AddBool("result", target_item->IsEnabled());
		else
			err = reply->AddBool("result", target_menu->IsEnabled());
	} else {
		bool	b;
		if ((err = msg->FindBool("data", &b)) == B_OK) {
			if (target_item)
				target_item->SetEnabled(b);
			else
				target_menu->SetEnabled(b);
		} else {
			reply->AddString("message", "expected data was missing");
		}
	}
	return err;
#else
	return B_UNSUPPORTED;
#endif
}

/* ---------------------------------------------------------------- */

status_t BMenu::DoLabelMsg(BMenuItem *_SCRIPTING_ONLY(target_item), BMenu *_SCRIPTING_ONLY(target_menu),
	BMessage *_SCRIPTING_ONLY(msg), BMessage *_SCRIPTING_ONLY(reply)) const
{
#if _SUPPORTS_FEATURE_SCRIPTING
	status_t err = B_OK;

	if (!target_item) {
		target_item = target_menu->Superitem();
		if (!target_item) {
			reply->AddString("message", "this menu doesn't have a label");
			return B_BAD_SCRIPT_SYNTAX;
		}
	}

	if (msg->what == B_GET_PROPERTY) {
			err = reply->AddString("result", target_item->Label());
	} else {
		const char	*str;
		if ((err = msg->FindString("data", &str)) == B_OK) {
			target_item->SetLabel(str);
		} else {
			reply->AddString("message", "expected data was missing");
		}
	}
	return err;
#else
	return B_UNSUPPORTED;
#endif
}

/* ---------------------------------------------------------------- */

status_t BMenu::DoMarkMsg(BMenuItem *_SCRIPTING_ONLY(target_item), BMenu *_SCRIPTING_ONLY(target_menu),
	BMessage *_SCRIPTING_ONLY(msg), BMessage *_SCRIPTING_ONLY(reply)) const
{
#if _SUPPORTS_FEATURE_SCRIPTING
	status_t err = B_OK;

	if (!target_item) {
		target_item = target_menu->Superitem();
		if (!target_item) {
			reply->AddString("message", "this menu can't have a mark");
			return B_BAD_SCRIPT_SYNTAX;
		}
	}

	if (msg->what == B_GET_PROPERTY) {
			err = reply->AddBool("result", target_item->IsMarked());
	} else {
		bool	b;
		if ((err = msg->FindBool("data", &b)) == B_OK) {
			target_item->SetMarked(b);
		} else {
			reply->AddString("message", "expected data was missing");
		}
	}
	return err;
#else
	return B_UNSUPPORTED;
#endif
}

/* ---------------------------------------------------------------- */

status_t BMenu::DoDeleteMsg(BMenuItem *_SCRIPTING_ONLY(target_item), BMenu *_SCRIPTING_ONLY(target_menu),
	BMessage *, BMessage *_SCRIPTING_ONLY(reply)) const
{
#if _SUPPORTS_FEATURE_SCRIPTING
	status_t err = B_OK;

	if (!target_item)
		target_item = target_menu->Superitem();

	if (target_item) {
		target_menu = target_item->Menu();
		ASSERT(target_menu);
		target_menu->RemoveItem(target_item);
		delete target_item;
	} else {
		reply->AddString("message", "This menu cannot be removed");
		err = B_BAD_VALUE;
	}

	return err;
#else
	return B_UNSUPPORTED;
#endif
}

/* ---------------------------------------------------------------- */

status_t BMenu::DoCreateMsg(BMenuItem *_SCRIPTING_ONLY(target_item), BMenu *_SCRIPTING_ONLY(target_menu),
	BMessage *_SCRIPTING_ONLY(msg), BMessage *_SCRIPTING_ONLY(reply), bool _SCRIPTING_ONLY(menu)) const
{
#if _SUPPORTS_FEATURE_SCRIPTING
	status_t err = B_OK;

	const char	*label;

	if (!target_item && !target_menu) {
		reply->AddString("message", "Cannot add a new menu or menu item here");
		return B_BAD_SCRIPT_SYNTAX;
	}

	if ((err = msg->FindString("data", &label)) == B_OK) {
		int32	index;
		if( !target_menu ) target_menu = target_item->Menu();
		ASSERT(target_menu);
		if( target_item ) index = target_menu->IndexOf(target_item);
		else index = target_menu->CountItems();
		
		if( !menu ) {
			// Create a normal menu item.
			BMessenger where;
			BMessage* invoke_message = new BMessage(B_CONTROL_INVOKED);
			if( msg->HasMessage("be:invoke_message") ) {
				msg->FindMessage("be:invoke_message", invoke_message);
			} else if( msg->HasInt32("what") ) {
				msg->FindInt32("what", (int32 *) &(invoke_message->what));
			}
			msg->FindMessenger("be:target", &where);
		
			BMenuItem	*new_item = new BMenuItem(label, invoke_message);
			target_menu->AddItem(new_item, index);
			if( where.IsValid() ) new_item->SetTarget(where);
			
		} else {
			// Create a menu item with a submenu.
			BMenu	*new_menu = new BMenu(label);
			target_menu->AddItem(new_menu, index);
			if( reply ) reply->AddMessenger("result", BMessenger(new_menu));
			
		}
	} else {
		reply->AddString("message", "expected 'data' field was missing");
	}
	return err;
#else
	return B_UNSUPPORTED;
#endif
}

/* ---------------------------------------------------------------- */

status_t BMenu::DoMenuItemMsg(BMenuItem **_SCRIPTING_ONLY(pnext), BMenu *_SCRIPTING_ONLY(target),
	BMessage *_SCRIPTING_ONLY(msg), BMessage *_SCRIPTING_ONLY(reply), BMessage *_SCRIPTING_ONLY(spec),
	int32 _SCRIPTING_ONLY(form)) const
{
#if _SUPPORTS_FEATURE_SCRIPTING
	BMenuItem	*next = NULL;
	status_t	err = B_OK;

	switch (form) {
		case B_REVERSE_INDEX_SPECIFIER:
		case B_INDEX_SPECIFIER: {
			int32	ii = spec->FindInt32("index");
			if (form == B_REVERSE_INDEX_SPECIFIER)
				ii = target->CountItems() - ii;
			next = target->ItemAt(ii);
			if( !next && msg->what == B_CREATE_PROPERTY && ii >= 0 ) {
				// When creating a new property, an index that is larger
				// than the last menu item is valid and means "create at end".
			} else if (!next) {
				err = B_BAD_INDEX;
				reply->AddString("message", "menu item index out of range");
			}
			break;
		}
		case B_NAME_SPECIFIER:
			const char *name = spec->FindString(B_PROPERTY_NAME_ENTRY);
			if (!name) {
				err = B_BAD_SCRIPT_SYNTAX;
				break;
			}
			next = target->FindItem(name);
			if (!next) {
				err = B_NAME_NOT_FOUND;
				reply->AddString("message", "menu item name not found");
			}
			break;
	}

	if (!err) {
		*pnext = next;
	}

	return err;
#else
	return B_UNSUPPORTED;
#endif
}

/* ---------------------------------------------------------------- */

status_t BMenu::DoMenuMsg(BMenuItem **_SCRIPTING_ONLY(pnext), BMenu *_SCRIPTING_ONLY(target), BMessage *_SCRIPTING_ONLY(msg),
	BMessage *_SCRIPTING_ONLY(reply), BMessage *_SCRIPTING_ONLY(spec), int32 _SCRIPTING_ONLY(form)) const
{
#if _SUPPORTS_FEATURE_SCRIPTING
	BMenuItem	*next = NULL;
	status_t	err = B_OK;

	switch (form) {
		case B_REVERSE_INDEX_SPECIFIER:
		case B_INDEX_SPECIFIER: {
			int32	ii = spec->FindInt32("index");
			if (form == B_REVERSE_INDEX_SPECIFIER)
				ii = target->CountItems() - ii;
			next = target->ItemAt(ii);
			if( (!next || !next->Submenu())
					&& msg->what == B_CREATE_PROPERTY && ii >= 0 ) {
				// When creating a new property, an index that is larger
				// than the last menu means "create at end".  An index
				// without a submenu means "create menu here".
			} else if (!next || !next->Submenu()) {
				err = B_BAD_INDEX;
				reply->AddString("message", "menu index out of range");
			}
			break;
		}
		case B_NAME_SPECIFIER:
			const char *name = spec->FindString(B_PROPERTY_NAME_ENTRY);
			if (!name) {
				err = B_BAD_SCRIPT_SYNTAX;
				break;
			}
			next = target->FindItem(name);
			if (!next || !next->Submenu()) {
				err = B_NAME_NOT_FOUND;
				reply->AddString("message", "menu name not found");
			}
			break;
	}

	if (!err) {
		*pnext = next;
	}

	return err;
#else
	return B_UNSUPPORTED;
#endif
}

/*-------------------------------------------------------------*/

#if _SUPPORTS_FEATURE_SCRIPTING
enum {
	PI_MENU,
	PI_MENU_SPECIAL,
	PI_ITEM,
	PI_ITEM_SPECIAL,
	PI_COUNT_ITEMS,
	PI_ENABLED,
	PI_LABEL,
	PI_MARK
};

static property_info prop_list[] = {
	{"Menu",
		{B_CREATE_PROPERTY},
		{B_NAME_SPECIFIER, B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER},
		"Create a new submenu at the selected position, with 'data' its label.  "
		"Returns a BMessenger to the new menu in 'result'.",
		PI_MENU_SPECIAL,
		{},
		{{
			{
				{"data", B_STRING_TYPE}
			}
		}},
		{}
	},
	{"Menu",
		{B_DELETE_PROPERTY},
		{B_NAME_SPECIFIER, B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER},
		"Delete the selected submenu.",
		PI_MENU_SPECIAL,
		{},
		{},
		{}
	},
	{"Menu",
		{},
		{B_NAME_SPECIFIER, B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER},
		0, PI_MENU,
		{},
		{},
		{}
	},
	{"MenuItem",
		{B_EXECUTE_PROPERTY, B_DELETE_PROPERTY},
		{B_NAME_SPECIFIER, B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER},
		"Invoke or delete the selected menu item.",
		PI_ITEM_SPECIAL,
		{},
		{},
		{}
	},
	{"MenuItem",
		{B_CREATE_PROPERTY},
		{B_NAME_SPECIFIER, B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER},
		"Create a new submenu at the selected position, with 'data' its label, "
		"'be_invoke_message' the message it sends when selected or "
		"'what' the code of the invocation message, and "
		"'be:target' the invocation destination.",
		PI_ITEM_SPECIAL,
		{},
		{{
			{
				{"data", B_STRING_TYPE},
				{"be:invoke_message", B_MESSAGE_TYPE},
				{"what", B_INT32_TYPE },
				{"be:target", B_MESSENGER_TYPE }
			}
		}},
		{}
	},
	{"MenuItem",
		{},
		{B_NAME_SPECIFIER, B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER},
		0, PI_ITEM,
		{},
		{},
		{}
	},
	{"MenuItem",
		{B_COUNT_PROPERTIES},
		{B_DIRECT_SPECIFIER},
		0, PI_COUNT_ITEMS,
		{ B_INT32_TYPE },
		{},
		{}
	},
	{"Enabled",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		0, PI_ENABLED,
		{ B_BOOL_TYPE },
		{},
		{}
	},
	{"Label",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		0, PI_LABEL,
		{ B_STRING_TYPE },
		{},
		{}
	},
	{"Mark",
		{B_GET_PROPERTY, B_SET_PROPERTY},
		{B_DIRECT_SPECIFIER},
		0, PI_MARK,
		{ B_BOOL_TYPE },
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
#endif

/* ---------------------------------------------------------------- */

status_t BMenu::ParseMsg(BMessage *_SCRIPTING_ONLY(msg), int32 *_SCRIPTING_ONLY(psindex),
	BMessage *_SCRIPTING_ONLY(spec), int32 *_SCRIPTING_ONLY(pform), const char **_SCRIPTING_ONLY(pprop),
	BMenu **_SCRIPTING_ONLY(tmenu), BMenuItem **_SCRIPTING_ONLY(titem), int32 *_SCRIPTING_ONLY(puser_data),
	BMessage *_SCRIPTING_ONLY(reply)) const
{
#if _SUPPORTS_FEATURE_SCRIPTING
	bool		handled = false;
	int32		sindex = *psindex;
	int32		form = *pform;
	const char	*prop = *pprop;
	status_t	err = B_OK;
	BMenu		*target_menu = const_cast<BMenu*>(this);
	BMenuItem	*target_item = NULL;
	BMenuItem	*menu_item = NULL;
	int32		user_data = 0;

	BPropertyInfo	pi(prop_list);
	int32			i;


	while ((i = pi.FindMatch(msg, sindex, spec, form, prop)) >= 0) {
		user_data = prop_list[i].extra_data;
		switch (user_data) {
			case PI_MENU:
			case PI_MENU_SPECIAL:
//+				PRINT(("Finding menu...\n"));
				err = DoMenuMsg(&menu_item, target_menu, msg, reply, spec, form);
				if( !err && menu_item && menu_item->Submenu() ) {
					target_menu = menu_item->Submenu();
				}
				if (err) {
					// This is an error case
				} else if (user_data == PI_MENU_SPECIAL) {
					ASSERT(sindex == 0);
					handled = true;
					target_item = menu_item;
				} else {
					ASSERT(target_menu);
				}
				break;
			case PI_ITEM:
			case PI_ITEM_SPECIAL:
				err = DoMenuItemMsg(&target_item, target_menu, msg, reply,
						spec, form);
				if( !err && target_item && target_item->Submenu() ) {
					target_menu = target_item->Submenu();
				}
				if (err) {
					// This is an error case
				} else if (user_data == PI_ITEM_SPECIAL) {
					ASSERT(sindex == 0);
					handled = true;
				} 
				break;
			case PI_COUNT_ITEMS:
			case PI_ENABLED:
			case PI_LABEL:
			case PI_MARK:
				// Various generic properties
				ASSERT(sindex == 0);
				handled = true;
				break;
			default:
				ASSERT_WITH_MESSAGE(0,"Someone change the prop_list table");
				err = B_ERROR;
				break;
		}

		if (err || handled)
			break;

		msg->PopSpecifier();
		err = msg->GetCurrentSpecifier(&sindex, spec, &form, &prop);
		if (err) {
			err = B_OK;
			break;
		}
	}

	if (handled) {
		ASSERT(target_menu);
		if (tmenu)
			*tmenu = target_menu;
		if (titem)
			*titem = target_item;
		*psindex = sindex;
		*pform = form;
		*pprop = prop;
		*puser_data = user_data;
	} else {
		if (tmenu)
			*tmenu = target_menu;
		if (titem)
			*titem = NULL;
	}

	return err;
#else
	return B_UNSUPPORTED;
#endif
}

/* ---------------------------------------------------------------- */

void BMenu::MessageReceived(BMessage *msg)
{
	bool		handled = false;
	BMessage	reply(B_REPLY);
#if _SUPPORTS_FEATURE_SCRIPTING
	status_t	err;
	BMessage	spec;
	int32		sindex;
	int32		form;
	const char	*prop;
	BMenu		*target_menu = this;
	BMenuItem	*target_item = NULL;

	if (msg->GetCurrentSpecifier(&sindex, &spec, &form, &prop) == B_OK) {
		BPropertyInfo	pi(prop_list);
		int32			user_data;

		err = ParseMsg(msg, &sindex, &spec, &form, &prop,
			&target_menu, &target_item, &user_data, &reply);

		if (!err && target_menu) {
			handled = true;
			switch (user_data) {
				case PI_MENU_SPECIAL: {
					switch (msg->what) {
						case B_CREATE_PROPERTY:
							DoCreateMsg(target_item, target_menu, msg, &reply, true);
							break;
						case B_DELETE_PROPERTY:
							DoDeleteMsg(NULL, target_menu, msg, &reply);
							break;
					}
					break;
					}
				case PI_ITEM_SPECIAL: {
					ASSERT(msg->what == B_CREATE_PROPERTY || target_item);
					ASSERT(msg->what == B_CREATE_PROPERTY || target_item->Menu());
					switch (msg->what) {
						case B_EXECUTE_PROPERTY:
							Window()->MenusBeginning();
							target_item->Menu()->InvokeItem(target_item, true);
							Window()->MenusEnded();
							break;
						case B_CREATE_PROPERTY:
							DoCreateMsg(target_item, target_menu, msg, &reply, false);
							break;
						case B_DELETE_PROPERTY:
							DoDeleteMsg(target_item, target_menu, msg, &reply);
							break;
					}
					break;
					}
				case PI_ENABLED:
					Window()->MenusBeginning();
					err = DoEnabledMsg(target_item, target_menu, msg, &reply);
					Window()->MenusEnded();
					break;
				case PI_LABEL:
					err = DoLabelMsg(target_item, target_menu, msg, &reply);
					break;
				case PI_MARK:
					err = DoMarkMsg(target_item, target_menu, msg, &reply);
					break;
				case PI_COUNT_ITEMS:
					err = reply.AddInt32("result", target_menu->CountItems());
					break;
				default:
					ASSERT_WITH_MESSAGE(0, "message parsing failed");
			}
		}

		if (err) {
			reply.AddInt32("error", err);
			handled = true;
		}
	}
#endif

	if (handled)
		msg->SendReply(&reply);
	else
		BView::MessageReceived(msg);
}

/*---------------------------------------------------------------*/

BHandler *BMenu::ResolveSpecifier(BMessage *_SCRIPTING_ONLY(msg), int32 _SCRIPTING_ONLY(index),
	BMessage *_SCRIPTING_ONLY(spec), int32 _SCRIPTING_ONLY(form), const char *_SCRIPTING_ONLY(prop))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	BHandler		*target = NULL;
	status_t		err = B_OK;
	BMessage		error_msg(B_MESSAGE_NOT_UNDERSTOOD);
	int32			start_index;

	BMenu		*tm;
	BMenuItem	*ti;
	int32		ud;

	start_index = index;

	err = ParseMsg(msg, &index, spec, &form, &prop, &tm, &ti, &ud, &error_msg);

	msg->SetCurrentSpecifier(start_index);
	
	if (tm) {
		target = this;
	} else if (err) {
		error_msg.AddInt32("error", err);
		msg->SendReply(&error_msg);
	} else {
		msg->SetCurrentSpecifier(start_index);
		target = BView::ResolveSpecifier(msg, index, spec, form, prop);
	}

	return target;
#else
	return NULL;
#endif
}

/*---------------------------------------------------------------*/

status_t	BMenu::GetSupportedSuites(BMessage *_SCRIPTING_ONLY(data))
{
#if _SUPPORTS_FEATURE_SCRIPTING
	data->AddString("suites", "suite/vnd.Be-menu");
	BPropertyInfo	pi(prop_list);
	data->AddFlat("messages", &pi);
	return BView::GetSupportedSuites(data);
#else
	return B_UNSUPPORTED;
#endif
}

/*---------------------------------------------------------------*/

void	BMenu::FrameMoved(BPoint new_position)
{
	BView::FrameMoved(new_position);
}

/*---------------------------------------------------------------*/

void	BMenu::FrameResized(float new_width, float new_height)
{
	BView::FrameResized(new_width, new_height);
}
/* ---------------------------------------------------------------- */

BMenu &BMenu::operator=(const BMenu &) {return *this;}

/*---------------------------------------------------------------*/

void	BMenu::MakeFocus(bool state)
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::MakeFocus(state);
}

/*---------------------------------------------------------------*/

void	BMenu::AllAttached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::AllAttached();
}

/*---------------------------------------------------------------*/

void	BMenu::AllDetached()
{
	/*
	 This function/override did not exist in R3. So if adding functionality
	 here must consider the implications of that fact. Only a concern on PPC,
	 as Intel compatibility was broken in R4.
	*/
	BView::AllDetached();
}

/* ---------------------------------------------------------------- */

//+void BMenu::_ReservedMenu1() {}
//+void BMenu::_ReservedMenu2() {}
//+void BMenu::_ReservedMenu3() {}
void BMenu::_ReservedMenu4() {}
void BMenu::_ReservedMenu5() {}
void BMenu::_ReservedMenu6() {}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

