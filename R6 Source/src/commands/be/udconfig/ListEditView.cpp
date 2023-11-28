//
// Copyright 2000 Be, Incorporated.  All Rights Reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Message.h>
#include <TextControl.h>
#include <List.h>

#include "ListEditView.h"
#include "Settings.h"

#define MSG_ITEM_INVOKED	'iINV'
#define MSG_ITEM_MODIFIED	'iMOD'

#define NAME_COL_WID (20*fBEm)

static const rgb_color kDirtyColor = {255, 0, 0, 255};
static const rgb_color kCleanColor = {0, 0, 0, 255};

ListEditView::ListEditView(BRect frame, const char *name, uint32 resizingMode, uint32 flags, BList *slist) :
	BView(frame, name, resizingMode, flags),
	fSettingsList(slist),
	fPrefWidth(0),
	fPrefHeight(0),
	fItemHeight(0),
	fDirty(0),
	fChangedInvoker(NULL)
{
	fEm = be_plain_font->StringWidth("m");
	fBEm = be_bold_font->StringWidth("m");
	fFEm = be_fixed_font->StringWidth("m");

	BuildList();
	PopulateList();
}


ListEditView::~ListEditView()
{
}

void
ListEditView::AttachedToWindow()
{
	char name[9];
	int nitems;
	int i;

	nitems = fSettingsList->CountItems();
	for(i = 0; i < nitems; i++) {
		Setting *s;
		BTextControl *tc;
		BMessage *msg;

		s = (Setting*)fSettingsList->ItemAt(i);
		if(s == NULL) {
			continue;
		}

		CookieToName(s->cookie, name, sizeof(name));
		tc = dynamic_cast<BTextControl*>(FindView(name));
		if(tc == NULL) {
			continue;
		}

		tc->SetTarget(this);

		msg = new BMessage(MSG_ITEM_MODIFIED);
		msg->AddInt32("Item", s->cookie);
		tc->SetModificationMessage(msg);
	}
	
	BView::AttachedToWindow();
}

void
ListEditView::MessageReceived(BMessage *msg)
{
	int32 cookie;

	switch(msg->what) {
	case MSG_ITEM_INVOKED:
		msg->FindInt32("Item", &cookie);
		ItemChanged(cookie);
		break;
	case MSG_ITEM_MODIFIED:
		msg->FindInt32("Item", &cookie);
		ItemChanged(cookie);
		break;
	default:
		BView::MessageReceived(msg);
	}
}

void
ListEditView::GetPreferredSize(float *width, float *height)
{
	*width = fPrefWidth;
	*height = fPrefHeight;
}

void
ListEditView::SetChangedInvoker(BInvoker *invoker)
{
	delete fChangedInvoker;
	fChangedInvoker = invoker;
}

void
ListEditView::GetItemHeight(float *height)
{
	*height = fItemHeight;
}

void
ListEditView::CookieToName(int32 cookie, char *oname, size_t len)
{
	if(len < 9) {
		memset(oname, 0, len);
	}
	sprintf(oname, "%08lx", cookie);
}

void
ListEditView::NameToCookie(const char *name, int32 *cookie)
{
	sscanf(name, "%lx", cookie);
}


void
ListEditView::AddItem(BRect &r, int32 cookie)
{
	BMessage *msg = new BMessage(MSG_ITEM_INVOKED);
	msg->AddInt32("Item", cookie);

	char name[9];
	CookieToName(cookie, name, sizeof(name));

	BTextControl *tc = new BTextControl(r, name, NULL, NULL, msg, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	tc->SetDivider(NAME_COL_WID + tc->Bounds().left);
	tc->SetFont(be_bold_font);
	tc->ChildAt(0)->SetFont(be_fixed_font);
	AddChild(tc);

	float w, h;
	tc->GetPreferredSize(&w, &h);
	tc->ResizeTo(r.Width(), h);
	r.top += h;

	if(fItemHeight < h) {
		fItemHeight += h;
	}
}


void
ListEditView::BuildList()
{
	BRect r;
	float em = fBEm;
	int nitems;
	int i;

	r = Bounds();
	r.InsetBy(em/2, em/2);

	if(fItemHeight < 1) {
		fItemHeight = em/2;
	}

	nitems = fSettingsList->CountItems();
	for(i = 0; i < nitems; i++) {
		Setting *s;

		s = (Setting*)fSettingsList->ItemAt(i);
		if(s == NULL) {
			continue;
		}

		AddItem(r, s->cookie);

		r.top += em/2;
	}

	fPrefWidth = Bounds().Width();
	fPrefHeight = r.top - Bounds().top;
}


void
ListEditView::PopulateList()
{
	char name[9];
	int nitems;
	int i;

	nitems = fSettingsList->CountItems();
	for(i = 0; i < nitems; i++) {
		Setting *s;
		BTextControl *tc;

		s = (Setting*)fSettingsList->ItemAt(i);
		if(s == NULL) {
			continue;
		}

		CookieToName(s->cookie, name, sizeof(name));
		tc = dynamic_cast<BTextControl*>(FindView(name));
		if(tc == NULL) {
			continue;
		}

		tc->SetLabel(s->name);
		tc->SetText(s->value);
		s->dirty = false;
		SetDirty(tc, false);
	}
}

void
ListEditView::SetDirty(BTextControl *tc, bool dirty)
{
	if(dirty) {
		tc->SetHighColor(kDirtyColor);
	}
	else {
		tc->SetHighColor(kCleanColor);
	}
	tc->Invalidate();
}

void
ListEditView::ItemChanged(int32 cookie)
{
	Setting *s;
	char name[9];
	int nitems;
	int i;

	CookieToName(cookie, name, sizeof(name));

	BTextControl *tc = dynamic_cast<BTextControl*>(FindView(name));
	if(tc == NULL) {
		return;
	}

	nitems = fSettingsList->CountItems();
	for(i = 0; i < nitems; i++) {
		s = (Setting*)fSettingsList->ItemAt(i);
		if(s->cookie == cookie) {
			break;
		}
	}
	if(i == nitems) {
		return;
	}

	if(strcmp(s->value, tc->Text()) == 0) {
		if(s->dirty) {
			SetDirty(tc, false);
		}
		s->dirty = false;

		--fDirty;
	}
	else {
		if(!s->dirty) {
			SetDirty(tc, true);
		}
		s->dirty = true;

		fDirty++;
	}

	if(fChangedInvoker) {
		fChangedInvoker->Invoke();
	}
}

void
ListEditView::Revert()
{
	PopulateList();
}

void
ListEditView::Commit()
{
	char name[9];
	int nitems;
	int i;

	nitems = fSettingsList->CountItems();
	for(i = 0; i < nitems; i++) {
		Setting *s;
		BTextControl *tc;

		s = (Setting*)fSettingsList->ItemAt(i);
		if(s == NULL || !s->dirty) {
			continue;
		}

		CookieToName(s->cookie, name, sizeof(name));
		tc = dynamic_cast<BTextControl*>(FindView(name));
		if(tc == NULL) {
			continue;
		}

		free(s->value);
		s->value = strdup(tc->Text());
		s->dirty = false;
		SetDirty(tc, false);
	}

	fDirty = 0;
}
