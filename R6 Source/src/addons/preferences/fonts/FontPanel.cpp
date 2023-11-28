//
//	FontPanel.cpp
//
//	pavel 3/19/97
// 
//	(c) 1997 Be Incorporated


#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <interface_misc.h>

#include <Alert.h>
#include <FindDirectory.h>
#include <Menu.h>
#include <MenuItem.h>
#include <Screen.h>

#include "FontPanel.h"

#include "shared_fonts.h"

const int32 kButtonWidth = 75;							// same as B_WIDTH_AS_USUAL

const int32 kPopUpHeight = 20;
const int32 kFontItemSizeItemSpacing = 6;
const int32 kSelectorSpacing = 10;
const int32 kFontSelectorHeight = kPopUpHeight * 2 + kFontItemSizeItemSpacing; 

const int32 kFontCacheDefault = 256*1024;

const rgb_color kViewGray = { 216, 216, 216, 255};
const rgb_color kWhite = { 255, 255, 255, 255};
const rgb_color kBlack = { 0, 0, 0, 255};
const rgb_color kDarkGray = { 100, 100, 100, 255};

enum {
	msg_defaults = 'dflt',
	msg_revert = 'rvrt',
	msg_save_cache = 'save',
	msg_rescan = 'scan',
	msg_font_settings = 'font',
	msg_cache_settings = 'cash',
	msg_control_changed = 'chng',
	msg_screen_change = 'schn',
	msg_printing_change = 'pchn'
};

const uint32 kFontSelectedMessage = 'fnsl';

const char *kSliderUnitString = "kB";
const char *kRegularString = "Regular";
const char *kSampleText = "The quick brown fox jumps over the lazy dog.";
const char* k_fonts_settings_file = "Font_Settings";

const char* kFontSettingsStr = "Font Settings";
const char* kCacheSettingsStr = "Cache Settings";

static float
FontHeight(BView* target, bool full)
{
	font_height finfo;		
	target->GetFontHeight(&finfo);
	float h = finfo.ascent + finfo.descent;

	if (full)
		h += finfo.leading;
	
	return h;
}

//****************************************************************************************

const int32 kWindowWidth = 350;
const int32 kWindowHeight = 310;

FontPanel::FontPanel()
 : BView(BRect(0, 0, kWindowWidth, kWindowHeight), "fonts", B_FOLLOW_NONE, B_WILL_DRAW)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BRect tabFrame(0, 0, Bounds().Width()-1, Bounds().Height() - 90);
	fContainer = new BTabView( tabFrame, "container", B_WIDTH_FROM_WIDEST);
	fContainer->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fContainer->SetFont(be_plain_font);
	AddChild(fContainer);
	
	BuildFontControls();
	BuildCacheControls();

	AddButtons();	
	CanRevert(false);
	
//	AddShortcut('R', B_COMMAND_KEY, new BMessage(msg_revert));
//	AddShortcut('D', B_COMMAND_KEY, new BMessage(msg_defaults));
	
//	AddShortcut('F', B_COMMAND_KEY, new BMessage(msg_font_settings));
//	AddShortcut('C', B_COMMAND_KEY, new BMessage(msg_cache_settings));
	
//	AddShortcut('N', B_COMMAND_KEY, new BMessage(msg_rescan));
//	AddShortcut('S', B_COMMAND_KEY, new BMessage(msg_save_cache));
}
	
void
FontPanel::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case msg_font_settings:
			fContainer->Select((int32)0);
			break;
		case msg_cache_settings:
			fContainer->Select((int32)1);
			break;
			
		case msg_defaults:
			SetDefaults();
			break;			
		case msg_revert:
			Revert();
			break;

		case msg_save_cache:
			_save_font_cache_state_(FC_SAVE_FONT_FILE_LIST|FC_SAVE_FONT_CACHE);
			break;
		case msg_rescan:
			update_font_families(FALSE);
			UpdateMenus();
			break;
					
		case msg_screen_change:
			CanRevert(true);
			break;
		case msg_printing_change:
			CanRevert(true);
			break;
			
		default:
			BView::MessageReceived(message);
			break;
	}
}

void FontPanel::DetachedFromWindow()
{
	Apply();
}

void
FontPanel::AddButtons()
{
	BRect r;
	r = Bounds();
	r.top = fContainer->Frame().bottom + 1;
	fBtnBar = new TButtonBar(r, true, true, false);
	fBtnBar->AddButton("Refresh font list", new BMessage(msg_rescan));
	AddChild(fBtnBar);
}

void
FontPanel::BuildFontControls()
{
	//	base the location of all the objects on the view container	
	BView* viewContainer = fContainer->ContainerView();

	fFontContainer = new BView( viewContainer->Bounds(), "Fonts", B_FOLLOW_NONE,
		B_WILL_DRAW | B_FRAME_EVENTS);
	fFontContainer->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fFontContainer->SetFont(be_plain_font);
	fContainer->AddTab(fFontContainer);
	AddFontControls();
}

// after a rescan these need to be rebuilt
void
FontPanel::AddFontControls()
{
	BRect menuRect(fFontContainer->Bounds());
	menuRect.left += 7; menuRect.right -= 10;
	menuRect.top += 10;
	menuRect.bottom = menuRect.top + kFontSelectorHeight;

	//	target for the font selection controls is the tabview (fContainer)
	//	container is the font container
	//	the 3 items that make up these controls are AddChild'd in the constructor
	fPlainFontSelector = new PlainFontMenu(menuRect, fFontContainer, this);

	menuRect.OffsetBy(0, 57);
	fBoldFontSelector = new BoldFontMenu(menuRect, fFontContainer, this);

	menuRect.OffsetBy(0, 57);
	fFixedFontSelector = new FixedFontMenu(menuRect, fFontContainer, this);
}

void FontPanel::AttachedToWindow()
{
	Window()->AddHandler(fPlainFontSelector);
	fPlainFontSelector->SetTarget(fPlainFontSelector);
	Window()->AddHandler(fBoldFontSelector);
	fBoldFontSelector->SetTarget(fBoldFontSelector);
	Window()->AddHandler(fFixedFontSelector);
	fFixedFontSelector->SetTarget(fFixedFontSelector);

	fBtnBar->SetTarget(this);
	fPrintingCacheSlider->SetTarget(this);
	fScreenCacheSlider->SetTarget(this);
	fSaveCacheBtn->SetTarget(this);

	fContainer->Select((int32)0);
}

const rgb_color kUsedColor = {102, 152, 203,0};

void
FontPanel::BuildCacheControls()
{
	rgb_color color = kUsedColor;
	font_cache_info set;	
	
	//	base the location of all the objects on the view container	
	BView* viewContainer = fContainer->ContainerView();

	fCacheContainer = new BView(viewContainer->Bounds(), "Cache", B_FOLLOW_NONE,
		B_WILL_DRAW | B_FRAME_EVENTS);
	fCacheContainer->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	fCacheContainer->SetFont(be_plain_font);
	fContainer->AddTab(fCacheContainer);
	
	//	the slider will resize itself to accomodate its settings when attached
	BRect sliderRect(10, 20, viewContainer->Bounds().Width() - 10, 36);
	if (get_font_cache_info(B_SCREEN_FONT_CACHE|B_DEFAULT_CACHE_SETTING, &set) != B_NO_ERROR)
		set.cache_size = kFontCacheDefault;

	fInitialScreenCache = set.cache_size/1024;
	fScreenCacheSlider = new TCustomSlider(sliderRect, "Screen font cache size", 
		new BMessage(msg_screen_change), 64, 4096,
		B_SCREEN_FONT_CACHE|B_DEFAULT_CACHE_SETTING);
	fScreenCacheSlider->SetLimitLabels("64 kB","4096 kB");
	fScreenCacheSlider->SetValue(fInitialScreenCache);
	fScreenCacheSlider->UseFillColor(true,&color);
	fCacheContainer->AddChild(fScreenCacheSlider);

	if (get_font_cache_info(B_PRINTING_FONT_CACHE|B_DEFAULT_CACHE_SETTING, &set) != B_NO_ERROR)
		set.cache_size = kFontCacheDefault;

	fInitialPrintingCache = set.cache_size/1024;

	//	base the next one on the previous sliders frame
	sliderRect = fScreenCacheSlider->Frame();
	sliderRect.top = sliderRect.bottom + 15;
	sliderRect.bottom = sliderRect.top + 16;
	fPrintingCacheSlider = new TCustomSlider(sliderRect, "Printing font cache size",
		new BMessage(msg_printing_change), 64, 4096,
		B_PRINTING_FONT_CACHE|B_DEFAULT_CACHE_SETTING);
	fPrintingCacheSlider->SetLimitLabels("64 kB","4096 kB");
	fPrintingCacheSlider->SetValue(fInitialPrintingCache);
	fPrintingCacheSlider->UseFillColor(true,&color);
	fCacheContainer->AddChild(fPrintingCacheSlider);
	
	BRect buttonRect;
	buttonRect.left = 9;
	buttonRect.right = buttonRect.left + be_plain_font->StringWidth("Save Cache") + 20;
	if (buttonRect.right - buttonRect.left < 75) buttonRect.right = buttonRect.left + 75;
	buttonRect.bottom = viewContainer->Bounds().Height() - 9;
	buttonRect.top = buttonRect.bottom - (FontHeight(fCacheContainer, true) + 10);

	fSaveCacheBtn = new BButton(buttonRect, "save","Save Cache",
		new BMessage(msg_save_cache), B_FOLLOW_BOTTOM | B_FOLLOW_RIGHT);
	fCacheContainer->AddChild(fSaveCacheBtn);
}

void FontPanel::UpdateMenus()
{
	fPlainFontSelector->RemoveAsChild(fFontContainer);
	delete fPlainFontSelector;

	fBoldFontSelector->RemoveAsChild(fFontContainer);
	delete fBoldFontSelector;

	fFixedFontSelector->RemoveAsChild(fFontContainer);
	delete fFixedFontSelector;

	AddFontControls();
}

void
FontPanel::Apply()
{
	fPlainFontSelector->Apply();
	fBoldFontSelector->Apply();
	fFixedFontSelector->Apply();
	fScreenCacheSlider->Apply();
	fPrintingCacheSlider->Apply();
	//_save_font_cache_state_(FC_SAVE_PREFS);
}

void
FontPanel::SetDefaults()
{
	fPlainFontSelector->SetDefault();
	fBoldFontSelector->SetDefault();
	fFixedFontSelector->SetDefault();

	fScreenCacheSlider->SetDefault();
	fPrintingCacheSlider->SetDefault();
	
	CanRevert(true);
}

void
FontPanel::Revert()
{
	fPlainFontSelector->Revert();
	fBoldFontSelector->Revert();
	fFixedFontSelector->Revert();
	
	fScreenCacheSlider->Revert(fInitialScreenCache);
	fPrintingCacheSlider->Revert(fInitialPrintingCache);
	
	CanRevert(false);
}

void
FontPanel::CanRevert(bool state)
{
	fBtnBar->CanRevert(state);
}

//****************************************************************************************

FontPopUpMenu::FontPopUpMenu(BRect frame, float divider, const BFont *initialFont,
	const char *label, bool fixed)
	:	BMenuField(frame, "", label, new FontMenu(initialFont, fixed), false)
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

//****************************************************************************************

FontSizePopUpMenu::FontSizePopUpMenu(BRect frame, float divider, const BFont *initialFont)
 : BMenuField(frame, "", "Size:", new BPopUpMenu("", true, true), true)
{
	SetDivider(divider);
	SetAlignment(B_ALIGN_RIGHT);

	char tmp[64];
	BPopUpMenu *menu = dynamic_cast<BPopUpMenu *>(Menu());

//	for (int32 index = 0; index < sizeof(defaultSizes) / sizeof(float); index++) {
	for (int32 index = 0; index < (int32) sizeof(defaultSizes) / 
	 	(int32) sizeof(int32); index++) {
		BMessage *message = new BMessage(kFontSelectedMessage);
//		message->AddFloat("size", defaultSizes[index]);
		message->AddInt32("size", defaultSizes[index]);

//		sprintf(tmp, "%.0f", defaultSizes[index]);
		sprintf(tmp, "%li", defaultSizes[index]);

		BMenuItem *item = new BMenuItem(tmp, message);
		menu->AddItem(item);

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

//****************************************************************************************

FontMenu::FontMenu(const BFont *font, bool fixed)
	:	BPopUpMenu("", false, false)
{
	int  RealIndex;
	font_family	fontFamily;
	font_style fontStyle;
	uint32 flags;

	int32 familyCount = count_font_families();

	font_family selectedFamilyName;
	font_style selectedStyleName;
	font->GetFamilyAndStyle(&selectedFamilyName, &selectedStyleName);

	RealIndex = 0;
	for (int32 familyIndex = 0; familyIndex < familyCount; familyIndex++)
		if (get_font_family(familyIndex, &fontFamily, &flags) == B_OK) {
			if (fixed && (!(flags & B_IS_FIXED)))
				continue;
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

					// mark if current
					item->SetMarked((currentFamily 
						&& strcmp(selectedStyleName, fontStyle) == 0));
				}

			AddItem(submenu);

			// selecting a family only is a shortcut for selecting the 
			// default style of the family; SetFamilyAndStyle will handle it

			BMessage *message = new BMessage(kFontSelectedMessage);
			message->AddString("familyName", fontFamily);
			BMenuItem *item = ItemAt(RealIndex);
			item->SetMessage(message);
			// if current family, mark the family item too
			item->SetMarked(currentFamily);
			RealIndex++;
		}
	SetCurrent(font);
}

BPoint
FontMenu::ScreenLocation()
{
	ASSERT((Supermenu()));
	ASSERT((Superitem()));
	
	BMenuItem	*parentItem = Superitem();
	BMenu		*parent = Supermenu();
	BMenuItem	*curItem;
	BRect		rect;
	BPoint		pt;
	
	// find the 'marked' subitem, instead of subitem with matching name
	curItem = FindMarked();
	
//	PRINT(("looking for \"%s\" (%d)\n", title, curItem != NULL));
	
	rect = parentItem->Frame();
	pt = rect.LeftTop();
	parent->ConvertToScreen(&pt);

	if (curItem) {
		rect = curItem->Frame();
		pt.y -= rect.top;
	}
	
	return pt;
}

void
FontMenu::SetCurrent(const BFont *font)
{
	const int32 kBufferLength = B_FONT_FAMILY_LENGTH+1+B_FONT_STYLE_LENGTH+1;
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
	if (addRegular || strcmp(styleName, kRegularString) != 0){
		strncat(resultBuffer, divider, bufferSize);
		strncat(resultBuffer, styleName, bufferSize);
	}
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

//****************************************************************************************

#define _MAX_ 0 // sorry MAX, this was causing trouble

SampleText::SampleText(BRect frame, const char *text, const BFont *font)
	:	BView(frame, text, 0, B_WILL_DRAW) 
{
	SetFont(font);
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

const rgb_color kgray1 = { 184, 184, 184, 255 };
const rgb_color kgray2 = { 152, 152, 152, 255 };
void
SampleText::Draw(BRect)
{
	PushState();
	
	BRect bounds(Bounds());

	SetHighColor(kBlack);
	SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	MovePenTo(bounds.left + 5, bounds.bottom - 5);
	DrawString(Name());
	
	//	draw the border
	BPoint pt1 = bounds.RightTop();
	BPoint pt2 = bounds.RightBottom();
	BeginLineArray(9);
	
	//	make the clipping symetrical
	pt1.x--; pt2.x--;
	AddLine(pt1, pt2, ui_color(B_PANEL_BACKGROUND_COLOR));
	pt1.x--; pt2.x--;
	AddLine(pt1, pt2, ui_color(B_PANEL_BACKGROUND_COLOR));
	pt1.x--; pt2.x--;
	AddLine(pt1, pt2, ui_color(B_PANEL_BACKGROUND_COLOR));

	// draw the border
	AddLine(bounds.LeftBottom(), bounds.LeftTop(), kgray1);
	AddLine(bounds.LeftTop(), bounds.RightTop(), kgray1);
	
	bounds.InsetBy(1,1);
	AddLine(bounds.LeftBottom(), bounds.LeftTop(), kgray2);
	AddLine(bounds.LeftTop(), bounds.RightTop(), kgray2);

	bounds.InsetBy(-1, -1);
	bounds.left++;
	bounds.top++;
	AddLine(bounds.RightTop(), bounds.RightBottom(), kWhite);
	AddLine(bounds.RightBottom(), bounds.LeftBottom(), kWhite);
	EndLineArray();

	PopState();
}

void
SampleText::FontChanged(const BFont *font)
{
	SetFont(font);
	Invalidate();
}

//****************************************************************************************

#if 0 // unused
static void
SetMenuFieldFont(BMenuField* t, const BFont* f, float s)
{
	t->SetFont(f);
	t->SetFontSize(s);
	BMenuBar*	mb = t->MenuBar();
	if (mb) {
		mb->SetFont(f);
		mb->SetFontSize(s);
	}

	BMenu* m = t->Menu();
	if (m) {
		m->SetFont(f);
		m->SetFontSize(s);
	}	
}
#endif

FontSelectionMenu::FontSelectionMenu(BRect frame, const BFont *initialFont,
 	const char *label, BView* container, RevertNotifier *n, bool fixed)
 : font(initialFont), fOriginalFont(initialFont), rev(n)
{
	//	pay attention to top, left and right, bottom is calculated from fontheight
	BRect cntrlFrame(frame);

	// create font face and style popup
	cntrlFrame.bottom = cntrlFrame.top + 20;
	cntrlFrame.right = frame.right;
	cntrlFrame.left = cntrlFrame.right - container->StringWidth("Size: XXX") - 25;
	fontSizeSelector = new FontSizePopUpMenu(cntrlFrame, container->StringWidth("Size:")+10, initialFont);
	
	cntrlFrame.right = cntrlFrame.left - 10;
	cntrlFrame.left = frame.left;
	fontFamilyAndStyleSelector = new FontPopUpMenu(cntrlFrame, container->StringWidth("Fixed font:")+10,
		initialFont, label, fixed);
	container->AddChild(fontFamilyAndStyleSelector);
	container->AddChild(fontSizeSelector);

	// create a sample text item
	cntrlFrame.top = fontFamilyAndStyleSelector->Frame().top + 20 + 8;
	cntrlFrame.bottom = cntrlFrame.top + FontHeight(container, true) + 9;
	cntrlFrame.left = frame.left + fontFamilyAndStyleSelector->Divider() + 1;
	cntrlFrame.right = frame.right - 1;
	sample = new SampleText(cntrlFrame, kSampleText, initialFont);
	container->AddChild(sample);
}

FontSelectionMenu::~FontSelectionMenu()
{
	// just in case?
	delete fontFamilyAndStyleSelector;
	delete fontSizeSelector;
	delete sample;
}

void FontSelectionMenu::SetTarget(BMessenger m)
{
	fontFamilyAndStyleSelector->Menu()->SetTargetForItems(m);
	fontSizeSelector->Menu()->SetTargetForItems(m);
}

// return the bounds that contain all of the 3 components
BRect
FontSelectionMenu::Bounds() const
{
	BRect bounds;
	
	bounds.left = fontFamilyAndStyleSelector->Frame().left;
	bounds.top = fontFamilyAndStyleSelector->Frame().top;
	bounds.right = sample->Frame().right;
	bounds.bottom = sample->Frame().bottom;
	
	return bounds;
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
FontSelectionMenu::RemoveAsChild(BView *parent)
{
	// FontSelectionMenu is not a BView, so we need a special
	// way of adding it to a superview
	parent->RemoveChild(fontFamilyAndStyleSelector);
	parent->RemoveChild(fontSizeSelector);
	parent->RemoveChild(sample);
}

void 
FontSelectionMenu::MessageReceived(BMessage *message)
{
	if (message->what != kFontSelectedMessage) {
		BHandler::MessageReceived(message);
		return;
	}

	char *familyName;
	char *styleName = NULL;
	BFont newFont(font);

	if (message->FindString("familyName", (const char**) &familyName) == B_OK){
		message->FindString("styleName", (const char**) &styleName);
		// we can safely ignore not getting a styleName here
		// since passing a NULL string will result in picking the 
		// default style for a given family
		newFont.SetFamilyAndStyle(familyName, styleName);
	}

//	float size;
//	if (message->FindFloat("size", &size) == B_OK) 
	int32 size;
	if (message->FindInt32("size", &size) == B_OK) 
		newFont.SetSize(size);

	// force the items to update themselves and call FontSelected
	SetSelectedFont(&newFont);
}

void
FontSelectionMenu::SetSelectedFont(const BFont *font)
{
	if (this->font.FamilyAndStyle() == font->FamilyAndStyle()
	    && this->font.Size() == font->Size())
		// do a shorthand equality test for values that we are setting
		return;

	this->font = *font;
	fontFamilyAndStyleSelector->ShowCurrent(font);
	fontSizeSelector->ShowCurrent(font);
	sample->FontChanged(font);

	FontSelected(font);
	
	rev->CanRevert(true);
}

void
FontSelectionMenu::Apply()
{
}

void
FontSelectionMenu::SetDefault()
{
}

void
FontSelectionMenu::Revert()
{
	SetSelectedFont(&fOriginalFont);
}

void
FontSelectionMenu::FontSelected(const BFont *)
{
}

//****************************************************************************************

static const BFont *
StandardFontFromIndex(int32 index)
{
	float        size;
	font_style   styleName;
	font_family  familyName;
	static BFont result;

	_get_standard_font_setting_(index, familyName, styleName, &size);
	result.SetFamilyAndStyle(familyName, styleName);
	result.SetSize(size);
	return &result;
}

StandardFontSettingsMenu::StandardFontSettingsMenu(BRect rect, int32 index, const char *label, 
	BView* container, RevertNotifier *n)
	:	FontSelectionMenu(rect, StandardFontFromIndex(index), label, container, n, (index == 2)),
    		standardFontSettingsIndex(index)
{
}

void 
StandardFontSettingsMenu::FontSelected(const BFont *)
{
}

void
StandardFontSettingsMenu::Apply()
{
	float        size;
	font_style   styleName;
	font_family  familyName;
	
	font.GetFamilyAndStyle(&familyName, &styleName);
	size = font.Size();
	_set_standard_font_setting_(standardFontSettingsIndex, familyName, styleName, size);
}

void
StandardFontSettingsMenu::SetDefault()
{
	float       size;
	BFont       result;
	font_style  styleName;
	font_family familyName;

	_get_standard_font_setting_(standardFontSettingsIndex-100, familyName, styleName, &size);
	result.SetFamilyAndStyle(familyName, styleName);
	result.SetSize(size);
	SetSelectedFont(&result);
}

//****************************************************************************************

TCustomSlider::TCustomSlider(BRect rect, const char *name, BMessage *msg,
	int32 minValue, int32 maxValue, int mask)
	: BSlider( rect, "custom slider", name, msg, minValue, maxValue,
		B_TRIANGLE_THUMB)
{
	fMask = mask;
	fStatusStr = (char*)malloc(64);
}


TCustomSlider::~TCustomSlider()
{
	if (fStatusStr)
		free(fStatusStr);
}

void
TCustomSlider::AttachedToWindow()
{
	BSlider::AttachedToWindow();
}

void TCustomSlider::KeyDown(const char* bytes, int32 count)
{
	int32 newVal;
		
	switch (bytes[0]) {
		case B_DOWN_ARROW:
			if (IsEnabled() && !IsHidden()) {
				newVal = Value() - 64;
				SetValue(newVal);
				Invoke();
			}
			break;
		case B_LEFT_ARROW:
			if (IsEnabled() && !IsHidden()) {
				newVal = Value() - 16;
				SetValue(newVal);
				Invoke();
			}
			break;
		case B_UP_ARROW:
			if (IsEnabled() && !IsHidden()) {
				newVal = Value() + 64;
				SetValue(newVal);
				Invoke();
			}
			break;
		case B_RIGHT_ARROW:
			if (IsEnabled() && !IsHidden()) {
				newVal = Value() + 16;
				SetValue(newVal);
				Invoke();
			}
			break;
		case B_ENTER:
		case B_SPACE:
			// I don't want to pass these on, if you think so, what should it do?
			break;
		default:
			BSlider::KeyDown(bytes, count);
			break;
	}
}

const int32	kYGap = 4;
const int32 kHashHeight = 6;
void
TCustomSlider::DrawText()
{
	BView* osView = OffscreenView();
	
	rgb_color textcolor = (IsEnabled()) ? kBlack : shift_color(kBlack, 0.5);
	
	osView->SetHighColor(textcolor);
	osView->SetLowColor(ViewColor());

	font_height finfo;
	osView->GetFontHeight(&finfo);
	float textHeight = ceil(finfo.ascent + finfo.descent + finfo.leading);

	char str[64];
	sprintf(str, "%s: %li kB", Label(), Value());
	float xoffset=0,offsetFromTop=0;
	if (Label()) {
		offsetFromTop = textHeight;
		osView->MovePenTo(2,offsetFromTop);
		osView->DrawString(str);
		offsetFromTop += kYGap;
	}
	
/*	if (UpdateText()) {
		offsetFromTop = textHeight;
		xoffset = osView->StringWidth(UpdateText());
		osView->MovePenTo(Bounds().Width()-xoffset-2,offsetFromTop);
		osView->DrawString(UpdateText());		
		offsetFromTop += kYGap;
	}*/

	offsetFromTop += kHashHeight;
	offsetFromTop += BarFrame().Height();
	offsetFromTop += kHashHeight;
		
	if (MinLimitLabel() && MaxLimitLabel()) {
		textHeight = ceil(finfo.ascent + finfo.descent);
		offsetFromTop += textHeight;
		osView->MovePenTo(2,offsetFromTop);
		osView->DrawString(MinLimitLabel());		

		xoffset = osView->StringWidth(MaxLimitLabel());
		osView->MovePenTo(Bounds().Width()-xoffset-2,offsetFromTop);
		osView->DrawString(MaxLimitLabel());		
	}
}

status_t
TCustomSlider::Invoke(BMessage *msg)
{
	Apply();
	return BSlider::Invoke(msg);
}

void 
TCustomSlider::Apply()
{
	font_cache_info    set;
	
	if (get_font_cache_info(fMask, (void*)&set) != B_NO_ERROR) return;
	set.cache_size = Value() * 1024;
	set_font_cache_info(fMask, (void*)&set);
}

void 
TCustomSlider::SetDefault()
{
	SetValue(kFontCacheDefault/1024);
	Apply();
}

void
TCustomSlider::Revert(int32 i)
{
	SetValue(i);
	Apply();
}

char*
TCustomSlider::UpdateText() const
{
	if (fStatusStr) {
		if (Window()->Lock()) {
			sprintf(fStatusStr,"%li kB",Value());
			Window()->Unlock();
			return fStatusStr;
		}
	}
	return NULL;
}

//*********************************************************************

TButtonBar::TButtonBar(BRect frame, bool defaultsBtn, bool revertBtn,
	bool drawBorder)
	: BView(frame, "button bar", B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW),
	fHasDefaultsBtn(defaultsBtn), fHasRevertBtn(revertBtn),
	fHasOtherBtn(false), fDrawBorder(drawBorder)
{
	BRect r;
	
	r.bottom = Bounds().Height();
	r.top = r.bottom - 25;
	r.left = 5;
	if (fHasDefaultsBtn) {
		r.right = r.left + 75;
		fDefaultsBtn = new BButton(r, "defaults", "Defaults",
			new BMessage(msg_defaults));
		AddChild(fDefaultsBtn);
	} else
		fDefaultsBtn=NULL;
	
	if (fHasRevertBtn) {
		if (fHasDefaultsBtn)
			r.left = fDefaultsBtn->Frame().right + 10;
		else
			r.left = 5;
		r.right = r.left + 75;
		fRevertBtn = new BButton(r, "revert", "Revert",
			new BMessage(msg_revert));
		AddChild(fRevertBtn);
	} else
		fRevertBtn=NULL;
		
	fOtherBtn=NULL;
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


TButtonBar::~TButtonBar()
{
}

void 
TButtonBar::Draw(BRect r)
{
	BView::Draw(r);

#if 0
	PushState();
	if (fDrawBorder) {
		BRect bounds = Bounds();
	
		BeginLineArray(4);
		AddLine(bounds.LeftBottom(), bounds.LeftTop(), kWhite);
		AddLine(bounds.LeftTop(), bounds.RightTop(), kWhite);
		AddLine(bounds.RightTop(), bounds.RightBottom(), kDarkGray);
		AddLine(bounds.RightBottom(), bounds.LeftBottom(), kDarkGray);
		EndLineArray();
	}
	
	if (fHasOtherBtn) {
		BPoint top, bottom;
		
		top.x = fOtherBtn->Frame().right + 10;
		top.y = fOtherBtn->Frame().top+1;
		bottom.x = top.x;
		bottom.y = fOtherBtn->Frame().bottom-1;
		
		SetHighColor(kDarkGray);
		SetLowColor(ViewColor());
		StrokeLine(top, bottom);
		SetHighColor(kWhite);
		top.x++;
		bottom.x++;
		StrokeLine(top, bottom);
	}
#endif
	PopState();
}

void 
TButtonBar::AddButton(const char* title, BMessage* m)
{
	BRect r;

	r.bottom = Bounds().Height();
	r.top = r.bottom - 25;
	r.right = Bounds().right;
	r.left = r.right - StringWidth(title) - 30;
	fOtherBtn = new BButton(r, "other", title, m);
	
#if 0
	int32 w = (int32) (21+fOtherBtn->Bounds().Width());
	if (fHasDefaultsBtn) {
		RemoveChild(fDefaultsBtn);
		if (fHasRevertBtn)
			RemoveChild(fRevertBtn);
		AddChild(fOtherBtn) /*, fDefaultsBtn)*/;
		AddChild(fDefaultsBtn);
		fDefaultsBtn->MoveBy(w, 0);
		if (fHasRevertBtn) {
			AddChild(fRevertBtn);
			fRevertBtn->MoveBy(w,0);
		}
	} else if (fHasRevertBtn) {
		RemoveChild(fRevertBtn);
		AddChild(fOtherBtn) /*, fRevertBtn)*/;
		AddChild(fRevertBtn);
		fRevertBtn->MoveBy(w,0);
	} else
		AddChild(fOtherBtn);
#endif		
	AddChild(fOtherBtn);
	fHasOtherBtn = (fOtherBtn != NULL);
}

void
TButtonBar::CanRevert(bool state)
{
	fRevertBtn->SetEnabled(state);
}

void TButtonBar::SetTarget(BMessenger m)
{
	fDefaultsBtn->SetTarget(m);
	fRevertBtn->SetTarget(m);
	fOtherBtn->SetTarget(m);
}
