#ifndef MAIN_H
#define MAIN_H

#ifndef _VIEW_H
#include <View.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif
#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _LIST_VIEW_H
#include <ListView.h>
#endif
#include <MenuField.h>
#ifndef _SCROLL_VIEW_H
#include <ScrollView.h>
#endif
#include <Slider.h>
#ifndef _BUTTON_H
#include <Button.h>
#endif
#ifndef _BITMAP_H
#include <Bitmap.h>
#endif
#ifndef _TEXT_VIEW_H
#include <TextView.h>
#endif
#ifndef _STRING_VIEW_H
#include <StringView.h>
#endif
#include <OS.h>
#include <MenuField.h>
#include <String.h>

#include <string.h>
#include <stdlib.h>

/*------------------------------------------------------------*/

enum {
	RUN_THROUGH = 101,
	TOGGLE_AA_CHECKBOX,

	NEW_SIZE = 0x1000,
	NEW_SHEAR,
	NEW_ROTATION,
	NEW_TEXT,
	NEW_FONT,
	NEW_SPACING,
	NEW_OUTLINE,
	NEW_AA_SETTING,
	NEW_BBOX_SETTING,
	NEW_ESC_SETTING,
	NEW_INVERT_SETTING,
	NEW_HINTING_SETTING,
	SET_STRING_SPACING,
	SET_CHARACTER_SPACING,
	SET_BITMAP_SPACING,
	SET_FIXED_SPACING,
	SET_FACE,
	SET_DRAWING_MODE
};

#define BUTTON_NAME	"RunThrough"

#ifndef NIL
#define NIL NULL
#endif

/*------------------------------------------------------------*/

short		    numFamilies = 0;
font_family	    *fontFamilies = 0L;
short		    numStyles = 0;
font_style      *fontStyles = 0L;

char*		sampleText = "Be, Inc.";
short		maxLength = 255;

const BRect	mw_rect(80, 30, 490, 300);
const BRect	cw_rect(500, 30, 700, 465);
#define mw_sizeH (mw_rect.right - mw_rect.left)
#define mw_sizeV (mw_rect.bottom - mw_rect.top)
const long	FONT_NAME_HEIGHT = 13;

class		TMainWindow;
TMainWindow* 	font_window = NIL;

class		TControlWindow;
TControlWindow*	ctrl_window = NIL;
//BBitmap*	the_offscreen = NIL;

class		TFontMenu;

//------------------------------------------------------------------------------

class TControlWindow : public BWindow
{
public:
				TControlWindow(BWindow *main_window, BRect bound,
					window_look look, window_feel feel, ulong flags, const char *name);
virtual	void	MessageReceived(BMessage* msg);

		void	SetFaceMenu(const BFont& src);
		void	GetFaceMenu(BFont* dest);
		
		long	control_sem;
		bool	fAutoCycle;
	TFontMenu*	fFontField;
	BMenuField*	fFaceField;
	BMenuField*	fAAField;
	BMenuField*	fHintingField;
	BMenuField*	fSpacingField;
	BMenuField*	fDrawingModeField;
	BCheckBox*	fBoundsCB;
	BCheckBox*	fEscCB;
	BCheckBox*	fInvertCB;
};

//------------------------------------------------------------------------------

class TControlScroller : public BSlider
{
public:
					TControlScroller(BRect view_bound, const char *label, int32 min, int32 max, 
									 BLooper *target, uint32 command, sem_id sem);

virtual status_t	Invoke(BMessage *msg = NULL);
void				DrawText();

private:
	sem_id			fSem;
};

/*------------------------------------------------------------*/

class TFontView : public BView
{

public:
				TFontView(BRect view_bound);

virtual	void	MouseDown(BPoint where);
virtual	void	MouseUp(BPoint where);
virtual	void	MouseMoved(	BPoint where,
							uint32 code,
							const BMessage *a_message);
virtual	void	WindowActivated(bool state);

		void	Redraw(bool everything=false);

virtual	void	Draw(BRect update_rect);
		void	SetText(const char *text);

		void	SetVisibleBounds(BRect b);
		
		bool	SetDescription(const char* d1, const char* d2);
		bool	SetDescription(BPoint where);
		
		float	mSpacing;
		int32	mOutline;
		bool	BBoxEnable;
		bool	EscEnable;
		bool	Invert;
		drawing_mode	fDrawingMode;
private:
		
		char	*fText;
		BPoint	fWhere;
		BString	fDescription1;
		BString	fDescription2;
		BRect	fVisBounds;
};
		
/*------------------------------------------------------------*/

class TFontApp : public BApplication
{
public:
				TFontApp();
virtual	void	AboutRequested();
virtual	bool	QuitRequested();
};

/*------------------------------------------------------------*/

class TMainWindow : public BWindow
{
public:
		
					TMainWindow(BRect bound, window_type type, long flags,
						const char *name);
virtual				~TMainWindow();
virtual	void		MessageReceived(BMessage* an_event);
virtual	void		FrameResized(float new_wdith, float new_height);
virtual	bool		QuitRequested();
		void		Redraw(bool forceCache = FALSE);

private:
		TFontView* 	fFontView;
};


class TFontMenu : public BMenuField {
public:
					TFontMenu(BRect frame, BLooper *target);

	virtual void	AttachedToWindow();
	virtual void	MessageReceived(BMessage *message);
	virtual void	Pulse();

	void			SetSelectedFont(const BFont *font, bool useStyle);
	void			SetSelectedFont(const font_family family);
	void			SetSelectedFont(const font_family family, const font_style style);

private:
 	void			FillOutMenu();
	void			UnmarkAll();
	void			SendMessageToTarget();

private:
	enum { msg_FontFamilySelected	= '1234',
		   msg_FontStyleSelected	= '2345' };

	BMessenger		fTarget;
	BFont			fSelectedFont;
};


#endif
