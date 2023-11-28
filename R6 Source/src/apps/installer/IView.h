//******************************************************************************
//
//	File:			IView.h
//
//	Description:	Installer view header.
//
//	Written by:		Steve Horowitz
//
//	Copyright 1994, Be Incorporated. All Rights Reserved.
//
//******************************************************************************

#ifndef IVIEW_H
#define IVIEW_H

#include <Box.h>
#include <CheckBox.h>
#include <PopUpMenu.h>
#include <MenuField.h>
#include <Bitmap.h>
#include <StringView.h>
#include <TextView.h>

#include "Options.h"

// constants
const uint32 OPTIONS_TOGGLE_SWITCH = 'opts';
const float EXPANDED_DELTA = 146.0;

class ProgressBar;
class BarberPoleView;

class TIView : public BBox {
public:
					TIView(BRect);
virtual				~TIView();
virtual	void		Draw(BRect);
virtual void		AttachedToWindow();
		BPopUpMenu*	VolumeMenu1();
		BPopUpMenu*	VolumeMenu2();
		BMenuField*	MenuField1();
		BMenuField*	MenuField2();
		BStringView* FromText();
		BTextView* StatusText();
		ProgressBar *SizeBar();
		OptionView *Options();
		void SetSizeBarVisible(bool on, bool barberPole);
		void SetSizeBarMaxValue();
		void ToggleOptionsExpanded();
		void ShowSourceVolumeAsText(bool);
		bool IsExpanded();
		void UpdateSpaceLabel();
		char* BuildDefaultText();
private:

		BBitmap*	fLogoBits;
		BTextView*	fTEView;
		BPopUpMenu*	fVolumeMenu1;
		BPopUpMenu*	fVolumeMenu2;
		BMenuField*	fMenuField1;
		BMenuField*	fMenuField2;
		BStringView *fFromText;
		ProgressBar	*m_progress_bar;
		BStringView *mSpaceLabel;
		BStringView *mOptionToggleLabel;
		BScrollView *mOptionScrollView;
		BButton *mDriveSetupButton;
		OptionView *mOptionView;
		
		bool m_bar_visible;
		bool m_expanded;	// whether or not the options are shown
//		bool m_use_bar;		// whether or not to use the progress bar
		BarberPoleView *fBarberPole;
		bool fBarberPoleVisible;

		void PopulateOptions(OptionView *options);
	typedef BBox inherited;
};

inline	BPopUpMenu*	TIView::VolumeMenu1()	{ return(fVolumeMenu1); }
inline	BPopUpMenu*	TIView::VolumeMenu2()	{ return(fVolumeMenu2); }
inline	BMenuField*	TIView::MenuField1()	{ return(fMenuField1); }
inline	BMenuField*	TIView::MenuField2()	{ return(fMenuField2); }
inline	BStringView* TIView::FromText()		{ return(fFromText); }
inline	ProgressBar *TIView::SizeBar()		{ return(m_progress_bar); }
inline	BTextView *TIView::StatusText()		{ return(fTEView); }
//inline	BButton *TIView::DriveSetupButton()	{ return(mDriveSetupButton); }
inline	OptionView *TIView::Options()		{ return(mOptionView); }
#endif
