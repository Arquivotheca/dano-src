//--------------------------------------------------------------------
//	
//	Sound.h
//
//	Written by: Robert Polic
//  Revised: 27-Jun-96 marc
//	
//	Copyright 1995, 1996 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef SOUND_H
#define SOUND_H

#include "Slider.h"
#include <Alert.h>
#include <Application.h>
#include <AudioStream.h>
#include <Bitmap.h>
#include <CheckBox.h>
#include <Point.h>
#include <RadioButton.h>
#include <Rect.h>
#include <Roster.h>
#include <View.h>
#include <Window.h>

enum	messages			{
	PREF_CD_ENABLE = 1, 
	PREF_DAC_ENABLE, 
	PREF_LINE_ENABLE, 
	PREF_ADC_ENABLE,
	PREF_MASTER_ENABLE,
	PREF_SPEAKER_ENABLE,
	PREF_LOOP_ENABLE,

	PREF_CD_LEFT,
	PREF_CD_RIGHT,
	PREF_DAC_LEFT, 
	PREF_DAC_RIGHT,
	PREF_LINE_LEFT,
	PREF_LINE_RIGHT,
	PREF_ADC_LEFT,
	PREF_ADC_RIGHT,
	PREF_MASTER_LEFT,
	PREF_MASTER_RIGHT,
	PREF_SPEAKER_GAIN,
	PREF_LOOPBACK_ATTN,

	PREF_CD_IN,
	PREF_LINE_IN,
	PREF_MIC_IN,
	PREF_MIC20_IN,

	PREF_REVERT,
	PREF_DEFAULTS
  };

//====================================================================

class TSoundApplication : public BApplication {

public:
				TSoundApplication();
void			AboutRequested();

bool			Initialized() {return fInitialized;};
platform_type	Platform() {return fPlatform;};

bool			fInitialized;
platform_type	fPlatform;
};

//====================================================================

class TSoundWindow : public BWindow {
public:
				TSoundWindow(BRect, char*);
bool			QuitRequested();
};

//====================================================================

class TSoundView : public BView {

public:
				TSoundView(BRect, char*); 
				~TSoundView();

void			AttachedToWindow();
void			BuildBeBoxWindow();
void			BuildMacWindow();
void			BuildIntelWindow();
void			MessageReceived(BMessage*);
void			Pulse();
void			Draw(BRect);
void			DrawBeBoxWindow();
void			DrawMacWindow();
void			DrawIntelWindow();
void			DrawBox(BRect box, char* label, bool label_only = FALSE);
void			DrawArrow(BPoint, BPoint, float);
platform_type	Platform();

BDACStream*			fDACStream;
BADCStream*			fADCStream;
BBitmap*			fSchematic;

float				fCDR;
float				fCDL;
float				fDACR;
float				fDACL;
float				fLineR;
float				fLineL;
float				fADCR;
float				fADCL;
float				fMasterR;
float				fMasterL;
float				fSpeaker;
float				fLoopback;
bool				fEnableLoop;
bool				fEnableCD;
bool				fEnableDAC;
bool				fEnableLine;
bool				fEnableADC;
bool				fEnableMaster;
bool				fEnableSpeaker;
bool				fMicBoost;
int32				fInput;

TStereoSlider*		fSliderCD;
TStereoSlider*		fSliderDAC;
TStereoSlider*		fSliderLine;
TStereoSlider*		fSliderADC;
TStereoSlider*		fSliderMaster;
TMonoSlider*		fSliderSpeaker;
TMonoSlider*		fSliderLoop;
BCheckBox*			fLoopBox;

BRadioButton*		fInputCD;
BRadioButton*		fInputLine;
BRadioButton*		fInputMIC;
BRadioButton*		fInputMIC20;

BButton*			fRevert;
BButton*			fDefaults;
};

#endif
