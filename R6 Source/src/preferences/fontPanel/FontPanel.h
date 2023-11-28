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

//*********************************************************************

class FontMenu : public BPopUpMenu, public FontSelector {
public:
	FontMenu(const BFont *, const BHandler *target,
			 bool fixed = false, bool family_only = false);

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
		const BHandler *target, bool fixed, bool family_only=false);

virtual void ShowCurrent(const BFont *);
		// font has changed, update yourself
};

//const float defaultSizes[] = {9.0, 10.0, 11.0, 12.0};
const int32 defaultSizes[] = {9, 10, 11, 12};

class FontSizePopUpMenu : public BMenuField, public FontSelector {
public:
	FontSizePopUpMenu(BRect, float divider, const BFont *, 
		const BHandler *target);

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
	void 				MouseDown(BPoint);
	void	 			Pulse();
	
	void 				FontChanged(const BFont *);
private:
	bool 				fAlternate;
	time_t 				fTime;
};

//*********************************************************************

class FontSelectionMenu : public BHandler {
	
public:

							FontSelectionMenu(BRect, const BFont *, const char *,
								BView *target, BView* container, bool);
		// passing BView here so that we can target the handler before we 
		// display it to work around a problem with not being able to 
		// target handlers without a looper

							~FontSelectionMenu();

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
	
		BView*				Parent() { return fParent; }
private:

		void 				MessageReceived(BMessage *);
			// respond to messages from FontSelectors
		
virtual void 				FontSelected(const BFont *font);
			// override to do something usefull
		
		FontPopUpMenu*		fontFamilyAndStyleSelector;
		FontSizePopUpMenu*	fontSizeSelector;
		SampleText*			sample;
		BView*				fParent;
		typedef BHandler inherited;
};

//*********************************************************************

// the following classes map FontSelectionMenu and the three default font
// settings

class StandardFontSettingsMenu : public FontSelectionMenu {
public:
							StandardFontSettingsMenu(BRect rect, font_which index, const char *label,
								BView *target,
								BView* container);
							~StandardFontSettingsMenu() {}

		void 				FontSelected(const BFont *font);

		void 				Apply();
		void 				SetDefault();
private:
		font_which			standardFontSettingsIndex;
};

//*********************************************************************

class PlainFontMenu : public StandardFontSettingsMenu {
public:
	PlainFontMenu(BRect rect, BView *target, BView* container)
		:	StandardFontSettingsMenu(rect, B_PLAIN_FONT, "Plain font:", target, container)
		{}
};

class BoldFontMenu : public StandardFontSettingsMenu {
public:
	BoldFontMenu(BRect rect, BView *target, BView* container)
		:	StandardFontSettingsMenu(rect, B_BOLD_FONT, "Bold font:", target, container)
		{}
};
 
class FixedFontMenu : public StandardFontSettingsMenu {
public:
	FixedFontMenu(BRect rect, BView *target, BView* container)
		:	StandardFontSettingsMenu(rect, B_FIXED_FONT, "Fixed font:", target, container)
		{}
};

class SerifFontMenu : public StandardFontSettingsMenu {
public:
	SerifFontMenu(BRect rect, BView *target, BView* container)
		:	StandardFontSettingsMenu(rect, B_SERIF_FONT, "Serif font:", target, container)
		{}
};

//*********************************************************************

class TUpdateSlider : public BSlider {
public:

						TUpdateSlider(BRect rect, const char *name, const char *label,
							BMessage *msg, int32 minValue, int32 maxValue,
							thumb_style thumbType = B_BLOCK_THUMB,
							uint32 resizingMode = B_FOLLOW_LEFT |
												B_FOLLOW_TOP,
							uint32 flags = B_NAVIGABLE | B_WILL_DRAW |
												B_FRAME_EVENTS);
						~TUpdateSlider();

	void				SetUpdateText(const char* text);
	char* 				UpdateText() const;
	
private:
	BString				fUpdateProto;
	char				fUpdateBuf[64];
};

//*********************************************************************

class TCustomSlider : public TUpdateSlider {
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

private:
	int  				fMask;
	char*				fStatusStr;
};

//*********************************************************************

class FontPanel : public BWindow {
public:
						FontPanel();
						~FontPanel();
	
	
	void 				MessageReceived(BMessage*);
	bool 				QuitRequested();
	void 				AddButtons();
	
	void 				GetPrefs();
	void 				SetPrefs();

	void 				BuildFontControls();
	void 				AddFontControls();
	void 				BuildOverlayControls();
	void 				BuildRenderControls();
	void 				BuildCacheControls();
	
	void				RefreshOverlayList(bool is_global);
	void				SelectOverlay(uint32 code);
	void				RefreshCurrentOverlay();
	void				InstallCurrentOverlay(bool read_controls=true);
	
	void				RefreshRenderControls(int32 antialising, int32 hinting);
	
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
	FontSelectionMenu*	fSerifFontSelector;

	BView*				fOverlayContainer;
	BMenuField*			fOverlayField;
	BPopUpMenu*			fOverlayMenu;
	BMenuField*			fBlockField;
	BPopUpMenu*			fBlockMenu;
	BTextControl*		fOverlayStart;
	BTextControl*		fOverlayEnd;
	FontPopUpMenu*		fOverlayFont;
	BButton*			fAddOverlayBtn;
	BButton*			fRemoveOverlayBtn;
	BButton*			fShowFontBtn;
	font_overlay		fCurrentOverlay;
	int32				fWorkOverlayIndex;
	BFont				fWorkOverlay;
	int32				fGlobalOverlayIndex;
	BFont				fGlobalOverlay;
	BFont				fInitialOverlay;
	
	BView*				fRenderContainer;
	BMenuField*			fAAField;
	BPopUpMenu*			fAAMenu;
	TUpdateSlider*		fHintingSlider;
	int32				fInitialAntialiasing;
	int32				fInitialHinting;
	
	BView*				fCacheContainer;	
	TCustomSlider*		fScreenCacheSlider;
	TCustomSlider*		fPrintingCacheSlider;
	BButton*			fSaveCacheBtn;
	
	int32				fInitialScreenCache;
	int32				fInitialPrintingCache;	
};

//*********************************************************************

class FontPanelApp : public BApplication {
public:
						FontPanelApp();
						~FontPanelApp();

	void				MessageReceived(BMessage*);
	void				AboutRequested();
};
