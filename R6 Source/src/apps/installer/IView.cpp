//******************************************************************************
//
//	File:			IView.cpp
//
//	Description:	Be Installer view.
//
//	Written by:		Steve Horowitz
//
//	Copyright 1994-95, Be Incorporated. All Rights Reserved.
//
//******************************************************************************

#include <stdlib.h>

#include <MenuItem.h>
#include <MenuField.h>
#include <Button.h>
#include <ScrollView.h>
#include <Resources.h>
#include <Roster.h>
#include <Debug.h>
#include <File.h>
#include <ScrollView.h>
#include <String.h>

#include "IApp.h"
#include "IView.h"
#include "IWindow.h"
#include "PaneSwitch.h"
#include "ProgressBar.h"

#ifdef OLD_LOGO
#include "oldlogo.h"
#else
#include "iconfile.h"
#endif // OLD_LOGO

//B_DEFINE_CLASS_INFO(TIView, BView);

#define MENU_WIDTH 160
#define MENU_HEIGHT 15

static const float TOP_MARGIN = 12.0;
static const float BOTTOM_MARGIN = 12.0;
static const float LEFT_MARGIN = 12.0;
static const float RIGHT_MARGIN = 12.0;

static const char *kNoDestVolumesString = "No destination volumes available.";

// --------------------------------------------------------------
TIView::TIView(BRect frame)
	:	BBox(frame, "IView", B_FOLLOW_ALL, B_WILL_DRAW |
											B_FRAME_EVENTS |
											B_FULL_UPDATE_ON_RESIZE),
		m_progress_bar(NULL),
		m_bar_visible(false),
		m_expanded(false),
		fBarberPole(NULL),
		fBarberPoleVisible(false)
{
}

// --------------------------------------------------------------
TIView::~TIView()
{
	delete(fLogoBits);
}


// --------------------------------------------------------------
void TIView::AttachedToWindow()
{
	app_info		info;
	BRect			rect;
	BButton*		button;
//	TIWindow *win(dynamic_cast<TIWindow *>(Window()));

	BRect bounds = Bounds();
	SetHighColor(216, 216, 216);

	rect.Set(0, 0, kLogoBitmapWidth - 1, kLogoBitmapHeight - 1);
	fLogoBits = new BBitmap(rect, kLogoBitmapColorSpace);
	fLogoBits->SetBits(kLogoBitmapBits, sizeof(kLogoBitmapBits), 0, kLogoBitmapColorSpace);
	
	rect.OffsetBy(LEFT_MARGIN, TOP_MARGIN);
	rect.left = rect.right + 11;
	rect.right = bounds.right - RIGHT_MARGIN;
		
	rect.bottom = rect.top + 50;
	rect.InsetBy(2, 2);
	BRect tmpRect(0, 0, rect.Width(), rect.Height());
	tmpRect.InsetBy(2, 2);
	fTEView = new BTextView(rect, "TextView",  tmpRect, B_FOLLOW_NONE, B_WILL_DRAW);
	AddChild(new BScrollView("", fTEView, 0, 0));
	fTEView->MakeEditable(false);
	fTEView->MakeSelectable(false);
	fTEView->SetWordWrap(true);
	rect.InsetBy(-2, -2);
	
	rect.top = rect.bottom - 12.0;
	m_progress_bar = new ProgressBar(rect, "");
	m_progress_bar->Hide();
	AddChild(m_progress_bar);

	BRect barberPoleRect(0, 0, kBarberPoleBitmapWidth - 1, kBarberPoleBitmapHeight - 1);
	barberPoleRect.InsetBy(-2, -2);
	barberPoleRect.OffsetTo(rect.LeftTop());
	barberPoleRect.OffsetBy(2, 0);
	fBarberPole = new BarberPoleView(barberPoleRect, "barberPole");
	fBarberPole->Hide();
	AddChild(fBarberPole);
	

	float alignmentOffset = kLogoBitmapWidth + 9;
	// add two menus for volume lists
	fVolumeMenu1 = new BPopUpMenu("Volumes1");
//	rect.Set(115, 120, 110 + MENU_WIDTH, 120 + MENU_HEIGHT);
	rect.left = bounds.left + LEFT_MARGIN;
	rect.right = bounds.right - RIGHT_MARGIN;
	rect.top = rect.bottom + 8;
	rect.bottom = rect.top + MENU_HEIGHT;
	fMenuField1 = new BMenuField(rect, "source", "Install from:", fVolumeMenu1);
	fMenuField1->SetDivider(alignmentOffset);
	fMenuField1->SetAlignment(B_ALIGN_RIGHT);
	fMenuField1->Hide();
	AddChild(fMenuField1);
	
	BRect oldRect = rect;
	// hack to properly align the "Install from:" text with the menu fields
	rect.left += alignmentOffset - be_plain_font->StringWidth("Install from:") - 5;
	rect.bottom += 3;
	fFromText = new BStringView(rect, "sourceText", "Install from: scanning...");
	fFromText->SetAlignment(B_ALIGN_LEFT);
	AddChild(fFromText);
	
	fVolumeMenu2 = new BPopUpMenu("scanning...");
	rect = oldRect;
	rect.top = rect.bottom + 8;
	rect.bottom = rect.top + MENU_HEIGHT;
	fMenuField2 = new BMenuField(rect, "destination", "Onto:", fVolumeMenu2);
	fMenuField2->SetDivider(alignmentOffset);
	fMenuField2->SetAlignment(B_ALIGN_RIGHT);
	AddChild(fMenuField2);

	rect.top = rect.bottom + 20;
	rect.left = bounds.left + LEFT_MARGIN;
	rect.right = rect.left + 12;
	rect.bottom = rect.top + 12;
		
	IPaneSwitch *opt_switch = new IPaneSwitch(rect, "Options Switch");
	opt_switch->SetMessage(new BMessage(OPTIONS_TOGGLE_SWITCH));
	AddChild(opt_switch);
	
	// create and place the more options label
	rect.left = rect.right + 5;
	rect.right = bounds.left + (bounds.Width() / 2) - 20;
	mOptionToggleLabel = new BStringView(rect, "optionToggleLabel", "More Options",
										 B_FOLLOW_LEFT | B_FOLLOW_TOP);
	AddChild(mOptionToggleLabel);
	
	// calculate option area
	rect.left = bounds.left + LEFT_MARGIN;
	rect.right = bounds.right - RIGHT_MARGIN - B_V_SCROLL_BAR_WIDTH - 3;
	rect.top = rect.bottom + 10.0;
	rect.bottom = rect.top + 96.0;
	
	mOptionView = new OptionView(rect);
	mOptionScrollView = new BScrollView("optionScrollView", mOptionView,
		B_FOLLOW_TOP | B_FOLLOW_LEFT, B_NAVIGABLE_JUMP, false, true); 
	mOptionView->ArrangeOptions();
	mOptionScrollView->Hide();
	AddChild(mOptionScrollView);

	// create and place the space label
	rect.left = bounds.left + LEFT_MARGIN;
	rect.right = bounds.right - RIGHT_MARGIN;
	rect.top = rect.bottom + 8;
	rect.bottom  = rect.top + 12;
	mSpaceLabel = new BStringView(rect, "spaceLabel", "");
	mSpaceLabel->SetAlignment(B_ALIGN_RIGHT);
	mSpaceLabel->Hide();
	AddChild(mSpaceLabel);
	
//	fMenuField1->MoveTo(rect.left, rect.bottom + 8);
//	fMenuField1->Hide();
//	AddChild(fMenuField1);
	
	// Position the buttons relative to the bottom ---------------------------------
	rect = bounds;
	rect.InsetBy(11, 15);
	rect.SetLeftTop(rect.RightBottom() - BPoint(80, 20));
	button = new BButton(rect, "Button", "Begin", new BMessage('BUTN'), B_FOLLOW_BOTTOM);
	Window()->SetDefaultButton(button);

	rect.OffsetTo(11.0, rect.top);
	rect.right += 20; 
	mDriveSetupButton = new BButton(rect, "driveSetup", "Setup partitions"B_UTF8_ELLIPSIS,
						 new BMessage(LAUNCH_DRIVE_SETUP), B_FOLLOW_BOTTOM);
	mDriveSetupButton->Hide();
	AddChild(mDriveSetupButton);

	AddChild(button); // add last so it's last in the keyboard nav list
}


// --------------------------------------------------------------
void TIView::Draw(BRect rect)
{
	inherited::Draw(rect);
	DrawBitmap(fLogoBits, BPoint(12, 12));
}

// --------------------------------------------------------------
void TIView::SetSizeBarVisible(bool on, bool barberPole)
{
	if ((!barberPole && m_bar_visible == on)
		|| barberPole && fBarberPoleVisible == on)
		return;
	
	if (!Window()->Lock())
		return;

	if (barberPole && !on && fBarberPoleVisible) {
		fTEView->ResizeBy(0, fBarberPole->Bounds().Height() + 3.0);
		fTEView->Parent()->ResizeBy(0, fBarberPole->Bounds().Height() + 3.0);
		fBarberPole->Hide();
		fBarberPoleVisible = false;
	}
	if (!barberPole && !on && m_bar_visible) {
		fTEView->ResizeBy(0, m_progress_bar->Bounds().Height() + 3.0);
		fTEView->Parent()->ResizeBy(0, m_progress_bar->Bounds().Height() + 3.0);
		m_progress_bar->Hide();
		m_bar_visible = false;
	}
	if (!barberPole && on && !m_bar_visible) {
		fTEView->ResizeBy(0, -(m_progress_bar->Bounds().Height() + 3.0));
		fTEView->Parent()->ResizeBy(0, -(m_progress_bar->Bounds().Height() + 3.0));
		m_progress_bar->Update(-m_progress_bar->CurrentValue());
		m_progress_bar->Show();
		m_bar_visible = true;
	}
	if (barberPole && on && !fBarberPoleVisible) {
		fTEView->ResizeBy(0, -(fBarberPole->Bounds().Height() + 3.0));
		fTEView->Parent()->ResizeBy(0, -(fBarberPole->Bounds().Height() + 3.0));
		fBarberPole->Show();
		fBarberPoleVisible = true;
	}

	Sync();
	Invalidate();
	Window()->UpdateIfNeeded();
	Window()->Unlock();
}

// --------------------------------------------------------------
void TIView::SetSizeBarMaxValue()
{
	TIWindow* win;
	
	if ((win = static_cast<TIWindow *>(Window())) == NULL
		|| win->InstallFileSize() <= 0
		|| win->InstallNum() <= 0)
	{
		return;
	}
	if (m_progress_bar != NULL) {
		m_progress_bar->SetMaxValue(win->InstallFileSize() + win->InstallAttrSize()
			+ mOptionView->CalculateSelectedSize());
	}
}

// called when the options toggle switch is flipped
void TIView::ToggleOptionsExpanded()
{
	if ((Window() != NULL) && (Window()->Lock())) {
		m_expanded = !m_expanded;
		if (m_expanded) {
			UpdateSpaceLabel();
			mDriveSetupButton->Show();
			mOptionScrollView->Show();
//			fMenuField1->Show();
			mSpaceLabel->Show();
			mOptionToggleLabel->SetText("Fewer options");
			mOptionToggleLabel->Invalidate();
		} else {
			mDriveSetupButton->Hide();
			mOptionScrollView->Hide();
//			fMenuField1->Hide();
			mSpaceLabel->Hide();
			mOptionToggleLabel->SetText("More options");
			mOptionToggleLabel->Invalidate();
		}
		Window()->Unlock();
	}
}

bool TIView::IsExpanded()
{
	return m_expanded;
}

static const char *kReqSpaceStr = "Disk space required: %s";
 
void TIView::UpdateSpaceLabel()
{
	char buf[32];
	off_t size(mOptionView->CalculateSelectedSize());
	TIWindow *window = dynamic_cast<TIWindow *>(Window());
	size += window->InstallFileSize() + window->InstallAttrSize();
	convert_size_to_string(size, buf);
	char *new_label = (char *)malloc(strlen(kReqSpaceStr) + strlen(buf));
	sprintf(new_label, kReqSpaceStr, buf);
	mSpaceLabel->SetText(new_label);
}

char *TIView::BuildDefaultText()
{
	BString buffer("Press the Begin button to install from '");
	BMenuItem *item = fVolumeMenu1->FindMarked();
	buffer << ((item != NULL) ? item->Label() : "") << "' onto '";
	item = fVolumeMenu2->FindMarked();
	if (item != NULL) {
		BMessage *msg = item->Message();
		if (msg != NULL) {
			char *dest_vol_name;
			msg->FindString("volume_name", (const char**)&dest_vol_name);
			buffer << dest_vol_name << "'";
		} else {
			// no destination volumes available
			buffer.SetTo(kNoDestVolumesString);
		}
	}
	return strdup(buffer.String());
}

void TIView::ShowSourceVolumeAsText(bool yes)
{
	if (yes) {
		fMenuField1->Hide();
		fFromText->Show();
	} else {
		fFromText->Hide();
		fMenuField1->Show();
	}
}


