//*********************************************************************
//	
//	VM.h
//
//	Written by: Robert Polic
//	
//	Copyright 1997 Be, Inc. All Rights Reserved.
//	
//*********************************************************************

#ifndef VM_H
#define VM_H

#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Slider.h>
#include <StringView.h>
#include <View.h>
#include <Window.h>

typedef struct {
	uint64	swap;
} vm_info;

class BEntry;

//*********************************************************************

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
		void			CanDefault(bool);
		void			DisableControls();
	
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

class TSlider : public BSlider {
public:
						TSlider(BRect frame, const char *name, const char *label,
							BMessage *message, int32 minValue, int32 maxValue );
						~TSlider();

		void			AttachedToWindow();
		void			DrawText();
		void 			KeyDown(const char *bytes, int32 n);
		void			SetAndInvoke(int32);
};

//*********************************************************************

class TVMView : public BBox {
public:
						TVMView(BRect frame, uint32 physicalMem,
							int64 actualSize,
							int64 min, int64 max);
						~TVMView();
		
		void			Draw(BRect);
		void			Pulse();
		
		void			ShowAlert(bool state, const char* msg=NULL);
		void			SetCurrentSwapSize(int32 size);

		void			DisableControls();
		void 			HideControls();
private:
		int 			when;
		time_t 			ptime;
		char			errorMsg[64];
		bool			fShowAlert;

		BStringView*	fPhysicalMemFld;
		BStringView*	fActualSizeFld;
		TSlider*		fSlider;
		BStringView*	fMsgFld;
		
		BBitmap*		fExclamationMark;
};

//*********************************************************************

class TVMWindow : public BWindow {
public:
						TVMWindow();
						~TVMWindow();
				
		void			MessageReceived(BMessage*);
		bool			QuitRequested(void);

		void			GetPrefs();
		void			SetPrefs();
		
		void 			SetDefaults();
		void 			Revert();
		void			CanRevert(bool);
		void			CanDefault(bool);

		void			GetVMSettings();
	
		status_t		KernelSettingsFile(BEntry*, bool create=false);
		void			SetKernelSetting(int64);
		void			KillKernelSettingsFile();
		int64			KernelSetting();

private:
		vm_info			fOriginal, fCurrent;
		uint64			fSwapMin;
		uint64			fSwapMax;
		uint32			fPhysicalMemory;
		uint64			fSwapSpaceAvailable;
		uint64			fSwapFileSize;

		BBox*			fBG;
		TVMView*		fVMView;
		TButtonBar*		fBtnBar;
};

//*********************************************************************

class TVMApp : public BApplication {
public:
						TVMApp();
						~TVMApp();
		void			MessageReceived(BMessage*);
		void			AboutRequested();
};

#endif
