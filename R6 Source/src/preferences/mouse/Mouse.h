//*********************************************************************
//	
//	Mouse.h
//
//	Written by: Robert Polic
//	
//	Copyright 1995 Be, Inc. All Rights Reserved.
//	
//*********************************************************************

#ifndef MOUSE_H
#define MOUSE_H

#include <Application.h>
#include <Box.h>
#include <CheckBox.h>
#include <MenuField.h>
#include <Slider.h>
#include <StringView.h>
#include <TextView.h>
#include <Window.h>

#include "kb_mouse_driver.h"
#include "MouseBits.h"

#define MOUSE_LEFT			42
#define MOUSE_TOP			33

#define MOUSE_UP			0
#define MOUSE_DOWN			1

enum	buttons		{MOUSE1 = 1,
					 MOUSE2,
					 MOUSE3,
					 BUT1,
					 BUT2,
					 BUT3,
					 DEFAULT_BUT,
					 CLICK_CHANGE,
					 TRACK_CHANGE};

enum	prefs		{MOUSE_TYPE = 1,
					 MOUSE_MAP};

const int32 msg_speed_change = 'sped';

//*********************************************************************

struct line {
	BRect 	frame;
	bool	orientation;	// true is horizontal
};

class TBox : public BBox {
public:
						TBox(BRect frame);
		void			Draw(BRect);
		void			SetLineCount(int32 c);
		void			SetLineLocation(int32 index, BRect lineFrame, bool orientation);
private:
		int32			fLineCount;
		line			fLineArray[256];
};

enum bb_border_type {
	BB_BORDER_NONE = 0,
	BB_BORDER_ALL,
	BB_BORDER_NO_TOP
};

const int32 msg_defaults = 'dflt';
const int32 msg_revert = 'rvrt';

class TButtonBar : public BView {
public:
						TButtonBar(BRect frame, bool defaultsBtn=true,
							bool revertBtn=true,
							bb_border_type borderType=BB_BORDER_ALL);
						~TButtonBar();
				
		void			Draw(BRect);
		
		void			AddButton(const char* title, BMessage* m);
		
		void			CanRevert(bool state);
	
private:
		bb_border_type	fBorderType;
		bool			fHasDefaultsBtn;
		BButton*		fDefaultsBtn;
		bool			fHasRevertBtn;
		BButton*		fRevertBtn;
		bool			fHasOtherBtn;
		BButton*		fOtherBtn;
};

//*********************************************************************

class TMouseType : public BBox {
public:
						TMouseType(BRect frame);
						~TMouseType();
		
		void			AttachedToWindow();
		void 			Draw(BRect where);
		void 			MouseDown(BPoint pt);
		void			MouseUp(BPoint);
		void 			Pulse();

		void 			SetDefaults();
		void 			Revert();
					
		void 			BuildMousePicture();
		void 			BuildMouseTypeMenu();
							
		void 			DrawMouse();
		void 			DrawMouseButton(short);

		void 			SetType(mouse_type type);
private:
		bool			fState[3];
		
		BBitmap*		fMouse[3];
		BBitmap*		fM1Buttons[2];
		BBitmap*		fM2Buttons[4];
		BBitmap*		fM3Buttons[6];
		
		BPopUpMenu*		fTypeMenu;
		BPopUpMenu*		fButtonMenu;
};

//*********************************************************************

class TSliderBox : public BBox {
public:
						TSliderBox(BRect bounds);
						~TSliderBox();
					
		void 			Draw(BRect bounds);
		void 			AttachedToWindow();

		void 			SetDefaults();
		void 			Revert();
		
		BSlider* 		DoubleClickSpeedSlider();
		BSlider* 		MouseAccelerationSlider();
			BSlider* MouseSpeedSlider();
private:
		BSlider*		fDoubleClickSlider;
		BBitmap*		fDoubleClickBits;
		BRect			fHandIconDest;
	
		BBitmap*		fMouseBits;
		BRect			fMouseIconDest;
//		BCheckBox		*fEnabled;
		BSlider			*fMouseSpeedSlider;
		BSlider*		fMouseAccelerationSlider;
	
		int32			fInitialClickSpeed;
		bool			fInitialMouseEnabled;
		int32			fInitialMouseSpeed;
		int32			fInitialMouseAcceleration;
};

//*********************************************************************

class TMouseView : public TBox {
public:
						TMouseView(BRect); 
						~TMouseView();
							
		void 			MouseDown(BPoint);
		void 			MouseUp(BPoint);
			
		void 			SetDefaults();
		void 			Revert();
public:
		TMouseType*		fMouseType;
		TSliderBox*		fSliderBox;
		BMenuField*		fFFMBtn;
};

//*********************************************************************

class TMouseSettings {
public:
						TMouseSettings();
						~TMouseSettings();
		
		void 			GetCurrent();
		void 			SetCurrent();
		mouse_settings 	Settings() { return fMouseMap; }
		
		mouse_type 		MouseType();
		void 			SetMouseType(mouse_type m);
		
		map_mouse 		MouseMap();
		void 			UpdateMouseMap();
		void 			SetMouseMap(map_mouse m);
		void 			SetMouseMap(int32 l, int32 m, int32 r);
		void 			SetLeftMouse(int32 m);
		void 			SetMiddleMouse(int32 m);
		void 			SetRightMouse(int32 m);
		
		mouse_accel 	MouseAcceleration();
		bool 			AccelerationEnabled();
		void 			SetAccelEnabled(bool e);
		int32 			AccelFactor();
		void 			SetAccelFactor(int32 a);
		int32	 		SpeedFactor();
		void 			SetSpeedFactor(int32 s);
		void 			SetMouseAcceleration(mouse_accel m);
		void 			SetMouseAcceleration(bool e, int32 a, int32 s);
	
		bigtime_t 		ClickSpeed();
		void 			SetClickSpeed(bigtime_t t);
	
		bool			FFMIsOn();
		void			SetFFM(bool);
		void			SetMouseMode(mode_mouse which);
		mode_mouse		MouseMode();
		mode_mouse		OriginalMouseMode() { return fFFMInitialState; }
		
		void 			SetDefaults();
		void 			Revert();
private:
		mouse_settings 	fMouseMap;
		mouse_settings 	fOriginal;
		mode_mouse		fFFMState;
		mode_mouse		fFFMInitialState;
};

//*********************************************************************

class TMouseWindow : public BWindow {
public:
						TMouseWindow(BRect frame);
			
		void 			MessageReceived(BMessage*);
		bool			QuitRequested();
				
		void 			GetPrefs();
		void 			SetPrefs();
	
		void 			SetDefaults();
		void 			Revert();
		void			CanRevert(bool);
private:
		BBox*			fBG;
		TMouseView*		fMouseView;
		TButtonBar*		fBtnBar;
};

//*********************************************************************

class TMouseApplication : public BApplication {
public:
					 	TMouseApplication();
					 	~TMouseApplication();
				
		void			MessageReceived(BMessage* m);
		void 			AboutRequested();
	
private:	
		TMouseSettings*	fMouse;
};

#endif
