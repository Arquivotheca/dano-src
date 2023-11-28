//******************************************************************************
//
//	File:		ListItem.cpp
//
//	Written by:	Peter Potrebic
//
//	Copyright 1996, Be Incorporated
//
//******************************************************************************

#include <Debug.h>
#include <Font.h>
#include <ListItem.h>
#include <Message.h>
#include <Window.h>

#include <archive_defs.h>
#include <MenuPrivate.h>
#include <interface_misc.h>

#include <malloc.h>
#include <string.h>

/* ---------------------------------------------------------------- */

BListItem::BListItem(uint32 outlineLevel, bool initialyExpanded)
{
	fWidth = 0;
	fHeight = 0;
	fSelected = false;
	fEnabled = true;
	fLevel = outlineLevel;
	fExpanded = initialyExpanded;
	fHasSubitems = 0;
	fVisible = 0;
}

/* ---------------------------------------------------------------- */

BListItem::BListItem(BMessage *data)
	: BArchivable(data)
{
	bool	disabled;

	fWidth = 0;
	fHeight = 0;
	fSelected = false;
	fLevel = 0;
	fExpanded = false;
	fHasSubitems = 0;
	fVisible = 0;
	data->FindBool(S_SELECTED, &fSelected);
	data->FindBool(S_DISABLED, &disabled);
	data->FindBool(S_EXPANDED, &fExpanded);
	data->FindInt32(S_OUTLINE_LEVEL, (int32 *)&fLevel);
	fEnabled = !disabled;
}

/* ---------------------------------------------------------------- */

BListItem::~BListItem()
{
}

/* ---------------------------------------------------------------- */

status_t BListItem::Archive(BMessage *data, bool deep) const
{
	BArchivable::Archive(data, deep);
	if (fSelected)
		data->AddBool(S_SELECTED, true);
	if (!fEnabled)
		data->AddBool(S_DISABLED, true);
	if (fExpanded)
		data->AddBool(S_EXPANDED, true);
	if (fLevel > 0)
		data->AddInt32(S_OUTLINE_LEVEL, fLevel);
	return 0;
}

/* ---------------------------------------------------------------- */

void BListItem::SetWidth(float width)
{
	fWidth = width;
}

/* ---------------------------------------------------------------- */

void BListItem::SetHeight(float height)
{
	fHeight = height;
}

/* ---------------------------------------------------------------- */

float BListItem::Width() const
{
	return fWidth;
}

/* ---------------------------------------------------------------- */

float BListItem::Height() const
{
	return fHeight;
}

/* ---------------------------------------------------------------- */

void BListItem::Select()
{
	fSelected = TRUE;
}

/* ---------------------------------------------------------------- */

void BListItem::Deselect()
{
	fSelected = FALSE;
}

/* ---------------------------------------------------------------- */

bool BListItem::IsSelected() const
{
	return fSelected;
}

/* ---------------------------------------------------------------- */

bool BListItem::IsEnabled() const
{
	return fEnabled;
}

/* ---------------------------------------------------------------- */

void BListItem::SetEnabled(bool state)
{
	fEnabled = state;
}

/* ---------------------------------------------------------------- */

void BListItem::Update(BView *owner, const BFont *font)
{
	SetWidth(owner->Bounds().Width());

	font_height finfo;
	font->GetHeight(&finfo);
	SetHeight(finfo.ascent + finfo.descent + finfo.leading);
}

/*----------------------------------------------------------------*/

status_t BListItem::Perform(perform_code d, void *arg)
{
	return BArchivable::Perform(d, arg);
}

/*----------------------------------------------------------------*/

void BListItem::SetExpanded(bool on)
{
	fExpanded = on;
}

/*----------------------------------------------------------------*/

bool BListItem::IsExpanded() const
{
	return fExpanded;
}

/*----------------------------------------------------------------*/

uint32 BListItem::OutlineLevel() const
{
	return fLevel;
}

/* ---------------------------------------------------------------- */

bool BListItem::HasSubitems() const
{
	return fHasSubitems;
}

/* ---------------------------------------------------------------- */

bool BListItem::IsItemVisible() const
{
	// used by OutlineListVew, true if all superitems are
	// expanded
	return fVisible;
}

/* ---------------------------------------------------------------- */

void BListItem::SetItemVisible(bool on)
{
	fVisible = on;
}

/* ---------------------------------------------------------------- */

void BListItem::_ReservedListItem1() {}
void BListItem::_ReservedListItem2() {}

/*-------------------------------------------------------------*/

BListItem::BListItem(const BListItem &)
	:	BArchivable()
	{}
BListItem &BListItem::operator=(const BListItem &) { return *this; }

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

BStringItem::BStringItem(const char *text, uint32 outlineLevel, 
	bool initialyExpanded)
	: BListItem(outlineLevel, initialyExpanded)
{
	fText = NULL;
	SetText(text);
	fBaselineOffset = 0;
}

/* ---------------------------------------------------------------- */

BStringItem::BStringItem(BMessage *data)
	: BListItem(data)
{
	const char *str;
	fText = NULL;
	if (data->FindString(S_LABEL, &str) == B_OK) {
		SetText(str);
	}
	fBaselineOffset = 0;
}

/* ---------------------------------------------------------------- */

status_t BStringItem::Archive(BMessage *data, bool deep) const
{
	BListItem::Archive(data, deep);
	if (fText)
		data->AddString(S_LABEL, fText);
	return 0;
}

/* ---------------------------------------------------------------- */

BArchivable *BStringItem::Instantiate(BMessage *data)
{
	if (!validate_instantiation(data, "BStringItem"))
		return NULL;
	return new BStringItem(data);
}

/* ---------------------------------------------------------------- */

BStringItem::~BStringItem()
{
	SetText(NULL);
}

/* ---------------------------------------------------------------- */

void BStringItem::DrawItem(BView *owner, BRect bounds, bool complete)
{
	if (!fText)
		return;

	BPoint where = bounds.LeftBottom();
	where.x += 4;
	where.y -= fBaselineOffset;

	rgb_color	h = owner->HighColor();
	rgb_color	low = owner->LowColor();
	rgb_color	bkg = low;

	if (IsSelected() || complete) {
		if (IsSelected()) {
//+			bkg = shift_color(low, HILITE_BKG_C);
			bkg = shift_color(ui_color(B_PANEL_BACKGROUND_COLOR), HILITE_BKG_C);
			owner->SetLowColor(bkg);		// for anti-aliasing text
		}
		owner->SetHighColor(bkg);
		owner->FillRect(bounds);
		owner->SetHighColor(h);
	}

	if (!IsEnabled()) {
//+		PRINT(("bkg=(%d,%d,%d)\n", bkg.red, bkg.green, bkg.blue));
		owner->SetHighColor(h.disable_copy(bkg));
//+		owner->SetHighColor(shift_color(bkg, DISABLED_C));
	}

	owner->MovePenTo(where);
	owner->SetDrawingMode(B_OP_COPY);
	owner->DrawString(fText);

	if (!IsEnabled())
		owner->SetHighColor(h);

	if (IsSelected())
		owner->SetLowColor(low);
}

/* ---------------------------------------------------------------- */

void BStringItem::Update(BView *owner, const BFont *font)
{
	if (fText)
		SetWidth(owner->StringWidth(fText));

	font_height finfo;
	font->GetHeight(&finfo);
	SetHeight(finfo.ascent + finfo.descent + finfo.leading);
	fBaselineOffset = finfo.descent;
}

/* ---------------------------------------------------------------- */

void BStringItem::SetText(const char *text)
{
	if (fText) {
		free(fText);
		fText = NULL;
	}

	if (text)
		fText = strdup(text);
}

/* ---------------------------------------------------------------- */

const char *BStringItem::Text() const
{
	return fText;
}

/* ---------------------------------------------------------------- */

void BStringItem::SetBaselineOffset(float off)
{
	fBaselineOffset = off;
}

/* ---------------------------------------------------------------- */

float BStringItem::BaselineOffset() const
{
	return fBaselineOffset;
}

/* ---------------------------------------------------------------- */

status_t BStringItem::Perform(perform_code d, void *arg)
{
	return BListItem::Perform(d, arg);
}

/* ---------------------------------------------------------------- */

void BStringItem::_ReservedStringItem1() {}
void BStringItem::_ReservedStringItem2() {}

/*-------------------------------------------------------------*/

BStringItem::BStringItem(const BStringItem &)
	:	BListItem()
	{}
BStringItem &BStringItem::operator=(const BStringItem &) { return *this; }

/*----------------------------------------------------------------*/
/*----------------------------------------------------------------*/
/*----------------------------------------------------------------*/
