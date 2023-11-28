//
//	FontPanel.h
//
//	pavel 3/19/97
// 
//	(c) 1997 Be Incorporated

#include <Debug.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Font.h>
#include <MenuField.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Slider.h>
#include <StringView.h>
#include <TabView.h>
#include <Window.h>

//*********************************************************************

class TButtonBar : public BView {
public:
						TButtonBar(BRect frame, bool defaultsBtn=true, bool revertBtn=true,
							bool drawBorder=true);
						~TButtonBar();
				
		void			Draw(BRect);
		
		void			AddButton(const char* title, BMessage* m);
		
		void			CanRevert(bool state);

		void			SetTarget(BMessenger m);

private:
		bool			fHasDefaultsBtn;
		BButton*		fDefaultsBtn;
		bool			fHasRevertBtn;
		BButton*		fRevertBtn;
		bool			fHasOtherBtn;
		BButton*		fOtherBtn;
		bool			fDrawBorder;
};

//*********************************************************************

class FontSelector {
	// abstract class representing a generic font attribute selector
	// FontSelectionMenu uses FontSelectors to operate on a font;
	// FontSelectors send messages to the FontSelectionMenu when a font
	// attribute is changed; 
public:
	virtual void ShowCurrent(const BFont *) = 0;
		// gets called when the font has been changed and the selector
		// needs to update itself
};

class RevertNotifier {
public:
	virtual void CanRevert(bool r) = 0;
};

//*********************************************************************

class FontMenu : public BPopUpMenu, public FontSelector {
public:
	FontMenu(const BFont *, bool fixed = FALSE);

virtual void ShowCurrent(const BFont *);
		// set current font name and force redraw
	
protected:
virtual	BPoint	ScreenLocation();

private:

	void SetCurrent(const BFont *);

	// utility calls for font name composing
	static void FontName(char *, int32 bufferSize, font_family, font_style, 
		const char * divider = " ", bool addRegular = false);
	static void FontName(char *, int32 bufferSize, const BFont *, const char *divider = " ", 
		bool addRegular = false);
	static const BFont *FontFromName(font_family, font_style);
};

class FontPopUpMenu : public BMenuField, public FontSelector {
	// a hierarchical menu of fonts grouped in families with submenus
	// for styles
public:
	FontPopUpMenu(BRect, float divider, const BFont *, const char *title, 
		bool fixed);

virtual void ShowCurrent(const BFont *);
		// font has changed, update yourself
};

//const float defaultSizes[] = {9.0, 10.0, 11.0, 12.0};
const int32 defaultSizes[] = {9, 10, 11, 12};

class FontSizePopUpMenu : public BMenuField, public FontSelector {
public:
	FontSizePopUpMenu(BRect, float divider, const BFont *);

virtual void ShowCurrent(const BFont *);
		// font has changed, update yourself
private:

	void 				SetCurrent(const BFont *);
};

//*********************************************************************

class SampleText : public BView {
	// used to display the font, selected by a font selector
public:
						SampleText(BRect, const char *, const BFont *);

	void 				Draw(BRect);
	
	void 				FontChanged(const BFont *);
private:
};

//*********************************************************************

class FontSelectionMenu : public BHandler
{
public:

							FontSelectionMenu(BRect, const BFont *, const char *,
								BView* container, RevertNotifier *n, bool);

		virtual				~FontSelectionMenu();

		void				SetTarget(BMessenger m);

		BRect 				Bounds() const;
		
		void 				SetCurrentFont(const BFont *);
		void 				AddAsChild(BView *);
		void 				RemoveAsChild(BView *);

virtual void 				Apply();
virtual void 				SetDefault();
virtual void 				Revert();
	
		void 				SetSelectedFont(const BFont *);
		// set the font and call FontSelected

		BFont 				font;
		BFont 				fOriginalFont;
		// the current state of the font we are editing

		RevertNotifier		*rev;
private:

		void 				MessageReceived(BMessage *);
			// respond to messages from FontSelectors
		
virtual void 				FontSelected(const BFont *font);
			// override to do something usefull
		
		FontPopUpMenu*		fontFamilyAndStyleSelector;
		FontSizePopUpMenu*	fontSizeSelector;
		SampleText*			sample;
};

//*********************************************************************

// the following classes map FontSelectionMenu and the three default font
// settings

class StandardFontSettingsMenu : public FontSelectionMenu {
public:
							StandardFontSettingsMenu(BRect rect, int32 index, const char *label,
								BView* container, RevertNotifier *n);
							~StandardFontSettingsMenu() {}

		void 				FontSelected(const BFont *font);

		void 				Apply();
		void 				SetDefault();
private:
		int32 				standardFontSettingsIndex;
};

//*********************************************************************

class PlainFontMenu : public StandardFontSettingsMenu {
public:
	PlainFontMenu(BRect rect, BView* container, RevertNotifier *n)
		:	StandardFontSettingsMenu(rect, 0, "Plain font:", container, n)
		{}
};

class BoldFontMenu : public StandardFontSettingsMenu {
public:
	BoldFontMenu(BRect rect, BView* container, RevertNotifier *n)
		:	StandardFontSettingsMenu(rect, 1, "Bold font:", container, n)
		{}
};
 
class FixedFontMenu : public StandardFontSettingsMenu {
public:
	FixedFontMenu(BRect rect, BView* container, RevertNotifier *n)
		:	StandardFontSettingsMenu(rect, 2, "Fixed font:", container, n)
		{}
};

//*********************************************************************

class TCustomSlider : public BSlider {
public:

						TCustomSlider(BRect rect, const char *name, BMessage *msg,
							int32 minValue, int32 maxValue, int mask);
						~TCustomSlider();

	void 				AttachedToWindow();
	
	void				DrawText();
	
	void 				KeyDown(const char* bytes, int32 numByte);
	status_t 			Invoke(BMessage *msg=NULL);
	
	void 				Apply();
	void 				SetDefault();
	void 				Revert(int32);

	char* 				UpdateText() const;
private:
	int  				fMask;
	char*				fStatusStr;
};

//*********************************************************************

class FontPanel : public BView, public RevertNotifier {
public:
						FontPanel();

	void				AttachedToWindow();
	void				DetachedFromWindow();
	void 				MessageReceived(BMessage*);
	void 				AddButtons();
	void 				BuildFontControls();
	void 				AddFontControls();
	void 				BuildCacheControls();
	
	void			 	UpdateMenus();

	void 				Apply();
	void 				SetDefaults();
	void 				Revert();
	void 				CanRevert(bool state);

private:

	BBox*				fBG;
	BTabView*			fContainer;
	TButtonBar*			fBtnBar;
	
	BView*				fFontContainer;
	FontSelectionMenu*	fPlainFontSelector;
	FontSelectionMenu*	fBoldFontSelector;
	FontSelectionMenu*	fFixedFontSelector;

	BView*				fCacheContainer;	
	TCustomSlider*		fScreenCacheSlider;
	TCustomSlider*		fPrintingCacheSlider;
	BButton*			fSaveCacheBtn;
	
	int32				fInitialScreenCache;
	int32				fInitialPrintingCache;	
};

