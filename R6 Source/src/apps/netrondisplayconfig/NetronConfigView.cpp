#include "NetronConfigView.h"
#include <Application.h>	// for be_app definition
#include <stdio.h>
#include <StringView.h>
#include <Input.h>
#include <Button.h>
#include <BitmapButton.h>
#include <List.h>
#include <Region.h> 
#include <TranslatorFormats.h>
#include <MessageRunner.h>
using namespace BExperimental;

#define SLIDER_END_SPACE	2.0f

static const float kHeight = 32.0;
static const float kPadding = 8.0;

enum BUTTONS {
	kApplyButton,
	kBasicButton,
	kCancelButton,
	kDegaussButton,
	kDefaultBasicButton,
	kDefaultGeometryButton,
	kOtherButton,
};

static char* kButtons[] = {
	"fApplyButton",
	"fBasicButton",
	"fCancelButton",
	"fDegaussButton",
	"fDefaultBasicButton",
	"fDefaultGeometryButton",
	"fOtherButton",
	NULL
};

static const int kLabelSecondPageStart = 7;
	
static char *kLabels[] = {
	// On the first page
	"Contrast",             //  0
	"Brightness",           //  1
	"H Center",             //  2
	"H Size",               //  3
	"V Center",             //  4
	"V Size",               //  5
	"Rotation",             //  6
	
	// On the second page
	
	"V Pincushion",         //  7
	"V Pin Balance",        //  8
	"V Keystone",           //  9
	"V Key Balance",        // 10
	NULL
};

static char *kImageNames[11][2] = {
	{"ContrastDown.png", "ContrastUp.png"},
	{"BrightnessDown.png", "BrightnessUp.png"},
	{"HCenterLeft.png", "HCenterRight.png"},
	{"HSizeSquish.png", "HSizeExpand.png"},
	{"VCenterDown.png", "VCenterUp.png"},
	{"VSizeSquish.png", "VSizeExpand.png"},
	{"RotateLeft.png", "RotateRight.png"},
	{"VPincushionPinch.png", "VPincushionBulge.png"},
	{"VPinBalanceDown.png", "VPinBalanceUp.png"},
	{"VKeystoneLeft.png", "VKeystoneRight.png"},
	{"VKeyBalanceLeft.png", "VKeyBalanceRight.png"}
};

static char *kDecrementImageName		= "decrement_up.png";
static char *kDecrementOverImageName	= "decrement_over.png";
static char *kDecrementDownImageName	= "decrement_down.png";
static char *kIncrementImageName		= "increment_up.png";
static char *kIncrementOverImageName	= "increment_over.png";
static char *kIncrementDownImageName	= "increment_down.png";

NetronConfigView::NetronConfigView(BRect frame)
	:	BView(frame, "netronconfigview", B_FOLLOW_NONE, B_WILL_DRAW),
		fDisplay(NULL),
		fDecrementBitmap(NULL),
		fDecrementOverBitmap(NULL),
		fDecrementDownBitmap(NULL),
		fIncrementBitmap(NULL),
		fIncrementOverBitmap(NULL),
		fIncrementDownBitmap(NULL),
		fDisplayMode(display_basic)
{
	fDisplay = new NetronDisplay();
	fResources.AddEnvDirectory("$RESOURCES/AppBitmaps/NetronDisplayConfig", "/boot/custom/resources/en/AppBitmaps/NetronDisplayConfig");

	fDecrementBitmap = fResources.FindBitmap(B_PNG_FORMAT, kDecrementImageName);
	fDecrementOverBitmap = fResources.FindBitmap(B_PNG_FORMAT, kDecrementOverImageName);
	fDecrementDownBitmap = fResources.FindBitmap(B_PNG_FORMAT, kDecrementDownImageName);
	fIncrementBitmap = fResources.FindBitmap(B_PNG_FORMAT, kIncrementImageName);
	fIncrementOverBitmap = fResources.FindBitmap(B_PNG_FORMAT, kIncrementOverImageName);
	fIncrementDownBitmap = fResources.FindBitmap(B_PNG_FORMAT, kIncrementDownImageName);
	
	if(fDisplay->InitCheck() != B_OK) {
		// should do clean up and do some sort of graceful exit...
		delete fDisplay;
		fDisplay = NULL;
		printf("NetronDisplay INITCHECK() failed!\n");
	}
	
	BRect   labelRect(10, 4, 
	         be_bold_font->StringWidth("V Key Balance") + 10,
	         kHeight + kPadding);
	BPoint  degauss;       // where degauss button goes
	BRect   scrollRect(labelRect);
	BRect   sliderRect;    // For button calculations
	BRect   bounds(Bounds());
	int32   idx = 0;
	
	SetViewColor(255,255,255);
	
	scrollRect.left = scrollRect.right + 10;
	scrollRect.right = bounds.right - 10;
	
	sliderRect.left = scrollRect.left;
	sliderRect.top = scrollRect.top;
	sliderRect.right = scrollRect.right;
	
	while(kLabels[idx] != NULL) {
		if(idx == kLabelSecondPageStart) {
			// At end of sliders for first page, so start over
			
			labelRect.top = 4;
			labelRect.bottom = kHeight + kPadding;
		}
		
		BString name("sv_");
		name << kLabels[idx];
		
		BStringView *sview;
		labelRect.bottom -= 8; 		// Account for font size
		AddChild(sview = new BStringView(labelRect, name.String(), kLabels[idx]));
		sview->SetFont(be_bold_font);
		labelRect.bottom += 8;	 		// Further font size accounting
		
		scrollRect.top = labelRect.top;
		scrollRect.bottom = labelRect.bottom;
		
		if (scrollRect.bottom > sliderRect.bottom)
		  sliderRect.bottom = scrollRect.bottom;
		
		AddChild(new NetronSlider(scrollRect, kLabels[idx], fDisplay,
		(register_control)(contrast_control + idx),
		fResources.FindBitmap(B_PNG_FORMAT, kImageNames[idx][0]),
		fResources.FindBitmap(B_PNG_FORMAT, kImageNames[idx][1]),
		fDecrementBitmap, fDecrementOverBitmap, fDecrementDownBitmap,
		fIncrementBitmap, fIncrementOverBitmap, fIncrementDownBitmap));
				
		labelRect.top = labelRect.bottom + kPadding;
		labelRect.bottom = labelRect.top + kHeight;
		idx++;
	}
	
	degauss.x = labelRect.left;
	degauss.y = labelRect.top + kPadding;
	
	// Hide geometry controls
	
	for(int32 i = 7; i < kSliderCount; i++) {
		BString viewName("sv_");
		viewName << kLabels[i];
		// BStringView
		BView *view = FindView(viewName.String());
		if(view) view->Hide();
		// Netron Slider View
		view = FindView(kLabels[i]);
		if(view) view->Hide();
	}
	
	CreateButtons(degauss, sliderRect);	
}


NetronConfigView::~NetronConfigView()
{
	delete fDisplay;
	// Resource class handles bitmap deleting
}

void NetronConfigView::AttachedToWindow()
{
	BButton*  button;
	int32     i;
	
	BView::AttachedToWindow();
	
	for (i = 0; kButtons[i]; i++)
	  {
	    button = dynamic_cast<BButton*>(FindView(kButtons[i]));
	    if (button) button->SetTarget(this);
	  }
	  
	// save the starting values so that they can be restored when the
	// user presses the Cancel button
	SaveValues();
	
	// Update the position of the sliders to match the current settings
	// in the MCU
	RefreshSliders(true, false);
}

void NetronConfigView::CreateButtons(BPoint degauss,
 BRect sliderRect)
{
	const BBitmap*  bDown;
	const BBitmap*  bOver;
	const BBitmap*  bUp;
	
	BBitmapButton*  apply;
	BBitmapButton*  cancel;
	
	BRect           applyRect;
	BRect           cancelRect;
	BRect           defaultsRect;
	BRect           degaussRect;
	BRect           modeRect;
	
	
	//*
	//***  NULL all pointers in case of errors
	//*
	
	fDegaussButton = NULL;
	fDefaultBasicButton = NULL;
	fDefaultGeometryButton = NULL;
	fBasicButton = NULL;
	fOtherButton = NULL;
	
	
	//*
	//***  Degauss button
	//*
			
	bOver = fResources.FindBitmap(B_PNG_FORMAT, "degauss_over.png");
	bDown = fResources.FindBitmap(B_PNG_FORMAT, "degauss_down.png");
	bUp = fResources.FindBitmap(B_PNG_FORMAT, "degauss_up.png");
			
    degaussRect.left = degauss.x;
    degaussRect.top = degauss.y;
		
	degaussRect.right = degaussRect.left + bUp->Bounds().Width();
	degaussRect.bottom = degaussRect.top + bUp->Bounds().Height();
			
	fDegaussButton = new BBitmapButton(degaussRect, kButtons[kDegaussButton],
	 NULL, new BMessage(M_DEGAUSS), bUp, bOver, bDown);
			 
	AddChild(fDegaussButton);

	
	//*		
	//***  Basic defaults button
	//*
	
	defaultsRect.left = sliderRect.left;
	defaultsRect.top = sliderRect.bottom += 70;
	
	bOver = fResources.FindBitmap(B_PNG_FORMAT, "default_over.png");
	bDown = fResources.FindBitmap(B_PNG_FORMAT, "default_down.png");
	bUp = fResources.FindBitmap(B_PNG_FORMAT, "default_up.png");
	
	defaultsRect.right = defaultsRect.left + bUp->Bounds().Width();
	defaultsRect.bottom = defaultsRect.top + bUp->Bounds().Height();
			
	fDefaultBasicButton = new BBitmapButton(defaultsRect,
	 kButtons[kDefaultBasicButton], NULL,
	 new BMessage(M_LOAD_BASIC_DEFAULTS),
	 bUp, bOver, bDown);
	 
	AddChild(fDefaultBasicButton);

	
	//*
	//***  Geometry defaults button
	//*
	
	bOver = fResources.FindBitmap(B_PNG_FORMAT, "default_over.png");
	bDown = fResources.FindBitmap(B_PNG_FORMAT, "default_down.png");
	bUp = fResources.FindBitmap(B_PNG_FORMAT, "default_up.png");
	
	fDefaultGeometryButton = new BBitmapButton(defaultsRect,
	 kButtons[kDefaultGeometryButton], NULL,
	 new BMessage(M_LOAD_GEOMETRY_DEFAULTS),
	 bUp, bOver, bDown);
	 
	AddChild(fDefaultGeometryButton);

	
	//*
	//***  Apply button
	//*
	
	applyRect.top = defaultsRect.top;
	applyRect.right = sliderRect.right;
	
	bOver = fResources.FindBitmap(B_PNG_FORMAT, "apply_over.png");
	bDown = fResources.FindBitmap(B_PNG_FORMAT, "apply_down.png");
	bUp = fResources.FindBitmap(B_PNG_FORMAT, "apply_up.png");
	
	applyRect.left = applyRect.right - bUp->Bounds().Width();
	applyRect.bottom = applyRect.top + bUp->Bounds().Height();
	
	apply = new BBitmapButton(applyRect,
	 kButtons[kApplyButton], NULL, new BMessage(M_APPLY),
	 bUp, bOver, bDown);
	 
	AddChild(apply);

    
	//*
	//***  Cancel button
	//*
	
	cancelRect.right = applyRect.left - 8;
	cancelRect.top = applyRect.top;
	
	bOver = fResources.FindBitmap(B_PNG_FORMAT, "cancel_over.png");
	bDown = fResources.FindBitmap(B_PNG_FORMAT, "cancel_down.png");
	bUp = fResources.FindBitmap(B_PNG_FORMAT, "cancel_up.png");
	
	cancelRect.left = cancelRect.right - bUp->Bounds().Width();
	cancelRect.bottom = cancelRect.top + bUp->Bounds().Height();
			
	cancel = new BBitmapButton(cancelRect,
	 kButtons[kCancelButton], NULL, new BMessage(M_CANCEL),
	 bUp, bOver, bDown);
	 
	AddChild(cancel);
	
	
	//*
	//***  "Basic Settings" button
	//*

	modeRect.top = applyRect.top - 60;
	modeRect.left = applyRect.left;
	
	bOver = fResources.FindBitmap(B_PNG_FORMAT, "basic_over.png");
	bDown = fResources.FindBitmap(B_PNG_FORMAT, "basic_down.png");
	bUp = fResources.FindBitmap(B_PNG_FORMAT, "basic_up.png");
	
	modeRect.right = modeRect.left + bUp->Bounds().Width();
	modeRect.bottom = modeRect.top + bUp->Bounds().Height();
			
	fBasicButton = new BBitmapButton(modeRect,
	 kButtons[kBasicButton], NULL, new BMessage(M_PAGE),
	 bUp, bOver, bDown);
	 
	AddChild(fBasicButton);

    
	//*
	//***  "Other Settings" button
	//*

	bOver = fResources.FindBitmap(B_PNG_FORMAT, "other_over.png");
	bDown = fResources.FindBitmap(B_PNG_FORMAT, "other_down.png");
	bUp = fResources.FindBitmap(B_PNG_FORMAT, "other_up.png");
	
	modeRect.right = modeRect.left + bUp->Bounds().Width();
	modeRect.bottom = modeRect.top + bUp->Bounds().Height();
			
	fOtherButton = new BBitmapButton(modeRect,
	 kButtons[kOtherButton], NULL, new BMessage(M_PAGE),
	 bUp, bOver, bDown);
	 
	AddChild(fOtherButton);


	//*
	//***  Hide buttons that are only on the second page
	//*

	fDefaultGeometryButton->Hide();
	fBasicButton->Hide();
	fDegaussButton->Hide();
}

void NetronConfigView::MessageReceived(BMessage *inMessage)
{
	switch(inMessage->what) {
		case M_LOAD_BASIC_DEFAULTS:
		case M_LOAD_GEOMETRY_DEFAULTS: {
			//printf("Load Defaults\n");
			bool basic = inMessage->what == M_LOAD_BASIC_DEFAULTS;
			bool geometry = inMessage->what == M_LOAD_GEOMETRY_DEFAULTS;
			if(fDisplay)
				fDisplay->RecallFactorySettings(basic, geometry);
			RefreshSliders(basic, geometry, true);
			break;
		}
		case M_DEGAUSS: {
			//printf("M_DEGAUSS\n");
			if(fDisplay)
				if(fDisplay->ActivateDegauss() != B_NO_ERROR)
					printf("Error while trying to degauss\n");
			break;
		}
		case M_PAGE: {
			//printf("M_PAGE\n");
			ToggleDisplayMode();
			break;
		}
		case M_CANCEL: {
			// revert settings and then quit
			RestoreValues();
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case M_APPLY: {
			// save settings and then quit
			if (fDisplay && (fDisplay->SaveSettings() != B_OK)) {
				printf("Error while trying to save settings\n");
				return;
			}
			be_app->PostMessage(B_QUIT_REQUESTED);
			break;
		}
		default:
			BView::MessageReceived(inMessage);
			break;
	}
}

void NetronConfigView::Draw(BRect rect)
{
	(void)rect;
	
	//SetHighColor(0,0,0);
	//StrokeRect(Bounds());
}

void NetronConfigView::ToggleDisplayMode()
{
	if(fDisplayMode == display_basic) {
		// Switch to geometry mode
		RefreshSliders(false, true);
		for(int32 i = 0; i < 7; i++) {
			BString viewName("sv_");
			viewName << kLabels[i];
			// BStringView
			BView *view = FindView(viewName.String());
			if(view) view->Hide();
			// Netron Slider View
			view = FindView(kLabels[i]);
			if(view) view->Hide();
		}
		for(int32 i = 7; i < kSliderCount; i++) {
			BString viewName("sv_");
			viewName << kLabels[i];
			// BStringView
			BView *view = FindView(viewName.String());
			if(view) view->Show();
			// Netron Slider View
			view = FindView(kLabels[i]);
			if(view) view->Show();
		}
		
		fDefaultBasicButton->Hide();
		fDefaultGeometryButton->Show();
		
		fOtherButton->Hide();
		fDegaussButton->Show();
		
		fBasicButton->Show();
		
		fDisplayMode = display_geometry;
		
	} else if(fDisplayMode == display_geometry) {
		// Switch to basic mode
		RefreshSliders(true, false);
		for(int32 i = 0; i < 7; i++) {
			BString viewName("sv_");
			viewName << kLabels[i];
			// BStringView
			BView *view = FindView(viewName.String());
			if(view) view->Show();
			// Netron Slider View
			view = FindView(kLabels[i]);
			if(view) view->Show();
		}
		for(int32 i = 7; i < kSliderCount; i++) {
			BString viewName("sv_");
			viewName << kLabels[i];
			// BStringView
			BView *view = FindView(viewName.String());
			if(view) view->Hide();
			// Netron Slider View
			view = FindView(kLabels[i]);
			if(view) view->Hide();
		}
		
		fDefaultGeometryButton->Hide();
		fDefaultBasicButton->Show();
		
		fBasicButton->Hide();
		fOtherButton->Show();
		
		fDegaussButton->Hide();
		
		fDisplayMode = display_basic;
	}
}

void 
NetronConfigView::RefreshSliders(bool basic, bool geometry, bool factorySetting)
{
	int32 start = basic ? 0 : 7;
	int32 end = geometry ? kSliderCount : 7;
	for (int32 i = start; i < end; i++) {
		NetronSlider *slider = dynamic_cast<NetronSlider *>(FindView(kLabels[i]));
		if(slider) {
			slider->Refresh(factorySetting);
		}
	}
}

// Retrieves the current settings from the MCU and saves them so that
// they can be restored later, if necessary
void
NetronConfigView::SaveValues()
{
	status_t err = B_ERROR;

	if (!fDisplay)
		return;

	for (int i = 0; i < (int)kNumRegisters; i++) {
		fOrigValues[i] = 0;
		err = fDisplay->GetRegister(kRegisters[i], &(fOrigValues[i]));
		if (err != B_OK) {
			printf("GetRegister(0x%02X) returned error code %ld\n", kRegisters[i], err);
		}
	}
}

// Writes the settings back to the MCU, restoring the state as of when
// SaveValues() was called
void
NetronConfigView::RestoreValues()
{
	status_t err = B_ERROR;

	if (!fDisplay)
		return;

	for (int i = 0; i < (int)kNumRegisters; i++) {
		err = fDisplay->SetRegister(kRegisters[i], fOrigValues[i]);
		if (err != B_OK) {
			printf("SetRegister(0x%02X, %d) returned error code %ld\n", kRegisters[i], fOrigValues[i], err);
		}
	}
}
		
NetronSlider::NetronSlider(BRect frame, const char *name, NetronDisplay *display,
							register_control rc, const BBitmap *leftBitmap, const BBitmap *rightBitmap,
							const BBitmap *dec, const BBitmap *decOver, const BBitmap *decDown,
							const BBitmap *inc, const BBitmap *incOver, const BBitmap *incDown)
	:	BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW),
		fControl(rc),
		fDisplay(display),
		fMinValue(0),
		fMaxValue(0),
		fStepSize(1),
		fCurrentValue(0),
		fSavedValue(-1),
		fMouseDown(false),
		fDelta(0.0f),
		fPos(0.0f),
		fCurrentAdjustment(0),
		fAdjustmentRunner(NULL),
		
		fLeftBitmap(leftBitmap),
		fRightBitmap(rightBitmap),
		fDecrementBitmap(dec),
		fDecrementOverBitmap(decOver),
		fDecrementDownBitmap(decDown),
		fIncrementBitmap(inc),
		fIncrementOverBitmap(incOver),
		fIncrementDownBitmap(incDown),
		fState(NORMAL_STATE)
{
	float iconWidth = 36.0f;
	float iconHeight = 36.0f;
	float buttonWidth = 16.0f;
	float buttonHeight = 16.0f;
	BRect frm;
	
	if (leftBitmap) {
		frm = leftBitmap->Bounds();
		iconWidth = frm.Width() + 4.0f;
		iconHeight = frm.Height() + 4.0f;
	}

	if (dec) {
		frm = dec->Bounds();
		buttonWidth = frm.Width();
		buttonHeight = frm.Height();
	}

 	fLeftRect.Set(iconWidth, 4.0f, iconWidth + buttonWidth, 4.0f + buttonHeight);
 	fRightRect.Set(frame.Width() - (iconWidth + buttonWidth), 4.0f, frame.Width() - iconWidth, 4.0f + buttonHeight);
	fSliderRect.Set(fLeftRect.right + 2.0f, 12.0f, fRightRect.left - 2.0f, 20.0f);
	SetViewColor(B_TRANSPARENT_COLOR);
	SetLowColor(255, 255, 255);

	// make sure that fPos starts out with a valid value
	UpdatePosition();
}


NetronSlider::~NetronSlider()
{
	// Resource class handles bitmap deleting
}

void NetronSlider::AttachedToWindow()
{
	BView::AttachedToWindow();

	uint8 factorySetting = 0;
	// Set the default values:
	switch(fControl) {
		case contrast_control:
			SetRange(50, 127);
			break;
		case brightness_control:
			SetRange(44, 150);
			break;
		case h_center_control:
			if(fDisplay) fDisplay->GetFactorySetting(kRegisters[h_center_control], &factorySetting);
			SetRange((int32)factorySetting - 32, (int32)factorySetting + 32,
			         0, 127);
			break;
		case h_size_control:
			if(fDisplay) fDisplay->GetFactorySetting(kRegisters[h_size_control], &factorySetting);
			SetRange((int32)factorySetting - 15, (int32)factorySetting + 15,
			         0, 127);
			break;
		case v_center_control:
			if(fDisplay) fDisplay->GetFactorySetting(kRegisters[v_center_control], &factorySetting);
			SetRange((int32)factorySetting - 32, (int32)factorySetting + 32);
			SetStepSize(2);
			break;
		case v_size_control:
			if(fDisplay) fDisplay->GetFactorySetting(kRegisters[v_size_control], &factorySetting);
			SetRange((int32)factorySetting - 40, (int32)factorySetting + 40);
			SetStepSize(2);
			break;
		case rotation_control:
			SetRange(0, 255);
			SetStepSize(2);
			break;
		case v_pincushion_control:
			SetRange(0, 127);
			break;
		case v_pin_balance_control:
			SetRange(0, 127);
			break;
		case v_keystone_control:
			SetRange(0, 127);
			break;
		case v_key_balance_control:
			SetRange(0, 127);
			break;
		default:
			break;
	}
}

void NetronSlider::Draw(BRect updateRect)
{
	BRegion fill;
	fill.Set(Bounds());
	fill.Exclude(fSliderRect);
	//FillRegion(&fill, B_SOLID_LOW);
	// Workaround for a app_server bug in FillRegion
	for (int32 i = 0; i < fill.CountRects(); i++) {
		BRect r = fill.RectAt(i);
		FillRect(r, B_SOLID_LOW);
	}

	const BBitmap *dec(fDecrementBitmap);
	const BBitmap *inc(fIncrementBitmap);

	switch (fState) {
		case LEFT_OVER_STATE:
			dec = fDecrementOverBitmap;
			break;

		case RIGHT_OVER_STATE:
			inc = fIncrementOverBitmap;
			break;

		case LEFT_DOWN_STATE:
			dec = fDecrementDownBitmap;
			break;

		case RIGHT_DOWN_STATE:
			inc = fIncrementDownBitmap;
			break;

		case NORMAL_STATE:	// fall through
		default:
			// do nothing
			break;
	}
	
	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	if(fLeftBitmap)
		DrawBitmap(fLeftBitmap, B_ORIGIN);
	if(dec)
		DrawBitmap(dec, fLeftRect.LeftTop());
	if(inc)
		DrawBitmap(inc, fRightRect.LeftTop());
		
	if(fRightBitmap)
		DrawBitmap(fRightBitmap, BPoint(fRightRect.right + 4, 0));

	SetDrawingMode(B_OP_COPY);

	if (updateRect.Intersects(fSliderRect)) {
		SetHighColor(0,0,0);
		FillRect(fSliderRect);
	
		// draw the current position as a two-pixel wide blue line
		
//		SetHighColor(255,255,255);
		SetHighColor(56,125,249);
		StrokeLine(BPoint(fPos, fSliderRect.top), BPoint(fPos, fSliderRect.bottom));
		float pos;
		if ((fPos - fSliderRect.left) > (fSliderRect.Width() / 2.0f)) {
			pos = fPos - 1.0f;
		} else {
			pos = fPos + 1.0f;
		}
		StrokeLine(BPoint(pos, fSliderRect.top), BPoint(pos, fSliderRect.bottom));		
	}
}

void NetronSlider::MessageReceived(BMessage *inMessage)
{
	switch(inMessage->what) {
		case M_REPEAT_ADJUST_START: {
			if(fAdjustmentRunner != NULL) {
				delete fAdjustmentRunner;
				fAdjustmentRunner = new BMessageRunner(BMessenger(this),
				                     new BMessage(M_REPEAT_ADJUST), 150000, -1);
				PerformAdjustment();
			}
			break;
		}
		case M_REPEAT_ADJUST: {
			if(fAdjustmentRunner != NULL) {
				PerformAdjustment();
			}
			break;
		}
		default:
			BView::MessageReceived(inMessage);
			break;
	}
}

void NetronSlider::MouseDown(BPoint pt)
{
	fMouseDown = true;
	SetMouseEventMask(B_POINTER_EVENTS);
	fAdjustmentRunner = new BMessageRunner(BMessenger(this),
	                           new BMessage(M_REPEAT_ADJUST_START), 600000, 1);
	if(fLeftRect.Contains(pt)) {
		// Decrease
		SetState(LEFT_DOWN_STATE);
		fCurrentAdjustment = -fStepSize;
		PerformAdjustment();
		//printf("Decrease, new value: %d\n", fCurrentValue);
	} else if(fRightRect.Contains(pt)) {
		// Increase
		SetState(RIGHT_DOWN_STATE);
		fCurrentAdjustment = fStepSize;
		PerformAdjustment();
		//printf("Increase, new value: %d\n", fCurrentValue);
	}
}

void NetronSlider::MouseMoved(BPoint pt, uint32 transit, const BMessage *msg)
{
	(void)transit;
	(void)msg;
	
	if (!fMouseDown) {
		if (fLeftRect.Contains(pt)) {
			SetState(LEFT_OVER_STATE);
		} else if (fRightRect.Contains(pt)) {
			SetState(RIGHT_OVER_STATE);
		} else {
			SetState(NORMAL_STATE);
		}
	}
}

void NetronSlider::MouseUp(BPoint pt)
{
	if(fMouseDown) {
		if(fAdjustmentRunner) {
			StopAdjustment();
		}
		fMouseDown = false;

		if (fLeftRect.Contains(pt)) {
			SetState(LEFT_OVER_STATE);	
		} else if (fRightRect.Contains(pt)) {
			SetState(RIGHT_OVER_STATE);
		} else {
			SetState(NORMAL_STATE);
		}
	}
}

void 
NetronSlider::PerformAdjustment()
{
	uint8 newValue = max_c(fMinValue,
	                       min_c(fMaxValue, fCurrentValue + fCurrentAdjustment));
	if(fCurrentValue == newValue) {
		StopAdjustment();
		return;
	}
	fCurrentValue = newValue;
	UpdatePosition();
	UpdateSettings();
}

void 
NetronSlider::StopAdjustment()
{
	fCurrentAdjustment = 0;
	delete fAdjustmentRunner;
	fAdjustmentRunner = NULL;
	if(fDisplay == NULL)
		return;
	//if(fSavedValue == fCurrentValue)
	//	return;
	//if(fDisplay->SaveSettings() != B_NO_ERROR) {
	//	printf("Error while trying to save settings\n");
	//	return;
	//}
	//fSavedValue = fCurrentValue;
}

void 
NetronSlider::Refresh(bool factorySetting)
{
	status_t err;
	if (fDisplay == NULL)
		return;

	if (factorySetting) {
		err = fDisplay->GetFactorySetting(kRegisters[fControl], &fCurrentValue);			
	} else {
		err = fDisplay->GetRegister(kRegisters[fControl], &fCurrentValue);	
	}

	if(err != B_NO_ERROR) {
		printf("Refresh: %s failed\n", factorySetting ? "GetFactorySetting" : "GetRegister");
		//fCurrentValue = fMinValue;	// avoid values that are out of range
	} else {
		//printf("Refresh: control: %d, register: 0x%02X, fCurrentValue:%d\n", fControl, kRegisters[fControl], fCurrentValue);
	}
	UpdatePosition();
}

void NetronSlider::SetRange(int32 minRange, int32 maxRange,
                            int32 minRange2, int32 maxRange2)
{
	fMinValue = max_c(minRange, minRange2);
	fMaxValue = min_c(maxRange, maxRange2);
	fDelta = (fSliderRect.Width() - ((SLIDER_END_SPACE * 2.0f) + 1.0f)) / (fMaxValue - fMinValue);
	//printf("SetRange: control: %d, register: 0x%02X, range %d-%d\n", fControl, kRegisters[fControl], fMinValue, fMaxValue);
	if (fCurrentValue < fMinValue) fCurrentValue = fMinValue;
	if (fCurrentValue > fMaxValue) fCurrentValue = fMaxValue;
	UpdatePosition();
}

void 
NetronSlider::SetStepSize(int32 newStepSize)
{
	fStepSize = newStepSize;
}

void NetronSlider::UpdateSettings()
{
	//printf("UpdateSettings: control: %d, register: 0x%02X, fCurrentValue:%d\n", fControl, kRegisters[fControl], fCurrentValue);
	if(fDisplay) fDisplay->SetRegister(kRegisters[fControl], fCurrentValue);
}

void NetronSlider::UpdatePosition()
{
	float oldPos = fPos;
	fPos = ((fCurrentValue - fMinValue) * fDelta) + fSliderRect.left + SLIDER_END_SPACE;
	Invalidate(BRect(oldPos - 1.0f, fSliderRect.top, oldPos + 1.0f, fSliderRect.bottom)); // invalidate old position
	Invalidate(BRect(fPos - 1.0f, fSliderRect.top, fPos + 1.0f, fSliderRect.bottom)); // invalidate new position
}

void NetronSlider::SetValue(uint8 value)
{
	fCurrentValue = value;
	UpdatePosition();
	UpdateSettings();
}

void NetronSlider::SetState(interaction_state nuState)
{
	interaction_state oldState = fState;
	if (nuState != oldState) {
		fState = nuState;

		bool invalidateLeft(false), invalidateRight(false);

		if (oldState == LEFT_OVER_STATE || oldState == LEFT_DOWN_STATE) {
			invalidateLeft = true;
		} else if (oldState == RIGHT_OVER_STATE || oldState == RIGHT_DOWN_STATE) {
			invalidateRight = true;
		}

		if (nuState == LEFT_OVER_STATE || nuState == LEFT_DOWN_STATE) {
			invalidateLeft = true;
		} else if (nuState == RIGHT_OVER_STATE || nuState == RIGHT_DOWN_STATE) {
			invalidateRight = true;
		}

		if (invalidateRight) {
			Invalidate(fRightRect);
		}
		if (invalidateLeft) {
			Invalidate(fLeftRect);
		}
	}	
}
