#ifndef _APPLICATION_H
#include <Application.h>
#endif

#include "twindow.h"
#include "bm_view.h"
#include "main.h"
#include <stdio.h>
#include <Alert.h>
#include <Application.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Button.h>
#include <Debug.h>
#include <Font.h>
#include <Menu.h>
#include <MenuItem.h>
#include <string.h>


const kPopUpDivider = 110;
const kPopUpHeight = 20;
const kFontItemSizeItemSpacing = 6;
const kSelectorSpacing = 10;
const kFontSelectorHeight = kPopUpHeight * 2 + kFontItemSizeItemSpacing; 
const kFontSelectorWidth = 300;
const kFontSampleHOffset = 100;

const BPoint kRevertButtonBottomLeftOffset(20, 20);
const BPoint kButtonSize(70, 20);
const kButtonSpacing = 20;

const kSampleTextHOffset = 20;
const kSizePopupWidth = 150;
const kSampleTextWidth = 120;

const char *kSampleText = "Sample text";

const rgb_color kViewGray = { 216, 216, 216, 0};
const rgb_color kWhite = { 255, 255, 255, 0};
const rgb_color kBlack = { 0, 0, 0, 0};
const rgb_color kDarkGray = { 160, 160, 160, 0};

const kFontSelectedMessage = 'fnsl';

TWindow::TWindow(BRect frame)
				: BWindow(frame, "Font Edit", B_TITLED_WINDOW, B_NOT_RESIZABLE)
{
	BRect menuRect(Bounds());
	menuRect.InsetBy(-20, 24);
	menuRect.bottom = menuRect.top + kFontSelectorHeight;
	menuRect.right = menuRect.left + kFontSelectorWidth;

// set gray background
	frame.OffsetTo(0, 0);
	background = new BView(frame, "", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
	AddChild(background);
	background->SetViewColor(212, 212, 212);

//	menuRect.OffsetBy(0, menuRect.Height());
	SelectFont = new FontSelectionMenu(menuRect, "Tuned Font:", this);
	SelectFont->AddAsChild(background);
}

//-----------------------------------------------------------

void	TWindow::MessageReceived(BMessage *an_event)
{
	BMView	*bm;
	
	bm = (BMView *)FindView("BMView");
	switch(an_event->what) {
	case SAVE :
		Save();
		break;
	case EDIT :
		Edit();
		break;
	case APPLY :
		Apply();
		break;
	case UNDO :
		bm->do_undo();
		break;
	case REVERSE :
		bm->do_reverse();
		break;	
	case COPY :
		bm->Copy();
		break;
	case CUT :
		bm->Cut();
		break;
	case PASTE :
		bm->Paste();
		break;
	case INVERTH :
		bm->InvertH();
		break;
	case INVERTV :
		bm->InvertV();
		break;
	}
}

//-----------------------------------------------------------

void TWindow::Edit() {
	BAlert	*alert;
	BMView	*bm;

	_font_control_(&SelectFont->font, FC_CMD_CREATE_TUNED_FONT, (void*)pathname);	
	bm = ((TApplication*)be_app)->bm_view;
	bm = (BMView *)FindView("BMView");
	if (bm->fm->LoadNewFont(pathname) >= 0) {
		bm->Reset();
		((TApplication*)be_app)->s_view->Reset(1);	
	}
	else {
		alert = new BAlert("",
						   "Fe was not able to open the selected font for edition.",
						   "Okay");
		alert->Go();		
	}
}

//-----------------------------------------------------------

void TWindow::Save() {
	BMView	*bm;

	bm = (BMView *)FindView("BMView");
	bm->fm->Save();
}

//-----------------------------------------------------------

void TWindow::Apply() {
	BMView	*bm;

	bm = (BMView *)FindView("BMView");
	bm->fm->Apply();	
}

//-----------------------------------------------------------

bool TWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return(TRUE);
}

//-----------------------------------------------------------

FontPopUpMenu::FontPopUpMenu(BRect frame, float divider, const BFont *initialFont, const char *label,
	const BHandler *target)
	:	BMenuField(frame, "", label, new FontMenu(initialFont, target))
{
	SetAlignment(B_ALIGN_RIGHT);
	SetDivider(divider);
	SetEnabled(true);
}

void
FontPopUpMenu::ShowCurrent(const BFont *currentFont)
{
	FontMenu *menu = dynamic_cast<FontMenu *>(Menu());
	ASSERT(menu);
	menu->ShowCurrent(currentFont);
}

FontSizePopUpMenu::FontSizePopUpMenu(BRect frame, float divider, const BFont *initialFont, 
	const BHandler *target)
  	:		BMenuField(frame, "", "Size:", new BPopUpMenu("", true, true))
{
	SetDivider(divider);
	SetAlignment(B_ALIGN_RIGHT);

	char tmp[64];
	BPopUpMenu *menu = dynamic_cast<BPopUpMenu *>(Menu());

	for (int32 index = 0; index < sizeof(defaultSizes) / sizeof(float); index++) {
		BMessage *message = new BMessage(kFontSelectedMessage);
		message->AddFloat("size", defaultSizes[index]);

		sprintf(tmp, "%.0f", defaultSizes[index]);

		BMenuItem *item = new BMenuItem(tmp, message);
		menu->AddItem(item);
		item->SetTarget(target);

		// mark item if current size
		if (initialFont->Size() == defaultSizes[index])
			item->SetMarked(true);
	}
	SetCurrent(initialFont);
}

void
FontSizePopUpMenu::ShowCurrent(const BFont *currentFont)
{
	SetCurrent(currentFont);
}

void
FontSizePopUpMenu::SetCurrent(const BFont *currentFont)
{
	char tmp[64];
	sprintf(tmp, "%.0f", currentFont->Size());

	BMenu *menu = Menu();

	// select the appropriate menuitem
	int32 itemCount = menu->CountItems();
	for (int32 itemIndex = 0; itemIndex < itemCount; itemIndex++) {
		BMenuItem *item = menu->ItemAt(itemIndex);
		item->SetMarked(!strcmp(tmp, item->Label()));
	}
}

FontMenu::FontMenu(const BFont *font, const BHandler *target)
	:	BPopUpMenu("", false, false)
{
	font_family	fontFamily;
	font_style fontStyle;
	BFont f;
	uint32 flags;
	const uint32  kBufferLength = B_FONT_FAMILY_LENGTH+1+B_FONT_STYLE_LENGTH+1;
	char temp[kBufferLength];

	int32 familyCount = count_font_families();

	font_family selectedFamilyName;
	font_style selectedStyleName;
	font->GetFamilyAndStyle(&selectedFamilyName, &selectedStyleName);

	for (int32 familyIndex = 0; familyIndex < familyCount; familyIndex++)
		if (get_font_family(familyIndex, &fontFamily, &flags) == B_OK) {

			BMenu *submenu = new BMenu(fontFamily);
			bool currentFamily = !strcmp(selectedFamilyName, fontFamily);

			int32 styleCount = count_font_styles(fontFamily);
			for (int32 styleIndex = 0; styleIndex < styleCount; styleIndex++)
				if (get_font_style(fontFamily, styleIndex, &fontStyle, &flags) == B_OK) {

					BMessage *message = new BMessage(kFontSelectedMessage);
					message->AddString("familyName", fontFamily);
					message->AddString("styleName", fontStyle);

					BMenuItem *item = new BMenuItem(fontStyle, message);
					submenu->AddItem(item);
					item->SetTarget(target);

					// mark if current
					item->SetMarked((currentFamily 
						&& strcmp(selectedStyleName, fontStyle) == 0));
				}

			AddItem(submenu);

			// selecting a family only is a shortcut for selecting the 
			// default style of the family; SetFamilyAndStyle will handle it

			BMessage *message = new BMessage(kFontSelectedMessage);
			message->AddString("familyName", fontFamily);
			BMenuItem *item = ItemAt(familyIndex);
			item->SetMessage(message);
			item->SetTarget(target);
			// if current family, mark the family item too
			item->SetMarked(currentFamily);
		}
	SetCurrent(font);
}

void
FontMenu::SetCurrent(const BFont *font)
{
	const kBufferLength = B_FONT_FAMILY_LENGTH+1+B_FONT_STYLE_LENGTH+1;
	char temp[kBufferLength];

	font_family selectedFamilyName;
	font_style selectedStyleName;
	font->GetFamilyAndStyle(&selectedFamilyName, &selectedStyleName);

	// select the correct menu item and unselect the old one
	int32 itemCount = CountItems();
	for (int32 itemIndex = 0; itemIndex < itemCount; itemIndex++) {

		BMenuItem *item = ItemAt(itemIndex);
		bool currentFamily = (strcmp(item->Label(), selectedFamilyName) == 0);
		item->SetMarked(currentFamily);

		BMenu *subMenu = item->Submenu();
		ASSERT(subMenu);

		if (subMenu){
			int32 subItemCount = subMenu->CountItems();
			for (int32 subItemIndex = 0; subItemIndex < subItemCount; subItemIndex++) {
				BMenuItem *subItem = subMenu->ItemAt(subItemIndex);
				subItem->SetMarked(currentFamily &&
					(strcmp(subItem->Label(), selectedStyleName) == 0));
			}
		}
	}

	FontName(temp, kBufferLength, font);

	SetName(temp);
	// SetName to make the string show up the first time
	if (Superitem())
	Superitem()->SetLabel(temp);
}

void
FontMenu::ShowCurrent(const BFont *font)
{
	SetCurrent(font);
	Invalidate();
}


void
FontMenu::FontName(char *resultBuffer, int32 bufferSize, font_family familyName, 
	font_style styleName, const char *divider, bool addRegular)
{
	strncpy(resultBuffer, familyName, bufferSize);
	strncat(resultBuffer, divider, bufferSize);
	strncat(resultBuffer, styleName, bufferSize);
}

void
FontMenu::FontName(char *resultBuffer, int32 bufferSize, const BFont *font,
	const char *divider, bool addRegular)
{
	font_family family;
	font_style style;
	font->GetFamilyAndStyle(&family, &style);
	FontName(resultBuffer, bufferSize, family, style, divider, addRegular);
}

SampleText::SampleText(BRect frame, const char *text, const BFont *font)
	:	BView(frame, text, 0, B_WILL_DRAW) 
{
	SetFont(font);
	SetViewColor(kViewGray);
}

void
SampleText::Draw(BRect rect)
{
	BRect bounds = Bounds();
	BeginLineArray(4);

	AddLine(bounds.LeftBottom(), bounds.LeftTop(), kDarkGray);
	AddLine(bounds.LeftTop(), bounds.RightTop(), kDarkGray);
	AddLine(bounds.RightTop(), bounds.RightBottom(), kWhite);
	AddLine(bounds.RightBottom(), bounds.LeftBottom(), kWhite);
	EndLineArray();

	bounds.InsetBy(3, 3);

	BPoint point(bounds.LeftBottom());

	MovePenTo(point);
	SetHighColor(0, 0, 0);
	DrawString(Name());
}

void
SampleText::FontChanged(const BFont *font)
{
	SetFont(font);
	Invalidate();
}

FontSelectionMenu::FontSelectionMenu(BRect frame, const char *label, BWindow *parent)
	:	BHandler("")
{
	parent->AddHandler(this);
	// make sure we have looper so targeting <this> works

	font = *be_plain_font;
	
	// create font face and style popup
	frame.bottom = frame.top + kPopUpHeight;
	fontFamilyAndStyleSelector = new FontPopUpMenu(frame, kPopUpDivider, &font, label, this);

	// create font size popup
	frame.OffsetBy(0, kPopUpHeight + kFontItemSizeItemSpacing);
	frame.right = frame.left + kSizePopupWidth;
	fontSizeSelector = new FontSizePopUpMenu(frame, kPopUpDivider, &font, this);

	// create a sample text item
	frame.left = frame.right + kSampleTextHOffset;
	frame.right = frame.left + kSampleTextWidth;
	sample = new SampleText(frame, kSampleText, &font);
}

FontSelectionMenu::~FontSelectionMenu()
{
	delete fontFamilyAndStyleSelector;
	delete fontSizeSelector;
}

void
FontSelectionMenu::AddAsChild(BView *parent)
{
	// FontSelectionMenu is not a BView, so we need a special
	// way of adding it to a superview
	parent->AddChild(fontFamilyAndStyleSelector);
	parent->AddChild(fontSizeSelector);
	parent->AddChild(sample);
}

void 
FontSelectionMenu::MessageReceived(BMessage *message)
{
	if (message->what != kFontSelectedMessage)
		return;

	char *familyName;
	char *styleName = NULL;
	BFont newFont(font);

	if (message->FindString("familyName", &familyName) == B_OK){
		message->FindString("styleName", &styleName);
		// we can safely ignore not getting a styleName here
		// since passing a NULL string will result in picking the 
		// dfault style for a given family
		newFont.SetFamilyAndStyle(familyName, styleName);
	}

	float size;
	if (message->FindFloat("size", &size) == B_OK) 
		newFont.SetSize(size);

	// force the items to update themselves and call FontSelected
	SetMyFont(&newFont);
}

void
FontSelectionMenu::SetMyFont(const BFont *font)
{
	if (this->font.FamilyAndStyle() == font->FamilyAndStyle()
	    && this->font.Size() == font->Size())
		// do a shorthand equality test for values that we are setting
		return;

	this->font = *font;
	fontFamilyAndStyleSelector->ShowCurrent(font);
	fontSizeSelector->ShowCurrent(font);
	sample->FontChanged(font);

//	FontSelected(font);
}




















