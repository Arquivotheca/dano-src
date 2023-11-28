//--------------------------------------------------------------------
//	
//	SCSIProbe.h
//
//	Written by: Robert Polic
//	
//	Copyright 1996 Robert Polic, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef SCSIPROBE_H
#define SCSIPROBE_H

#include <Alert.h>
#include <AppFileInfo.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Directory.h>
#include <Entry.h>
#include <Font.h>
#include <File.h>
#include <FindDirectory.h>
#include <ListView.h>
#include <Path.h>
#include <Point.h>
#include <Rect.h>
#include <ScrollView.h>
#include <StringView.h>
#include <View.h>
#include <Window.h>

#define	TITLE_BAR_HEIGHT	 25
#define	WIND_WIDTH			570
#define WIND_HEIGHT			308

#define PATH_BOX_X1			  5
#define PATH_BOX_Y1			 15
#define PATH_BOX_X2			(WIND_WIDTH - PATH_BOX_X1)
#define PATH_BOX_Y2			108
#define PATH_LABEL_TEXT		"SCSI Buses"

#define PATH_LIST_X1		  8
#define PATH_LIST_Y1		 34
#define PATH_LIST_X2		(PATH_BOX_X2 - PATH_BOX_X1 - 10)
#define PATH_LIST_Y2		(PATH_BOX_Y2 - PATH_BOX_Y1 - 8)

#define PATH_ID_X1			PATH_LIST_X1
#define PATH_ID_Y1			(PATH_LIST_Y1 - 17)
#define PATH_ID_X2			(PATH_ID_X1 + 17)
#define PATH_ID_Y2			(PATH_ID_Y1 + 12)
#define PATH_ID_TEXT		"ID"

#define PATH_SIM_X1			PATH_ID_X2
#define PATH_SIM_Y1			PATH_ID_Y1
#define PATH_SIM_X2			(PATH_SIM_X1 + 109)
#define PATH_SIM_Y2			PATH_ID_Y2
#define PATH_SIM_TEXT		"SIM Vendor"

#define PATH_SIM_VERS_X1	PATH_SIM_X2
#define PATH_SIM_VERS_Y1	PATH_ID_Y1
#define PATH_SIM_VERS_X2	(PATH_SIM_VERS_X1 + 37)
#define PATH_SIM_VERS_Y2	PATH_ID_Y2
#define PATH_SIM_VERS_TEXT	"Vers"

#define PATH_HBA_X1			PATH_SIM_VERS_X2
#define PATH_HBA_Y1			PATH_ID_Y1
#define PATH_HBA_X2			(PATH_HBA_X1 + 109)
#define PATH_HBA_Y2			PATH_ID_Y2
#define PATH_HBA_TEXT		"HBA Vendor"

#define PATH_HBA_VERS_X1	PATH_HBA_X2
#define PATH_HBA_VERS_Y1	PATH_ID_Y1
#define PATH_HBA_VERS_X2	(PATH_HBA_VERS_X1 + 37)
#define PATH_HBA_VERS_Y2	PATH_ID_Y2
#define PATH_HBA_VERS_TEXT	PATH_SIM_VERS_TEXT

#define PATH_FAMILY_X1		PATH_HBA_VERS_X2
#define PATH_FAMILY_Y1		PATH_ID_Y1
#define PATH_FAMILY_X2		(PATH_FAMILY_X1 + 109)
#define PATH_FAMILY_Y2		PATH_ID_Y2
#define PATH_FAMILY_TEXT	"Controller Family"

#define PATH_TYPE_X1		PATH_FAMILY_X2
#define PATH_TYPE_Y1		PATH_ID_Y1
#define PATH_TYPE_X2		(PATH_LIST_X2 - B_V_SCROLL_BAR_WIDTH)
#define PATH_TYPE_Y2		PATH_ID_Y2
#define PATH_TYPE_TEXT		"Controller Type"

#define DEVICE_BOX_X1		PATH_BOX_X1
#define DEVICE_BOX_Y1		(PATH_BOX_Y2 + 13)
#define DEVICE_BOX_X2		(DEVICE_BOX_X1 + 276)
#define DEVICE_BOX_Y2		(DEVICE_BOX_Y1 + 145)
#define DEVICE_LABEL_TEXT	"SCSI Devices"

#define DEVICE_LIST_X1		  8
#define DEVICE_LIST_Y1		 34
#define DEVICE_LIST_X2		(DEVICE_BOX_X2 - DEVICE_BOX_X1 - 10)
#define DEVICE_LIST_Y2		(DEVICE_BOX_Y2 - DEVICE_BOX_Y1 - 8)

#define DEVICE_ID_X1		DEVICE_LIST_X1
#define DEVICE_ID_Y1		(DEVICE_LIST_Y1 - 17)
#define DEVICE_ID_X2		(DEVICE_ID_X1 + 17)
#define DEVICE_ID_Y2		(DEVICE_ID_Y1 + 12)
#define DEVICE_ID_TEXT		PATH_ID_TEXT

#define DEVICE_TYPE_X1		DEVICE_ID_X2
#define DEVICE_TYPE_Y1		DEVICE_ID_Y1
#define DEVICE_TYPE_X2		(DEVICE_TYPE_X1 + 36)
#define DEVICE_TYPE_Y2		DEVICE_ID_Y2
#define DEVICE_TYPE_TEXT	"Type"

#define DEVICE_VENDOR_X1	DEVICE_TYPE_X2
#define DEVICE_VENDOR_Y1	DEVICE_ID_Y1
#define DEVICE_VENDOR_X2	(DEVICE_VENDOR_X1 + 58)
#define DEVICE_VENDOR_Y2	DEVICE_ID_Y2
#define DEVICE_VENDOR_TEXT	"Vendor"

#define DEVICE_PRODUCT_X1	DEVICE_VENDOR_X2
#define DEVICE_PRODUCT_Y1	DEVICE_ID_Y1
#define DEVICE_PRODUCT_X2	(DEVICE_PRODUCT_X1 + 103)
#define DEVICE_PRODUCT_Y2	DEVICE_ID_Y2
#define DEVICE_PRODUCT_TEXT	"Product"

#define DEVICE_VERS_X1		DEVICE_PRODUCT_X2
#define DEVICE_VERS_Y1		DEVICE_ID_Y1
#define DEVICE_VERS_X2		(DEVICE_LIST_X2 - B_V_SCROLL_BAR_WIDTH)
#define DEVICE_VERS_Y2		DEVICE_ID_Y2
#define DEVICE_VERS_TEXT	PATH_SIM_VERS_TEXT

#define LUN_BOX_X1			(DEVICE_BOX_X2 + 8)
#define LUN_BOX_Y1			DEVICE_BOX_Y1
#define LUN_BOX_X2			(LUN_BOX_X1 + 276)
#define LUN_BOX_Y2			DEVICE_BOX_Y2
#define LUN_LABEL_TEXT		"Logical Units"

#define LUN_LIST_X1			  8
#define LUN_LIST_Y1			 34
#define LUN_LIST_X2			(LUN_BOX_X2 - LUN_BOX_X1 - 10)
#define LUN_LIST_Y2			(LUN_BOX_Y2 - LUN_BOX_Y1 - 8)

#define LUN_ID_X1			LUN_LIST_X1
#define LUN_ID_Y1			(LUN_LIST_Y1 - 17)
#define LUN_ID_X2			(LUN_ID_X1 + 17)
#define LUN_ID_Y2			(LUN_ID_Y1 + 12)
#define LUN_ID_TEXT			PATH_ID_TEXT

#define LUN_TYPE_X1			LUN_ID_X2
#define LUN_TYPE_Y1			LUN_ID_Y1
#define LUN_TYPE_X2			(LUN_TYPE_X1 + 36)
#define LUN_TYPE_Y2			LUN_ID_Y2
#define LUN_TYPE_TEXT		DEVICE_TYPE_TEXT

#define LUN_VENDOR_X1		LUN_TYPE_X2
#define LUN_VENDOR_Y1		LUN_ID_Y1
#define LUN_VENDOR_X2		(LUN_VENDOR_X1 + 58)
#define LUN_VENDOR_Y2		LUN_ID_Y2
#define LUN_VENDOR_TEXT		DEVICE_VENDOR_TEXT

#define LUN_PRODUCT_X1		LUN_VENDOR_X2
#define LUN_PRODUCT_Y1		LUN_ID_Y1
#define LUN_PRODUCT_X2		(LUN_PRODUCT_X1 + 103)
#define LUN_PRODUCT_Y2		LUN_ID_Y2
#define LUN_PRODUCT_TEXT	DEVICE_PRODUCT_TEXT

#define LUN_VERS_X1			LUN_PRODUCT_X2
#define LUN_VERS_Y1			LUN_ID_Y1
#define LUN_VERS_X2			(LUN_LIST_X2 - B_V_SCROLL_BAR_WIDTH)
#define LUN_VERS_Y2			LUN_ID_Y2
#define LUN_VERS_TEXT		PATH_SIM_VERS_TEXT

#define BUTTON_WIDTH		 70
#define BUTTON_HEIGHT		 20

#define ABOUT_BUTTON_X1		(LUN_BOX_X2 - BUTTON_WIDTH - 6)
#define ABOUT_BUTTON_Y1		(WIND_HEIGHT - (BUTTON_HEIGHT + 10))
#define ABOUT_BUTTON_X2		(ABOUT_BUTTON_X1 + BUTTON_WIDTH)
#define ABOUT_BUTTON_Y2		(UPDATE_BUTTON_Y1 + BUTTON_HEIGHT)
#define ABOUT_BUTTON_TEXT	"About"B_UTF8_ELLIPSIS

#define UPDATE_BUTTON_X1	(ABOUT_BUTTON_X1 - (BUTTON_WIDTH + 10))
#define UPDATE_BUTTON_Y1	ABOUT_BUTTON_Y1
#define UPDATE_BUTTON_X2	(UPDATE_BUTTON_X1 + BUTTON_WIDTH)
#define UPDATE_BUTTON_Y2	ABOUT_BUTTON_Y2
#define UPDATE_BUTTON_TEXT	"Update"
#define RESET_BUTTON_TEXT	"Bus Reset"

#define LED_BUTTON_X1		(UPDATE_BUTTON_X1 - (BUTTON_WIDTH + 10))
#define LED_BUTTON_Y1		UPDATE_BUTTON_Y1
#define LED_BUTTON_X2		(LED_BUTTON_X1 + BUTTON_WIDTH)
#define LED_BUTTON_Y2		UPDATE_BUTTON_Y2
#define LED_BUTTON_TEXT		"Flash LED"

typedef struct {
	int32	path_id;
	int32	path_sim;
	int32	path_sim_vers;
	int32	path_hba;
	int32	path_hba_vers;
	int32	path_family;
	int32	path_type;

	int32	device_id;
	int32	device_type;
	int32	device_vendor;
	int32	device_product;
	int32	device_vers;

	int32	lun_id;
	int32	lun_type;
	int32	lun_vendor;
	int32	lun_product;
	int32	lun_vers;
} field_offsets;

typedef struct {
	char	sim_vendor[32];
	char	sim_vers[32];
	char	hba_vendor[32];
	char	hba_vers[32];
	char	family[32];
	char	type[32];
	uchar	host_id;
	uchar	devices;
} path_data;

typedef struct {
	int32	flags;
	char	type[16];
	char	vendor[32];
	char	product[32];
	char	version[32];
} scsi_data;

typedef struct {
	int32	flag;
	int32	device;
	int32	lun;
} flash_data;

enum	ITEMS				{I_PATH = 0, I_DEVICE, I_LUN, I_END};
enum	MESSAGES			{M_PATH = 256, M_DEVICE, M_LUN,
							 M_UPDATE, M_LED, M_ABOUT};

#define	VIEW_COLOR			224
#define LINE_COLOR			160
#define SELECT_COLOR		180
#define	GRAY_COLOR			224
#define BUTTON_COLOR		208
#define BUTTON_SHADOW		232
#define BUTTON_DOWN			128
#define BUTTON_SHADOW_DOWN	 80

class	TItem;
class	TList;
class	TButton;


//====================================================================

class TSCSIApp: public BApplication {

public:

BFile			*fPrefs;

				TSCSIApp(void);
				~TSCSIApp(void);
};

//--------------------------------------------------------------------

class TSCSIWindow: public BWindow {

private:

public:

				TSCSIWindow(BRect, char*);
virtual bool	QuitRequested(void);
};

//--------------------------------------------------------------------

class TSCSIView : public BView {

private:
bool			fFirstUpdateDone;
bool			fReset;
int32			fDevice;
int32			fLUN;
int32			fPath;
int32			fSelected;
uint32			fPlatform;
TButton			*fAboutButton;
TButton			*fUpdateButton;
TButton			*fLEDButton;
TItem			*fItems[I_END];
path_data		fPathData[16];
scsi_data		fLUNData[8];
scsi_data		fSCSIData[32];
version_info	fVersion;
public:

				TSCSIView(BRect, char*);
virtual void	AllAttached(void);
virtual void	Draw(BRect);
virtual void	KeyDown(const char *, int32);
virtual void	MessageReceived(BMessage*);
virtual void	Pulse(void);
void			About(void);
void			DeviceInquire(int32);
void			FlashLED(flash_data*);
bool			Inquire(int32, int32, int32, int32, scsi_data*);
void			LUNInquire(int32, int32);
void			NiceString(uchar*, char*, int32);
void			Reset(void);
void			Select(int32);
void			Update(bool);
};

//--------------------------------------------------------------------

class TItem : public BView {

private:

bool			fSelected;
int32			fItem;
BBox			*fBox;
BScrollView		*fScroll;
BStringView		*fLabels[7];
TList			*fList;
TSCSIView		*fParent;

public:

				TItem(BRect, char*, int32, TSCSIView*);
virtual	void	Draw(BRect);
virtual void	KeyDown(const char *, int32);
void			AddItem(void*);
int32			CountItems(void);
int32			CurrentSelection(void);
void			Select(bool);
void			SelectItem(int32);
void			SetItemCount(int32);
void			SetSelectionMessage(BMessage*, BView*);
void			UpdateItem(int32, scsi_data*, scsi_data*);
};


//--------------------------------------------------------------------

class TList : public BListView {

private:

int32			fItem;
float			fHeight;
TSCSIView		*fParent;

public:

				TList(BRect, int32, TSCSIView*);
virtual	void	Draw(BRect);
virtual void	MouseDown(BPoint);
};


//--------------------------------------------------------------------

class TListItem : public BListItem {

private:

void			*fData;
int32			fItem;
int32			fNum;
int32			fItemOffsets[I_END];

public:

				TListItem(void*, int32, int32);
virtual	void	DrawItem(BView*, BRect, bool);
void			FitString(BView*, BFont*, char*, int32);
bool			UpdateItem(scsi_data*, scsi_data*);
};


//--------------------------------------------------------------------

class TButton : public BButton {

private:

char			*fLabel;
int32			fMessage;
TSCSIView		*fParent;

public:

				TButton(BRect, const char*, const char*, int32, TSCSIView*);
				~TButton(void);
virtual	void	Draw(BRect);
virtual	void	MouseDown(BPoint);
virtual void	SetLabel(const char*);
};


//--------------------------------------------------------------------

static int32	MountThread(void*);

uchar size_cursor[] =	{16, 1, 8, 8,
						 0x00, 0x00, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40,
						 0x02, 0x40, 0x0a, 0x50, 0x1a, 0x58, 0x3e, 0x7c,
						 0x3e, 0x7c, 0x1a, 0x58, 0x0a, 0x50, 0x02, 0x40,
						 0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x00, 0x00,
						 0x07, 0xe0, 0x07, 0xe0, 0x07, 0xe0, 0x07, 0xe0,
						 0x0f, 0xf0, 0x1f, 0xf8, 0x3f, 0xfc, 0x7f, 0xfe,
						 0x7f, 0xfe, 0x3f, 0xfc, 0x1f, 0xf8, 0x0f, 0xf0,
						 0x07, 0xe0, 0x07, 0xe0, 0x07, 0xe0, 0x07, 0xe0};

char *scsi_types[] =	{"DISK", "TAPE", "PRINT", "CPU",  "WORM",
						 "ROM",  "SCAN", "OPTIC", "JUKE", "LAN", "???"};

#define BE_VENDOR			"BE INC."
#define BE_PRODUCT			"BEBOX"
#define MAC_PRODUCT			"POWERMAC"
#define INTEL_PRODUCT		"AT CLONE"

#define ERROR1_STR			"The 'scsiprobe' driver needs to be installed \
before SCSIProbe can be used."
#define ERROR2_STR			"A newer version of the scsiprobe driver needs \
to be installed before SCSIProbe can be used."
#define ERROR3_STR			"Sorry, there are no SCSI devices installed on this machine."

#define SETTINGS_FILE		"SCSIProbe_prefs"

#define DRIVER				B_SCSIPROBE_DRIVER

#endif
