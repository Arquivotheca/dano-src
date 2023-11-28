#ifndef NuScreen_H
#define NuScreen_H

#include <Application.h>
#include <Box.h>
#include <ColorControl.h>
#include <ListItem.h>
#include <ListView.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <Slider.h>
#include <String.h>
#include <Window.h>

#define SHOW_PICTURE_BTN 0
#if SHOW_PICTURE_BTN
class TPictureButton : public BControl {  
public:
							TPictureButton(BRect, BMessage*);
							~TPictureButton();
							
		void				Draw(BRect);
		status_t			Invoke(BMessage* m=NULL);
		void				MouseDown(BPoint);
		void 				MouseUp(BPoint pt);
		void 				MouseMoved(BPoint pt, uint32 , const BMessage *);
		
private:
		BBitmap*			fBits;
};
#endif

class TRefreshSlider : public BSlider {
public:
							TRefreshSlider(BRect, BMessage*,
								int32 min, int32 max,
								float bottom, float top,
								bool applyNow, float rate);
							~TRefreshSlider();
		void				AttachedToWindow();
		void				KeyDown(const char *bytes, int32 numBytes);
		void				SetValue(int32);
		status_t 			Invoke(BMessage *msg=NULL);
		char*				UpdateText() const;
		float				Rate() const;
		void				SetApplyNow(bool);		
private:
		char*				fStr;
		int32				fMin;
		int32				fMax;
		float				fTop;
		float 				fBottom;
		bool				fApplyNow;
		float				fRefreshRate;
		
		//	type ahead variables
		char				fKeyBuffer[10];
		bigtime_t			fLastTime;
};
							
const int32 kRefreshRateWindWidth = 310;	//250;
const int32 kRefreshRateWindHeight = 135;

class TConfigRefreshRateWind : public BWindow {
public:
							TConfigRefreshRateWind(BPoint loc, BWindow* owner,
								bool applyNow, float rate);
							~TConfigRefreshRateWind();
			
		void				FrameResized(float, float);
		void				MessageReceived(BMessage*);
		bool				QuitRequested();
		
		void 				AddButtons();
private:
		float				fOriginalRefreshRate;
		float				fMax;
		float 				fMin;
		BBox*				fBG;
		BWindow*			fOwner;
		TRefreshSlider*		fRefreshRateSlider;
		bool				fApplyNow;
		BButton*			fDoneBtn;
		BButton*			fCancelBtn;
};

const int32 kMaxWorkspaceCount = 32;

const int32 msg_config_index = 'cnin';

#define SHOW_CONFIG_WS 0
#if SHOW_CONFIG_WS
const int32 msg_config_other = 'cnot';
const int32 msg_config_okay = 'coka';
const int32 msg_config_cancel = 'ccan';

const int32 kCWWidth = 188;
const int32 kCWHeight = 96;

class TConfigWorkspaceWind : public BWindow {
public:
							TConfigWorkspaceWind(BPoint loc, BWindow* owner);
							~TConfigWorkspaceWind();
			
		void				FrameResized(float, float);
		void				MessageReceived(BMessage*);
		bool				QuitRequested();
		
		void 				AddButtons();
		void				AddWorkspaceConfigControl();
		void				AddWorkspaceList();
private:
		BBox*				fBG;

		BPopUpMenu*			fWSMenu;
		BMenuField*			fWSBtn;
			
		BButton*			fOkayBtn;
		BButton*			fCancelBtn;

		BWindow*			fOwner;
		int32				fWorkspaceCount;
		int32				fOriginalWSCount;
};
#endif

class TPulseBox : public BView {
public:
					TPulseBox(BRect);
					~TPulseBox();

		void		AttachedToWindow();
		void		Draw(BRect);				
		void		Pulse();
		
		void		GetIcon();
private:
		alert_type	fMsgType;
		BBitmap*	fIconBits;
		int32		fTimeRemaining;
};

class TConfirmWindow : public BWindow {
public:
							TConfirmWindow();
		void				MessageReceived(BMessage*);
		
		int32				Go();
			
		bool				Okay() { return fOK; }
private:
		bool				fOK;
		
		TPulseBox*			fBG;
			
		BTextView*			fMsgFld;
		
		BButton*			fAcceptBtn;
		BButton*			fCancelBtn;
				
		sem_id				fAlertSem;
		int32				fAlertVal;
};

//	a custom color control that sets the desktop color
class TBox;
class TDesktopColorControl : public BColorControl {
public:
							TDesktopColorControl(BPoint, BMessage*, TBox*);
							~TDesktopColorControl();
		
		void				AttachedToWindow();
		void				SetValue(int32 v);		// will pass the msg
		
		void				SetColor(rgb_color);	// wont pass this msg back
		void				SyncColor(rgb_color);	// wont change the dc color
private:
		bool				fDontChange;
		bool				fDontPassToSibling;
		TBox*				fSibling;
};

class TScreenThing : public BBox {
public:
							TScreenThing(BRect, resolution, rgb_color);
							~TScreenThing();
		
		void 				AttachedToWindow();
		void				MessageReceived(BMessage*);
		void				MouseDown(BPoint pt);
		void 				Draw(BRect);
		void 				DrawMonitor();
		void 				DrawMiniWindow(BRect, BPoint, float);

		void 				SetResolution(resolution);
		void 				SetColor(rgb_color);
private:
		resolution 			fResolution;
		rgb_color			fColor;
//		int32				fCurrentWorkspace;
};

typedef struct {
	int32			workspace;
	resolution		dimensions;
	color_space		colorSpace;
	float			refreshRate;
	rgb_color		desktopColor;
} monitor_settings;

//	the main background that grabs key events
//		to configure the monitor
class TBox : public BBox {
public:
								TBox(BRect frame, bool applyCustomNow);
								~TBox();
			
		void					AttachedToWindow();
		void					Draw(BRect);
		void					KeyDown(const char *bytes, int32 numBytes);
		void					MessageReceived(BMessage*);

		void					WorkspaceActivated(int32 ws, bool state);
		
		void 					AddParts();
		
		void 					AddMiniScreen(BPoint);
		void 					AddScreenControls();
		void 					AddDesktopColorSelector();
		void					AddButtons();
		
		void 					UpdateControls(bool);
		void					UpdateDesktopColor();

		void 					GetMonitorSettings();		
		void 					DesktopColorChange();

		void 					SetMonitorColors(color_space);
		void 					SetMonitorRefreshRate(float f);
		void 					SetMonitorResolution(resolution r);
		
		void 					SetRevertSettings();
		void 					RevertSettings();
		
		void					SetMonitorWithRefreshRate(float rate,
									bool confirmChanges=true);
		void 					SetMonitorForWorkspace(bool confirmChanges=true);
		void 					SetMonitorForWorkspace(monitor_settings s,
									bool confirmChanges=true);
		
		void 					SetDefaultResolution();
		void 					SetDefaultRefreshRate();
		void 					SetDefaultCRTPosition();

		void					ShowRefreshRateConfig();
		void 					ShowWorkspaceConfig();
		
		bool					CanRevert();
		void					SetRevert(bool);
		void					Revert();
		void					Defaults();
		
		void 					SaveSettings();
		void					UseSettings(monitor_settings settings, bool all=false);
		void					GetSettings(monitor_settings *settings);
		
		bool					ApplyCustomNow() { return fApplyCustomNow; }
		
#if SHOW_PICTURE_BTN
		void					ShowHelp();
#endif		
private:
		bool					fCanRevert;
		bool					fAllWorkspaces;
		int32					fTargetWorkspace;
		
		int32					fLastWorkspaceCount;
		int32					fInitialWorkspaceCount;
		
		monitor_settings		fSavedSettings;
		resolution				fResolution;
		resolution				fLastResolution;
		resolution				fInitialResolution;
		
		color_space				fColors;
		color_space				fLastColors;
		color_space				fInitialColors;
		
		float					fRefreshRate;
		float					fLastRefreshRate;
		float					fInitialRefreshRate;
		
		rgb_color				fDesktopColor;
		rgb_color				fLastDesktopColor;
		rgb_color				fInitialDesktopColor;

		BString					fDecorName;
		BString					fLastDecorName;
		BString					fInitialDecorName;
		
		TScreenThing*			fMiniScreen;
		BBox*					fScreenBox;
				
		bool					fApplyCustomNow;
		
		BPopUpMenu*				fTargetMenu;
		BMenuField*				fTargetBtn;
		
		BPopUpMenu*				fResolutionMenu;
		BMenuField*				fResolutionBtn;

		BPopUpMenu*				fColorsMenu;
		BMenuField*				fColorsBtn;

		BPopUpMenu*				fRefreshRateMenu;
		BMenuField*				fRefreshRateBtn;

		BButton*				fApplyBtn;

		TDesktopColorControl*	fDesktopColorCntl;
		
		BPopUpMenu*				fDecorMenu;
		BMenuField*				fDecorBtn;
		
#if SHOW_CONFIG_WS
		BButton*				fWorkspaceInfoBtn;
#endif
		BButton*				fRevertBtn;
		BButton*				fDefaultsBtn;
		
		bool					fHelpShowing;
#if SHOW_PICTURE_BTN
		TPictureButton*			fHelpBtn;
#endif
};

class TWindow : public BWindow {
public:
								TWindow();
								~TWindow();
			
		void 					FrameResized(float, float);
		void 					MessageReceived(BMessage*);
		void					GetPrefs(bool* applyCustomNow);
		bool 					QuitRequested();
				
		void					DoKeyboardControl(int8 bytes, ulong modifiers);

		void 					WorkspacesChanged(uint32 old_ws, uint32 new_ws);
		void 					WorkspaceActivated(int32 ws, bool state);
		
private:
		TBox*					fMainBG;
};

class TApp : public BApplication {
public:
								TApp();
								~TApp();
				
		void 					AboutRequested();
		void 					ArgvReceived(int32 argc, char** argv);
		void 					MessageReceived(BMessage*);
		void 					ReadyToRun();
		
		void 					PrintHelp();
private:
		bool 					fShowGUI;
		bool					fConfirmChanges;
		TWindow*				fWindow;
};

#endif

