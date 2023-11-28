//--------------------------------------------------------------------
//	
//	ScrollBar.h
//
//	Written by: Robert Polic
//	
//	Copyright 1996 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef SCROLL_BAR_H
#define SCROLL_BAR_H

#ifndef _APPLICATION_H
#include <Application.h>
#endif
#ifndef _BITMAP_H
#include <Bitmap.h>
#endif
#ifndef _BUTTON_H
#include <Button.h>
#endif
#ifndef _VIEW_H
#include <View.h>
#endif
#ifndef _WINDOW_H
#include <Window.h>
#endif

#include <Invoker.h>

//*********************************************************************

const int32 kScrollBarWidth 	= 128;
const int32 kScrollBarHeight 	= 14;
const int32 kMinThumbWidth		= 8;
const int32 kMaxThumbWidth		= 50;
const int32 kThumbStart			= ((kScrollBarWidth - kMaxThumbWidth) / 2);

const rgb_color kViewGray = { 216, 216, 216, 255};
const rgb_color kWhite = { 255, 255, 255, 255};
const rgb_color kBlack = { 0, 0, 0, 255};
const rgb_color kDarkGray = { 120, 120, 120, 255};

const rgb_color kFillGray = {200, 200, 200, 255};
const rgb_color kShadeGray = {184, 184, 184, 255};
const rgb_color kHiliteGray = {152, 152, 152, 255};
const rgb_color kSelectionGray = {128, 128, 128, 255};

const int32 msg_selector_change = 'schg';

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
		void			SetTarget(BMessenger target);

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

#include <Box.h>
class TBox : public BBox {
public:
						TBox(	BRect frame, const char *name,
								const char* text,
								uint32 resizeMask=B_FOLLOW_LEFT | B_FOLLOW_TOP,
								uint32 flags=B_WILL_DRAW | B_NAVIGABLE,
								border_style style=B_FANCY_BORDER);
						~TBox();
		
		void			WindowActivated(bool state);
		void			Draw(BRect);
		void			MakeFocus(bool state = true);
};

//*********************************************************************

enum sb_selector {
	sb_none,
	sb_double,
	sb_single,
	sb_proportional,
	sb_fixed,
	sb_simple,
	sb_square,
	sb_bar,
	sb_size
};

enum selector_type {
	arrow_selector=0,
	thumb_type_selector,
	thumb_style_selector,
	thumb_size_selector
};

class TSBSelector : public TBox, public BInvoker {
public:

						TSBSelector(BRect frame, const char* name,
							const char* label, selector_type); 
						~TSBSelector();

		void			AttachedToWindow();
		void			Draw(BRect);
		
		bool			ChangeSelection(bool);
		bool			ChangeSize(bool);
		void			KeyDown(const char*, int32);
		
		sb_selector		HitTest(BPoint where, BRect* frame);
		void			UpdateSelection();
		bool			DoHiliteTracking(BRect);
		bool			DoSizeTracking(BRect);
		void			MouseDown(BPoint);

		void			DrawArrowStyleSelector();
		void			DrawKnobTypeSelector();
		void			DrawKnobStyleSelector();		
		void			DrawKnobSizeSelector();
		void			CacheSelectorFrames();
		
		void			DrawScrollBar(BRect, bool, bool, short, short);
		void			DrawKnob(BRect, short);
		void			DrawThumb(BRect, short);
		void			DrawSizeScrollBar(BRect);
		
		rgb_color		CurrentSizeColor() const;

		void			FrameItem(BRect, short);

		void			GetScrollBarInfo();
		void			SetScrollBarInfo();
		void			SetScrollBarInfo(bool arrows, bool proportional,
							short thumbStyle, short thumbWidth);
							
		void			SyncWithSystem();
private:
		selector_type	fType;
		
		bool			fDoubleArrows;
		bool			fProportionalThumb;
		short			fThumbStyle;
		float			fThumbWidth;
		
		BBitmap* 		fBits;
		BBitmap*		fSizeBits;
		BView*			fOffView;
		
		BRect			fDoubleArrowFrame;
		BRect			fSingleArrowFrame;

		BRect			fProportionalThumbFrame;
		BRect			fFixedThumbFrame;
		
		BRect			fSimpleKnobStyleFrame;
		BRect			fSquareKnobStyleFrame;
		BRect			fBarKnobStyleFrame;
		
		BRect			fKnobSizeFrame;
		rgb_color		fSizerColor;
		time_t			fTime;
};

//*********************************************************************

class TScrollBarView : public BView
{
public:
						TScrollBarView();
		void			MessageReceived(BMessage*);
		void			AttachedToWindow();

private:
		BBox*			fBG;
		TButtonBar*		fBtnBar;

		TSBSelector*	fArrowSelector;		
		TSBSelector*	fThumbSelector;
		TSBSelector*	fKnobSelector;
		TSBSelector*	fKnobSizeSelector;

		bool			fDoubleArrows;
		bool			fProportionalThumb;
		short			fThumbStyle;
		float			fThumbWidth;
};

#endif
