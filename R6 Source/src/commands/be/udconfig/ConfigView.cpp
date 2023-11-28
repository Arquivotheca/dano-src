//
// Copyright 2000 Be, Incorporated.  All Rights Reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Message.h>
#include <Invoker.h>
#include <StringView.h>
#include <ScrollView.h>
#include <Button.h>
#include <Box.h>
#include <Alert.h>
#include <Application.h>

#include "ListEditView.h"
#include "ConfigView.h"
#include "Settings.h"
#include "ushSF.h"
#include "attrSF.h"

#define MSG_BUTTON_REVERT	'bRVT'
#define MSG_BUTTON_SAVE		'bSAV'
#define MSG_LIST_CHANGED	'lCHG'

#define REVERT_BUTTON_NAME	"Revert Button"
#define SAVE_BUTTON_NAME	"Save Button"

ConfigView::ConfigView(BRect frame, const char *name, uint32 resizingMode, uint32 flags, const char *fname, BInvoker *invoker) :
	BView(frame, name, resizingMode, flags),
	fEm(be_plain_font->StringWidth("m")),
	fErr(B_NO_INIT),
	fFileName(strdup(fname)),
	fDoneInvoker(invoker),
	fSettingsFile(NULL)
{
	BRect r;
	ushSF *ush;
	attrSF *attr;
	status_t err;

	ush = new ushSF(fname);
	err = ush->Load();
	if(err == B_OK) {
		fSettingsList = ush->AcquireSettingsList();
		if(fSettingsList->CountItems() > 0) {
			fSettingsFile = ush;
		}
		else {
			ush->ReleaseSettingsList();
		}
	}
	if(fSettingsFile == NULL) {
		attr = new attrSF(fname);
		err = attr->Load();
		if(err == B_OK) {
			fSettingsList = attr->AcquireSettingsList();
			if(fSettingsList->CountItems() > 0) {
				fSettingsFile = attr;
			}
			else {
				attr->ReleaseSettingsList();
			}
		}
	}
	if(fSettingsFile == NULL) {
		char *buf = (char*)malloc(strlen(fname) + 128);
		sprintf(buf, "\"%s\": Unknown file format", fname);
		BAlert *a = new BAlert("Bad File Format", buf, "Stop", NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_STOP_ALERT);
		free(buf);
		a->Go();
		be_app->PostMessage(B_QUIT_REQUESTED, be_app);
		return;
	}

	r = Bounds();
	r.InsetBy(fEm/2, fEm/2);

	AddFileName(r);
	r.top += fEm/2;

	AddButtons(r);
	r.bottom -= fEm;

	AddListEditView(r);

	fErr = B_OK;
}


ConfigView::~ConfigView()
{
	if(fListEditView->DirtyCount() > 0) {
		BAlert *a = new BAlert("Save", "Would you like to save your changes?", "Don't Save", "Save", NULL, B_WIDTH_AS_USUAL,
				B_OFFSET_SPACING, B_WARNING_ALERT);
		int32 index = a->Go();
		if(index == 1) {
			Save();
			a = new BAlert("OK", "Changes have been saved.", "OK", NULL, NULL, B_WIDTH_AS_USUAL,
				B_OFFSET_SPACING, B_INFO_ALERT);
			a->Go();
		}
	}

	free(fFileName);
}

status_t
ConfigView::InitCheck()
{
	return fErr;
}

void
ConfigView::AttachedToWindow()
{
	if(Parent()) {
		SetViewColor(Parent()->ViewColor());
	}

	BButton *b;

	b = dynamic_cast<BButton*>(FindView(REVERT_BUTTON_NAME));
	b->SetTarget(this);

	b = dynamic_cast<BButton*>(FindView(SAVE_BUTTON_NAME));
	b->SetTarget(this);

	BInvoker *invoker = new BInvoker(new BMessage(MSG_LIST_CHANGED), this);
	fListEditView->SetChangedInvoker(invoker);

	BView::AttachedToWindow();
}

void
ConfigView::FixScrollBar()
{
	if(fScrollView) {
		BScrollBar *sb = fScrollView->ScrollBar(B_VERTICAL);

		sb->SetRange(0, MAX(fListHeight-fScrollHeight,0));
		sb->SetProportion(fScrollHeight/fListHeight);
		sb->SetSteps(fItemHeight, fScrollHeight);
	}
}

void
ConfigView::AllAttached()
{
	FixScrollBar();

	BView::AllAttached();
}

void
ConfigView::FrameResized(float width, float height)
{
	if(fScrollView) {
		fScrollHeight = fListEditView->Bounds().Height();
		FixScrollBar();
	}

	BView::FrameResized(width, height);
}

void
ConfigView::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
	case MSG_BUTTON_REVERT:
		fListEditView->Revert();
		break;
	case MSG_BUTTON_SAVE:
		Save();
		break;
	case MSG_LIST_CHANGED:
		EnableButtons(fListEditView->DirtyCount());
		break;
	default:
		// not handling this msg
		BView::MessageReceived(msg);
		break;
	}
}

void
ConfigView::AddFileName(BRect &r)
{
	BRect br = r;
	br.bottom = br.top + 4 * fEm;

	BBox *b = new BBox(br, "Box:FileName", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	AddChild(b);

	br.InsetBy(fEm,fEm);
	br.bottom -= fEm/4;

	BStringView *s = new BStringView(br, "String:FileName", fFileName, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	b->AddChild(s);
	s->SetFont(be_bold_font);

	r.top += b->Bounds().Height();
}

void
ConfigView::AddListEditView(BRect &r)
{
	float f;

	BRect vr(r.InsetByCopy(3,3));
	vr.right -= B_V_SCROLL_BAR_WIDTH;

	fListEditView = new ListEditView(vr, "ListEditView",
			B_FOLLOW_ALL_SIDES,
			B_WILL_DRAW | B_FRAME_EVENTS, fSettingsList);
	fScrollView = new BScrollView("Scroll View", fListEditView, B_FOLLOW_ALL_SIDES, B_WILL_DRAW | B_FRAME_EVENTS, false, true);
	AddChild(fScrollView);

	fListEditView->GetItemHeight(&fItemHeight);
	fListEditView->GetPreferredSize(&f, &fListHeight);

	fScrollHeight = fListEditView->Bounds().Height();

	r.top += fScrollView->Bounds().Height();
}

void
ConfigView::AddButtons(BRect &r)
{
	BRect br;
	BButton *b;
	float w, h;

	br = BRect(r.left, r.bottom, 0, 0);


	b = new BButton(br, REVERT_BUTTON_NAME, "Revert", new BMessage(MSG_BUTTON_REVERT), B_FOLLOW_BOTTOM|B_FOLLOW_LEFT);
	AddChild(b);
	b->ResizeToPreferred();
	b->GetPreferredSize(&w, &h);
	b->MoveBy(0, -h);
	b->SetEnabled(false);
	br.left += w + fEm;

	b = new BButton(br, SAVE_BUTTON_NAME, "Save", new BMessage(MSG_BUTTON_SAVE), B_FOLLOW_BOTTOM|B_FOLLOW_LEFT);
	AddChild(b);
	b->ResizeTo(w, h);
	b->MoveBy(0, -h);
	b->SetEnabled(false);

	r.bottom -= h;
}

void
ConfigView::EnableButtons(bool enabled)
{
	BButton *b;

	b = dynamic_cast<BButton*>(FindView(REVERT_BUTTON_NAME));
	b->SetEnabled(enabled);

	b = dynamic_cast<BButton*>(FindView(SAVE_BUTTON_NAME));
	b->SetEnabled(enabled);
}

void
ConfigView::Save()
{
	status_t err;

	fListEditView->Commit();

	fSettingsFile->ReleaseSettingsList();
	err = fSettingsFile->Save();
	if(err != B_OK) {
		fprintf(stderr, "ConfigView: Error saving settings: %s\n", strerror(err));
	}
	(void)fSettingsFile->AcquireSettingsList();
}
