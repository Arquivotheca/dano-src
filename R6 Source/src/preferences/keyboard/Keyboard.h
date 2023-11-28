//****************************************************************************************
//
//	Keyboard preference application
//	
//	Copyright 1995 Be, Inc. All Rights Reserved.
//	
//****************************************************************************************

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <kb_mouse_driver.h>

#include <Application.h>
#include <Box.h>
#include <Slider.h>
#include <TextControl.h>
#include <Window.h>

//*********************************************************************

class TBox : public BBox {
public:
					TBox(BRect frame);
		void		Draw(BRect);
		void		SetLineLocation(BPoint start, BPoint end);
private:
		BPoint		fStart;
		BPoint		fEnd;			
};

enum bb_border_type {
	BB_BORDER_NONE = 0,
	BB_BORDER_ALL,
	BB_BORDER_NO_TOP
};

class TButtonBar : public BView {
public:
			TButtonBar(BRect frame, bool defaultsBtn=true,
				bool revertBtn=true,
				bb_border_type borderType=BB_BORDER_ALL);
			~TButtonBar();
			
	void	Draw(BRect);
	
	void	AddButton(const char* title, BMessage* m);
	
	void	CanRevert(bool state);
	
private:
		bb_border_type	fBorderType;
		bool			fHasDefaultsBtn;
		BButton*		fDefaultsBtn;
		bool			fHasRevertBtn;
		BButton*		fRevertBtn;
		bool			fHasOtherBtn;
		BButton*		fOtherBtn;
};

class TIconThing : public BView {
public:
					TIconThing(BRect frame, const uchar* bits);
					~TIconThing();
		void		AttachedToWindow();
		void		Draw(BRect);
private:
		BBitmap*	fBits;
};

class TKeyboardWindow : public BWindow {
public:
					TKeyboardWindow(BRect frame);
			
		void 		MessageReceived(BMessage*);
		bool		QuitRequested();
	
		void 		BuildParts();
					
		void 		GetPrefs();
		void 		SetPrefs();
		
		
		void 		SetDefaults();
		void 		Revert();
		void 		CanRevert(bool state);

private:
		BBox*			fBG;
		TBox* 			fBox;
		BSlider*		fKeyRepeatSlider;
		BSlider*		fDelaySlider;
		BTextControl*	fTextControl;
		
		TButtonBar*		fBtnBar;
	
		kb_settings 	fCurrentSettings;
		kb_settings		fOriginalSettings;
};

class TKeyboardPrefApp : public BApplication {
public:
						TKeyboardPrefApp();
		
		void			MessageReceived(BMessage* m);					
		void			AboutRequested();
private:
		TKeyboardWindow* fWind;
};

#endif
