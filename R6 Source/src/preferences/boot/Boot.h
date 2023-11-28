// **************************************************************************
//	
//	Boot.h
//
//	Written by: Robert Polic
//	
//	Copyright 1996 Be, Inc. All Rights Reserved.
//	
// **************************************************************************

#ifndef BOOT_H
#define BOOT_H

#ifdef __INTEL__
#define _BUILD_INTEL
#define PPC_ONLY(x)
#else
#define PPC_ONLY(x) x
#endif

#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <ListView.h>
#include <MenuField.h>
#include <RadioButton.h>
#include <StringView.h>
#include <TextControl.h>
#include <View.h>
#include <Window.h>

// **************************************************************************

class TApp : public BApplication {
public:
							TApp();
							~TApp();
		
		void				MessageReceived(BMessage*);		
		void				AboutRequested();

};

// **************************************************************************

class TBootView;
class TWindow : public BWindow {
public:
							TWindow();
							~TWindow();
				
		void				MessageReceived(BMessage*);
		bool				QuitRequested();
		
		void				GetPrefs(short *mode);
		void				SetPrefs(short mode);

private:
		TBootView*			fMainView;
};

// **************************************************************************

class TListItem : public BListItem {
public:
							TListItem(dev_t deviceNumber,
								char* volumeName, char* deviceName,
								int32 session, int32 partition,
								bool isBoot);
							~TListItem(void);
				
		void				DrawItem(BView*, BRect, bool);
		void				Update(BView*, const BFont*);
		
		bool				Boot()const { return fBoot; }
		char*				DeviceName()  { return fDevice; }
		char*				VolumeName()  { return fVolume; }
		char*				TruncatedName() { return fTruncated; }
		int32 				Session() const { return fSession; }
		void				SetSession(int32 s) { fSession = s; }
		int32				Partition() const { return fPartition; }
		void				SetPartition(int32 p) { fPartition = p; }
		BBitmap*			Icon() const { return fIcon; }
		
		bool				IsSCSI();
		bool				IsMaster();
		int32				Device();
		int32				Bus();
		int32				ID();
		int32				Lun();
		bool				IsNextBoot();
		
		void 				MakeBootVolume();
private:
		dev_t				fDeviceNumber;
		bool				fBoot;
		char				fDevice[B_FILE_NAME_LENGTH];
		char				fVolume[B_FILE_NAME_LENGTH];
		char				fTruncated[B_FILE_NAME_LENGTH];
		int32				fSession;
		int32				fPartition;
		BBitmap*			fIcon;

};

// **************************************************************************

class TVolumeList : public BListView {
public:
							TVolumeList(BRect frame);
							~TVolumeList();
						
		TListItem*			ItemAt(int32 index);
		void				EmptyList();

		const char*			BootVolumeName();
		const char*			SelectedVolumeName();		

		void				DrawCheckMark(BPoint);
		BBitmap*			CheckMark() const;
private:
		BBitmap* 			fCheckMark;
};

// **************************************************************************

class TEasyView : public BView {
public:
							TEasyView(BRect);
							~TEasyView();
						
		void				Draw(BRect);
								
		void				RefreshList();
		
		void				SelectVolume(const char* deviceName);

		void				SelectVolume(int32 bus, int32 id, int32 lun,
								int32 partition);
		void				SelectVolume(int32 bus, bool master, int32 partition);
		
		void				SetBootVolume();
		void				ChooseVolume();
		void				UpdateStatus();

		void				NeedToUpdate(bool s) { fNeedUpdate = s; }
private:
		bool				fNeedUpdate;
		TVolumeList*		fVolumeList;
		BStringView*		fStatusFld;
		BButton*			fSetBtn;
		BBitmap*			fExclamationMark;
		bool				fShowExclamationMark;
};

// **************************************************************************

class TArrowTextControl : public BView {
public:
							TArrowTextControl(BRect, const char* label,
								const char* text, BMessage*,
								BTextControl* tc=NULL);
							~TArrowTextControl();
		
		void				AttachedToWindow();
		void				Draw(BRect);
		void				MessageReceived(BMessage*);
		void				KeyDown(const char *key, int32 numBytes);
		void				MouseDown(BPoint);

		void				SetEnabled(bool);
		bool				IsEnabled() const;
		
		short				Target() const;
		void				SetTarget(short);
		
		bool				Value() const;
		void				SetValue(bool);
		
		void				Increment();
		void				Decrement();

		const char*			Text() const;
		void				SetText(const char*);
		
		void				SetDivider(float);
		void				SetAlignment(alignment label, alignment text);
private:
		bool				fEnabled;
		bool				fNeedToInvert;
		short				fTarget;
		bool				fValue;
		BTextControl*		fTC;
		
		BBitmap*			fUpArrow;
		BBitmap*			fHiliteUpArrow;
		BBitmap*			fDisabledUpArrow;
		BRect				fUpFrame;

		BBitmap*			fDownArrow;
		BBitmap*			fHiliteDownArrow;
		BBitmap*			fDisabledDownArrow;
		BRect				fDownFrame;
		
		color_map*			fCurrColorMap;
};

// **************************************************************************

#ifndef _BUILD_INTEL
class TExpertView : public BView {
public:
							TExpertView(BRect, int32 partitionID);
							~TExpertView();
						
		void				MessageReceived(BMessage*);
		
		void				AddSCSIParts();
		void				AddIDEParts();
		
		void				ValidatePartition();
		
		void				SetBootVolume();
		void				SelectVolume(const char* deviceName,
								int32 p, int32 s);
							
		bool				Settings(int32 *bus, int32 *id, int32 *lun,
								int32 *partition);
		bool				Settings(int32 *bus, bool *master, int32 *partition);
		
		void				SyncControls();
		void				EnableDisableControls(bool scsi);
private:
		BBox*				fSCSIBox;
		BRadioButton*		fSCSIBtn;

		BMenuField*			fSCSIBusMenu;
		BPopUpMenu*			fSCSIBus;

		BMenuField*			fSCSIIDMenu;
		BPopUpMenu*			fSCSIID;
		
		BMenuField*			fSCSILUNMenu;
		BPopUpMenu*			fSCSILUN;

		int32				fSCSIPartitionID;
		TArrowTextControl*	fSCSIPartition;

		//
		BBox*				fIDEBox;
		BRadioButton*		fIDEBtn;

		BMenuField*			fIDEBusMenu;
		BPopUpMenu*			fIDEBus;

		BMenuField*			fIDEDeviceMenu;
		BPopUpMenu*			fIDEDevice;

		int32				fIDEPartitionID;
		TArrowTextControl*	fIDEPartition;
};
#endif	//	only ppc

// **************************************************************************

class TBootView : public BBox {

public:
						TBootView(BRect, short); 
						~TBootView();
				
		void			AttachedToWindow();
		void			AllAttached();
		void			Draw(BRect);
		void 			MessageReceived(BMessage*);

		void			HandleDrop(BMessage*);
		
		void			BuildEasy();
		void			ShowEasy();
		void			HideEasy();

#ifndef _BUILD_INTEL
		void 			BuildExpert();
		void			ShowExpert();
		void			HideExpert();
		void			SwitchMode(short);
#endif
		
		short			Mode() const;
		bool			EasyMode() const;
private:
		short			fMode;

#ifndef _BUILD_INTEL
		BMenuField*		fModeMenu;
#else
		BStringView*	fLabel;
#endif

		TEasyView*		fEasyView;
#ifndef _BUILD_INTEL
		TExpertView*	fExpertView;
#endif
	
};

// **************************************************************************

#ifndef _BUILD_INTEL
class BootInfo {
public:
						BootInfo(bool read=true);
				
		void			Read();
		void			Write();
				
		void			Set(int32 d, int32 b, int32 i, int32 l,
							int32 s, int32 p);
		void			Get(int32 *d, int32 *b, int32 *i, int32 *l,
							int32 *s, int32 *p);
	
		int32			Device() 	{ return fDevice; }
		void			SetDevice(int32 d) { fDevice = d; }
		int32 			Bus() 		{ return fBus; }
		void			SetBus(int32 b) { fBus = b; }
		int32 			ID() 		{ return fID; }
		void 			SetID(int32 i) { fID = i; }
		int32 			Lun()		{ return fLun; }
		void			SetLun(int32 l) { fLun = l; }
		int32 			Session()	{ return fSession; }
		void 			SetSession(int32 s) { fSession = s; }
		int32 			Partition()	{ return fPartition; }
		void 			SetPartition(int32 p) { fPartition = p; }
		
		bool			DeviceIsBoot(const char* deviceName);
		
		bool			IsSCSI();
		
private:
		int32 			fDevice;
		int32 			fBus;
		int32 			fID;
		int32 			fLun;
		int32			fSession;
		int32			fPartition;
};
#endif		// only ppc

#endif		// boot header
