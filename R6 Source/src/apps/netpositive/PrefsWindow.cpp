// ===========================================================================
//	PrefsWindow.cpp
//  Copyright 1998 by Be Incorporated.
// ===========================================================================

#include "NPApp.h"
#include "Protocols.h"
#include "Cache.h"
#include "HTMLView.h"
#include "BeDrawPort.h"
#ifdef ADFILTER
#include "AdFilter.h"
#endif
#include "Strings.h"
#include "MessageWindow.h"

#include <TextControl.h>
#include <Box.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <CheckBox.h>
#include <Button.h>
#include <stdlib.h>
#include <TabView.h>
#include <StringView.h>
#include <stdio.h>
#include <StorageKit.h>

const uint32 kFontSettingCount = 6;

struct FontSetting {
	font_family	proFamily;
	int			proSize;
	int			proMinSize;
	font_family	fixFamily;
	int			fixSize;
	int			fixMinSize;	
};

static void GetFontSettings(int index, struct FontSetting *fs);
static void SetFontSettings(int index, struct FontSetting *fs);

class PrefsView : public BView {
public:
							PrefsView(BRect frame);

virtual	void				AttachedToWindow();
virtual void				DetachedFromWindow();
virtual	void				MessageReceived(BMessage *message);

		void				ApplyPrefs();

private:
		void				ResetMenus(FontSetting *setting);

		void				MarkMenu(BMenu *menu, const char *mark);
		void				MarkMenu(BMenu *menu, int32 mark);
		
		void				AddGeneralPrefsView(BTabView *tabView);
		void				AddDisplayPrefsView(BTabView *tabView);
		void				AddCookiePrefsView(BTabView *tabView);
		void				AddProxyPrefsView(BTabView *tabView);
		void				AddCachePrefsView(BTabView *tabView);
#ifdef ADFILTER
		void				AddAdFilterPrefsView(BTabView *tavView);
#endif
		void				AddSecurityPrefsView(BTabView *tabView);

private:
		FontSetting			fFontSettings[kFontSettingCount];
		FontSetting*		fCurFontSetting;
		bool				fDirtyFontSettings;
		BMenuField*			mProField;
		BMenuField*			mProSizeField;
		BMenuField*			mFixedField;
		BMenuField*			mFixedSizeField;
		BTextControl*		mHTTPName;
		BTextControl*		mHTTPPort;
		BTextControl*		mFTPName;
		BTextControl*		mFTPPort;
		BCheckBox*			mEnableProxies;
		BCheckBox*			mAutoLaunch;
		BTextControl*		mDefaultURL;
		BTextControl*		mSearchURL;
		BTextControl*		mCacheSize;
		int					mCacheOption;
		int					mNewWindowOption;
		int					mFormSubmitOption;
		int					mCookieOption;
		int					mSecureProxyPort;
		BTextControl*		mCacheLocation;
		BCheckBox*			mShowImages;
		BCheckBox*			mShowBGImages;
		BCheckBox*			mUseBGColors;
		BCheckBox*			mUseFGColors;
		BCheckBox*			mShowAnimations;
		BCheckBox*			mPlayMovies;
		BCheckBox*			mPlaySounds;
		BCheckBox*			mUseFonts;
		BCheckBox*			mUnderlineLinks;
		BCheckBox*			mSSLEnter;
		BCheckBox*			mSSLLeave;
		BCheckBox*			mOffscreen;
		BCheckBox*			mHaiku;
		BMenuField*			mMinFixSizeField;
		BMenuField*			mMinProSizeField;
		BTextControl*		mNumConnections;
		BTextControl*		mHistoryLength;
#ifdef JAVASCRIPT
		BCheckBox*			mEnableJavaScript;
#endif
#ifdef ADFILTER
		BCheckBox*			mEnableAdFilter;
#endif
		BTextControl*		mDownloadDir;
};



BWindow* sPrefsWindow = NULL;


void DoPrefs() {
	if (sPrefsWindow)
		sPrefsWindow->Activate();
	else {
		BRect r(0,0,400,300);
		CenterWindowRect(&r);
		sPrefsWindow = new BWindow(r, kPrefWindowTitle, 
			  B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE);
		sPrefsWindow->AddChild(new PrefsView(sPrefsWindow->Bounds()));
		sPrefsWindow->Show();
	}
}



PrefsView::PrefsView(
	BRect	frame)
		: BView(frame, "PrefsView", B_FOLLOW_ALL, B_WILL_DRAW) 
{
	for (uint32 i = 0; i < kFontSettingCount; i++) {
		GetFontSettings(i, &fFontSettings[i]);
	}

	fCurFontSetting = &fFontSettings[0];
	fDirtyFontSettings = false;
}


void AddNewMenuItem(BMenu *menu, const char *itemName, uint32 command, bool isMarked = false)
{
	BMenuItem *item = new BMenuItem(itemName, new BMessage(command));
	menu->AddItem(item);
	if (isMarked)
		item->SetMarked(true);
}


BMenuField *AddPopupMenuField(const BRect rect, const char *name, const char *label, BView *parentView, BView *targetView, float *dividerJust, const StaticMenuItem *menuData, int32 markedItem)
{
	BPopUpMenu *menu = new BPopUpMenu("");
	menu->SetRadioMode(true);
	
	const StaticMenuItem *info = menuData;
	for (int32 i = 0; info->title && *info->title; i++, info = &menuData[i])
		AddNewMenuItem(menu, info->title, info->cmd, i == markedItem);
	
	menu->SetTargetForItems(targetView);

	BMenuField *menuField = new BMenuField(rect, name, label, menu);
	float divider = menuField->StringWidth(menuField->Label()) + 5;
	if (dividerJust)
		*dividerJust = (*dividerJust > divider) ? *dividerJust : divider;	
	menuField->SetAlignment(B_ALIGN_RIGHT);
	parentView->AddChild(menuField);
	
	return menuField;
}

BMenuField *AddFontFamilyMenuField(const BRect rect, const char *name, const char *label, BView *parentView, BView *targetView, float *dividerJust, uint32 command)
{
	int32 numFonts = count_font_families();
	StaticMenuItem *fontFamilies = new StaticMenuItem[numFonts + 1];
	font_family *familyNames = new font_family[numFonts];

	int32 i;
	for (i = 0; i < numFonts; i++) {
		get_font_family(i, &familyNames[i]);
		fontFamilies[i].title = familyNames[i];
		fontFamilies[i].cmd = command;
	}
	fontFamilies[i].title = "";
	
	BMenuField *retval = AddPopupMenuField(rect, name, label, parentView, targetView, dividerJust, fontFamilies, -1);
	delete[] fontFamilies;
	delete[] familyNames;
	return retval;
}


BMenuField *AddFontSizeMenuField(const BRect rect, const char *name, const char *label, BView *parentView, BView *targetView, float *dividerJust, uint32 command)
{
	const char **kFontSizes = GetPrefFontSizes();
	int32 numSizes;
	for (numSizes = 0; kFontSizes[numSizes]; numSizes++)
		;
	StaticMenuItem *fontFamilies = new StaticMenuItem[numSizes + 1];

	int32 i;
	for (i = 0; i < numSizes; i++) {
		fontFamilies[i].title = kFontSizes[i];
		fontFamilies[i].cmd = command;
	}
	fontFamilies[i].title = "";
	
	BMenuField *retval = AddPopupMenuField(rect, name, label, parentView, targetView, dividerJust, fontFamilies, -1);
	delete[] fontFamilies;
	return retval;
}

BTextControl *AddTextControl(const BRect rect, const char *name, const char *label, const char *value, BView *parentView, float *dividerJust, bool enabled)
{
	BTextControl *tc = new BTextControl(rect, name, label, value, NULL);
	float divider = tc->StringWidth(tc->Label()) + 5;
	if (dividerJust) {
		*dividerJust = (*dividerJust > divider) ? *dividerJust : divider;
		tc->SetDivider(*dividerJust);
	} else
		tc->SetDivider(divider);
	tc->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	tc->SetEnabled(enabled);
	parentView->AddChild(tc);
	return tc;
}




BView *SetupPrefsView(BTabView *tabView, BRect& bounds, const char *viewName)
{
	bounds = tabView->Frame();
	bounds.OffsetTo(0,0);
	BView *view = new BView(bounds, viewName, B_FOLLOW_ALL, B_WILL_DRAW | B_NAVIGABLE);
	view->SetViewColor(216, 216, 216);
	tabView->AddTab(view);
	return view;
}


enum {positionByRightBottom, positionByLeftTop};

BButton *MakeButton(BView *targetView, const char *name, int32 command, float h, float v, int orientation = positionByRightBottom)
{
	BRect frame(0, 0, 0, 0);
	BButton *btn = new BButton(frame, NULL, name, new BMessage(command));
	if (targetView)
		btn->SetTarget(targetView);
	btn->ResizeToPreferred();
	frame = btn->Frame();
	if (orientation == positionByRightBottom)
		frame.OffsetTo(h - frame.Width(),
					   v - frame.Height());
	else
		frame.OffsetTo(h, v);
	btn->MoveTo(frame.LeftTop());
	return btn;
}

BCheckBox *AddCheckBox(const BRect rect, const char *name, const char *label, BView *parentView, BView *targetView, int32 command, bool checked)
{
	BCheckBox *cb = new BCheckBox(rect, name, label, new BMessage(command));
	cb->SetTarget(targetView);
	cb->SetValue(checked);
	cb->ResizeToPreferred();
	if (parentView)
		parentView->AddChild(cb);
	return cb;
}

BStringView *AddStringView(const BRect rect, const char *label, BView *parentView)
{
	BStringView *sv = new BStringView(rect, "", label);
	parentView->AddChild(sv);
	return sv;
}


BBox *AddBox(const BRect rect, BView *parentView)
{
	BBox *b = new BBox(rect);
	parentView->AddChild(b);
	return b;
}


BBox *AddHorizontalLine(const BRect rect, BView *parentView)
{
	BRect bounds = rect;
	bounds.top = bounds.bottom = (bounds.top + bounds.bottom) / 2;
	bounds.bottom++;
	return AddBox(bounds, parentView);
}


BBox *AddVerticalLine(const BRect rect, BView *parentView)
{
	BRect bounds = rect;
	bounds.left = bounds.right = (bounds.left + bounds.right) / 2;
	bounds.right++;
	return AddBox(bounds, parentView);
}


void PrefsView::AddGeneralPrefsView(BTabView *tabView)
{
	BRect bounds;
	BView *view = SetupPrefsView(tabView, bounds, kPrefTabGeneral);
	float dividerJust1 = 0;
	float dividerJust2 = 0;

	BString dURL = gPreferences.FindString("DefaultURL");
	BString sURL = gPreferences.FindString("SearchURL");
	
	BRect rect = bounds;

	rect.InsetBy(15, 0);
	rect.top += 10;
	rect.bottom = rect.top + 20;
	
	if (gPreferences.FindBool("AllowChangeHomePage")) {
		mDefaultURL = AddTextControl(rect, "URL", kPrefHomePageLabel, dURL.String(), view, &dividerJust1, true);
	
		rect.OffsetBy(0, rect.Height() + 5);
	}
	
	mSearchURL = AddTextControl(rect, "sURL", kPrefSearchPageLabel, sURL.String(), view, &dividerJust1, true);
	
	rect.OffsetBy(0, rect.Height() + 5);
	BString newDefaultDir = gPreferences.FindString("DLDirectory");
	mDownloadDir = AddTextControl(rect, "DownloadDir", kDownloadDirLabel, newDefaultDir.String(), view, &dividerJust1, true);

	rect.OffsetBy(0, rect.Height() + 5);
	mNewWindowOption = gPreferences.FindInt32("NewWindowOption");
	BMenuField *mf = AddPopupMenuField(rect, "NewWindowOption", kPrefNewWindowLabel, view, this, &dividerJust1, GetPrefNewWindowOptions(), mNewWindowOption);

	rect.OffsetBy(0, rect.Height() + 7);
	mCookieOption = gPreferences.FindInt32("CookieOption");
	BMenuField *mf2 = AddPopupMenuField(rect, "CookieOption", kPrefCookieOptionLabel, view, this, &dividerJust1, GetPrefCookieOptions(), mCookieOption);

#ifdef JAVASCRIPT
	rect.OffsetBy(0, rect.Height() + 7);
	mEnableJavaScript = AddCheckBox(rect, "EnableJavaScript", kPrefEnableJavaScriptLabel, view, this, 0, gPreferences.FindBool("EnableJavaScript"));
	
	rect.OffsetBy(0, rect.Height() - 1);
	rect.InsetBy(-5,0);
	AddHorizontalLine(rect, view);
	
	char nc[6];
	sprintf(nc, "%ld", gPreferences.FindInt32("MaxConnections"));

	rect.OffsetBy(0, rect.Height());
	rect.InsetBy(5,0);
	rect.right = rect.left + 300;
	mNumConnections = AddTextControl(rect, "NumConnections", kPrefMaxConnectionsLabel, nc, view, &dividerJust2, true);
#else
	rect.OffsetBy(0, rect.Height() + 9);
	rect.InsetBy(-5,0);
	AddHorizontalLine(rect, view);
	
//	sprintf(nc, "%ld", gPreferences.FindInt32("MaxConnections"));

	rect.OffsetBy(0, rect.Height() + 5);
	rect.InsetBy(5,0);
	rect.right = rect.left + 300;
//	mNumConnections = AddTextControl(rect, "NumConnections", kPrefMaxConnectionsLabel, nc, view, &dividerJust2, true);
	mAutoLaunch = AddCheckBox(rect, "AutoLaunch", kPrefAutoLaunchAfterDownload, view, this, 0, gPreferences.FindBool("AutoLaunchDownloadedFiles"));
#endif
	
	char nc[6];
	sprintf(nc, "%ld", gPreferences.FindInt32("HistoryLength"));
	rect.OffsetBy(0, rect.Height() + 5);
	mHistoryLength = AddTextControl(rect, "HistoryLength", kPrefHistoryLengthLabel, nc, view, &dividerJust2, true);
	
	if (gPreferences.FindBool("AllowChangeHomePage"))
		mDefaultURL->SetDivider(dividerJust1);
	mSearchURL->SetDivider(dividerJust1);
	mDownloadDir->SetDivider(dividerJust1);
	mf->SetDivider(dividerJust1 + 2);
	mf2->SetDivider(dividerJust1 + 2);
//	mNumConnections->SetDivider(dividerJust2);
	mHistoryLength->SetDivider(dividerJust2);
}


void PrefsView::AddDisplayPrefsView(BTabView *tabView)
{
	float dividerJust = 0;

	BRect bounds;
	BView *view = SetupPrefsView(tabView, bounds, kPrefTabDisplay);

	BRect rect = bounds;
	rect.left = bounds.left + 15;
	rect.right = rect.left + 185;
	rect.top += 10;
	rect.bottom = rect.top + 20;	
	BMenuField *encodingsField = AddPopupMenuField(rect, NULL, kPrefEncodingPopupLabel, view, this, &dividerJust, GetPrefEncodings(), 0);

	rect.OffsetBy(0, rect.Height() + 5);
	mProField = AddFontFamilyMenuField(rect, "ProFamily", kPrefFontProLabel, view, this, &dividerJust, msg_ProFamilySelected);

	rect.left = rect.right + 10;
	rect.right = rect.left + 75;
	mProSizeField = AddFontSizeMenuField(rect, "ProSize", kPrefFontSizeLabel, view, this, NULL, msg_ProSizeSelected);

	rect.left = rect.right + 10;
	rect.right = bounds.right - 10;
	mMinProSizeField = AddFontSizeMenuField(rect, "MinProSize", kPrefFontMinSizeLabel, view, this, NULL, msg_MinProSizeSelected);

	rect.OffsetBy(0, rect.Height() + 5);
	rect.left = bounds.left + 15;
	rect.right = rect.left + 185;
	mFixedField = AddFontFamilyMenuField(rect, "FixFamily", kPrefFontFixLabel, view, this, &dividerJust, msg_FixFamilySelected);

	rect.left = rect.right + 10;
	rect.right = rect.left + 75;
	mFixedSizeField = AddFontSizeMenuField(rect, "FixSize", kPrefFontSizeLabel, view, this, NULL, msg_FixSizeSelected);

	rect.left = rect.right + 10;
	rect.right = bounds.right - 10;
	mMinFixSizeField = AddFontSizeMenuField(rect, "MinFixSize", kPrefFontMinSizeLabel, view, this, NULL, msg_MinFixSizeSelected);

	rect.OffsetBy(0, rect.Height());
	rect.left = bounds.left + 10;
	rect.right = bounds.right - 10;
	AddHorizontalLine(rect, view);
	
	rect.InsetBy(5,0);
	rect.bottom = rect.top + 15;
	rect.OffsetBy(0, rect.Height());
	float top = rect.top;
	
	mShowImages = AddCheckBox(rect, "ShowImages", kPrefShowImagesLabel, view, this, 0, gPreferences.FindBool("ShowImages"));

	rect.OffsetBy(0, rect.Height() + 5);
	mShowBGImages = AddCheckBox(rect, "ShowBGImages", kPrefShowBGImagesLabel, view, this, 0, gPreferences.FindBool("ShowBGImages"));

	rect.OffsetBy(0, rect.Height() + 5);
	mShowAnimations = AddCheckBox(rect, "ShowAnimations", kPrefShowAnimationsLabel, view, this, 0, gPreferences.FindBool("ShowAnimations"));

	rect.OffsetBy(0, rect.Height() + 5);
	mUnderlineLinks = AddCheckBox(rect, "UnderlineLinks", kPrefUnderlineLinksLabel, view, this, 0, gPreferences.FindBool("UnderlineLinks"));

	rect.OffsetBy(0, rect.Height() + 5);
	mHaiku = AddCheckBox(rect, "HaikuErrors", kPrefHaikuLabel, view, this, 0, gPreferences.FindBool("HaikuErrorMessages"));

	float bottom = rect.bottom;
	rect.left = (bounds.right - bounds.left) / 2 + 10;
	rect.right = bounds.right - 10;
	rect.top = top;
	rect.bottom = rect.top + 15;
	mUseFonts = AddCheckBox(rect, "UseFonts", kPrefUseFontsLabel, view, this, 0, gPreferences.FindBool("UseFonts"));

	rect.OffsetBy(0, rect.Height() + 5);
	mUseBGColors = AddCheckBox(rect, "UseBGColors", kPrefUseBGColorsLabel, view, this, 0, gPreferences.FindBool("UseBGColors"));

	rect.OffsetBy(0, rect.Height() + 5);
	mUseFGColors = AddCheckBox(rect, "UseFGColors", kPrefUseFGColorsLabel, view, this, 0, gPreferences.FindBool("UseFGColors"));

	rect.OffsetBy(0, rect.Height() + 5);
	mPlaySounds = AddCheckBox(rect, "PlaySounds", kPrefPlaySoundsLabel, view, this, 0, gPreferences.FindBool("PlaySounds"));
	
	rect.OffsetBy(0, rect.Height() + 5);
	mOffscreen = AddCheckBox(rect, "Offscreen", kPrefOffscreenLabel, view, this, 0, gPreferences.FindInt32("DrawOffscreen"));	

	rect.top = top;
	rect.left = rect.right = (bounds.right - bounds.left) / 2;
	rect.bottom = bottom;
	AddVerticalLine(rect, view);
	
	encodingsField->SetDivider(dividerJust);
	mProField->SetDivider(dividerJust);
	mFixedField->SetDivider(dividerJust);

	ResetMenus(fCurFontSetting);
}


void PrefsView::AddProxyPrefsView(BTabView *tabView)
{
	float dividerJust = 0;

	BRect bounds, rect;
	BView *view = SetupPrefsView(tabView, bounds, kPrefTabConnections);

	bool enabled = HTTP::ProxyActive() && FTP::ProxyActive();

	BRect boxRect;
	boxRect.left = bounds.left + 15;
	boxRect.top = bounds.top + 10;
	boxRect.right = bounds.right - 15;
	boxRect.bottom = rect.top + 100;	

	BBox *box = AddBox(boxRect, view);
	
	mEnableProxies = AddCheckBox(rect, "EnableProxy", kPrefEnableProxiesLabel, NULL, this, msg_ToggleProxy, enabled);
	box->SetLabel(mEnableProxies);

	BString	hName;
	int		hPort = 0;
	HTTP::GetProxyNameAndPort(hName, &hPort, &mSecureProxyPort);
	char hPortStr[6] = "";
	if (hPort != 0)
		sprintf(hPortStr, "%d", hPort);

	rect.left = 15;
	rect.top = 25;
	rect.right = rect.left + 250;
	rect.bottom = rect.top + 20;
	mHTTPName = AddTextControl(rect, "HTTPName", kPrefProxyHTTPLabel, hName.String(), box, &dividerJust, enabled);

	rect.OffsetBy(rect.Width() + 10, 0);
	rect.right = rect.left + 80;
	mHTTPPort = AddTextControl(rect, "HTTPPort", kPrefProxyPortLabel, hPortStr, box, NULL, enabled);

	BString	fName;
	int		fPort = 0;
	FTP::GetProxyNameAndPort(fName, &fPort);
	char fPortStr[6] = "";
	if (fPort != 0)
		sprintf(fPortStr, "%d", fPort);

	rect.left =  15;
	rect.top = rect.bottom + 10;
	rect.right = rect.left + 250;
	rect.bottom = rect.top + 20;
	mFTPName = AddTextControl(rect, "FTPName", kPrefProxyFTPLabel, fName.String(), box, &dividerJust, enabled);

	rect.OffsetBy(rect.Width() + 10, 0);
	rect.right = rect.left + 80;
	mFTPPort = AddTextControl(rect, "FTPPort", kPrefProxyPortLabel, fPortStr, box, NULL, enabled);
	
	float dividerJust2 = 0;
	char nc[6];
	sprintf(nc, "%ld", gPreferences.FindInt32("MaxConnections"));
	rect = box->Frame();
	rect.OffsetBy(0.0, box->Bounds().Height() + 10.0);
	rect.right = rect.left + StringWidth(kPrefMaxConnectionsLabel) + 60.0;
	mNumConnections = AddTextControl(rect, "NumConnections", kPrefMaxConnectionsLabel, nc, view, &dividerJust2, true);
	mNumConnections->SetDivider(dividerJust2);
}


void PrefsView::AddCachePrefsView(BTabView *tabView)
{
	float dividerJust = 0;
	BRect bounds, rect;
	BView *view = SetupPrefsView(tabView, bounds, kPrefTabCache);

	rect = bounds;
	rect.InsetBy(15,10);
	rect.bottom = rect.top + 20;
	
	uint64 cacheSize = UResourceCache::GetMaxCacheSize() / (1024 * 1024);
	char cs[6];

	mCacheLocation = AddTextControl(rect, "CacheLoc", kPrefCacheLocationLabel, UResourceCache::GetCacheLocation(), view, &dividerJust, true);

	rect.OffsetBy(0, rect.Height() + 5);
	mCacheOption = UResourceCache::GetCacheOption();
	BMenuField *mf = AddPopupMenuField(rect, "CacheOption", kPrefCacheRefreshLabel, view, this, &dividerJust, GetPrefCacheOptions(), mCacheOption);

	rect.OffsetBy(0, rect.Height() + 9);
	sprintf(cs, "%Ld", cacheSize);
	mCacheSize = AddTextControl(rect, "CacheSize", kPrefCacheSizeLabel, cs, view, &dividerJust, true);

	mCacheLocation->SetDivider(dividerJust);
	mf->SetDivider(dividerJust + 2);
	mCacheSize->SetDivider(dividerJust);
	

	rect.OffsetBy(0, rect.Height() + 5);
	BButton *clr = MakeButton(this, kPrefCacheClearButtonTitle,	msg_ClearCacheNow,	dividerJust + 17, rect.top, positionByLeftTop);
	view->AddChild(clr);
	
}


/*
void PrefsView::AddCookiePrefsView(BTabView *tabView)
{
	BRect bounds;
	BView *view = SetupPrefsView(tabView, bounds, kPrefTabCookies);
}
*/


#ifdef ADFILTER
void PrefsView::AddAdFilterPrefsView(BTabView *tabView)
{
	BRect bounds, rect;
	BView *view = SetupPrefsView(tabView, bounds, kPrefTabAdFilter);
	
	rect.left = bounds.left + 15;
	rect.top = bounds.top + 20;
	rect.right = bounds.right - 15;
	rect.bottom = rect.top + 20;	

	bool enabled = gPreferences.FindBool("FilterEnabled");
	mEnableAdFilter = AddCheckBox(rect, "EnableAdFilter", kPrefEnableAdFilterLabel, view, this, 0, enabled);
		
	BButton *show = MakeButton(this, kPrefShowAdFilters, msg_ShowFilters, rect.right, rect.bottom);
	view->AddChild(show);
}
#endif

void PrefsView::AddSecurityPrefsView(BTabView *tabView)
{
	BRect bounds;
	BView *view = SetupPrefsView(tabView, bounds, kPrefTabSecurity);


	BRect rect = bounds;
	rect.InsetBy(5,10);
	rect.bottom = rect.top + 20;
	mFormSubmitOption = gPreferences.FindInt32("FormSubmitOption");
	AddPopupMenuField(rect, "FormSubmitOption", kPrefFormSubmitLabel, view, this, NULL, GetPrefFormSubmitOptions(), mFormSubmitOption);
	
	rect.OffsetBy(10, rect.Height() + 2);
	mSSLEnter = AddCheckBox(rect, "SSLEnter", kPrefSecureEnterLabel, view, this, 0, gPreferences.FindBool("SSLEnterWarning"));

	rect.OffsetBy(0, rect.Height());
	mSSLLeave = AddCheckBox(rect, "SSLLeave", kPrefSecureLeaveLabel, view, this, 0, gPreferences.FindBool("SSLLeaveWarning"));
}


void
PrefsView::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetViewColor(216, 216, 216);

	BRect bounds = Bounds();	
	bounds.InsetBy(0,10);
	bounds.bottom -= 35;
	
	BTabView *tabView = new BTabView(bounds, "", B_WIDTH_FROM_LABEL, B_FOLLOW_ALL, B_WILL_DRAW | B_NAVIGABLE);
	
	AddGeneralPrefsView(tabView);
	AddDisplayPrefsView(tabView);
//	AddCookiePrefsView(tabView);
	AddProxyPrefsView(tabView);
	AddCachePrefsView(tabView);
#ifdef ADFILTER
	AddAdFilterPrefsView(tabView);
#endif
	AddSecurityPrefsView(tabView);
	
	AddChild(tabView);

	bounds = Bounds();
	
	BButton *ok  = MakeButton(this, kOKButtonTitle,		msg_WritePrefs,		bounds.right - 10, bounds.bottom - 10);
	BButton *can = MakeButton(NULL, kCancelButtonTitle,	B_CLOSE_REQUESTED,	ok->Frame().left - 10, bounds.bottom - 10);
	AddChild(can);
	AddChild(ok);
	ok->MakeDefault(true);
//	defaultURL->MakeFocus(true);
}

void PrefsView::DetachedFromWindow()
{
	sPrefsWindow = NULL;
}


void
PrefsView::MessageReceived(
	BMessage	*message)
{
	switch (message->what) {
		case msg_WritePrefs:
			ApplyPrefs();
			Window()->PostMessage(B_CLOSE_REQUESTED);
			break;

		case msg_ToggleProxy:
		{	
			BCheckBox *cb = NULL;
			if (message->FindPointer("source", (void **)&cb) == B_NO_ERROR) {
				bool enabled = cb->Value() != 0;
				pprint("Enabled = %d", enabled);

				mHTTPName->SetEnabled(enabled);
				mHTTPPort->SetEnabled(enabled);
				mFTPName->SetEnabled(enabled);
				mFTPPort->SetEnabled(enabled);
			}
			break;
		}
		
#ifdef ADFILTER
		case msg_ShowFilters:
			AdFilter::ShowAdFilters();
			break;
#endif

		case msg_WesternEncoding:
		case msg_JapaneseEncoding:
		case msg_UnicodeEncoding:
		case msg_GreekEncoding:
		case msg_CyrillicEncoding:
		case msg_CEEncoding:
			switch(message->what) {
				case msg_WesternEncoding:
					fCurFontSetting = &fFontSettings[0];
					break;		
				case msg_JapaneseEncoding:
					fCurFontSetting = &fFontSettings[1];
					break;			
				case msg_UnicodeEncoding:
					fCurFontSetting = &fFontSettings[2];
					break;			
				case msg_GreekEncoding:
					fCurFontSetting = &fFontSettings[3];
					break;			
				case msg_CyrillicEncoding:
					fCurFontSetting = &fFontSettings[4];
					break;			
				case msg_CEEncoding:
					fCurFontSetting = &fFontSettings[5];
					break;			
			}
			ResetMenus(fCurFontSetting);
			break;

		case msg_ProFamilySelected:
		{
			BMenuItem *item = NULL;
			if (message->FindPointer("source", (void **)&item) == B_NO_ERROR) {
				strcpy(fCurFontSetting->proFamily, item->Label());	
				fDirtyFontSettings = true;	
			}
			break;
		}

		case msg_ProSizeSelected:
		{
			BMenuItem *item = NULL;
			if (message->FindPointer("source", (void **)&item) == B_NO_ERROR) {
				fCurFontSetting->proSize = atoi(item->Label());
				fDirtyFontSettings = true;
			}
			break;
		}


		case msg_FixFamilySelected:
		{
			BMenuItem *item = NULL;
			if (message->FindPointer("source", (void **)&item) == B_NO_ERROR) {
				strcpy(fCurFontSetting->fixFamily, item->Label());		
				fDirtyFontSettings = true;
			}
			break;
		}

		case msg_FixSizeSelected:
		{
			BMenuItem *item = NULL;
			if (message->FindPointer("source", (void **)&item) == B_NO_ERROR) {
				fCurFontSetting->fixSize = atoi(item->Label());
				fDirtyFontSettings = true;
			}
			break;
		}
		
		case msg_MinFixSizeSelected:
		{
			BMenuItem *item = NULL;
			if (message->FindPointer("source", (void **)&item) == B_NO_ERROR) {
				fCurFontSetting->fixMinSize = atoi(item->Label());
				fDirtyFontSettings = true;
			}
			break;
		}
			
		case msg_MinProSizeSelected:
		{
			BMenuItem *item = NULL;
			if (message->FindPointer("source", (void **)&item) == B_NO_ERROR) {
				fCurFontSetting->proMinSize = atoi(item->Label());
				fDirtyFontSettings = true;
			}
			break;
		}

		case msg_CacheEveryTime:
			mCacheOption = 0;
			break;
			
		case msg_CacheOncePerSession:
			mCacheOption = 1;
			break;
		
		case msg_CacheOncePerDay:
			mCacheOption = 2;
			break;
			
		case msg_CacheNever:
			mCacheOption = 3;
			break;	
			
		case msg_ClearCacheNow:
			UResourceCache::EraseCache();
			break;
			
		case msg_NewWindowClone:
			mNewWindowOption = 0;
			break;
			
		case msg_NewWindowHome:
			mNewWindowOption = 1;
			break;
			
		case msg_NewWindowBlank:
			mNewWindowOption = 2;
			break;
			
		case msg_CookieAccept:
			mCookieOption = 0;
			break;
			
		case msg_CookieReject:
			mCookieOption = 1;
			break;
			
		case msg_CookieAsk:
			mCookieOption = 2;
			break;
			
		case msg_FormSubmitNever:
			mFormSubmitOption = 0;
			break;
			
		case msg_FormSubmitOneLine:
			mFormSubmitOption = 1;
			break;
		
		case msg_FormSubmitAlways:
			mFormSubmitOption = 2;
			break;

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
PrefsView::ApplyPrefs()
{
	if (gPreferences.FindBool("AllowChangeHomePage"))
		gPreferences.ReplaceString("DefaultURL", mDefaultURL->Text());
	gPreferences.ReplaceString("SearchURL", mSearchURL->Text());
	
	
	gPreferences.ReplaceBool("AutoLaunchDownloadedFiles", mAutoLaunch->Value());

	int historyLength = atoi(mHistoryLength->Text());
	historyLength = MIN(historyLength, 30);
	historyLength = MAX(historyLength, 0);
	gPreferences.ReplaceInt32("HistoryLength", historyLength);

	for (uint32 i = 0; i < kFontSettingCount; i++) {
		SetFontSettings(i, &fFontSettings[i]);
	}

	bool enable = mEnableProxies->Value() != 0;

	HTTP::SetProxyNameAndPort(mHTTPName->Text(), atoi(mHTTPPort->Text()), mSecureProxyPort);
	HTTP::SetProxyActive(enable);

	FTP::SetProxyNameAndPort(mFTPName->Text(), atoi(mFTPPort->Text()));
	FTP::SetProxyActive(enable);

	int numConnections = atoi(mNumConnections->Text());
	numConnections = MAX(numConnections, 1);
	numConnections = MIN(numConnections, 16);
	gPreferences.ReplaceInt32("MaxConnections", numConnections);

	if (fDirtyFontSettings) {
		be_app->PostMessage(msg_ReloadAll);
		fDirtyFontSettings = false;
	}
	int cacheSize = atoi(mCacheSize->Text());
	if (cacheSize < 1)
		cacheSize = 1;
	UResourceCache::SetMaxCacheSize((uint64)cacheSize * 1048576);
	UResourceCache::SetCacheOption(mCacheOption);
	UResourceCache::SetCacheLocation(mCacheLocation->Text());
	
	
	gPreferences.ReplaceBool("ShowImages", mShowImages->Value());
	gPreferences.ReplaceBool("ShowBGImages", mShowBGImages->Value());
	gPreferences.ReplaceBool("UseBGColors", mUseBGColors->Value());
	gPreferences.ReplaceBool("UseFGColors", mUseFGColors->Value());
	gPreferences.ReplaceBool("ShowAnimations", mShowAnimations->Value());
	gPreferences.ReplaceBool("UseFonts", mUseFonts->Value());
	gPreferences.ReplaceBool("UnderlineLinks", mUnderlineLinks->Value());
	gPreferences.ReplaceBool("PlaySounds", mPlaySounds->Value());
	gPreferences.ReplaceBool("HaikuErrorMessages", mHaiku->Value());
	gPreferences.ReplaceInt32("DrawOffscreen", mOffscreen->Value() ? offscreenAlways : offscreenNever);
#ifdef JAVASCRIPT
	gPreferences.ReplaceBool("EnableJavaScript", mEnableJavaScript->Value());
#endif
#ifdef ADFILTER
	gPreferences.ReplaceBool("FilterEnabled", mEnableAdFilter->Value());
#endif
	
	BString downloadDir = mDownloadDir->Text();
	if (downloadDir.Length() > 0) {
		if (downloadDir[downloadDir.Length() - 1] != '/')
			downloadDir += "/";
	} else {
		BPath settingsPath;
		find_directory(B_USER_DIRECTORY, &settingsPath);
		downloadDir = settingsPath.Path();
		downloadDir += kDefaultDownloadDirectory;
	}
	gPreferences.ReplaceString("DLDirectory", downloadDir.String());
	gPreferences.ReplaceInt32("NewWindowOption", mNewWindowOption);
	
	gPreferences.ReplaceInt32("FormSubmitOption", mFormSubmitOption);
	gPreferences.ReplaceBool("SSLEnterWarning", mSSLEnter->Value());
	gPreferences.ReplaceBool("SSLLeaveWarning", mSSLLeave->Value());
	
	gPreferences.ReplaceInt32("CookieOption", mCookieOption);
	
	NetPositive::WritePrefs();
}


void
PrefsView::MarkMenu(
	BMenu		*menu,
	const char	*mark)
{
	int32 count = menu->CountItems();
	for (int32 i = 0; i < count; i++) {
		BMenuItem *item = menu->ItemAt(i);
		if (strcmp(item->Label(), mark) == 0) {
			item->SetMarked(true);
			return;
		}
	}
}


void
PrefsView::MarkMenu(
	BMenu	*menu,
	int32	mark)
{
	int32 count = menu->CountItems();
	for (int32 i = 0; i < count; i++) {
		BMenuItem *item = menu->ItemAt(i);
		if (atoi(item->Label()) == mark) {
			item->SetMarked(true);
			return;
		}
	}
}


void
PrefsView::ResetMenus(
	FontSetting	*setting)
{
	MarkMenu(mProField->Menu(), setting->proFamily);
	MarkMenu(mProSizeField->Menu(), setting->proSize);
	MarkMenu(mMinProSizeField->Menu(), setting->proMinSize);
	MarkMenu(mFixedField->Menu(), setting->fixFamily);
	MarkMenu(mFixedSizeField->Menu(), setting->fixSize);
	MarkMenu(mMinFixSizeField->Menu(), setting->fixMinSize);
}

static const char *GetLangName(int index)
{
	switch(index) {
		case 0:			return "Western";
		case 1:			return "Japanese";
		case 2:			return "Unicode";
		case 3:			return "Greek";
		case 4:			return "Cyrillic";
		case 5:			return "CentralEuropean";
		default:		return "";
	}
}

void GetFontSettings(int index, struct FontSetting *fs)
{
	const char *langName = GetLangName(index);

	BString str = langName;
	str += "ProFace";
	strcpy(fs->proFamily, gPreferences.FindString(str.String()));
	
	str = langName;
	str += "ProSize";
	fs->proSize = gPreferences.FindInt32(str.String());
	
	str = langName;
	str += "ProMinSize";
	fs->proMinSize = gPreferences.FindInt32(str.String());

	str = langName;
	str += "FixFace";
	strcpy(fs->fixFamily, gPreferences.FindString(str.String()));
	
	str = langName;
	str += "FixSize";
	fs->fixSize = gPreferences.FindInt32(str.String());
	
	str = langName;
	str += "FixMinSize";
	fs->fixMinSize = gPreferences.FindInt32(str.String());
}

void SetFontSettings(int index, struct FontSetting *fs)
{
	const char *langName = GetLangName(index);

	BString str = langName;
	str += "ProFace";
	gPreferences.ReplaceString(str.String(), fs->proFamily);
	
	str = langName;
	str += "ProSize";
	gPreferences.ReplaceInt32(str.String(), fs->proSize);
	
	str = langName;
	str += "ProMinSize";
	gPreferences.ReplaceInt32(str.String(), fs->proMinSize);

	str = langName;
	str += "FixFace";
	gPreferences.ReplaceString(str.String(), fs->fixFamily);
	
	str = langName;
	str += "FixSize";
	gPreferences.ReplaceInt32(str.String(), fs->fixSize);
	
	str = langName;
	str += "FixMinSize";
	gPreferences.ReplaceInt32(str.String(), fs->fixMinSize);
}
