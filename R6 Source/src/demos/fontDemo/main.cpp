//******************************************************************************
//
//	File:		main.cpp
//
//	Description:	Font Demo application.
//
//	Copyright 1993-95, Be Incorporated
//
//******************************************************************************

#ifndef _DEBUG_H
#include <Debug.h>
#endif

#ifndef _ALERT_H
#include <Alert.h>
#endif
#ifndef _BOX_H
#include <Box.h>
#endif
#include <CheckBox.h>
#include <Screen.h>
#include <Shape.h>
#include <StreamIO.h>
#include <StringIO.h>
#include <TextControl.h>
#include <PopUpMenu.h>
#include <MenuItem.h>
#include <TranslationUtils.h>
#include <Entry.h>
#include "main.h"

/*------------------------------------------------------------*/

inline int32
UTF8_CHAR_LEN(uchar c)
	{ return (((0xE5000000 >> (((c) >> 3) & 0x1E)) & 3) + 1); }

inline uint32
UTF8_TO_UNICODE(const uchar *src)
{
	if ((src[0]&0x80) == 0) {
		return *src;
	} else if ((src[1] & 0xC0) != 0x80) {
		return 0xfffdfffd;
	} else if ((src[0]&0x20) == 0) {
		return ((src[0]&31)<<6) | (src[1]&63);
	} else if ((src[2] & 0xC0) != 0x80) {
		return 0xfffdfffd;
	} else if ((src[0]&0x10) == 0) {
		return ((src[0]&15)<<12) | ((src[1]&63)<<6) | (src[2]&63);
	} else if ((src[3] & 0xC0) != 0x80) {
		return 0xfffdfffd;
	} else {
		return ((src[0]&7)<<18) | ((src[1]&63)<<12)
				| ((src[2]&63)<<6) | (src[3]&63);
	}
}

TControlScroller::TControlScroller(
	BRect		view_bound, 
	const char	*label,
	int32		min,
	int32		max,
	BLooper		*target,
	uint32		command,
	sem_id		sem)
		: BSlider(view_bound, B_EMPTY_STRING, label, NULL, min, max)
{
	fSem = sem;

	SetTarget(target);

	BMessage *invMsg = new BMessage(command);
	invMsg->AddInt32("sem", fSem);
	SetMessage(invMsg);

	BMessage *modMsg = new BMessage(command);
	modMsg->AddInt32("sem", fSem);
	SetModificationMessage(modMsg);
}

/*------------------------------------------------------------*/

status_t
TControlScroller::Invoke(
	BMessage	*msg)
{
	status_t result = BSlider::Invoke(msg);
	if (result == B_NO_ERROR)
		result = acquire_sem(fSem);
	return result;
}

/*------------------------------------------------------------*/

void
TControlScroller::DrawText()
{
	char	theLabel[256] = "";
	int32	value = Value();
	sprintf(theLabel, "%s %ld", Label(), value);		

	BView *offView = OffscreenView();
	offView->SetHighColor(ui_color(B_CONTROL_TEXT_COLOR));
	offView->SetLowColor(ViewColor());

	font_height finfo;
	offView->GetFontHeight(&finfo);

	offView->MovePenTo(2, ceil(finfo.ascent + finfo.descent + finfo.leading));
	offView->DrawString(theLabel);
}

/*------------------------------------------------------------*/

TControlWindow::TControlWindow(BWindow *main_window, BRect bound,
		window_look look, window_feel feel, ulong flags, const char *name) :
	BWindow(bound, name, look, feel, flags | B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_ASYNCHRONOUS_CONTROLS)
{
	BRect				a_rect;
	BRect				dst_rect;
	TControlScroller	*sb;
	BBox*				top;

	BMessage*			msg;
	BMenu*				menu;
	BMenuItem*			item;
	
	float				max_div;
	float				div;
	
	Run();
	
	Lock();

	control_sem = create_sem(1, "update_lock");
	acquire_sem(control_sem);	

	fAutoCycle = FALSE;

#define set_rect(R, t, l, b, r) R.Set(l, t, r, b)

	a_rect = Bounds();
	a_rect.right++;
	a_rect.bottom++;
	top = new BBox(a_rect, "", B_FOLLOW_ALL, B_WILL_DRAW |
				   B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_PLAIN_BORDER);
	top->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	top->SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	AddChild(top);

	set_rect(a_rect, 11, 10, 11, a_rect.right - 11);
	BTextControl *tc = new BTextControl(a_rect, B_EMPTY_STRING, "Text:", sampleText, new BMessage(NEW_TEXT));
	tc->SetModificationMessage(new BMessage(NEW_TEXT));
	max_div = tc->StringWidth(tc->Label()) + 6.0;

	set_rect(a_rect, 38, 10, 56, a_rect.right + 1);
	fFontField = new TFontMenu(a_rect, main_window);
	div = fFontField->Divider();
	if (div > max_div) max_div = div;

	set_rect(a_rect, 60, 10, 78, a_rect.right);
	static const char* face_names[] = {
		"Bold", "Italic", NULL
	};
	static const uint16 face_bits[] = {
		B_BOLD_FACE, B_ITALIC_FACE, 0
	};
	menu = new BPopUpMenu("face", false, false);
	for (int32 i=0; face_names[i]; i++) {
		msg = new BMessage(SET_FACE);
		msg->AddInt32("face", face_bits[i]);
		item = new BMenuItem(face_names[i], msg);
		menu->AddItem(item);
	}
	fFaceField = new BMenuField(a_rect, "face", "Face:", menu);
	menu->SetTargetForItems(main_window);
	div = fFaceField->StringWidth(fFaceField->Label())+6;
	if (div > max_div) max_div = div;
	
	tc->SetDivider(max_div);
	top->AddChild(tc);
	tc->SetTarget(main_window);
	
	fFontField->SetDivider(max_div);
	top->AddChild(fFontField);
	fFontField->SetSelectedFont(be_plain_font, true);
	
	fFaceField->SetDivider(max_div);
	fFaceField->ResizeToPreferred();
	top->AddChild(fFaceField);
	SetFaceMenu(*be_plain_font);
	
	/*
	 The 'Size' area of the control window
	*/
	set_rect(a_rect, 86, 11, 116, a_rect.right - 1);
	sb = new TControlScroller(a_rect, "Size:", 4, 360, main_window, NEW_SIZE, control_sem);
	top->AddChild(sb);
	sb->SetValue(50);
	
	/*
	 The 'Shear' area of the control window
	*/
	a_rect.OffsetBy(0, a_rect.Height() + 6);
	sb = new TControlScroller(a_rect, "Shear:", 45, 135, main_window, NEW_SHEAR, control_sem);
	top->AddChild(sb);
	sb->SetValue(90);
	
	/*
	 The 'Rotation' area of the control window
	*/
	a_rect.OffsetBy(0, a_rect.Height() + 6);
	sb = new TControlScroller(a_rect, "Rotation:", 0, 360, main_window, NEW_ROTATION, control_sem);
	top->AddChild(sb);
	sb->SetValue(0);


	/*
	 The 'Spacing' area of the control window
	*/
	a_rect.OffsetBy(0, a_rect.Height() + 6);
	sb = new TControlScroller(a_rect, "Spacing:", -5, 50, main_window, NEW_SPACING, control_sem);
	top->AddChild(sb);
	sb->SetValue(0);

	/*
	 The Outline area of the control window
	*/
	a_rect.OffsetBy(0, a_rect.Height() + 6);
	sb = new TControlScroller(a_rect, "Outline:", 0, 20, main_window, NEW_OUTLINE, control_sem);
	top->AddChild(sb);
	sb->SetValue(0);

	a_rect.OffsetBy(0, a_rect.Height() + 8);
	menu = new BPopUpMenu("antialiasing");
	msg = new BMessage(NEW_AA_SETTING);
	msg->AddInt32("value", 0);
	menu->AddItem(item = new BMenuItem("Default", msg));
	menu->AddSeparatorItem();
	msg = new BMessage(NEW_AA_SETTING);
	msg->AddInt32("value", B_DISABLE_ANTIALIASING);
	menu->AddItem(new BMenuItem("Disabled", msg));
	msg = new BMessage(NEW_AA_SETTING);
	msg->AddInt32("value", B_NORMAL_ANTIALIASING);
	menu->AddItem(new BMenuItem("Normal", msg));
	msg = new BMessage(NEW_AA_SETTING);
	msg->AddInt32("value", B_TV_ANTIALIASING);
	menu->AddItem(new BMenuItem("TV", msg));
	item->SetMarked(true);
	fAAField = new BMenuField(a_rect, "", "Anti-aliasing:", menu);
	menu->SetTargetForItems(main_window);
	max_div = fAAField->StringWidth(fAAField->Label())+6;

	a_rect.OffsetBy(0, 23);
	menu = new BPopUpMenu("hinting");
	msg = new BMessage(NEW_HINTING_SETTING);
	msg->AddInt32("value", 0);
	menu->AddItem(item = new BMenuItem("Default", msg));
	menu->AddSeparatorItem();
	msg = new BMessage(NEW_HINTING_SETTING);
	msg->AddInt32("value", B_DISABLE_HINTING);
	menu->AddItem(new BMenuItem("Disabled", msg));
	msg = new BMessage(NEW_HINTING_SETTING);
	msg->AddInt32("value", B_ENABLE_HINTING);
	menu->AddItem(new BMenuItem("Enabled", msg));
	item->SetMarked(true);
	fHintingField = new BMenuField(a_rect, "", "Hinting:", menu);
	menu->SetTargetForItems(main_window);
	div = fHintingField->StringWidth(fHintingField->Label())+6;
	if (div > max_div) max_div = div;

	a_rect.OffsetBy(0, 23);
	menu = new BPopUpMenu("spacing");
	item = new BMenuItem("Bitmap", new BMessage(SET_BITMAP_SPACING));
	menu->AddItem(new BMenuItem("Character", new BMessage(SET_CHARACTER_SPACING)));
	menu->AddItem(new BMenuItem("String", new BMessage(SET_STRING_SPACING)));
	menu->AddItem(item);
	menu->AddItem(new BMenuItem("Fixed", new BMessage(SET_FIXED_SPACING)));
	item->SetMarked(true);
	fSpacingField = new BMenuField(a_rect, "", "Spacing mode:", menu);
	menu->SetTargetForItems(main_window);
	div = fSpacingField->StringWidth(fSpacingField->Label())+6;
	if (div > max_div) max_div = div;
	
	a_rect.OffsetBy(0, 23);
	menu = new BPopUpMenu("op");
	BMessage *op;
	op = new BMessage(SET_DRAWING_MODE); op->AddInt32("op", B_OP_COPY);		menu->AddItem(item = new BMenuItem("B_OP_COPY", op));
	op = new BMessage(SET_DRAWING_MODE); op->AddInt32("op", B_OP_OVER);		menu->AddItem(new BMenuItem("B_OP_OVER", op));
	op = new BMessage(SET_DRAWING_MODE); op->AddInt32("op", B_OP_ERASE);	menu->AddItem(new BMenuItem("B_OP_ERASE", op));
	op = new BMessage(SET_DRAWING_MODE); op->AddInt32("op", B_OP_INVERT);	menu->AddItem(new BMenuItem("B_OP_INVERT", op));
	op = new BMessage(SET_DRAWING_MODE); op->AddInt32("op", B_OP_ADD);		menu->AddItem(new BMenuItem("B_OP_ADD", op));
	op = new BMessage(SET_DRAWING_MODE); op->AddInt32("op", B_OP_SUBTRACT);	menu->AddItem(new BMenuItem("B_OP_SUBTRACT", op));
	op = new BMessage(SET_DRAWING_MODE); op->AddInt32("op", B_OP_BLEND);	menu->AddItem(new BMenuItem("B_OP_BLEND", op));
	op = new BMessage(SET_DRAWING_MODE); op->AddInt32("op", B_OP_MIN);		menu->AddItem(new BMenuItem("B_OP_MIN", op));
	op = new BMessage(SET_DRAWING_MODE); op->AddInt32("op", B_OP_MAX);		menu->AddItem(new BMenuItem("B_OP_MAX", op));
	op = new BMessage(SET_DRAWING_MODE); op->AddInt32("op", B_OP_SELECT);	menu->AddItem(new BMenuItem("B_OP_SELECT", op));
	op = new BMessage(SET_DRAWING_MODE); op->AddInt32("op", B_OP_ALPHA);	menu->AddItem(new BMenuItem("B_OP_ALPHA", op));
	item->SetMarked(true);
	fDrawingModeField = new BMenuField(a_rect, "", "Drawing mode:", menu);
	menu->SetTargetForItems(main_window);
	div = fDrawingModeField->StringWidth(fDrawingModeField->Label())+6;
	if (div > max_div) max_div = div;

	fAAField->SetDivider(max_div);
	fAAField->ResizeToPreferred();
	top->AddChild(fAAField);
	
	fHintingField->SetDivider(max_div);
	fHintingField->ResizeToPreferred();
	top->AddChild(fHintingField);
	
	fSpacingField->SetDivider(max_div);
	fSpacingField->ResizeToPreferred();
	top->AddChild(fSpacingField);

	fDrawingModeField->SetDivider(max_div);
	fDrawingModeField->ResizeToPreferred();
	top->AddChild(fDrawingModeField);

	a_rect.OffsetBy(0, 25);
	fBoundsCB = new BCheckBox(a_rect, "", "Bounding boxes", new BMessage(NEW_BBOX_SETTING));
	fBoundsCB->SetValue(0);
	fBoundsCB->SetTarget(main_window);
	fBoundsCB->ResizeToPreferred();
	top->AddChild(fBoundsCB);

	const float indent = fBoundsCB->Bounds().Width()+5;
	a_rect.OffsetBy(indent, 0);
	fEscCB = new BCheckBox(a_rect, "", "Escapements", new BMessage(NEW_ESC_SETTING));
	fEscCB->SetValue(0);
	fEscCB->SetTarget(main_window);
	fEscCB->ResizeToPreferred();
	top->AddChild(fEscCB);
	
	a_rect.OffsetBy(-indent, 21);
	fInvertCB = new BCheckBox(a_rect, "", "White on black", new BMessage(NEW_INVERT_SETTING));
	fInvertCB->SetValue(0);
	fInvertCB->SetTarget(main_window);
	fInvertCB->ResizeToPreferred();
	top->AddChild(fInvertCB);
	
	a_rect.OffsetBy(0, 21);
	a_rect.left--;
	a_rect.right++;
	a_rect.bottom = a_rect.top;
	top->AddChild(new BButton(a_rect, BUTTON_NAME, "Cycle Fonts", new BMessage(RUN_THROUGH)));
	
	tc->Invoke();

	Unlock();
}

/*------------------------------------------------------------*/

void	TControlWindow::SetFaceMenu(const BFont& src)
{
	fFontField->SetSelectedFont(&src, true);
	
	BMenu* m = fFaceField->Menu();
	if (!m) return;
	
	BString label;
	for (int32 i=0; i<m->CountItems(); i++) {
		BMenuItem* it = m->ItemAt(i);
		BMessage* msg = it ? it->Message() : NULL;
		if (msg) {
			int32 face;
			if (msg->FindInt32("face", &face) == B_OK) {
				if ((face&src.Face()) != 0) {
					it->SetMarked(true);
					if (label.Length() > 0) label += "+";
					label += it->Label();
				} else {
					it->SetMarked(false);
				}
			}
		}
	}
	
	if (fFaceField->MenuItem()) {
		if (label == "") label = "Regular";
		fFaceField->MenuItem()->SetLabel(label.String());
	}
}

/*------------------------------------------------------------*/

void	TControlWindow::GetFaceMenu(BFont* dest)
{
	BMenu* m = fFaceField->Menu();
	if (!m) return;
	
	uint16 face = 0;
	for (int32 i=0; i<m->CountItems(); i++) {
		BMenuItem* it = m->ItemAt(i);
		BMessage* msg = it ? it->Message() : NULL;
		if (msg && it->IsMarked()) {
			int32 val;
			if (msg->FindInt32("face", &val) == B_OK) {
				face |= (uint16)val;
			}
		}
	}
	
	dest->SetFace(face == 0 ? B_REGULAR_FACE : face);
}

/*------------------------------------------------------------*/

void	TControlWindow::MessageReceived(BMessage* an_event)
{
	if (an_event->what == RUN_THROUGH) {
		BButton* b = (BButton*)FindView(BUTTON_NAME);
		ASSERT(b);

		if (fAutoCycle == FALSE) {
			fAutoCycle = TRUE;
			SetPulseRate(1000000);
			b->SetLabel("Stop Cycling");
		} else {
			fAutoCycle = FALSE;
			SetPulseRate(0);
			b->SetLabel("Cycle Fonts");
		}
	} else if (an_event->what == TOGGLE_AA_CHECKBOX) {
		fAAField->SetEnabled(an_event->FindBool("enabled"));
		fBoundsCB->SetEnabled(an_event->FindBool("enabled"));
		fEscCB->SetEnabled(an_event->FindBool("enabled"));
	}
	else {
		BWindow::MessageReceived(an_event);
	}
}

/*------------------------------------------------------------*/

TMainWindow::TMainWindow(BRect bound, window_type type, long flags,
		const char *name) :
	BWindow(bound, name, type, flags)
{
	// Add main window view
	fFontView = new TFontView(Bounds());
	AddChild(fFontView);
	
	// Make font list
	uint32 flag;
	numFamilies = count_font_families();
	fontFamilies = (font_family*)malloc(sizeof(font_family) * numFamilies);
		for (int i = 0; i < numFamilies; i++)
			get_font_family(i, fontFamilies+i, &flag);

	// Set initial font, set, and size
	fFontView->SetFontSize(50);
}

/*------------------------------------------------------------*/

TMainWindow::~TMainWindow()
{
	free(fontFamilies);
	free(fontStyles);
}

/*------------------------------------------------------------*/

void	TMainWindow::MessageReceived(BMessage* msg)
{
	BRect	r;
	ulong 	what = msg->what;

	switch(what) {
		case B_SIMPLE_DATA:
		{
			entry_ref ref;
			if (msg->FindRef("refs", &ref) == B_OK) {
				BBitmap *bitmap = BTranslationUtils::GetBitmap(&ref);
				if (bitmap) {
					fFontView->SetViewBitmap(bitmap);
					Redraw();
					delete bitmap; // safe to delete, the app_server keeps a reference on it.
				} else { // Ok, this is to remove the bitmap. Not pretty but efficient.
					fFontView->SetViewBitmap(NULL);
					Redraw();
				}
			}
			break;
		}
		case 	NEW_SIZE:
			{
			long value = msg->FindInt32("be:value");
			fFontView->SetFontSize(value);
			Redraw();
			release_sem(msg->FindInt32("sem"));
			break;
			}
		case	NEW_AA_SETTING:
			{
			int32 value;
			if (msg->FindInt32("value", &value) == B_OK) {
				BFont	font;
				fFontView->GetFont(&font);
				font.SetFlags((font.Flags()&(~B_ANTIALIASING_MASK)) | value);
				fFontView->SetFont(&font, B_FONT_FLAGS);
				Redraw();
			}
			break;
			}
		case	NEW_HINTING_SETTING:
			{
			int32 value;
			if (msg->FindInt32("value", &value) == B_OK) {
				BFont	font;
				fFontView->GetFont(&font);
				font.SetFlags((font.Flags()&(~(B_ENABLE_HINTING|B_DISABLE_HINTING)))
								| value);
				fFontView->SetFont(&font, B_FONT_FLAGS);
				Redraw();
			}
			break;
			}
		case	NEW_BBOX_SETTING:
			{
			BControl *control = NULL;
			if (msg->FindPointer("source", (void **)&control) == B_NO_ERROR) {
				fFontView->BBoxEnable = (control->Value() != 0);
				Redraw();
			}
			break;
			}
		case	NEW_ESC_SETTING:
			{
			BControl *control = NULL;
			if (msg->FindPointer("source", (void **)&control) == B_NO_ERROR) {
				fFontView->EscEnable = (control->Value() != 0);
				Redraw();
			}
			break;
			}
		case	NEW_INVERT_SETTING:
			{
			BControl *control = NULL;
			if (msg->FindPointer("source", (void **)&control) == B_NO_ERROR) {
				fFontView->Invert = (control->Value() != 0);
				if (fFontView->Invert) {
					fFontView->SetHighUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
					fFontView->SetLowUIColor(B_UI_DOCUMENT_TEXT_COLOR);
					fFontView->SetViewUIColor(B_UI_DOCUMENT_TEXT_COLOR);
				} else {
					fFontView->SetHighUIColor(B_UI_DOCUMENT_TEXT_COLOR);
					fFontView->SetLowUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
					fFontView->SetViewUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
				}
				Redraw();
			}
			break;
			}
		case	SET_CHARACTER_SPACING:
		case	SET_STRING_SPACING:
		case	SET_BITMAP_SPACING:
		case	SET_FIXED_SPACING:
			{
			BFont	font;
			fFontView->GetFont(&font);
			switch(what) {
				case SET_CHARACTER_SPACING:	font.SetSpacing(B_CHAR_SPACING);	break;
				case SET_STRING_SPACING:	font.SetSpacing(B_STRING_SPACING);	break;
				case SET_BITMAP_SPACING:	font.SetSpacing(B_BITMAP_SPACING);	break;
				case SET_FIXED_SPACING:		font.SetSpacing(B_FIXED_SPACING);	break;
			}
			fFontView->SetFont(&font);
			Redraw();
			break;
			}
		case	NEW_SPACING:
			{
			long value = msg->FindInt32("be:value");
			fFontView->mSpacing = value;
			BFont font;
			font.SetSpacing(B_CHAR_SPACING);
			fFontView->SetFont(&font, B_FONT_SPACING);
			Redraw();
			release_sem(msg->FindInt32("sem"));
			break;
			}
		case	NEW_OUTLINE:
			{
			long value = msg->FindInt32("be:value");
			fFontView->mOutline = value;
			Redraw();

			BMessage message(TOGGLE_AA_CHECKBOX);
			message.AddBool("enabled", value == 0);
			ctrl_window->PostMessage(&message);

			release_sem(msg->FindInt32("sem"));
			break;
			}
		case	NEW_SHEAR:
			{
			long value = msg->FindInt32("be:value");
			//fFontView->SetFontShear(value);
			BFont font;
			font.SetShear(value);
			fFontView->SetFont(&font, B_FONT_SHEAR);
			Redraw();
			release_sem(msg->FindInt32("sem"));
			break;
			}
		case	NEW_ROTATION:
			{
			long value = msg->FindInt32("be:value");
			//fFontView->SetFontRotation(value);
			BFont font;
			font.SetRotation(value);
			fFontView->SetFont(&font, B_FONT_ROTATION);
			Redraw();
			release_sem(msg->FindInt32("sem"));
			break;
			}
		case	NEW_TEXT:
			{
			BTextControl *control = NULL;
			if (msg->FindPointer("source", (void **)&control) == B_NO_ERROR) {
				fFontView->SetText(control->Text());
				Redraw();
			}
			break;
			}
		case	NEW_FONT:
			{
				BFont font;
				if (msg->FindFlat("font", &font) >= B_OK) {
					fFontView->SetFont(&font, B_FONT_FAMILY_AND_STYLE);
					Redraw();
					if (ctrl_window) {
						ctrl_window->SetFaceMenu(&font);
					}
				}
			}
			break;
		case	SET_FACE:
			if (ctrl_window) {
				int32 val;
				if (msg->FindInt32("face", &val) == B_OK) {
					BFont font;
					fFontView->GetFont(&font);
					uint16 face = font.Face();
					if (face == B_REGULAR_FACE)
						face = 0;
					face ^= (uint16)val;
					if (face == 0)
						face = B_REGULAR_FACE;
					font.SetFace(face);
					fFontView->SetFont(&font);
					Redraw();
					if (ctrl_window) {
						ctrl_window->SetFaceMenu(font);
					}
				}
			}
			break;
		case	SET_DRAWING_MODE:
			{
				int32 op;
				if (msg->FindInt32("op", &op) == B_OK) {
					fFontView->fDrawingMode = (drawing_mode)op;
					Redraw();
				}
			}
			break;
		default		:
			BWindow::MessageReceived(msg);
			break;
	}
}

/*-------------------------------------------------------------*/

void	TMainWindow::FrameResized(float new_width, float new_height)
{
	Redraw();
}


/*------------------------------------------------------------*/

bool	TMainWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return FALSE;
}

/*------------------------------------------------------------*/

void TMainWindow::Redraw(bool forceCache)
{
	fFontView->SetVisibleBounds(fFontView->Bounds());
	fFontView->Invalidate();
}

/*------------------------------------------------------------*/

void TFontView::WindowActivated(bool state)
{
	if (!state && LockLooper()) {
		if (SetDescription("",""))
			Redraw(true);
		UnlockLooper();
	}
}

/*-------------------------------------------------------------*/

void	TFontView::MouseDown(BPoint where)
{
	BView::MouseDown(where);
}

/*-------------------------------------------------------------*/

void	TFontView::MouseUp(BPoint where)
{
	BView::MouseUp(where);
}

/*-------------------------------------------------------------*/

void TFontView::MouseMoved(	BPoint where,
							uint32 code,
							const BMessage *)
{
	bool changed = false;
	
	if (!LockLooper()) return;
	
	if (code == B_EXITED_VIEW)
		changed = SetDescription("","");
	else
		changed = SetDescription(where);
	
	if (changed)
		Redraw(true);
	
	UnlockLooper();
}

/*------------------------------------------------------------*/

void TFontView::Redraw(bool everything)
{
	Invalidate();
}

/*-------------------------------------------------------------*/

TFontView::TFontView(BRect view_bound):
	BView(view_bound, NULL, B_FOLLOW_ALL, B_WILL_DRAW | B_FRAME_EVENTS)
{
	fText = (char *)malloc(strlen(sampleText)+1);
	strcpy(fText, sampleText);
	fDrawingMode = B_OP_COPY;
	mSpacing = 0.0;
	mOutline = 0;
	BBoxEnable = false;
	EscEnable = false;
	SetDoubleBuffering(B_UPDATE_RESIZED | B_UPDATE_INVALIDATED | B_UPDATE_EXPOSED);
	SetHighUIColor(B_UI_DOCUMENT_TEXT_COLOR);
	SetLowUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	SetViewUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
}

/*------------------------------------------------------------*/

void	TFontView::SetText(const char *text)
{
	if (fText) {
		free(fText);
		fText = NULL;
	}

	if (text) {
		fText = (char *) malloc(strlen(text)+1);
		strcpy(fText, text);
	}
}

/*------------------------------------------------------------*/

void	TFontView::Draw(BRect update_rect)
{
	BRect r = font_window->Bounds();
	r.InsetBy(5, 5);
	
	const rgb_color high(HighColor());
	
	BFont font;
	GetFont(&font);
	
	BPoint where;
//	float	rot=font.Rotation();
	float	h=r.Height();
	float	w=r.Width();
	float	s=font.Size()*0.7;
	float	t=font.Rotation()*3.1459/180.0;
	float	sint = sin(t);
	float	cost = cos(t);
	
	where.x=w*(1-cost)/2+sint*s/2;
	where.y=h*(1+sint)/2+cost*s/2;
	
	where.x = floor(where.x+0.5+5);
	where.y = floor(where.y+0.5+5);

	fWhere = where;
	
	escapement_delta delta;
	delta.nonspace = mSpacing;
	delta.space = 0;
	int32 offset = 0;
	int32 count = 0;
	while (fText[offset] != 0) {
		count++;
		offset += UTF8_CHAR_LEN(((uchar)fText[offset]));
	}

	if (fText)
	{
		if (mOutline == 0) {
			if (BBoxEnable) {
				char				*p_fText[1];
				int32				i;
				BRect				*rect_chars;
				BRect				rect_string[1];

				p_fText[0] = fText;
				rect_chars = (BRect*)malloc(sizeof(BRect)*count);
				font.GetBoundingBoxesAsString(fText, count, B_SCREEN_METRIC,
											  &delta, rect_chars);
				font.GetBoundingBoxesForStrings((const char**)p_fText, 1, B_SCREEN_METRIC,
											  &delta, rect_string);
										   
				SetHighColor(64,64,256);
				rect_string[0].OffsetBy(where);
				rect_string[0].InsetBy(-1.0, -1.0);
				StrokeRect(rect_string[0]);
				
				for (i=0; i<count; i++) {
					if (i%2) SetHighColor(64,200,64);
					else SetHighColor(255,64,64);
					rect_chars[i].OffsetBy(where);
					rect_chars[i].InsetBy(-1.0, -1.0);
					StrokeRect(rect_chars[i]);
				}
				free(rect_chars);
			}
			if (EscEnable) {
				BPoint *esc;
				esc = (BPoint*)malloc(sizeof(BPoint)*count*2);
				font.GetEscapements(fText, count, &delta, esc, esc+count);
				const float width = font.StringWidth(fText);
				const float size = font.Size();
				
				font_height fh;
				font.GetHeight(&fh);
				const BPoint delta(sint*fh.descent, cost*fh.descent);
				
				SetHighColor(high);
				BPoint pos = where + delta;
				for (int32 i=-1;i<count;i++) {
					if (i >= 0) {
						pos.x += esc[i].x*size;
						pos.y += esc[i].y*size;
					}
					StrokeLine(pos, pos+delta);
				}
				free(esc);
				
				const BPoint normal(cost, -sint);
				StrokeLine(where + delta, where + delta + normal*width);
			}
			SetHighColor(high);
			MovePenTo(where);
			SetDrawingMode(fDrawingMode);
			if (fDrawingMode == B_OP_ALPHA) {
				rgb_color c = high; c.alpha=128;
				SetHighColor(c);
			}
			DrawString(fText, &delta);
			SetDrawingMode(B_OP_COPY);
		} else {
			BShape **shapes;
			float size;
			BPoint *esc;
			shapes = (BShape**)malloc(sizeof(BShape*)*count);
			esc = (BPoint*)malloc(sizeof(BPoint)*count*2);
			for (int32 i=0;i<count;i++) shapes[i] = new BShape();
			SetPenSize(mOutline);
			font.GetGlyphShapes(fText, count, shapes);
			font.GetEscapements(fText, count, &delta, esc, esc+count);
			size = font.Size();
			
			SetHighColor(200,200,200);
			MovePenTo(where);
			DrawString(fText,&delta);
			SetHighColor(high);
			for (int32 i=0;i<count;i++) {
				MovePenTo(floor(where.x+esc[i+count].x+0.5),
						  floor(where.y+esc[i+count].y+0.5)-1.0);
				StrokeShape(shapes[i]);
				where.x += esc[i].x*size;
				where.y += esc[i].y*size;
			};

			for (int32 i=0;i<count;i++) delete shapes[i];
			free(shapes);
			free(esc);
		}
	}
	
	if (fDescription1.Length() > 0) {
		PushState();
		SetDrawingMode(B_OP_INVERT);
		SetFont(be_plain_font);
		SetHighColor(high);
		font_height fh;
		GetFontHeight(&fh);
		const float w1 = StringWidth(fDescription1.String());
		const float w2 = StringWidth(fDescription2.String());
		
		MovePenTo(	BPoint(fVisBounds.left+(fVisBounds.Width()-w1)/2,
					fVisBounds.bottom-fh.descent-(fh.ascent+fh.descent+fh.leading)));
		DrawString(fDescription1.String());
		MovePenTo(	BPoint(fVisBounds.left+(fVisBounds.Width()-w2)/2,
					fVisBounds.bottom-fh.descent));
		DrawString(fDescription2.String());
		PopState();
	}
}

void	TFontView::SetVisibleBounds(BRect b)
{
	fVisBounds = b;
}

/*-------------------------------------------------------------*/

bool TFontView::SetDescription(const char* d1, const char* d2)
{
	if (fDescription1 == d1 && fDescription2 == d2) return false;
	
	fDescription1 = d1;
	fDescription2 = d2;
	
	return true;
}

/*-------------------------------------------------------------*/

bool TFontView::SetDescription(BPoint where)
{
	if (!fText || !*fText) return SetDescription("", "");
	
	BFont font;
	GetFont(&font);
	
	escapement_delta delta;
	delta.nonspace = mSpacing;
	delta.space = 0;
	
	BRect rect_string;
	const char *text = fText;
	font.GetBoundingBoxesForStrings(&text, 1, B_SCREEN_METRIC, &delta, &rect_string);
	rect_string.OffsetBy(fWhere);
	
	if (!rect_string.Contains(where)) return SetDescription("", "");
	
	const float size = font.Size();
	int32 offset = 0;
	int32 count = 0;
	while (fText[offset] != 0) {
		count++;
		offset += UTF8_CHAR_LEN(((uchar)fText[offset]));
	}
	
	int32 i;
	BRect *rect_chars;
	BRect *sep_chars;
	BPoint *esc_chars;
	char *width_chars;
	rect_chars = (BRect*)malloc(sizeof(BRect)*count);
	sep_chars = (BRect*)malloc(sizeof(BRect)*count);
	esc_chars = (BPoint*)malloc(sizeof(BPoint)*count*2);
	width_chars = (char*)malloc(strlen(fText)+1);
	strcpy(width_chars, fText);
	
	font.GetBoundingBoxesAsGlyphs(fText, count, B_SCREEN_METRIC, sep_chars);
	font.GetBoundingBoxesAsString(fText, count, B_SCREEN_METRIC, &delta, rect_chars);
	font.GetEscapements(fText, count, &delta, esc_chars, esc_chars+count);
	
	int32 closest_index = -1;
	int32 closest_offset = -1;
	BPoint closest_pos;
	float closest_dist = 100000;
	BPoint pos(0, 0);
	for (i=0, offset=0; i<count; i++) {
		pos.x += esc_chars[i].x*size;
		pos.y += esc_chars[i].y*size;
	
		BRect r(rect_chars[i].OffsetByCopy(fWhere));
		if (r.Contains(where)) {
			closest_index = i;
			closest_offset = offset;
			closest_pos = pos;
			break;
		}
		
		BPoint center((r.left+r.right)/2, (r.top+r.bottom)/2);
		float dist = sqrt((where.x-center.x)*2 + (where.y-center.y)*2);
		if (closest_index < 0 || dist < closest_dist) {
			closest_index = i;
			closest_offset = offset;
			closest_pos = pos;
			closest_dist = dist;
		}
		
		offset += UTF8_CHAR_LEN(((uchar)fText[offset]));
	}
	
	const BRect bbox = rect_chars[closest_index];
	const BRect gbox = sep_chars[closest_index];
	const BPoint esc = esc_chars[closest_index];
	width_chars[closest_offset+UTF8_CHAR_LEN(((uchar)fText[closest_offset]))] = 0;
	const float width = font.StringWidth(width_chars);
	
	const int32 charLen = UTF8_CHAR_LEN((uchar)fText[closest_offset]);
	const uint32 charCode = UTF8_TO_UNICODE((const uchar*)fText+closest_offset);
	
	BStringIO d1;
	d1 << "Character " << (void*)(charCode) << " '";
	d1.Write(fText+closest_offset, charLen);
	d1 << "': BBox=(" << bbox.left << "," << bbox.top << ")-("
	  << bbox.right << "," << bbox.bottom << ")/("
	  << gbox.left << "," << gbox.top << ")-("
	  << gbox.right << "," << gbox.bottom << ")";
	BStringIO d2;
	d2 << "Esc=(" << esc.x
	  << "," << esc.y << "), SizeEsc=(" << (esc.x*size) << "," << (esc.y*size)
	  << "), Cursor=(" << pos.x << "," << pos.y << "), Width=" << width;
	
	free(width_chars);
	free(esc_chars);
	free(sep_chars);
	free(rect_chars);
	
	return SetDescription(d1.String(), d2.String());
}

/*-------------------------------------------------------------*/

TFontApp::TFontApp()
	: BApplication("application/x-vnd.Be-FDEM")
{
}

/*-------------------------------------------------------------*/

bool TFontApp::QuitRequested()
{
	// need to close windows in specific order
	if (ctrl_window->Lock())
		ctrl_window->Quit();
	if (font_window->Lock())
		font_window->Quit();

	return TRUE;
}

/*-------------------------------------------------------------*/

void TFontApp::AboutRequested()
{
	BAlert				*myAlert;

	myAlert = new BAlert("", "...a demo application", "Big Deal");
	myAlert->Go();
}

/*-------------------------------------------------------------*/

int
main(int argc, char* argv[])
{
	BApplication*		fontApp;

	fontApp = new TFontApp();

	font_window = new TMainWindow(mw_rect, B_TITLED_WINDOW,
									B_ASYNCHRONOUS_CONTROLS, "FontDemo");

	ctrl_window = new TControlWindow(font_window, cw_rect, B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 
					 B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_WILL_ACCEPT_FIRST_CLICK, 
					 "Controls");

	srand(system_time());

//	if (ctrl_window->Lock()) {
//		family_list->Select(rand() % numFamilies);
//	    update_style_list();
//		family_list->ScrollToSelection();
//		style_list->Select(rand() % numStyles);
//		style_list->ScrollToSelection();
//		family_list->Invalidate();
//		ctrl_window->Unlock();
//	}
	font_window->Show();
	ctrl_window->Show();

	fontApp->Run();

	delete fontApp;
	return(0);
}


TFontMenu::TFontMenu(
	BRect		frame,
	BLooper		*target)
		: BMenuField(frame, B_EMPTY_STRING, "Font:", new BMenu(B_EMPTY_STRING), true)
{
	fTarget = BMessenger(target);
	fSelectedFont = *be_plain_font;

	SetDivider(StringWidth(Label()) + 6.0);
}


void
TFontMenu::AttachedToWindow()
{
	BMenuField::AttachedToWindow();

	SetFlags(Flags() | B_PULSE_NEEDED);
	FillOutMenu();
}


void
TFontMenu::MessageReceived(
	BMessage	*message)
{
	switch (message->what) {
		case msg_FontFamilySelected:
			UnmarkAll();
			SetSelectedFont(message->FindString("family"));
			SendMessageToTarget();
			break;

		case msg_FontStyleSelected:
			UnmarkAll();
			SetSelectedFont(message->FindString("family"), message->FindString("style"));
			SendMessageToTarget();
			break;

		default:
			BMenuField::MessageReceived(message);
			break;
	}
}


void
TFontMenu::Pulse()
{
	if (!((TControlWindow *)Window())->fAutoCycle)
		return;

	BMenu *familyMenu = Menu();
	BMenuItem *markedItem = familyMenu->FindMarked();
	if (markedItem == NULL)
		return;

	int32 familyIndex = familyMenu->IndexOf(markedItem);
	familyIndex = (familyIndex < (familyMenu->CountItems() - 1)) ? familyIndex + 1 : 0;	

	BMenu *styleMenu = markedItem->Submenu();
	markedItem = styleMenu->FindMarked();
	if (markedItem == NULL)
		return;

	BMessage	*message = NULL;
	int32		styleIndex = styleMenu->IndexOf(markedItem);
	if (styleIndex < (styleMenu->CountItems() - 1))
		message = ((BMenuItem *)styleMenu->ItemAt(styleIndex + 1))->Message();
	else {
		BMenuItem *item = familyMenu->ItemAt(familyIndex);
		message = ((BMenuItem *)item->Submenu()->ItemAt(0))->Message();
	}

	UnmarkAll();
	SetSelectedFont(message->FindString("family"), message->FindString("style"));
	SendMessageToTarget();
}


void
TFontMenu::SetSelectedFont(
	const BFont	*font,
	bool		useStyle)
{
	BMenu		*menu = Menu();
	font_family	family = "";
	font_style 	style = "";
	
	font->GetFamilyAndStyle(&family, &style);

	int32 numItems = menu->CountItems();
	for (int32 i = 0; i < numItems; i++) {
		BMenuItem *item = menu->ItemAt(i);

		if (strcmp(item->Label(), family) != 0) 
			continue;

		item->SetMarked(true);

		BMenu	*styleMenu = item->Submenu();
		int32	numStyles = styleMenu->CountItems();
		for (int32 j = 0; j < numStyles; j++) {
			BMenuItem *styleItem = styleMenu->ItemAt(j);

			if ((!useStyle) || (strcmp(styleItem->Label(), style) == 0)) {
				styleItem->SetMarked(true);
				break;
			}
		}

		break;
	}

	char fullFontName[sizeof(font_family) + 1 + sizeof(font_style) + 1] = "";
	sprintf(fullFontName, "%s %s", family, style);
	MenuItem()->SetLabel(fullFontName);

	fSelectedFont = *font;
}


void
TFontMenu::SetSelectedFont(
	const font_family	family)
{
	BFont font;
	font.SetFamilyAndStyle(family, NULL);
	SetSelectedFont(&font, false);
}


void
TFontMenu::SetSelectedFont(
	const font_family	family,
	const font_style	style)
{
	BFont font;
	font.SetFamilyAndStyle(family, style);
	SetSelectedFont(&font, true);
}


void
TFontMenu::FillOutMenu()
{
	BMenu *menu = Menu();

	int32 numFamilies = count_font_families();
	for (int32 i = 0; i < numFamilies; i++) {
		font_family family = "";
		get_font_family(i, &family);

		BMenu	*styleMenu = new BMenu(family);
		styleMenu->SetRadioMode(true);
		int32	numStyles = count_font_styles(family);
		for (int32 j = 0; j < numStyles; j++) {
			font_style style = "";
			get_font_style(family, j, &style);

			BMessage *message = new BMessage(msg_FontStyleSelected);
			message->AddString("family", family);
			message->AddString("style", style);

			styleMenu->AddItem(new BMenuItem(style, message));
		}
		
		BMessage *message = new BMessage(msg_FontFamilySelected);
		message->AddString("family", family);

		menu->AddItem(new BMenuItem(styleMenu, message));
		styleMenu->SetTargetForItems(this);
	}

	menu->SetTargetForItems(this);
}


void
TFontMenu::UnmarkAll()
{
	BMenu *familyMenu = Menu();
	BMenuItem *markedItem = familyMenu->FindMarked();
	if (markedItem == NULL)
		return;

	markedItem->SetMarked(false);

	BMenu *styleMenu = markedItem->Submenu();
	markedItem = styleMenu->FindMarked();
	if (markedItem == NULL)
		return;

	markedItem->SetMarked(false);
}


void
TFontMenu::SendMessageToTarget()
{
	BMessage msg(NEW_FONT);
	msg.AddFlat("font", &fSelectedFont);
	fTarget.SendMessage(&msg);
}
