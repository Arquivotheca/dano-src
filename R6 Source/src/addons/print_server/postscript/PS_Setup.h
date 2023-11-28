/* ++++++++++

   FILE:  HP_Setup.h
   REVS:  $Revision: 1.5 $
   NAME:  Robert Polic

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef HP_SETUP_H
#define HP_SETUP_H

#ifndef _ALERT_H
#include <Alert.h>
#endif
#ifndef _BITMAP_H
#include <Bitmap.h>
#endif
#ifndef _BUTTON_H
#include <Button.h>
#endif
#ifndef _LOOPER_H
#include <Looper.h>
#endif
#ifndef _MESSAGE_FILTER_H
#include <MessageFilter.h>
#endif
#ifndef _MENU_FIELD_H
#include <MenuField.h>
#endif
#ifndef _MENU_ITEM_H
#include <MenuItem.h>
#endif
#ifndef _MESSAGE_H
#include <Message.h>
#endif
#ifndef _POINT_H
#include <Point.h>
#endif
#ifndef _POP_UP_MENU_H
#include <PopUpMenu.h>
#endif
#ifndef _RECT_H
#include <Rect.h>
#endif
#ifndef _TEXT_CONTROL_H
#include <TextControl.h>
#endif
#ifndef _VIEW_H
#include <View.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef	_STRINGVIEW_H
#include <StringView.h>
#endif
#include <PictureButton.h>
#include <CheckBox.h>
#include <RadioButton.h>
#include <ListItem.h>
#include <OutlineListView.h>
#include <Box.h>

#include "Postscript.h"

#define	SETUP_WIDTH				400
#define SETUP_HEIGHT			200
#define SETUP_LARGE_HEIGHT		400

// Margin box

#define BOX_OFFSET_HOR		10
#define BOX_OFFSET_VERT		5
#define BOX_OFFSET_W		170
#define BOX_OFFSET_H		141

#define BOX_LABEL			"Margins"

// margin view

#define VIEW_MARGIN_H				10
#define VIEW_MARGIN_V				20
#define VIEW_MARGIN_MAX_W			60
#define VIEW_MARGIN_MAX_H			70

// Text controls rects

#define TC_TOP_V					17
#define TC_VERT_SHIFT				20
#define TC_RIGHT					160

#define TV_LEFT					72
#define TV_DIVIDER					50.

#define TV_LABEL_FOR_TOP			"Top"
#define TV_LABEL_FOR_BOTTOM			"Bottom"
#define TV_LABEL_FOR_LEFT			"Left"
#define TV_LABEL_FOR_RIGHT			"Right"


// PopUp menu for units

#define	UNITS_LABEL_INCH			"Inches"
#define	UNITS_LABEL_CM				"cm"
#define	UNITS_LABEL_POINTS			"Points"

#define	UNITS_LABEL_INCH_MESSAGE	'unin'
#define	UNITS_LABEL_CM_MESSAGE		'uncm'
#define	UNITS_LABEL_POINTS_MESSAGE	'unpt'

#define	UNITS_HOR				   	 95
#define	UNITS_VERT					115
#define	UNITS_W						 65
#define	UNITS_H						 20

#define	SIZE_STRING_HOR				 10
#define	SIZE_STRING_VERT		   	112
#define	SIZE_STRING_W			 	 60
#define	SIZE_STRING_H			  	 20

// Pop up for paper sizes

#define	PAPER_SIZE_HOR				215 //195
#define	PAPER_SIZE_VERT				 20
#define	PAPER_SIZE_W		 	  	180	//220
#define	PAPER_SIZE_H			   	 20
#define	PAPER_SIZE_LABEL			"Paper"

// Pop up for layouts

#define	LAYOUT_VERT_SHIFT		    	25
#define LAYOUT_FIRST_TOP_LABEL		"First Up"
#define LAYOUT_LAST_TOP_LABEL		"Last Up"
#define LAYOUT_LABEL				"Layout"

#define BUTTON_W					80
#define BUTTON_H					23
#define BUTTON_SPACE				10


#define ORIENTATION_TEXT_LEFT		185 //235	
#define ORIENTATION_TEXT_TOP		85
#define ORIENTATION_TEXT			"Orientation"

#define RASTERIZE_H			  		195
#define RASTERIZE_V					110
#define RASTERIZE_WIDTH				190
#define RASTERIZE_HEIGHT			 20
#define RATERIZE_TEXT				"PostScript"
#define RATERIZE_PS_TEXT			"Send Native PostScript"
#define RATERIZE_8_TEXT				"Host Rendered 8-bit (faster, lower quality)"
#define RATERIZE_24_TEXT			"Host Rendered 24-bit (slower, higher quality)"


#define COLOR_H			  			256
#define COLOR_V						133
#define COLOR_WIDTH					100
#define COLOR_HEIGHT				 20
#define COLOR_TEXT					"Output in Color"


#define MANUALFEED_TEXT				"Use manual paper feed"
#define MANUALFEED					'mfed'

#define PORTRAIT_LEFT				256
#define LANDSCAPE_LEFT				305 //335
#define ORIENTATION_TOP				75	
#define ORIENTATION_WIDTH			25	
#define ORIENTATION_HEIGHT			30	


#define	MENU_HEIGHT			 16

#define	PAPER_SIZE_CHANGED	 'pach'
#define	LAYOUT_1_UP			 'l1up'
#define	LAYOUT_LAST_UP		 'llup'
#define	UNIT_INCHED			 'uinc'
#define	UNIT_CM				 'uncm'
#define	UNIT_POINT			 'upoi'

const uint32 MIN_MARGIN = 3;
const uint32 MARGIN_CHG =		'mchg';
const uint32 MARGIN_DIRTY = 	'mdrt';

#define PAPER_H				 17
#define PAPER_V				 12
#define PAPER_TEXT			"Page Size"
enum	PAPER_SIZE			{PAGE_LETTER = 100, PAGE_LEGAL, PAGE_A4, PAGE_B5,
							 PAGE_TABLOID, PAGE_LEDGER, PAGE_ENVELOPE,
							 PAGE_OTHER};

enum	UNIT_TYPE			{INCHES,CM,POINTS};

typedef struct {
	char	name[32];
	float	width;
	float	height;
	long	message;
	BRect	imageable;
	char	realname[32];
} PAGESIZE;


#define WIDTH_H				 38
#define WIDTH_V				 40
#define WIDTH_WIDTH			 	90
#define WIDTH_HEIGHT		 	16
#define WIDTH_TEXT				"Width"

#define HEIGHT_H			(WIDTH_H + WIDTH_WIDTH + 10)
#define HEIGHT_V			WIDTH_V
#define HEIGHT_WIDTH		WIDTH_WIDTH
#define HEIGHT_HEIGHT		WIDTH_HEIGHT
#define HEIGHT_TEXT			"Height"

// Scale textcontrol

#define SCALE_LEFT					222
#define SCALE_RIGHT					288

#define	SCALE_H				 33
#define SCALE_V				 66
#define SCALE_WIDTH			 	95
#define SCALE_HEIGHT			WIDTH_HEIGHT
#define SCALE_TEXT				"Scale"
#define SCALE_DIVIDER			40.

enum	SIZE				{SIZE_WIDTH = 300, SIZE_HEIGHT, SIZE_SCALING};

#define ORIENT_H			  8
#define ORIENT_V			 125 //94
#define ORIENT_TEXT			"Orientation"
enum	ORIENT				{ORIENT_PORTRAIT = 200, ORIENT_LANDSCAPE};

#define SAMPLE_H			(SETUP_WIDTH - PORT_HEIGHT - 20)
#define SAMPLE_V			  8
#define SAMPLE_WIDTH		(PORT_HEIGHT + 16)
#define SAMPLE_HEIGHT		(PORT_HEIGHT + 16)

#define BUTTON_WIDTH		 60
#define BUTTON_HEIGHT		 20

#define LINE_V				(SETUP_HEIGHT - BUTTON_HEIGHT - 23)

#define SAVE_BUTTON_H		  6
#define SAVE_BUTTON_V		(SETUP_HEIGHT - BUTTON_HEIGHT - 11)
#define SAVE_BUTTON_TEXT	"Save..."
#define SET_OK_BUTTON_H		(SETUP_WIDTH - BUTTON_WIDTH - 10)
#define SET_OK_BUTTON_V		SAVE_BUTTON_V
#define SET_OK_BUTTON_TEXT	"OK"
#define SET_CANCEL_BUTTON_H	(SET_OK_BUTTON_H - BUTTON_WIDTH - 12)
#define SET_CANCEL_BUTTON_V	SET_OK_BUTTON_V
#define SET_CANCEL_BUTTON_TEXT	"Cancel"

#define SET_ADV_BUTTON_H	BOX_OFFSET_HOR
#define SET_ADV_BUTTON_V	SET_CANCEL_BUTTON_V
#define SET_ADV_BUTTON_TEXT	"Advanced"
#define UI_SELECTED			'uisl'
#define CBOX				'cbox'
#define UI_ITEM_SELECTED	'item'

#define ADV_OPT_CLOSE		'aocl'

enum	BUTTONS				{M_OK = 1, M_ADV, M_CANCEL, M_SAVE};

const uint32 OPTION_OK		= 'opok';
const uint32 OPTION_CANCEL	= 'opcl';

class	TSetupView;
class	TMarginView;

filter_result	SetupKeyFilter(BMessage*, BHandler**, BMessageFilter*);	

class OptionWindow;

//====================================================================

class BSetup : public BWindow {

public:
TSetupView		*fView;

char			ppd_fullname[512];
TPrintDrv		*parent;

private:
char			fResult;
long			fSetupSem;
BMessage		*fSetupMessage;
BMessageFilter	*fFilter;

OptionWindow	*fOptionWindow;

		void	OpenOptionWindow();

public:
				BSetup(BMessage*, char*, TPrintDrv*);
				~BSetup(void);
virtual	void	MessageReceived(BMessage*);
		long	Go(void);
		bool	QuitRequested();
};

//--------------------------------------------------------------------

class TSetupView : public BView {
public:


BTextControl	*fScaling;
BTextControl	*margin_left;
BTextControl	*margin_right;
BTextControl	*margin_top;
BTextControl	*margin_bottom;

private:

long			fOrient;
float			fPageHeight;
float			fPageWidth;
float			fScale;
BRect 			fPPDImageable;
BRect			fImageable;
bool			fFirstUp;
bool			use_bitmap;
bool			raster_quality;
bool			use_color;
bool			isMarginDirty;
BMenuField		*fOrientation;
BMenuField		*fPaper;
BMenuField		*fLayout;
BMenuField		*fUnits;
TMarginView		*fView;
BList			page_size_list;
BStringView		*sizeString;

//BCheckBox		*rasterize_box;
//BCheckBox		*rasterize_24_box;

BMenu *ps_generation_menu;
BMenuField *ps_generation_field;

BCheckBox		*color_box;
BCheckBox		*manualfeed_box;


class OrientationButton *landscape;
class OrientationButton *portrait;

public:
				TSetupView(BRect, BMessage*);
				~TSetupView();
				
virtual void	AttachedToWindow(void);
virtual	void	Draw(BRect rect);
virtual	void	MessageReceived(BMessage*);
virtual void	ReadPPD(void);
		void	UpdateMessage(BMessage*);
		void	UpdateSize(bool update_margin_fields = true);
		void	UpdateFromPPD(BMessage*);
		BFile 	*ppd_file;
	UNIT_TYPE	unit;

		BList	fontList;
private:
		void	NiceString(char*, float);
		void 	UpdateMargins(void);
		void	UpdatePaperSizeMenu(PAGESIZE*);
	PAGESIZE*	LookupPageSize(const char*);
		
		BButton	*font_button;
		BButton	*cancel_button;
		BButton	*valid_button;
		BButton *advanced_button;
				
		BString page_size_name;
};

//--------------------------------------------------------------------

class TMarginView : public BView {

private:

float			fWidth;
float			fHeight;
BBitmap			*fLandscape;
BBitmap			*fPortrait;

float			penSize;
float			pointsPerPixelH;
float			pointsPerPixelV;
BRect 			marginBox;

float			maxWidth;
float			maxHeight;
BPoint			center;

public:
				TMarginView(BRect, float, float);
				~TMarginView(void);
virtual	void	Draw(BRect rect);
		void	SetPageSize(float, float, BRect, long);
};

//--------------------------------------------------------------------

class OrientationButton : public BPictureButton {

public:
				OrientationButton(BRect rect,BPicture *pict1,BPicture *pict2);
				void Init(ORIENT orient);
};

//--------------------------------------------------------------------
class PPDMenuItem : public BMenuItem
{
 public:
						PPDMenuItem(const char *label, BMessage *msg);
	virtual				~PPDMenuItem();

	virtual void		Draw();

	void				SetConflicted(bool);
	bool				IsConflicted() const;

 private:

	bool				fIsConflicted;
};

//--------------------------------------------------------------------
class InvocationRadioButton : public BRadioButton
{
 public:
			InvocationRadioButton(BRect rect, Invocation *target);
	void	SetValue(int32 value);		

 private:
	Invocation*	fTarget;
};

//--------------------------------------------------------------------
class TOutlineListView : public BOutlineListView
{
 public:
						TOutlineListView(BRect, const char*);
						~TOutlineListView();
};

//--------------------------------------------------------------------
class UIStringItem : public BStringItem
{
 public:
						UIStringItem(const char *name,
											int32 level = 0, UI *target = NULL);
	UI*					Target() const;

	void				DrawItem(BView *owner, BRect itemRect, bool complete);
 
 private:
	UI*					fTarget;
};					

//--------------------------------------------------------------------
class OptionView : public BBox
{
private:
	BOutlineListView	*fOptionList;
	BSetup				*fSetupWindow;
	UI*					fCurrentUI;
	void				BuildInterface(UI*);
	void				BuildBooleanInterface(UI*);
	void				BuildListInterface(UI*, UIType);
	
	void				AttachJclInvocations(BMessage*);
	
public:
						OptionView(BRect, BSetup*);
						~OptionView();

	void				MessageReceived(BMessage*);
	void				AttachedToWindow();
	
	void				UpdateMessage(BMessage*);
	void				UpdateSetupWindow(Invocation*);
	void				UpdateStringItems();
};

//--------------------------------------------------------------------
class OptionWindow : public BWindow
{
private:
	BSetup		*fParent;
	OptionView	*fOptionView;
	BMessage	*fSetupMsg;
	long		fResult;
			
public:
						OptionWindow(BRect rect, BMessage *msg, BSetup*);
						~OptionWindow();
	
	long				Go();
	void				MessageReceived(BMessage*);
	void				UpdateStringItems();
	
	const BMessage*		GetSetupMessage() const;
};

#endif
