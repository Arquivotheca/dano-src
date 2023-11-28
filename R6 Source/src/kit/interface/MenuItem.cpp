//*****************************************************************************
//
//	File:		MenuItem.cpp
//
//	Written by:	Peter Potrebic
//
//	Copyright 1994-97, Be Incorporated
//
//*****************************************************************************

#include <Bitmap.h>
#include <Debug.h>
#include <DataIO.h>
#include <MenuItem.h>
#include <MenuBar.h>
#include <Message.h>
#include <OS.h>
#include <PopUpMenu.h>
#include <SupportDefs.h>
#include <Window.h>

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <message_strings.h>
#include <AppDefsPrivate.h>

#include "MenuWindow.h"
#include "MenuPrivate.h"


const char	*kUTF8LeftArrow		= "\xE2\x86\x90";
const char	*kUTF8UpArrow		= "\xE2\x86\x91";
const char	*kUTF8RightArrow	= "\xE2\x86\x92";
const char	*kUTF8DownArrow		= "\xE2\x86\x93";
const char	*kUTF8Enter			= "\xE2\x86\xB5";

/* ---------------------------------------------------------------- */

BMenuItem::BMenuItem(const char *label, BMessage *msg, char shortcut,
	uint32 modifiers)
{
	InitData();

	fLabel = strdup(label);
	SetMessage(msg);

	fShortcutChar = shortcut;
	fModifiers = shortcut ? modifiers : 0;
	if( !(fModifiers&B_NO_COMMAND_KEY) ) fModifiers |= B_COMMAND_KEY;
}

/* ---------------------------------------------------------------- */

BMenuItem::BMenuItem(BMenu *menu, BMessage *message)
{
	InitData();
	
	SetMessage(message);
	InitMenuData(menu);
}

/* ---------------------------------------------------------------- */

void BMenuItem::InitMenuData(BMenu *menu)
{
	fSubmenu = menu;
	menu->fSuperitem = this;

	// Get the view name from the menu and set it here.
	BMenuItem *item;
	if (menu->IsRadioMode() && menu->IsLabelFromMarked() &&
		(item = menu->FindMarked()) != 0)
		SetLabel(item->Label());
	else
		SetLabel(menu->Name());
}

/* ---------------------------------------------------------------- */

#define S_LABEL			"_label"
#define	S_DISABLED		"_disable"
#define S_MARKED		"_marked"
#define S_USER_TRIGGER	"_user_trig"
#define S_MESSAGE		"_msg"
#define	S_SUBMENU		"_submenu"
#define S_SHORTCUT_CHAR	"_shortcut"
#define S_MOD_KEYS		"_mods"

BMenuItem::BMenuItem(BMessage *data)
	: BArchivable(data)
{
	const char	*str;
	long		i1;
	long		i2;
	bool		b;

	InitData();

	if (data->HasString(S_LABEL)) {
		data->FindString(S_LABEL, &str);
		SetLabel(str);
	}

	data->FindBool(S_DISABLED, &b);
	SetEnabled(!b);
	data->FindBool(S_MARKED, &b);
	SetMarked(b);

	if (data->HasInt32(S_USER_TRIGGER)) {
		data->FindInt32(S_USER_TRIGGER, &i1);
		SetTrigger(i1);
	}

	if (data->HasInt32(S_SHORTCUT_CHAR)) {
		data->FindInt32(S_SHORTCUT_CHAR, &i1);
		data->FindInt32(S_MOD_KEYS, &i2);
		SetShortcut(i1, i2);
	}

	if (data->HasMessage(S_MESSAGE)) {
		BMessage	*msg = new BMessage();
		data->FindMessage(S_MESSAGE, msg);
		SetMessage(msg);
	}

	BArchivable	*obj;
	BMessage	archive;

	if (data->FindMessage(S_SUBMENU, &archive) == B_OK) {
		obj = instantiate_object(&archive);
		if (obj) {
			BMenu *menu = cast_as(obj, BMenu);
			if (menu)
				InitMenuData(menu);
		}
	}
}

/* ---------------------------------------------------------------- */

status_t BMenuItem::Archive(BMessage *data, bool deep) const
{
	BArchivable::Archive(data, deep);

	if (fLabel)
		data->AddString(S_LABEL, fLabel);

	if (!IsEnabled())
		data->AddBool(S_DISABLED, true);
	if (IsMarked())
		data->AddBool(S_MARKED, true);

	if (fUserTrigger)
		data->AddInt32(S_USER_TRIGGER, fUserTrigger);

	if (fShortcutChar) {
		data->AddInt32(S_SHORTCUT_CHAR, fShortcutChar);
		data->AddInt32(S_MOD_KEYS, fModifiers);
	}

	BMessage *msg = Message();
	if (msg) 
		data->AddMessage(S_MESSAGE, msg);

	if (deep && fSubmenu) {
		BMessage archive;
		long err = fSubmenu->Archive(&archive);
		if (err == B_OK)
			data->AddMessage(S_SUBMENU, &archive);
	}
	return 0;
}

/* ---------------------------------------------------------------- */

BArchivable *BMenuItem::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BMenuItem"))
		return NULL;
	return new BMenuItem(data);
}

/* ---------------------------------------------------------------- */

void BMenuItem::InitData()
{
	fLabel = NULL;
	fTriggerIndex = -1;
	fSubmenu = NULL;
	fUserTrigger = 0;
	fSysTrigger = 0;
	fShortcutChar = 0;
	fModifiers = 0;
	fMark = false;
	fEnabled = true;
	fSelected = false;
	fWindow = NULL;
	fSuper = NULL;
	fCachedWidth = fCachedHeight = 0;
}

/* ---------------------------------------------------------------- */

BMenuItem::~BMenuItem()
{
	free(fLabel);
	delete fSubmenu;
}

/* ---------------------------------------------------------------- */

const char *BMenuItem::Label() const
{
	return fLabel;
}

/* ---------------------------------------------------------------- */

void BMenuItem::SetLabel(const char *name)
{
	free(fLabel);
	
	if (name)
		fLabel = strdup(name);
	else
		fLabel = NULL;

	if (fSuper) {
		fSuper->InvalidateLayout();
		if (fSuper->LockLooper()) {
			fSuper->Invalidate();
			fSuper->UnlockLooper();
		}
	}
}

/* ---------------------------------------------------------------- */

void BMenuItem::SetShortcut(char shortcut, uint32 modifiers)
{
	if (fShortcutChar && (fModifiers&B_COMMAND_KEY) && fWindow)
		fWindow->RemoveShortcut(fShortcutChar, fModifiers);

	fShortcutChar = shortcut;
	if (shortcut) {
		fModifiers = modifiers
				   | ( (modifiers&B_NO_COMMAND_KEY) ? 0 : B_COMMAND_KEY);
	} else
		fModifiers = 0;

	if (fShortcutChar && (fModifiers&B_COMMAND_KEY) && fWindow)
		fWindow->AddShortcut(fShortcutChar, fModifiers, this);

	if (fSuper) {
		fSuper->InvalidateLayout();
		if (fSuper->LockLooper()) {
			fSuper->Invalidate();
			fSuper->UnlockLooper();
		}
	}
}

/* ---------------------------------------------------------------- */
BBitmap *mark_bm = NULL;

BBitmap *long_button_bm = NULL;
BBitmap *short_button_bm = NULL;
BBitmap	*alt_str_bm = NULL;
BBitmap	*option_str_bm = NULL;
BBitmap	*control_str_bm = NULL;
BBitmap	*shift_str_bm = NULL;
BBitmap	*cmd_sym_str_bm = NULL;

BBitmap *alt_bm = NULL;				// regular versions of the key bitmaps
BBitmap *shift_bm = NULL;
BBitmap *control_bm = NULL;
BBitmap *option_bm = NULL;
BBitmap *alt_d_bm = NULL;			// dimmed versions of the key bitmaps
BBitmap *shift_d_bm = NULL;
BBitmap *control_d_bm = NULL;
BBitmap *option_d_bm = NULL;
BBitmap *cmd_sym_d_bm = NULL;

#include "MenuBitmaps.cpp"

void _delete_menu_bitmaps_()
{
	delete alt_bm;
	alt_bm = NULL;
	delete mark_bm;
	mark_bm = NULL;
	delete shift_bm;
	shift_bm = NULL;
	delete control_bm;
	control_bm = NULL;
	delete option_bm;
	option_bm = NULL;
	delete alt_d_bm;
	alt_d_bm = NULL;
	delete shift_d_bm;
	shift_d_bm = NULL;
	delete control_d_bm;
	control_d_bm = NULL;
	delete option_d_bm;
	option_d_bm = NULL;
}

/* ---------------------------------------------------------------- */

void BMenuItem::Draw()
{
	ASSERT(fSuper && fSuper->Window());
	
	fSuper->CacheFontInfo();

	const rgb_color low = fSuper->LowColor();
	const rgb_color high = fSuper->HighColor();
	
	const rgb_color background = fSuper->ViewColor();
	const bool enabled = IsEnabled();
	
	const bool drawSnake = fSuper->DrawNewLook();
	const bool drawSelection = fSelected && (enabled || Submenu());
	// set the right low color
	if (drawSelection) {
		if (drawSnake)
			fSuper->SetLowColor(ui_color(B_MENU_SELECTION_BACKGROUND_COLOR));
		else
			fSuper->SetLowColor(tint_color(background, HILITE_BKG_C));
	} else
		fSuper->SetLowColor(background);
	
	// determine the item high color for disabled and/or selected items
	if (!enabled) {
		if (fSelected) {
			if (drawSnake)
				fSuper->SetHighColor(tint_color(ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR), cDARKEN_3));
			else
				fSuper->SetHighColor(tint_color(background, cDARKEN_4));
		} else
			fSuper->SetHighColor(tint_color(background, DISABLED_C));
	} else if (fSelected && drawSnake)
		fSuper->SetHighColor(ui_color(B_MENU_SELECTED_ITEM_TEXT_COLOR));
	//else
	//	fSuper->SetHighColor(ui_color(B_MENU_ITEM_TEXT_COLOR));
	
	fSuper->SetDrawingMode(B_OP_COPY);
	fSuper->MovePenTo(ContentLocation());
	
	// draw selection background if needed
	if (drawSelection) {
		if (drawSnake) {
			BRect f(Frame());
			f.right -= 1;
			f.bottom -= 1;
			fSuper->FillRoundRect(f, kItemSelectionRadius, kItemSelectionRadius, B_SOLID_LOW);
		} else
			fSuper->FillRect(Frame(), B_SOLID_LOW);
	}
	
	// draw the content
	DrawContent();

	// draw the rest
	if (fSuper->Layout() != B_ITEMS_IN_MATRIX) {
		if (IsMarked())
			DrawMarkSymbol();
		if (fShortcutChar)
			DrawShortcutSymbol();
		if (Submenu())
			DrawSubmenuSymbol();
	}
	
	fSuper->SetLowColor(low);
	fSuper->SetHighColor(high);
}

/* ---------------------------------------------------------------- */

void BMenuItem::DrawContent()
{
	fSuper->MovePenBy(0, fSuper->fAscent);
	/*
	 There's a tricky case here. Can't do the simple 'DrawString' case
	 simply if the menu isn't currently in sticky mode. The reason is
	 that if the menu was just in sticky mode and now it isn't in
	 sticky mode then you need to 'erase' the underline beneath the
	 trigger char. So you have to do the complex draw case.
	 */
	const bool sticky = fSuper->IsStickyMode()
		|| (fSuper->sMenuInfo.triggers_always_shown
			&& fSuper->sMenuInfo.click_to_open);
	const bool enabled = IsEnabled();
	
	BPoint penStart(fSuper->PenLocation());
	const char *the_label = NULL;
	char *trunc_label = NULL;
	float max = fSuper->MaxContentWidth();
	if (max > 0) {		
	 	float lw = fCachedWidth;
		float offset = penStart.x - fBounds.left;
	 	// If the string width is greater than our frame width then we're
	 	// being ask to truncate!
	 	if (lw + offset > max) {
			trunc_label = (char *) malloc(strlen(fLabel) + 4);
			if (!trunc_label)
				return;
			TruncateLabel(max - offset, trunc_label);
			the_label = trunc_label;
		}
	}
	if (the_label == NULL) 
		the_label = Label();
	

#if 1
	// we are setting low color here to stay compatible with old confused versions
	// of overriden DrawContent - these would go and set the low color even though
	// it had no effect.
	//
	// This is really wastefull and messy, we should gradually have everyone use the
	// correct setup where the low color is determined/set in BMenuItem::Draw and
	// DrawContent doesn't touch it.
	
	fSuper->SetDrawingMode(B_OP_COPY);

	bool drawSnake = fSuper->DrawNewLook();
	bool drawSelection = fSelected && (enabled || Submenu());
	// set the right low color
	if (drawSelection) {
		if (drawSnake)
			fSuper->SetLowColor(ui_color(B_MENU_SELECTION_BACKGROUND_COLOR));
		else
			fSuper->SetLowColor(tint_color(fSuper->ViewColor(), HILITE_BKG_C));
	} else
		fSuper->SetLowColor(fSuper->ViewColor());
#endif


	fSuper->DrawString(the_label);
	// don't draw trigger if the label is being truncated.
	// Don't know how to properly deal with this case.
	if (fSuper->AreTriggersEnabled()
		&& (sticky || fSuper->fRedrawAfterSticky)
		&& fTriggerIndex != -1 && !trunc_label) {


		// ??? This should really use the_label instead of Label(), but
		// we've got a problem with the trigger character.
		const char *str = Label();
		int32 strLength = strlen(str);
		float *escapements = (float *)malloc(strLength * sizeof(float));

		BFont theFont;
		fSuper->GetFont(&theFont);
		theFont.GetEscapements(str, strLength, escapements);

		// Draw the line underneath the Trigger char
		rgb_color oldHighColor = fSuper->HighColor();
		if (!sticky)
			// if not showing sticky characters, set the color to low so
			// we can undraw the sticky underlines
			fSuper->SetHighColor(fSuper->LowColor());

		BPoint start(penStart);
		for (int32 i = 0; i < fTriggerIndex; i++)
			start.x += escapements[i] * theFont.Size();

		start.y += 1.0;
		BPoint end(start);
		start.x -= 1.0;
		end.x += escapements[fTriggerIndex] * theFont.Size() - 1;

		fSuper->StrokeLine(start, end);

		fSuper->SetHighColor(oldHighColor);
		free(escapements);
	}

	free(trunc_label);
}

/* ---------------------------------------------------------------- */

void BMenuItem::TruncateLabel(float max, char *trunc_label)
{
	// ??? We've got a problem with the 'Trigger' character. We can't
	// remove that character from the label so we need a really smart
	// truncation routine.

	BFont font;
	fSuper->GetFont(&font);
	font.GetTruncatedStrings((const char **) &fLabel, 1, B_TRUNCATE_END,
		max, &trunc_label);
}

/* ---------------------------------------------------------------- */

void BMenuItem::Select(bool on)
{
	ASSERT(fSuper && fSuper->Window());

	if (Submenu()) {
		// We are allowing disabled menu to get selected (i.e. shown).
		// That's why we're not checking to ensure that the item is enabled.
		fSelected = on;
		Highlight(on);
	} else if (IsEnabled()) {
		fSelected = on;
		Highlight(on);
	}
}

/* ---------------------------------------------------------------- */

void BMenuItem::Highlight(bool)
{
	ASSERT(fSuper && fSuper->Window());
	
	BRect drawFrame(Frame());
	// make sure we get the selection shadow too
	drawFrame.right += 1;
	drawFrame.bottom += 1;
	fSuper->DrawItemDirect(drawFrame);
}

/* ---------------------------------------------------------------- */
static void _draw_alt_(BView *v, bool enabled, BPoint pt, BBitmap *bm)
{
	if (enabled) {
		v->SetDrawingMode(B_OP_BLEND);
		v->DrawBitmapAsync(short_button_bm, pt);
		v->SetDrawingMode(B_OP_OVER);
		v->DrawBitmapAsync(alt_str_bm, pt+BPoint(3,1));
	} else {
		v->SetDrawingMode(B_OP_BLEND);
		v->DrawBitmapAsync(bm, pt);
	}
}

/* ---------------------------------------------------------------- */
static void _draw_control_(BView *v, bool enabled, BPoint pt, BBitmap *bm)
{
	if (enabled) {
		v->SetDrawingMode(B_OP_BLEND);
		v->DrawBitmapAsync(short_button_bm, pt);
		v->SetDrawingMode(B_OP_OVER);
		v->DrawBitmapAsync(control_str_bm, pt+BPoint(2,1));
	} else {
		v->SetDrawingMode(B_OP_BLEND);
		v->DrawBitmapAsync(bm, pt);
	}
}

/* ---------------------------------------------------------------- */

void BMenuItem::DrawShortcutSymbol()
{
	BPoint	pt;
	BBitmap	*abm;
	BBitmap	*cbm;
	BBitmap	*sbm;
	BBitmap	*obm;
	BPoint	contentLoc(ContentLocation());

	const rgb_color background = fSuper->ViewColor();
	const bool enabled = IsEnabled();
	const bool drawSnake = fSuper->DrawNewLook();

	fSuper->PushState();
	
	if (!alt_bm) {
		system_info info;
		get_system_info(&info);
		bool	mac_plat = (info.platform_type == B_MAC_PLATFORM);

		long_button_bm = new BBitmap(BRect(0,0,_LONG_BUTTON_W_-1,10),
			B_COLOR_8_BIT);
		long_button_bm->SetBits(_menudata_long_button_,
			long_button_bm->BitsLength(), 0, B_COLOR_8_BIT);

		short_button_bm = new BBitmap(BRect(0,0,_SHORT_BUTTON_W_-1,10),
			B_COLOR_8_BIT);
		short_button_bm->SetBits(_menudata_short_button_,
			short_button_bm->BitsLength(), 0, B_COLOR_8_BIT);

		if (mac_plat) {
			alt_str_bm = new BBitmap(BRect(0,0,_CMD_SYM_STR_W_-1,7),
				B_COLOR_8_BIT);
			alt_str_bm->SetBits(_menudata_cmd_sym_str_,
				alt_str_bm->BitsLength(), 0, B_COLOR_8_BIT);
		} else {
			alt_str_bm = new BBitmap(BRect(0,0,_ALT_STR_W_-1,7),
				B_COLOR_8_BIT);
			alt_str_bm->SetBits(_menudata_alt_str_,
				alt_str_bm->BitsLength(), 0, B_COLOR_8_BIT);
		}

		option_str_bm = new BBitmap(BRect(0,0,_OPTION_STR_W_-1,7),
			B_COLOR_8_BIT);
		option_str_bm->SetBits(_menudata_option_str_,
			option_str_bm->BitsLength(), 0, B_COLOR_8_BIT);

		control_str_bm = new BBitmap(BRect(0,0,_CONTROL_STR_W_-1,7),
			B_COLOR_8_BIT);
		control_str_bm->SetBits(_menudata_control_str_,
			control_str_bm->BitsLength(), 0, B_COLOR_8_BIT);

		shift_str_bm = new BBitmap(BRect(0,0,_SHIFT_STR_W_-1,7),
			B_COLOR_8_BIT);
		shift_str_bm->SetBits(_menudata_shift_str_,
			shift_str_bm->BitsLength(), 0, B_COLOR_8_BIT);


		// initialize the regular versions of the bitmaps

		alt_bm = new BBitmap(BRect(0,0,_BM_ALT_W_-1,10), B_COLOR_8_BIT);
		alt_bm->SetBits(_menudata_alt_, alt_bm->BitsLength(), 0,
			B_COLOR_8_BIT);

		shift_bm = new BBitmap(BRect(0,0,_BM_SHIFT_W_-1,10), B_COLOR_8_BIT);
		shift_bm->SetBits(_menudata_shift_, shift_bm->BitsLength(), 0,
			B_COLOR_8_BIT);

		control_bm = new BBitmap(BRect(0,0,_BM_CONTROL_W_-1,10), B_COLOR_8_BIT);
		control_bm->SetBits(_menudata_control_, control_bm->BitsLength(), 0,
			B_COLOR_8_BIT);

		option_bm = new BBitmap(BRect(0,0,_BM_OPTION_W_-1,10), B_COLOR_8_BIT);
		option_bm->SetBits(_menudata_option_, option_bm->BitsLength(), 0,
			B_COLOR_8_BIT);
		
		// initialize the dimmed versions of the bitmaps
		if (mac_plat) {
			alt_d_bm = new BBitmap(BRect(0,0,_BM_CMD_SYM_W_-1,10),
				B_COLOR_8_BIT);
			alt_d_bm->SetBits(_menudata_cmd_sym_d_,
				alt_d_bm->BitsLength(), 0, B_COLOR_8_BIT);
		} else {
			alt_d_bm = new BBitmap(BRect(0,0,_BM_ALT_W_-1,10), B_COLOR_8_BIT);
			alt_d_bm->SetBits(_menudata_alt_d_, alt_d_bm->BitsLength(), 0,
				B_COLOR_8_BIT);
		}

		shift_d_bm = new BBitmap(BRect(0,0,_BM_SHIFT_W_-1,10), B_COLOR_8_BIT);
		shift_d_bm->SetBits(_menudata_shift_d_, shift_d_bm->BitsLength(), 0,
			B_COLOR_8_BIT);

		control_d_bm = new BBitmap(BRect(0,0,_BM_CONTROL_W_-1,10),
			B_COLOR_8_BIT);
		control_d_bm->SetBits(_menudata_control_d_,
			control_d_bm->BitsLength(), 0, B_COLOR_8_BIT);

		option_d_bm = new BBitmap(BRect(0,0,_BM_OPTION_W_-1,10),
			B_COLOR_8_BIT);
		option_d_bm->SetBits(_menudata_option_d_,
			option_d_bm->BitsLength(), 0, B_COLOR_8_BIT);
	}

	// pick either the regular version of bitmap of the disabled version.

	if (enabled) {
		abm = !BMenu::sSwapped ? alt_bm : control_bm;
		cbm = !BMenu::sSwapped ? control_bm : alt_bm;
		sbm = shift_bm;
		obm = option_bm;
	} else {
		abm = !BMenu::sSwapped ? alt_d_bm : control_d_bm;
		cbm = !BMenu::sSwapped ? control_d_bm : alt_d_bm;
		sbm = shift_d_bm;
		obm = option_d_bm;
	}

	// leave space for <char> and padding
	pt.x = fBounds.right - (1.6 * fSuper->fAscent);

	if (fModifiers & B_COMMAND_KEY) pt.x -= (_BM_SHORT_W_ + _BM_SPACING_);
	if (fModifiers & B_SHIFT_KEY) pt.x -= (_BM_SHIFT_W_ + _BM_SPACING_);
	if (fModifiers & B_CONTROL_KEY) pt.x -= (_BM_CONTROL_W_ + _BM_SPACING_);
	if (fModifiers & B_OPTION_KEY) pt.x -= (_BM_OPTION_W_ + _BM_SPACING_);

	if (Submenu())
		pt.x -= 20;

	pt.y = contentLoc.y + fSuper->fAscent - 9;

	fSuper->SetDrawingMode(B_OP_BLEND);
	if (fModifiers & B_SHIFT_KEY) {
		if (!fSuper->fRedrawAfterSticky) 
			fSuper->DrawBitmapAsync(sbm, pt);
		pt.x += (_BM_SHIFT_W_ + _BM_SPACING_);
	}

	if (fModifiers & B_CONTROL_KEY) {
		if (!fSuper->fRedrawAfterSticky) {
			if (!BMenu::sSwapped)
				_draw_control_(fSuper, enabled, pt, cbm);
			else
				_draw_alt_(fSuper, enabled, pt, abm);
		}
		pt.x += (_BM_SHORT_W_ + _BM_SPACING_);
	}

	if (fModifiers & B_OPTION_KEY) {
		if (!fSuper->fRedrawAfterSticky) {
			if (enabled) {
				fSuper->SetDrawingMode(B_OP_BLEND);
				fSuper->DrawBitmapAsync(short_button_bm, pt);
				fSuper->SetDrawingMode(B_OP_OVER);
				fSuper->DrawBitmapAsync(option_str_bm, pt+BPoint(2,1));
			} else {
				fSuper->SetDrawingMode(B_OP_BLEND);
				fSuper->DrawBitmapAsync(obm, pt);
			}
		}
		pt.x += (_BM_OPTION_W_ + _BM_SPACING_);
	}

	// Draw the <alt> key symbol
	if (fModifiers & B_COMMAND_KEY) {
		if (!fSuper->fRedrawAfterSticky) {
			if (!BMenu::sSwapped)
				_draw_alt_(fSuper, enabled, pt, abm);
			else
				_draw_control_(fSuper, enabled, pt, cbm);
		}
		pt.x += (_BM_SHORT_W_ + _BM_SPACING_);
	}

	fSuper->SetDrawingMode(B_OP_COPY);

	pt.x += 4;
	pt.y = contentLoc.y + fSuper->fAscent;

	fSuper->MovePenTo(pt);

	
	// Draw the Trigger char
	rgb_color c = fSuper->HighColor();
	if (fSelected)
		fSuper->SetHighColor(tint_color(background, HILITE_BKG_C));
	else
		fSuper->SetHighColor(BMenu::sMenuInfo.background_color);
	
	BPoint start(fSuper->PenLocation());
	BPoint ss(start);
	ss.y += 1;

	BPoint end(ss);
	end.x += (fSuper->StringWidth(&fShortcutChar, 1) - 1.0);
	ss.x -= 1;
	fSuper->StrokeLine(ss, end);

	fSuper->MovePenTo(start);

	fSuper->SetHighColor(c);

	if (IsSelected() && (enabled || Submenu())) {
		if (drawSnake)
			fSuper->SetLowColor(ui_color(B_MENU_SELECTION_BACKGROUND_COLOR));
		else
			fSuper->SetLowColor(tint_color(background, HILITE_BKG_C));
	} else
		fSuper->SetLowColor(background);
	
	switch (fShortcutChar) {
		case B_LEFT_ARROW:
			DrawControlChar(kUTF8LeftArrow);
			break;

		case B_UP_ARROW:
			DrawControlChar(kUTF8UpArrow);
			break;

		case B_RIGHT_ARROW:
			DrawControlChar(kUTF8RightArrow);
			break;

		case B_DOWN_ARROW:
			DrawControlChar(kUTF8DownArrow);
			break;

		case B_ENTER:
			DrawControlChar(kUTF8Enter);
			break;

		default:
			fSuper->DrawChar(fShortcutChar);
			break;
	}

	fSuper->PopState();
}

/* ---------------------------------------------------------------- */

void BMenuItem::DrawSubmenuSymbol()
{
	if (fSuper->fRedrawAfterSticky)
		return;

	if (!dynamic_cast<BMenuBar *>(fSuper)
		|| (fSuper->fLayout == B_ITEMS_IN_COLUMN &&	fSuper->CountItems() > 1) ) {
		BPoint p1;
		BPoint p2;
		BPoint p3;
		BPoint contentLoc(ContentLocation());

		p1.x = fBounds.right - 14;
		p1.y = contentLoc.y + fSuper->fAscent + 1;
		p2 = p1;
		p2.x += 9;
		p2.y -= 4;
		p3 = p1;
		p3.y -= 8;
		
		const rgb_color background = fSuper->ViewColor();
		
		if (IsSelected())
			fSuper->SetHighColor(background);
		else
			fSuper->SetHighColor(ui_color(B_SHINE_COLOR));
		fSuper->FillTriangle(p1, p2, p3);
		
		BPoint p4(p1);
		BPoint p5;

		fSuper->SetHighColor(tint_color(background, cDARKEN_2));
		p4.y -= 1;
		p5.x = p4.x + 5;
		p5.y = p4.y - 2;
		fSuper->StrokeLine(p4, p5);

		p4.y = p5.y - 1;
		p4.x = p5.x + 2;
		p5 = p4;
		fSuper->StrokeLine(p4, p5);

		fSuper->SetHighColor(tint_color(background, cDARKEN_3));
		fSuper->StrokeTriangle(p1, p2, p3);

		p1.x += 2;
		p1.y += -3;
		p2.x += -4;
		p3.x += 2;
		p3.y += 3;
		if (IsSelected())
			fSuper->SetHighColor(tint_color(background, cDARKEN_1));
		else
			fSuper->SetHighColor(tint_color(background, cLIGHTEN_1));
		fSuper->StrokeTriangle(p1, p2, p3);
		fSuper->FillTriangle(p1, p2, p3);
	}
}

/* ---------------------------------------------------------------- */

void BMenuItem::DrawMarkSymbol()
{
	const rgb_color background = fSuper->ViewColor();
	
	BPoint startp(6, fBounds.bottom - ((fBounds.Height() - 10) / 2.0));
	rgb_color mark_color = tint_color(background, cDARKEN_4);

	BPoint	p;

	rgb_color c = tint_color(background, cDARKEN_1);
	fSuper->BeginLineArray(6);

	// draw the shadow
	p = startp;
	fSuper->AddLine(p, p + BPoint(4, -9), c);
	p.x += 1;
	fSuper->AddLine(p, p + BPoint(4, -9), c);

	// draw the right side of the check
	p = startp;
	p.y -= 1;
	fSuper->AddLine(p, p + BPoint(4, -9), mark_color);
	p.x -= 1;
	fSuper->AddLine(p, p + BPoint(4, -9), mark_color);

	// draw the left side of the check
	p.y += 1;
	fSuper->AddLine(p, p + BPoint(-2, -6), mark_color);
	p.y -= 1;

	p.x -= 1;
	fSuper->AddLine(p, p + BPoint(-2, -5), mark_color);

	fSuper->EndLineArray();
}

/* ---------------------------------------------------------------- */

void
BMenuItem::DrawControlChar(const char *control)
{
	BFont saveFont;
	fSuper->GetFont(&saveFont);

	BFont symbolFont(_be_symbol_font_);
	symbolFont.SetSize(saveFont.Size());
	fSuper->SetFont(&symbolFont);
	fSuper->DrawString(control);

	fSuper->SetFont(&saveFont);
}

/* ---------------------------------------------------------------- */

void BMenuItem::GetContentSize(float *pwidth, float *pheight)
{
	BPoint		pt;
	
	ASSERT((fSuper));
	//removed assertion because Wagner trips over it.
	//ASSERT((fSuper->Window()));		// needs to be in a window
	
	fSuper->CacheFontInfo();
	pt.x = fSuper->StringWidth(Label());
	pt.y = fSuper->fFontHeight;
	
	if (Submenu() && (fSuper->fLayout == B_ITEMS_IN_COLUMN)) {
		pt.x += 18;
	}
	
	*pwidth = fCachedWidth = pt.x;
	*pheight = fCachedHeight = pt.y;
	return;
}

/* ---------------------------------------------------------------- */

status_t BMenuItem::Invoke(BMessage *msg)
{
	if (!fSuper)
		return B_ERROR;

	if (!IsEnabled())
		return B_ERROR;

	if (fSuper->IsRadioMode())
		SetMarked(true);

	bool notify = false;
	uint32 kind = InvokeKind(&notify);
	
	BMessage clone(kind);
	status_t err = B_BAD_VALUE;
	
	if (!msg && !notify)
		msg = Message();
	if (!msg) {
		// If not being watched, there is nothing to do.
		if( !fSuper->IsWatched() ) return err;
	} else {
		clone = *msg;
	}

	clone.SetWhen(system_time());
	clone.AddInt32("index", fSuper->IndexOf(this));
	clone.AddPointer("source", this);
	clone.AddMessenger(B_NOTIFICATION_SENDER, BMessenger(fSuper));
	if( msg ) err = BInvoker::Invoke(&clone);
	
	// Also send invocation to any observers of this handler.
	fSuper->SendNotices(kind, &clone);
	
	return err;
}

/* ---------------------------------------------------------------- */

bool BMenuItem::IsSelected() const
{
	return fSelected;
}

/* ---------------------------------------------------------------- */

BMenu *BMenuItem::Submenu() const
{
	return fSubmenu;
}

/* ---------------------------------------------------------------- */

char BMenuItem::Shortcut(uint32 *modifiers) const
{
	if (modifiers)
		*modifiers = fModifiers;
	return fShortcutChar;
}

/* ---------------------------------------------------------------- */

BRect BMenuItem::Frame() const
{
	return fBounds;
}

/* ---------------------------------------------------------------- */

BPoint BMenuItem::ContentLocation() const
{
	BPoint pos = fBounds.LeftTop();
	
	if (fBounds.Width() >= (fCachedWidth+fSuper->fPad.left+fSuper->fPad.right)) {
		pos.x += fSuper->fPad.left;
	} else if (fBounds.Width() > fCachedWidth) {
		pos.x += floor( (fBounds.Width()-fCachedWidth)/2 + .5 );
	}
	if (fBounds.Height() >= (fCachedHeight+fSuper->fPad.top+fSuper->fPad.bottom)) {
		pos.y += fSuper->fPad.top;
	} else if (fBounds.Height() > fCachedHeight) {
		pos.y += floor( (fBounds.Height()-fCachedHeight)/2 + .5 );
	}
	
	return pos;
}

/* ---------------------------------------------------------------- */

void BMenuItem::SetSuper(BMenu *super)
{
	if (fSuper && super) {
		// ??? need to throw an exception here to properly abort
		debugger("Error - can't add menu or menu item to more "
			"than 1 container (either menu or menubar).");
		return;
	}

	fSuper = super;
	BMenu *sub = Submenu();

	if (sub)
		sub->fSuper = super;
}

/* ---------------------------------------------------------------- */

void BMenuItem::Install(BWindow *window)
{
	BMenu *menu = Submenu();
	if (menu)
		menu->Install(window);

	fWindow = window;

	if (fShortcutChar && (fModifiers&B_COMMAND_KEY) && fWindow)
		fWindow->AddShortcut(fShortcutChar, fModifiers, this);

	// if a target wasn't set then default to the window.
	if (!Messenger().IsValid())
		SetTarget(window);
}

/* ---------------------------------------------------------------- */

void BMenuItem::Uninstall()
{
	BMenu	*menu = Submenu();
	
	if (menu)
		menu->Uninstall();

	BHandler *target = Target();
	if (target == fWindow) 
		// manually set target/looper to NULL.
		SetTarget(BMessenger());

	if (fShortcutChar && (fModifiers&B_COMMAND_KEY) && fWindow)
		fWindow->RemoveShortcut(fShortcutChar, fModifiers);

	fWindow = NULL;
}

/* ---------------------------------------------------------------- */

void BMenuItem::SetEnabled(bool state)
{
	fEnabled = state;
	
	if (Submenu() && Submenu()->fEnabled != state) {
		Submenu()->SetEnabled(state);
	}

	BMenu *menu = Menu();
	if (menu) {
		if (menu->LockLooper()) {
			menu->Invalidate(Frame());
			menu->UnlockLooper();
		}
	}
}

/* ---------------------------------------------------------------- */

bool BMenuItem::IsEnabled() const
{
	if (Submenu())
		return Submenu()->IsEnabled();

	// Need to check all the way up the parent chain for enabling. Of course,
	// if there isn't a parent yet then simply return the local state.

	BMenu *parent = Menu();
	return fEnabled && (!parent || parent->IsEnabled());
}

/* ---------------------------------------------------------------- */

BMenu *BMenuItem::Menu() const
{
	return fSuper;
}

/* ---------------------------------------------------------------- */

void BMenuItem::SetMarked(bool state)
{
	if (state && fSuper) 
		fSuper->ItemMarked(this);

	fMark = state;
}

/* ---------------------------------------------------------------- */

bool BMenuItem::IsMarked() const
{
	return fMark;
}

/* ---------------------------------------------------------------- */

void BMenuItem::SetTrigger(char ch)
{
	/*
	This function should only be used by 'users'. Shouldn't use it
	internally Trigger the Trigger char if one isn't specified by
	user. Unless we're doing the initial copy of state from a Menu
	to its owning MenuItem (See the BMenuItem(BMenu *) constructor).
	*/

	fUserTrigger = ch;

	/*
	Calculate the "index" of the Trigger char. Need to use the
	some logic as in BMenu::CalcTrigger for choosing the letter.
	*/
	const char *title = Label();
	const char *p = strchr(title, toupper(ch));
	if (!p)
		p = strchr(title, ch);

	if (p)
		fTriggerIndex = p - title;
	else 
		fTriggerIndex = -1;

	if (fSuper)
		fSuper->InvalidateLayout();
}

/* ---------------------------------------------------------------- */

void BMenuItem::SetSysTrigger(char ch)
{
	fSysTrigger = ch;
}

/* ---------------------------------------------------------------- */

char BMenuItem::Trigger() const
{
	// only returns the user defined Trigger char - not the one
	// calculated by the system.

	return fUserTrigger;
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

BSeparatorItem::BSeparatorItem()
	: BMenuItem("", (ulong) 0)
{
	BMenuItem::SetEnabled(false);
}

/* ---------------------------------------------------------------- */

BSeparatorItem::BSeparatorItem(BMessage *data)
	: BMenuItem(data)
{
	BMenuItem::SetEnabled(false);
}

/* ---------------------------------------------------------------- */

BArchivable *BSeparatorItem::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BSeparatorItem"))
		return NULL;
	return new BSeparatorItem(data);
}


/* ---------------------------------------------------------------- */

BSeparatorItem::~BSeparatorItem()
{
}

/* ---------------------------------------------------------------- */

void BSeparatorItem::GetContentSize(float *pw, float *ph)
{
	*pw = 0;
	*ph = 8;
}

/* ---------------------------------------------------------------- */

void BSeparatorItem::Draw()
{
	BRect frame(Frame());

	menu_info minfo;
	get_menu_info(&minfo);

	if (minfo.separator > 0) {
		frame.left += 10;
		frame.right -= 10;
	} else {
		frame.left += 1;
		frame.right -= 1;
	}

	BMenu *parent = Menu();
	ASSERT(parent);

	const rgb_color background = parent->ViewColor();
	
	int32 y = (int32) (frame.top + (frame.bottom - frame.top) / 2);

	parent->BeginLineArray(minfo.separator == 2 ? 3 : 2);
	parent->AddLine(BPoint(frame.left, y), BPoint(frame.right, y),
		tint_color(background, cDARKEN_1));

	if (minfo.separator == 2) {
		y++;
		frame.left++;
		frame.right--;
		parent->AddLine(BPoint(frame.left,y), BPoint(frame.right, y),
			tint_color(background, cDARKEN_1));
	}
	y++;
	if (minfo.separator == 2) {
		frame.left++;
		frame.right--;
	}
	parent->AddLine(BPoint(frame.left, y), BPoint(frame.right, y),
		tint_color(background, cLIGHTEN_2));

	parent->EndLineArray();
}

/* ---------------------------------------------------------------- */

void BSeparatorItem::SetEnabled(bool)
{
	// intercept calls to SetEnable to ensure that it stays DISABLED.
	return;
}


/* ---------------------------------------------------------------- */

status_t BSeparatorItem::Archive(BMessage *data, bool deep) const
{
	return BMenuItem::Archive(data, deep);
}

/* ---------------------------------------------------------------- */

void BSeparatorItem::_ReservedSeparatorItem1() {}
void BSeparatorItem::_ReservedSeparatorItem2() {}

/* ---------------------------------------------------------------- */

BSeparatorItem &BSeparatorItem::operator=(const BSeparatorItem &) { return *this; }

/* ---------------------------------------------------------------- */

void BMenuItem::_ReservedMenuItem1() {}
void BMenuItem::_ReservedMenuItem2() {}
void BMenuItem::_ReservedMenuItem3() {}
void BMenuItem::_ReservedMenuItem4() {}

/* ---------------------------------------------------------------- */

BMenuItem::BMenuItem(const BMenuItem &) 
	:	BArchivable(),
		BInvoker()
	{}
BMenuItem &BMenuItem::operator=(const BMenuItem &) { return *this; }

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
