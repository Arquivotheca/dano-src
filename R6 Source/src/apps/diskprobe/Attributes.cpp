//--------------------------------------------------------------------
//	
//	Attributes.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1998 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#pragma once

#include <parsedate.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "DiskProbe.h"
#include "ProbeWindow.h"
#include "Attributes.h"


//====================================================================

TAttributesWindow::TAttributesWindow(BRect rect, attribute *attr,
					TProbeWindow *window, bool read_only)
			 :BWindow(rect, "", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
			 			B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	char	*title;
	BRect	r;

	r = rect;
	r.OffsetTo(0,0);
	fAttributesView = new TAttributesView(r, attr, window, read_only);
	AddChild(fAttributesView);
	title = (char *)malloc(strlen(window->Title()) + strlen(attr->name) + 10);
	sprintf(title, "%s - %s", window->Title(), attr->name);
	SetTitle(title);
	free(title);
}

//--------------------------------------------------------------------

void TAttributesWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_OK:
		case M_CANCEL:
		case M_REVERT:
		case M_MODIFIED:
		case M_CHANGED:
			fAttributesView->MessageReceived(msg);

		default:
			BWindow::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------

bool TAttributesWindow::QuitRequested(void)
{
	return(fAttributesView->Quit());
}


//====================================================================

TAttributesView::TAttributesView(BRect rect, attribute *attr,
					TProbeWindow *window, bool read_only)
		  :BBox(rect, "", B_FOLLOW_LEFT_RIGHT, B_WILL_DRAW, B_PLAIN_BORDER)
{
	int32		loop;
	uint32		type[2] = {0, 0};
	BFont		font = *be_plain_font;
	BRect		r;
	BRect		text;
	BStringView	*str;

	fAttr = attr;
	fWindow = window;
	fReadOnly = read_only;

	r.Set(TYPE_X1, TYPE_Y1, TYPE_X1 + font.StringWidth(TYPE_TEXT), TYPE_Y2);
	str = new BStringView(r, "", TYPE_TEXT);
	str->SetFont(&font);
	AddChild(str);

	type[0] = B_HOST_TO_BENDIAN_INT32(attr->type);
	r.left = r.right;
	r.right = r.left + font.StringWidth((char *)type);
	str = new BStringView(r, "", (char *)type);
	str->SetFont(&font);
	AddChild(str);

	r.Set(NAME_X1, NAME_Y1, NAME_X1 + font.StringWidth(NAME_TEXT), NAME_Y2);
	str = new BStringView(r, "", NAME_TEXT);
	str->SetFont(&font);
	AddChild(str);

	r.left = r.right;
	r.right = r.left + font.StringWidth(attr->name);
	str = new BStringView(r, "", attr->name);
	str->SetFont(&font);
	AddChild(str);

	switch (fAttr->type) {
		case B_POINT_TYPE:
			r.Set(FIELD1_X1, FIELD1_Y1, FIELD1_X2, FIELD1_Y2);
			fFields[X] = new BTextControl(r, "", "X", "", new BMessage(M_CHANGED));
			fView = fFields[X]->ChildAt(0);

			r.Set(FIELD2_X1, FIELD2_Y1, FIELD2_X2, FIELD2_Y2);
			fFields[Y] = new BTextControl(r, "", "Y", "", new BMessage(M_CHANGED));

			for (loop = X; loop <= Y; loop++) {
				fFields[loop]->SetModificationMessage(new BMessage(M_MODIFIED));
				fFields[loop]->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
				fFields[loop]->SetDivider(font.StringWidth("X "));
				AddChild(fFields[loop]);
			}
			break;

		case B_RECT_TYPE:
			r.Set(FIELD1_X1, FIELD1_Y1, FIELD1_X2, FIELD1_Y2);
			fFields[LEFT] = new BTextControl(r, "", "Left", "", new BMessage(M_CHANGED));
			fView = fFields[LEFT]->ChildAt(0);

			r.Set(FIELD2_X1, FIELD2_Y1, FIELD2_X2, FIELD2_Y2);
			fFields[TOP] = new BTextControl(r, "", "Top", "", new BMessage(M_CHANGED));

			r.Set(FIELD3_X1, FIELD3_Y1, FIELD3_X2, FIELD3_Y2);
			fFields[RIGHT] = new BTextControl(r, "", "Right", "", new BMessage(M_CHANGED));

			r.Set(FIELD4_X1, FIELD4_Y1, FIELD4_X2, FIELD4_Y2);
			fFields[BOTTOM] = new BTextControl(r, "", "Bottom", "", new BMessage(M_CHANGED));

			for (loop = LEFT; loop <= BOTTOM; loop++) {
				fFields[loop]->SetModificationMessage(new BMessage(M_MODIFIED));
				fFields[loop]->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
				fFields[loop]->SetDivider(font.StringWidth("Bottom "));
				AddChild(fFields[loop]);
			}
			break;

		default:
			r.Set(FIELD_X1, FIELD_Y1, FIELD_X2, FIELD_Y2);
			text = r;
			text.OffsetTo(B_ORIGIN);
			text.InsetBy(2,2);
			fTextView = new TTextView(r, text, &fDirty);
			AddChild(fTextView);
			fTextView->MakeEditable(!fReadOnly);
			fView = fTextView;
	}

	r.Set(OK_BUTTON_X1, OK_BUTTON_Y1, OK_BUTTON_X2, OK_BUTTON_Y2);
	fOKButton = new BButton(r, "", OK_BUTTON_TEXT, new BMessage(M_OK));
	AddChild(fOKButton);

	r.Set(CANCEL_BUTTON_X1, CANCEL_BUTTON_Y1, CANCEL_BUTTON_X2, CANCEL_BUTTON_Y2);
	fCancelButton = new BButton(r, "", CANCEL_BUTTON_TEXT, new BMessage(M_CANCEL));
	AddChild(fCancelButton);

	r.Set(REVERT_BUTTON_X1, REVERT_BUTTON_Y1, REVERT_BUTTON_X2, REVERT_BUTTON_Y2);
	fRevertButton = new BButton(r, "", REVERT_BUTTON_TEXT, new BMessage(M_REVERT));
	AddChild(fRevertButton);
}

//--------------------------------------------------------------------

void TAttributesView::AttachedToWindow(void)
{
	BView::AttachedToWindow();
	SetViewColor(216,216,216);
	Window()->SetDefaultButton(fOKButton);
	fView->MakeFocus(true);
	Window()->Hide();
	Window()->Show();
	if (SetData() == B_ERROR)
		Window()->PostMessage(M_CANCEL, this);
	else {
		fRevertButton->SetEnabled(false);
		fOKButton->SetEnabled(false);
		Window()->Show();
	}
	fDirty = false;
}

//--------------------------------------------------------------------

void TAttributesView::Draw(BRect where)
{
	BRect	r;

	r = fView->Frame();
	r.InsetBy(-1,-1);
	fView->Parent()->SetHighColor(128,128,128);
	fView->Parent()->StrokeRect(r);
	fView->Parent()->SetHighColor(0,0,0);
	
	BBox::Draw(where);
}

//--------------------------------------------------------------------

void TAttributesView::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case M_OK:
		case M_CANCEL:
			Quit(msg->what);
			break;

		case M_REVERT:
			SetData();
			fDirty = false;
			break;

		case M_MODIFIED:
			fDirty = CheckData();
			//fall through

		case M_CHANGED:
			fRevertButton->SetEnabled(fDirty);
			fOKButton->SetEnabled(fDirty);
			break;

		default:
			BView::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------

bool TAttributesView::CheckData(void)
{
	switch (fAttr->type) {
		case B_POINT_TYPE:
			BPoint	*p;
			p = (BPoint *)fAttr->data;
			return ((p->x != atof(fFields[X]->Text())) ||
					(p->y != atof(fFields[Y]->Text())));

		case B_RECT_TYPE:
			BRect	*r;
			r = (BRect *)fAttr->data;
			return ((r->left != atof(fFields[LEFT]->Text())) ||
					(r->top != atof(fFields[TOP]->Text())) ||
					(r->right != atof(fFields[RIGHT]->Text())) ||
					(r->bottom != atof(fFields[BOTTOM]->Text())));

		default:
			return false;
	}
}

//--------------------------------------------------------------------

status_t TAttributesView::GetData(void)
{
	status_t	result = B_NO_ERROR;
	time_t		t;

	switch (fAttr->type) {
		case B_ASCII_TYPE:
			fAttr->size = fTextView->TextLength();
			fAttr->data = realloc(fAttr->data, fAttr->size);
			fTextView->GetText(0, fAttr->size, (char *)fAttr->data);
			break;

		case B_CHAR_TYPE:
			fTextView->GetText(0, 1, (char *)fAttr->data);
			break;

		case B_DOUBLE_TYPE:
			*((double *)fAttr->data) = strtod(fTextView->Text(), NULL);
			break;

		case B_FLOAT_TYPE:
			*((float *)fAttr->data) = atof(fTextView->Text());
			break;

		case B_BOOL_TYPE:
			*((bool *)fAttr->data) = strtol(fTextView->Text(), NULL, 0);
			break;

		case B_INT8_TYPE:
			*((int8 *)fAttr->data) = strtol(fTextView->Text(), NULL, 0);
			break;

		case B_UINT8_TYPE:
			*((uint8 *)fAttr->data) = strtoul(fTextView->Text(), NULL, 0);
			break;

		case B_INT64_TYPE:
			*((int64 *)fAttr->data) = strtoll(fTextView->Text(), NULL, 0);
			break;

		case B_OFF_T_TYPE:
			*((off_t *)fAttr->data) = strtoll(fTextView->Text(), NULL, 0);
			break;

		case B_SIZE_T_TYPE:
			*((size_t *)fAttr->data) = strtoull(fTextView->Text(), NULL, 0);
			break;

		case B_SSIZE_T_TYPE:
			*((ssize_t *)fAttr->data) = strtoll(fTextView->Text(), NULL, 0);
			break;

		case B_UINT64_TYPE:
			*((uint64 *)fAttr->data) = strtoull(fTextView->Text(), NULL, 0);
			break;

		case B_INT32_TYPE:
		case B_POINTER_TYPE:
			*((int32 *)fAttr->data) = strtol(fTextView->Text(), NULL, 0);
			break;

		case B_INT16_TYPE:
			*((int16 *)fAttr->data) = strtol(fTextView->Text(), NULL, 0);
			break;

		case B_UINT16_TYPE:
			*((uint16 *)fAttr->data) = strtoul(fTextView->Text(), NULL, 0);
			break;

		case B_TIME_TYPE:
			t = parsedate(fTextView->Text(), time((time_t *)NULL));
			if (t != -1)
				*((time_t *)fAttr->data) = t;
			else {
				(new BAlert("", "Entered date is unrecognizable.  Try again.", "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
				result = B_ERROR;
			}
			break;

		case B_STRING_TYPE:
			fAttr->size = strlen(fTextView->Text());
			fAttr->data = realloc(fAttr->data, fAttr->size);
			memcpy((char *)fAttr->data, fTextView->Text(), fAttr->size);
			break;

		case B_MIME_TYPE:
		case 'MIMS':
			fAttr->size = strlen(fTextView->Text()) + 1;
			fAttr->data = realloc(fAttr->data, fAttr->size);
			strcpy((char *)fAttr->data, fTextView->Text());
			break;

		case B_POINT_TYPE:
			((BPoint *)fAttr->data)->x = atof(fFields[X]->Text());
			((BPoint *)fAttr->data)->y = atof(fFields[Y]->Text());
			break;

		case B_RECT_TYPE:
			((BRect *)fAttr->data)->left = atof(fFields[LEFT]->Text());
			((BRect *)fAttr->data)->top = atof(fFields[TOP]->Text());
			((BRect *)fAttr->data)->right = atof(fFields[RIGHT]->Text());
			((BRect *)fAttr->data)->bottom = atof(fFields[BOTTOM]->Text());
			break;
	}
	return result;
}

//--------------------------------------------------------------------

bool TAttributesView::Quit(int32 quit)
{
	int32	result = false;

	if (quit == M_OK)
		result = true;
	else if ((!quit) && (fDirty)) {
		Window()->Activate(true);
		result = (new BAlert("", "Save changes to attribute before closing?",
			"Cancel", "Don't Save", "Save", B_WIDTH_AS_USUAL,
			B_OFFSET_SPACING, B_WARNING_ALERT))->Go();
		if (result == 0)
			return false;
		if (result == 1)
			result = 0;
	}
	if ((fDirty) && (result)) {
		if (GetData() != B_NO_ERROR)
			return false;
	}
	fWindow->AttrQuit(result, (TAttributesWindow *)Window());
	return true;
}

//--------------------------------------------------------------------

status_t TAttributesView::SetData(void)
{
	char		str[256];
	status_t	result = B_NO_ERROR;
	struct tm	*date;

	switch (fAttr->type) {
		case B_ASCII_TYPE:
			fTextView->SetText((char *)fAttr->data, fAttr->size);
			break;

		case B_CHAR_TYPE:
			fTextView->SetText(((char *)fAttr->data), 1);
			break;

		case B_DOUBLE_TYPE:
			sprintf(str, "%f", *((double *)fAttr->data));
			fTextView->SetText(str);
			break;

		case B_FLOAT_TYPE:
			sprintf(str, "%f", *((float *)fAttr->data));
			fTextView->SetText(str);
			break;

		case B_BOOL_TYPE:
		case B_INT8_TYPE:
		case B_UINT8_TYPE:
			sprintf(str, "%d", *((char *)fAttr->data));
			fTextView->SetText(str);
			break;

		case B_INT64_TYPE:
		case B_OFF_T_TYPE:
		case B_SIZE_T_TYPE:
		case B_SSIZE_T_TYPE:
		case B_UINT64_TYPE:
			sprintf(str, "%Ld", *((int64 *)fAttr->data));
			fTextView->SetText(str);
			break;

		case B_INT32_TYPE:
		case B_POINTER_TYPE:
		case B_UINT32_TYPE:
			sprintf(str, "%li", *((int32 *)fAttr->data));
			fTextView->SetText(str);
			break;

		case B_INT16_TYPE:
		case B_UINT16_TYPE:
			sprintf(str, "%d", *((int16 *)fAttr->data));
			fTextView->SetText(str);
			break;

		case B_TIME_TYPE:
			date = localtime((long *)fAttr->data);
			strftime(str, 255, "%a, %d %b %Y %H:%M:%S %Z", date);
			fTextView->SetText(str);
			break;

		case B_MIME_TYPE:
		case 'MIMS':
			fTextView->SetText((char *)fAttr->data);
			break;

		case B_STRING_TYPE:
			fTextView->SetText((char *)fAttr->data, fAttr->size);
			break;

		case B_POINT_TYPE:
			sprintf(str, "%f", ((BPoint *)fAttr->data)->x);
			fFields[X]->SetText(str);
			sprintf(str, "%f", ((BPoint *)fAttr->data)->y);
			fFields[Y]->SetText(str);
			break;

		case B_RECT_TYPE:
			sprintf(str, "%f", ((BRect *)fAttr->data)->left);
			fFields[LEFT]->SetText(str);
			sprintf(str, "%f", ((BRect *)fAttr->data)->top);
			fFields[TOP]->SetText(str);
			sprintf(str, "%f", ((BRect *)fAttr->data)->right);
			fFields[RIGHT]->SetText(str);
			sprintf(str, "%f", ((BRect *)fAttr->data)->bottom);
			fFields[BOTTOM]->SetText(str);
			break;

		default:
			(new BAlert("", "Sorry no editor available for this attribute type.", "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
			result = B_ERROR;
	}
	return result;
}


//====================================================================

TTextView::TTextView(BRect rect, BRect text, bool *dirty)
		  :BTextView(rect, "", text, B_FOLLOW_ALL, B_WILL_DRAW | B_NAVIGABLE)
{
	fDirty = dirty;
}

//--------------------------------------------------------------------

void TTextView::InsertText(const char *text, int32 len, int32 offset,
							const text_run_array *run)
{
	*fDirty = true;
	Window()->PostMessage(M_CHANGED, Parent());
	BTextView::InsertText(text, len, offset, run);
}

//--------------------------------------------------------------------

void TTextView::DeleteText(int32 from, int32 to)
{
	*fDirty = true;
	Window()->PostMessage(M_CHANGED, Parent());
	BTextView::DeleteText(from, to);
}
