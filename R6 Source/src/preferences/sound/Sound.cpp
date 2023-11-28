//--------------------------------------------------------------------
//	
//	Sound.cpp
//
//	Written by: Robert Polic
//  Revised: 23-Apr-97 marc
//	
//	Copyright 1995, 1996, 1997 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <math.h>
#include <stdio.h>

#include "Sound.h"
#include <AudioMsgs.h>
#include <R3MediaDefs.h>
#include <Alert.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Screen.h>

#define	BROWSER_WIND		 82
#define	TITLE_BAR_HEIGHT	 25
#define	WIND_BORDER			  3	// in pixels
#define WIND_BE_WIDTH		369
#define WIND_BE_HEIGHT		440
#define WIND_MAC_WIDTH		369
#define WIND_MAC_HEIGHT		217
#define WIND_BACKGROUND		216 //232

extern char schematic[];
#define SCHEMATIC_WIDTH		112
#define SCHEMATIC_HEIGHT	17
#define SCHEMATIC_BACKGROUND 0x1b

#define SETTINGS_FILE "Sound_panel"

#define X(N) printf("<%d>\n",(N));

//====================================================================

int main()
{	
	TSoundApplication	*myApplication;

	myApplication = new TSoundApplication();
	if (myApplication->Initialized())
	  myApplication->Run();
	else {
	  BAlert* bogus = new BAlert("Sound", "Sound preferences panel:\n"
								 "Failed to connect to server.",
								 "Quit", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
	  if (bogus)
		bogus->Go();
	}

	delete myApplication;
	return B_NO_ERROR;
}

static
status_t server_request(uint32 command)
{
	BMessenger server(AUDIO_SERVER_ID);
	if (command)
	  return server.SendMessage(command);
	return (server.IsValid() ? B_NO_ERROR : B_ERROR);
}

//--------------------------------------------------------------------

TSoundApplication::TSoundApplication()
  : BApplication("application/x-vnd.Be-SOND")
{
	BPoint win_pos;
	BRect r;
   
	// Intentionally construct a temporary BScreen
	BRect sframe = BScreen(B_MAIN_SCREEN_ID).Frame();
	fInitialized = false;

	if (server_request(NULL) != B_NO_ERROR)
	  if (be_roster->Launch(AUDIO_SERVER_ID) != B_NO_ERROR
		  || server_request(NULL) != B_NO_ERROR)
		return;

	fPlatform = B_BEBOX_PLATFORM;
	system_info si;
	if (get_system_info(&si) == B_NO_ERROR)
	  fPlatform = si.platform_type;

	if (fPlatform == B_MAC_PLATFORM)
	  r.Set(0, 0, WIND_MAC_WIDTH, WIND_MAC_HEIGHT);
	else
	  r.Set(0, 0, WIND_BE_WIDTH, WIND_BE_HEIGHT);
	r.OffsetTo(BROWSER_WIND, TITLE_BAR_HEIGHT);

	BPath path;
	if (find_directory (B_COMMON_SETTINGS_DIRECTORY, &path) == B_OK) {
	  path.Append (SETTINGS_FILE);
	  int ref = open(path.Path(), O_RDONLY);
	  if (ref >= 0) {
		read(ref, &win_pos, sizeof(BPoint));
		close(ref);
		if (sframe.Contains(win_pos))
		  if (sframe.Contains(win_pos+BPoint(8, 8)))
			r.OffsetTo(win_pos);
	  }
	}
	TSoundWindow* mySoundWindow = new TSoundWindow(r, "Sound");
	mySoundWindow->Show();

	fInitialized = true;
}

//--------------------------------------------------------------------

void TSoundApplication::AboutRequested()
{
	(new BAlert("", "...by Robert Polic" ,"Big Deal"))->Go();
}

//====================================================================

TSoundWindow::TSoundWindow(BRect rect, char *title)
	          :BWindow(rect, title, B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	Lock ();
	SetPulseRate(100000);
	rect.OffsetTo(0, 0);
	TSoundView* sound_view = new TSoundView(rect, "SoundView");
	AddChild(sound_view);
	Unlock();
}

//--------------------------------------------------------------------

bool TSoundWindow::QuitRequested()
{
	BPath path;

	if (find_directory (B_COMMON_SETTINGS_DIRECTORY, &path, true) == B_OK) {
	  path.Append (SETTINGS_FILE);
	  int ref = open(path.Path(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
	  if (ref >= 0) {
		BPoint win_pos = Frame().LeftTop();
		write(ref, &win_pos, sizeof(BPoint));
		close(ref);
	  }
	}

	// kludge for audio server not quitting:
	server_request(SAVE_SOUND_HARDWARE_INFO);

	be_app->PostMessage(B_QUIT_REQUESTED);
	return TRUE;
}

//====================================================================

TSoundView::TSoundView(BRect rect, char *title)
  : BView (rect, title, B_FOLLOW_ALL,
		   B_WILL_DRAW | B_FRAME_EVENTS | B_FULL_UPDATE_ON_RESIZE | B_PULSE_NEEDED)
{
	fDACStream = new BDACStream ();
	fADCStream = new BADCStream ();

	char tan = 0x1d - SCHEMATIC_BACKGROUND;
	if (tan)
	  for (int i = 0; i < SCHEMATIC_WIDTH * SCHEMATIC_HEIGHT; i++)
		if (schematic[i] > tan)
		  schematic[i] -= tan;

	fSchematic = new BBitmap(BRect(0, 0, SCHEMATIC_WIDTH - 1, SCHEMATIC_HEIGHT - 1),
							 B_COLOR_8_BIT);
	fSchematic->SetBits(schematic, fSchematic->BitsLength(), 0, B_COLOR_8_BIT);
}

//--------------------------------------------------------------------

TSoundView::~TSoundView()
{
	delete fDACStream;
	fDACStream = NULL;
	delete fADCStream;
	fADCStream = NULL;
	delete fSchematic;
}

//--------------------------------------------------------------------

platform_type
TSoundView::Platform ()
{
	return ((TSoundApplication*)be_app)->Platform();
}

//--------------------------------------------------------------------

void
TSoundView::AttachedToWindow ()
{
    SetViewColor (WIND_BACKGROUND, WIND_BACKGROUND, WIND_BACKGROUND);

	fSliderCD = NULL;
	fSliderDAC = NULL;
	fSliderLine = NULL;
	fSliderADC = NULL;
	fSliderMaster = NULL;
	fSliderSpeaker = NULL;
	fSliderLoop = NULL;
	fLoopBox = NULL;

	fDACStream->GetVolume(B_CD_THROUGH, &fCDL, &fCDR, &fEnableCD);
	fDACStream->GetVolume(B_LINE_IN_THROUGH, &fLineL, &fLineR, &fEnableLine);
	fDACStream->GetVolume(B_ADC_IN, &fADCL, &fADCR, &fEnableADC);
	fDACStream->GetVolume(B_DAC_OUT, &fDACL, &fDACR, &fEnableDAC);
	fDACStream->GetVolume(B_MASTER_OUT, &fMasterL, &fMasterR, &fEnableMaster);
	fDACStream->GetVolume(B_SPEAKER_OUT, &fSpeaker, NULL, &fEnableSpeaker);
	fDACStream->GetVolume(B_LOOPBACK, &fLoopback, NULL, &fEnableLoop);

	fInput = -1;

	switch (Platform()) {
	case B_MAC_PLATFORM:		BuildMacWindow();	break;
	case B_AT_CLONE_PLATFORM:	BuildIntelWindow();	break;
	default:					BuildBeBoxWindow();	break;
	}
}

//--------------------------------------------------------------------

void
TSoundView::BuildBeBoxWindow ()
{
	BRect r (0, 0, 63, 16);

	r.OffsetTo (12, 10);
	fRevert = new BButton (r, "Revert", "Revert",
						   new BMessage (PREF_REVERT),
						   B_FOLLOW_LEFT | B_FOLLOW_TOP,
						   B_WILL_DRAW | B_NAVIGABLE | B_NAVIGABLE_JUMP);
	AddChild (fRevert);
	fRevert->SetTarget (this);

	r.OffsetBy (71, 0);
	fDefaults = new BButton (r, "Defaults", "Defaults",
							 new BMessage (PREF_DEFAULTS));
	AddChild (fDefaults);
	fDefaults->SetTarget (this);

	r.Set (0, 0, 72, 16);

	r.OffsetTo (33, 74);
	fInputCD = new BRadioButton (r, "CD", "CD", new BMessage (PREF_CD_IN),
								 B_FOLLOW_LEFT | B_FOLLOW_TOP,
								 B_WILL_DRAW | B_NAVIGABLE | B_NAVIGABLE_JUMP);
	AddChild (fInputCD);
	fInputCD->SetTarget (this);

	r.OffsetBy (0, 30);
	fInputLine = new BRadioButton (r, "Line In", "Line In",
								   new BMessage (PREF_LINE_IN));
	AddChild (fInputLine);
	fInputLine->SetTarget (this);

	r.OffsetBy (0, 30);
	fInputMIC = new BRadioButton (r, "Mic", "Mic",
								  new BMessage (PREF_MIC_IN));
	AddChild (fInputMIC);
	fInputMIC->SetTarget (this);

	r.OffsetBy (0, 30);
	fInputMIC20 = new BRadioButton (r, "Mic+20dB", "Mic+20dB",
									new BMessage (PREF_MIC20_IN));
	AddChild (fInputMIC20);
	fInputMIC20->SetTarget (this);

	BPoint where (192, 38);
	fSliderDAC = new TStereoSlider (where, "DAC", fDACL, fDACR,
									this, PREF_DAC_LEFT, PREF_DAC_RIGHT,
									PREF_DAC_ENABLE, !fEnableDAC);
	where.x += 60;
	fSliderCD = new TStereoSlider (where, "CD", fCDL, fCDR,
								   this, PREF_CD_LEFT, PREF_CD_RIGHT,
								   PREF_CD_ENABLE, !fEnableCD);
	where.x += 60;
	fSliderLine = new TStereoSlider (where, "Line In", fLineL, fLineR,
									 this, PREF_LINE_LEFT, PREF_LINE_RIGHT,
									 PREF_LINE_ENABLE, !fEnableLine);
	where.Set (25, 236);
	fSliderADC = new TStereoSlider (where, NULL, fADCL, fADCR,
									this, PREF_ADC_LEFT, PREF_ADC_RIGHT,
									PREF_ADC_ENABLE, !fEnableADC);
	where.x += 80;
	fSliderLoop = new TMonoSlider (where, NULL, fLoopback,
								   this, PREF_LOOPBACK_ATTN,
								   PREF_LOOP_ENABLE, !fEnableLoop);
	where.Set (202, 236);
	fSliderSpeaker = new TMonoSlider (where, NULL, fSpeaker,
									  this, PREF_SPEAKER_GAIN,
									  PREF_SPEAKER_ENABLE, !fEnableSpeaker);
	where.x += 100;
	fSliderMaster = new TStereoSlider (where, NULL, fMasterL, fMasterR,
									   this, PREF_MASTER_LEFT, PREF_MASTER_RIGHT,
									   PREF_MASTER_ENABLE, !fEnableMaster);
}

//--------------------------------------------------------------------

void
TSoundView::BuildIntelWindow ()
{
	BRect r (0, 0, 72, 16);

	r.OffsetTo (31, 246);
	fInputCD = new BRadioButton (r, "CD", "CD", new BMessage (PREF_CD_IN),
								 B_FOLLOW_LEFT | B_FOLLOW_TOP,
								 B_WILL_DRAW | B_NAVIGABLE | B_NAVIGABLE_JUMP);
	AddChild (fInputCD);
	fInputCD->SetTarget (this);

	r.OffsetBy (0, 30);
	fInputLine = new BRadioButton (r, "Line In", "Line In",
								   new BMessage (PREF_LINE_IN));
	AddChild (fInputLine);
	fInputLine->SetTarget (this);

	r.OffsetBy (0, 30);
	fInputMIC = new BRadioButton (r, "Mic", "Mic",
								  new BMessage (PREF_MIC_IN));
	AddChild (fInputMIC);
	fInputMIC->SetTarget (this);

	r.OffsetBy (0, 30);
	fInputMIC20 = new BRadioButton (r, "Mic+20dB", "Mic+20dB",
									new BMessage (PREF_MIC20_IN));
	AddChild (fInputMIC20);
	fInputMIC20->SetTarget (this);

	r.Set (0, 0, 52, 16);

	r.OffsetTo (14, 394);
	fRevert = new BButton (r, "Revert", "Revert",
						   new BMessage (PREF_REVERT),
						   B_FOLLOW_LEFT | B_FOLLOW_TOP,
						   B_WILL_DRAW | B_NAVIGABLE | B_NAVIGABLE_JUMP);
	AddChild (fRevert);
	fRevert->SetTarget (this);

	r.OffsetBy (58, 0);
	fDefaults = new BButton (r, "Defaults", "Defaults",
							 new BMessage (PREF_DEFAULTS));
	AddChild (fDefaults);
	fDefaults->SetTarget (this);

	BPoint where (72, 38);
	fSliderDAC = new TStereoSlider (where, "DAC", fDACL, fDACR,
									this, PREF_DAC_LEFT, PREF_DAC_RIGHT,
									PREF_DAC_ENABLE, !fEnableDAC);
	where.x += 66;
	fSliderCD = new TStereoSlider (where, "CD", fCDL, fCDR,
								   this, PREF_CD_LEFT, PREF_CD_RIGHT,
								   PREF_CD_ENABLE, !fEnableCD);
	where.x += 66;
	fSliderLine = new TStereoSlider (where, "Line In", fLineL, fLineR,
									 this, PREF_LINE_LEFT, PREF_LINE_RIGHT,
									 PREF_LINE_ENABLE, !fEnableLine);
	where.x += 66;
	fSliderLoop = new TMonoSlider (where, "Mic", fLoopback,
								   this, PREF_LOOPBACK_ATTN,
								   PREF_LOOP_ENABLE, !fEnableLoop);
	where.Set (151, 236);
	fSliderADC = new TStereoSlider (where, NULL, fADCL, fADCR,
									this, PREF_ADC_LEFT, PREF_ADC_RIGHT,
									PREF_ADC_ENABLE, !fEnableADC);
	where.x += 81;
	fSliderSpeaker = new TMonoSlider (where, NULL, fSpeaker,
									  this, PREF_SPEAKER_GAIN,
									  PREF_SPEAKER_ENABLE, !fEnableSpeaker);
	where.x += 81;
	fSliderMaster = new TStereoSlider (where, NULL, fMasterL, fMasterR,
									   this, PREF_MASTER_LEFT, PREF_MASTER_RIGHT,
									   PREF_MASTER_ENABLE, !fEnableMaster);
}

//--------------------------------------------------------------------

void
TSoundView::BuildMacWindow()
{
	BRect r (0, 0, 51, 16);

	r.OffsetTo (12, 10);
	fRevert = new BButton (r, "Revert", "Revert",
						   new BMessage (PREF_REVERT),
						   B_FOLLOW_LEFT | B_FOLLOW_TOP,
						   B_WILL_DRAW | B_NAVIGABLE | B_NAVIGABLE_JUMP);
	AddChild (fRevert);
	fRevert->SetTarget (this);

	r.OffsetBy (59, 0);
	fDefaults = new BButton (r, "Defaults", "Defaults",
							 new BMessage (PREF_DEFAULTS));
	AddChild (fDefaults);
	fDefaults->SetTarget (this);

	r.Set (0, 0, 72, 11);

	r.OffsetTo (33, 68);
	fInputMIC = new BRadioButton (r, "Mic", "Mic",
								  new BMessage (PREF_MIC_IN),
								  B_FOLLOW_LEFT | B_FOLLOW_TOP,
								  B_WILL_DRAW | B_NAVIGABLE | B_NAVIGABLE_JUMP);
	AddChild (fInputMIC);
	fInputMIC->SetTarget (this);

	r.OffsetBy (0, 26);
	fInputMIC20 = new BRadioButton (r, "Mic+20dB", "Mic+20dB",
									new BMessage (PREF_MIC20_IN));
	AddChild (fInputMIC20);
	fInputMIC20->SetTarget (this);

	r.OffsetBy (0, 26);
	fInputCD = new BRadioButton (r, "CD", "CD", new BMessage (PREF_CD_IN));
	AddChild (fInputCD);
	fInputCD->SetTarget (this);

	r.OffsetBy (0, 26);
	fInputLine = new BRadioButton (r, "Line In", "Aux",
								   new BMessage (PREF_LINE_IN));
	AddChild (fInputLine);
	fInputLine->SetTarget (this);

	r.OffsetBy (0, 41);
	r.right += 60;
	fLoopBox = new BCheckBox (r, "Loopback", "Feedthrough to output",
							  new BMessage (PREF_LOOP_ENABLE));
	fLoopBox->SetValue (fEnableLoop);
	fLoopBox->SetTarget (this);
	AddChild (fLoopBox);

	BPoint where (160, 27);
	fSliderADC = new TStereoSlider (where, NULL, fADCL, fADCR,
									this, PREF_ADC_LEFT, PREF_ADC_RIGHT,
									-1, false);

	where.x += 90;
	fSliderDAC = new TStereoSlider (where, NULL, fDACL, fDACR,
									this, PREF_DAC_LEFT, PREF_DAC_RIGHT,
									PREF_DAC_ENABLE, !fEnableDAC);
	where.x += 70;
	fSliderSpeaker = new TMonoSlider (where, NULL, fSpeaker,
									  this, PREF_SPEAKER_GAIN,
									  PREF_SPEAKER_ENABLE, !fEnableSpeaker);
}

//--------------------------------------------------------------------

void TSoundView::MessageReceived(BMessage* theMessage)
{
  switch (theMessage->what) {
  case PREF_CD_ENABLE:
	fEnableCD = !fEnableCD;
	fDACStream->EnableDevice(B_CD_THROUGH,fEnableCD);
	break;

  case PREF_DAC_ENABLE:
	fEnableDAC = !fEnableDAC;
	fDACStream->EnableDevice(B_DAC_OUT, fEnableDAC);
	break;

  case PREF_LINE_ENABLE:
	fEnableLine = !fEnableLine;
	fDACStream->EnableDevice(B_LINE_IN_THROUGH, fEnableLine);
	break;

  case PREF_ADC_ENABLE:
	fEnableADC = !fEnableADC;
	fDACStream->EnableDevice(B_ADC_IN, fEnableADC);
	break;

  case PREF_MASTER_ENABLE:
	fEnableMaster = !fEnableMaster;
	fDACStream->EnableDevice(B_MASTER_OUT,fEnableMaster);
	break;

  case PREF_SPEAKER_ENABLE:
	fEnableSpeaker = !fEnableSpeaker;
	fDACStream->EnableDevice(B_SPEAKER_OUT, fEnableSpeaker);
	break;

  case PREF_LOOP_ENABLE:
	fEnableLoop = !fEnableLoop;
	fDACStream->EnableDevice(B_LOOPBACK, fEnableLoop);
	break;

  case PREF_LOOPBACK_ATTN:
	fLoopback = theMessage->FindFloat("value");
	fDACStream->SetVolume(B_LOOPBACK,fLoopback,fLoopback);
	break;

  case PREF_CD_LEFT:
	fCDL = theMessage->FindFloat("value");
	fDACStream->SetVolume(B_CD_THROUGH,fCDL, fCDR);
	break;

  case PREF_CD_RIGHT:
	fCDR = theMessage->FindFloat("value");
	fDACStream->SetVolume(B_CD_THROUGH, fCDL, fCDR);
	break;

  case PREF_DAC_LEFT:
	fDACL = theMessage->FindFloat("value");
	fDACStream->SetVolume(B_DAC_OUT,fDACL, fDACR);
	break;

  case PREF_DAC_RIGHT:
	fDACR = theMessage->FindFloat("value");
	fDACStream->SetVolume(B_DAC_OUT,fDACL, fDACR);
	break;

  case PREF_LINE_LEFT:
	fLineL = theMessage->FindFloat("value");
	fDACStream->SetVolume(B_LINE_IN_THROUGH,fLineL, fLineR);
	break;

  case PREF_LINE_RIGHT:
	fLineR = theMessage->FindFloat("value");
	fDACStream->SetVolume(B_LINE_IN_THROUGH,fLineL, fLineR);
	break;

  case PREF_ADC_LEFT:
	fADCL = theMessage->FindFloat("value");
	fDACStream->SetVolume(B_ADC_IN,fADCL, fADCR);
	break;

  case PREF_ADC_RIGHT:
	fADCR = theMessage->FindFloat("value");
	fDACStream->SetVolume(B_ADC_IN,fADCL, fADCR);
	break;

  case PREF_MASTER_LEFT:
	fMasterL = theMessage->FindFloat("value");
	fDACStream->SetVolume(B_MASTER_OUT,fMasterL, fMasterR);
	break;

  case PREF_MASTER_RIGHT:
	fMasterR = theMessage->FindFloat("value");
	fDACStream->SetVolume(B_MASTER_OUT,fMasterL, fMasterR);
	break;

  case PREF_SPEAKER_GAIN:
	fSpeaker = theMessage->FindFloat("value");
	fDACStream->SetVolume(B_SPEAKER_OUT,fSpeaker,fSpeaker);
	break;

  case PREF_CD_IN:
	if (fInput != B_CD_IN) {
	  fInput = B_CD_IN;
	  fADCStream->SetADCInput (fInput);
	  fMicBoost = FALSE;
	  fADCStream->BoostMic (fMicBoost);
	}
	break;

  case PREF_LINE_IN:
	if (fInput != B_LINE_IN) {
	  fInput = B_LINE_IN;
	  fADCStream->SetADCInput (fInput);
	  fMicBoost = FALSE;
	  fADCStream->BoostMic (fMicBoost);
	}
	break;

  case PREF_MIC_IN:
	if (fInput != B_MIC_IN || fMicBoost) {
	  fMicBoost = FALSE;
	  fADCStream->BoostMic (fMicBoost);
	  fInput = B_MIC_IN;
	  fADCStream->SetADCInput (fInput);
	}
	break;

  case PREF_MIC20_IN:
	if (fInput != B_MIC_IN || !fMicBoost) {
	  fMicBoost = TRUE;
	  fADCStream->BoostMic (fMicBoost);
	  fInput = B_MIC_IN;
	  fADCStream->SetADCInput (fInput);
	}
	break;

  case PREF_REVERT:
	server_request(REVERT_SOUND_HARDWARE_INFO);
	break;

  case PREF_DEFAULTS:
	server_request(DEFAULT_SOUND_HARDWARE_INFO);
	break;

  default:
    BView::MessageReceived(theMessage);
	break;
  }
}

//--------------------------------------------------------------------

void TSoundView::Pulse()
{
	float	cdL;
	float	cdR;
	float	dacL;
	float	dacR;
	float	lineL;
	float	lineR;
	float	ADCL;
	float	ADCR;
	float	masterL;
	float	masterR;
	float	speaker;
	float	loopback;
	bool	enableCD;
	bool	enableDAC;
	bool	enableLine;
	bool	enableADC;
	bool	enableMaster;
	bool	enableSpeaker;
	bool	enableLoopback;
	int32	input;
	bool	micBoost;

	if (!fDACStream)
	  return;

	if (fSliderCD)
	  if (!fDACStream->GetVolume(B_CD_THROUGH, &cdL, &cdR, &enableCD)) {
		if (cdL != fCDL) {
		  fCDL = cdL;
		  fSliderCD->SetLValue(fCDL);
		}
		if (cdR != fCDR) {
		  fCDR = cdR;
		  fSliderCD->SetRValue(fCDR);
		}
		if (enableCD != fEnableCD) {
		  fEnableCD = enableCD;
		  fSliderCD->SetMute(!fEnableCD);
		}
	  }


	if (fSliderDAC)
	  if (!fDACStream->GetVolume(B_DAC_OUT, &dacL, &dacR, &enableDAC)) {
		if (dacL != fDACL) {
		  fDACL = dacL;
		  fSliderDAC->SetLValue(fDACL);
		}
		if (dacR != fDACR) {
		  fDACR = dacR;
		  fSliderDAC->SetRValue(fDACR);
		}
		if (enableDAC != fEnableDAC) {
		  fEnableDAC = enableDAC;
		  fSliderDAC->SetMute(!fEnableDAC);
		}
	  }

	if (fSliderLine)
	  if (!fDACStream->GetVolume(B_LINE_IN_THROUGH, &lineL, &lineR, &enableLine)) {
		if (lineL != fLineL) {
		  fLineL = lineL;
		  fSliderLine->SetLValue(fLineL);
		}
		if (lineR != fLineR) {
		  fLineR = lineR;
		  fSliderLine->SetRValue(fLineR);
		}
		if (enableLine != fEnableLine) {
		  fEnableLine = enableLine;
		  fSliderLine->SetMute(!fEnableLine);
		}
	  }

	if (fSliderADC)
	  if (!fDACStream->GetVolume(B_ADC_IN, &ADCL, &ADCR, &enableADC)) {
		if (ADCL != fADCL) {
		  fADCL = ADCL;
		  fSliderADC->SetLValue(fADCL);
		}
		if (ADCR != fADCR) {
		  fADCR = ADCR;
		  fSliderADC->SetRValue(fADCR);
		}
		if (enableADC != fEnableADC) {
		  fEnableADC = enableADC;
		  fSliderADC->SetMute(!fEnableADC);
		}
	  }

	if (fSliderMaster)
	  if (!fDACStream->GetVolume(B_MASTER_OUT, &masterL, &masterR, &enableMaster)) {
		if (masterL != fMasterL) {
		  fMasterL = masterL;
		  fSliderMaster->SetLValue(fMasterL);
		}
		if (masterR != fMasterR) {
		  fMasterR = masterR;
		  fSliderMaster->SetRValue(fMasterR);
		}
		if (enableMaster != fEnableMaster) {
		  fEnableMaster = enableMaster;
		  fSliderMaster->SetMute(!fEnableMaster);
		}
	  }

	if (fSliderSpeaker)
	  if (!fDACStream->GetVolume(B_SPEAKER_OUT, &speaker, NULL, &enableSpeaker)) {
		if (speaker != fSpeaker) {
		  fSpeaker = speaker;
		  fSliderSpeaker->SetValue(fSpeaker);
		}
		if (enableSpeaker != fEnableSpeaker) {
		  fEnableSpeaker = enableSpeaker;
		  fSliderSpeaker->SetMute(!fEnableSpeaker);
		}
	  }

	if (fSliderLoop)
	  if (!fDACStream->GetVolume(B_LOOPBACK, &loopback, NULL, &enableLoopback)) {
		if (loopback != fLoopback) {
		  fLoopback = loopback;
		  fSliderLoop->SetValue(fLoopback);
		}
		if (enableLoopback != fEnableLoop) {
		  fEnableLoop = enableLoopback;
		  fSliderLoop->SetMute(!fEnableLoop);
		}
	  }

	if (fLoopBox)
	  if (!fDACStream->GetVolume(B_LOOPBACK, &loopback, NULL, &enableLoopback))
		if (enableLoopback != fEnableLoop) {
		  fEnableLoop = enableLoopback;
		  fLoopBox->SetValue(fEnableLoop);
		}

	if (!fADCStream)
	  return;

	fADCStream->ADCInput(&input);
	micBoost = fADCStream->IsMicBoosted();

	if (input >= 0)
	  if (input != fInput || micBoost != fMicBoost) {
		fInput = input;
		fMicBoost = micBoost;
		if (fInput == B_CD_IN)
		  fInputCD->SetValue (B_CONTROL_ON);
		else if (fInput == B_LINE_IN)
		  fInputLine->SetValue (B_CONTROL_ON);
		else if (fInput == B_MIC_IN)
		  if (fMicBoost)
			fInputMIC20->SetValue (B_CONTROL_ON);
		  else
			fInputMIC->SetValue (B_CONTROL_ON);
	  }
}

//--------------------------------------------------------------------

void
TSoundView::DrawBox (BRect box, char* label, bool label_only)
{
  BRect r (box);
  if (!label_only) {
	SetHighColor (100, 100, 100);
	StrokeRect (r);
	r.OffsetBy (1, 1);
	SetHighColor (255, 255, 255);
	StrokeRect (r);
  }
  if (label) {
	SetFont (be_bold_font);
	SetFontSize (12);
	SetHighColor (ViewColor());
	float width = StringWidth (label);
	BPoint where ((box.left + box.right - width) / 2 + 1, box.top + 5);
	r = BRect (where.x - 3, box.top, where.x + width + 1, where.y);
	FillRect (r);
	SetHighColor (0, 0, 0);
	MovePenTo (where);
	DrawString (label);
  }
}

void
TSoundView::DrawArrow (BPoint tail, BPoint head, float size)
{
  static const uchar shade = 80;
  SetHighColor (0, 0, 0);
  StrokeLine (tail, head);

  float angle;
  float span = 50;
  BRect r (head, head);
  r.InsetBy (-size, -size);
  if (head.x == tail.x) {
	r.InsetBy (-size, 0);
	angle = (head.y < tail.y) ? 270 : 90;
  }
  else if (head.y == tail.y) {
	r.InsetBy (0, -size);
	angle = (head.x < tail.x) ? 0 : 180;
	span = 56;
  }
  else
	return;

  SetHighColor (shade, shade, shade);
  FillArc (r, angle - span / 2 - 8, span + 16);
  SetHighColor (0, 0, 0);
  FillArc (r, angle - span / 2, span);
}

void
TSoundView::Draw(BRect)
{
  SetDrawingMode (B_OP_COPY);
  SetLowColor (ViewColor());

  switch (Platform()) {
  case B_MAC_PLATFORM:		DrawMacWindow();	break;
  case B_AT_CLONE_PLATFORM:	DrawIntelWindow();	break;
  default:					DrawBeBoxWindow();	break;
  }
}

void
TSoundView::DrawBeBoxWindow()
{
  BPoint where = fInputCD->Frame().LeftTop() - BPoint (20, 22);
  BRect input_rect (where, where + BPoint (108, 144));
  DrawBox (input_rect, "Input Source");

  where = fSliderADC->fLSliderView->Frame().LeftTop() - BPoint (13, 12);
  BRect gain_rect (where, where + BPoint (53, 166));
  DrawBox (gain_rect, "Gain");

  where = fSliderLoop->fSliderView->Frame().LeftTop() - BPoint (13, 12);
  BRect loop_rect (where, where + BPoint (53, 166));
  DrawBox (loop_rect, "Loopback");

  where = fSliderDAC->fLSliderView->Frame().LeftTop() - BPoint (17, 27);
  BRect output_rect (where, where + BPoint (180, 185));
  DrawBox (output_rect, "Output Mix");

  where = fSliderMaster->fLSliderView->Frame().LeftTop() - BPoint (13, 12);
  BRect master_rect (where, where + BPoint (53, 166));
  DrawBox (master_rect, "Master");

  where = fSliderSpeaker->fSliderView->Frame().LeftTop() - BPoint (13, 12);
  BRect speaker_rect (where, where + BPoint (53, 166));
  DrawBox (speaker_rect, "Speaker");

  BPoint head ((gain_rect.left + gain_rect.right) / 2, gain_rect.top - 7);
  BPoint tail (head.x, input_rect.bottom + 2);
  DrawArrow (tail, head, 5);

  head.Set ((gain_rect.left + gain_rect.right) / 2, gain_rect.bottom + 30);
  tail.Set (head.x, gain_rect.bottom + 2);
  DrawArrow (tail, head, 5);
  MovePenBy (1 - StringWidth ("ADC") / 2, 12);
  DrawString ("ADC");

  MovePenBy (3, -5);
  head.Set ((gain_rect.right + loop_rect.left) / 2 - 2, PenLocation().y);
  StrokeLine (head);
  tail.Set (head.x, (loop_rect.top + loop_rect.bottom) / 2);
  StrokeLine (tail);
  head.Set (loop_rect.left - 2, tail.y);
  DrawArrow (tail, head, 5);

  head.Set ((loop_rect.left + loop_rect.right) / 2, loop_rect.bottom + 30);
  tail.Set (head.x, loop_rect.bottom + 2);
  DrawArrow (tail, head, 5);
  MovePenBy (1 - StringWidth ("DAC") / 2, 12);
  DrawString ("DAC");

  head.Set ((master_rect.left + master_rect.right) / 2,
			master_rect.top - 7);
  tail.Set (head.x, output_rect.bottom + 2);
  DrawArrow (tail, head, 5);

  head.Set (speaker_rect.right + 2,
			(speaker_rect.top + speaker_rect.bottom) / 2);
  tail.Set (master_rect.left - 1, head.y);
  DrawArrow (tail, head, 5);

  head.Set ((speaker_rect.left + speaker_rect.right) / 2,
			speaker_rect.bottom + 15);
  tail.Set (head.x, speaker_rect.bottom + 2);
  StrokeLine (tail, head);
  BRect dest (head.x - 11, head.y + 1, head.x + 12, head.y + 16);
  BRect source (dest);
  source.OffsetTo (74, 1);
  DrawBitmap (fSchematic, source, dest);
  MovePenBy (1 - StringWidth ("Speaker") / 2, 27);
  DrawString ("Speaker");

  tail.Set ((master_rect.left + master_rect.right) / 2,
			master_rect.bottom + 2);
  head.Set (tail.x, master_rect.bottom + 9);
  StrokeLine (tail, head);
  head.x += 18;
  StrokeLine (head);
  DrawArrow (head, head + BPoint (0, 21), 5);
  MovePenBy (1 - StringWidth ("Line Out") / 2, 12);
  DrawString ("Line Out");
  tail.y = master_rect.bottom + 9;
  head.Set (tail.x - 39, tail.y);
  StrokeLine (tail, head);
  head.y += 6;
  StrokeLine (head);
  dest.Set (head.x - 10, head.y + 1, head.x + 10, head.y + 18);
  source = dest;
  source.OffsetTo (38, 0);
  DrawBitmap (fSchematic, source, dest);
  MovePenBy (1 - StringWidth ("Phones") / 2, 27);
  DrawString ("Phones");

  fSliderCD->DrawLabel();
  fSliderDAC->DrawLabel();
  fSliderLine->DrawLabel();
  fSliderADC->DrawLabel();
  fSliderMaster->DrawLabel();
  fSliderSpeaker->DrawLabel();
  fSliderLoop->DrawLabel();

  tail.Set ((loop_rect.right + output_rect.left) / 2 - 1, 8);
  head.Set (tail.x, Frame().bottom - 8);
  SetHighColor (184, 184, 184);
  StrokeLine (tail, head);
  tail.x += 1;
  head.x += 1;
  SetHighColor (255, 255, 255);
  StrokeLine (tail, head);

  //SetDrawingMode(B_OP_COPY);
  //DrawBitmap(fSchematic, BPoint(0, 0));
}

void
TSoundView::DrawIntelWindow()
{
  BPoint where = fInputCD->Frame().LeftTop() - BPoint (16, 22);
  BRect input_rect (where, where + BPoint (101, 144));
  DrawBox (input_rect, "Input Source");

  where = fSliderADC->fLSliderView->Frame().LeftTop() - BPoint (13, 12);
  BRect gain_rect (where, where + BPoint (53, 166));
  DrawBox (gain_rect, "Input");

  where = fSliderDAC->fLSliderView->Frame().LeftTop() - BPoint (17, 27);
  BRect output_rect (where, where + BPoint (258, 185));
  DrawBox (output_rect, "Levels");

  where = fSliderMaster->fLSliderView->Frame().LeftTop() - BPoint (13, 12);
  BRect master_rect (where, where + BPoint (53, 166));
  DrawBox (master_rect, "Output");

  where = fSliderSpeaker->fSliderView->Frame().LeftTop() - BPoint (13, 12);
  BRect speaker_rect (where, where + BPoint (53, 166));
  DrawBox (speaker_rect, "Speaker");

  BPoint head (gain_rect.left - 1, (input_rect.top + input_rect.bottom) / 2);
  BPoint tail (input_rect.right + 2, head.y);
  DrawArrow (tail, head, 5);

  head.Set ((gain_rect.left + gain_rect.right) / 2, gain_rect.bottom + 30);
  tail.Set (head.x, gain_rect.bottom + 2);
  DrawArrow (tail, head, 5);
  MovePenBy (1 - StringWidth ("ADC") / 2, 12);
  DrawString ("ADC");

  head.Set ((speaker_rect.left + speaker_rect.right) / 2,
			speaker_rect.bottom + 15);
  tail.Set (head.x, speaker_rect.bottom + 2);
  StrokeLine (tail, head);
  BRect dest (head.x - 11, head.y + 1, head.x + 12, head.y + 16);
  BRect source (dest);
  source.OffsetTo (74, 1);
  DrawBitmap (fSchematic, source, dest);
  MovePenBy (1 - StringWidth ("Speaker") / 2, 27);
  DrawString ("Speaker");

  tail.Set ((master_rect.left + master_rect.right) / 2,
			master_rect.bottom + 2);
  head.Set (tail.x, master_rect.bottom + 30);
  DrawArrow (tail, head, 5);
  MovePenBy (1 - StringWidth ("Line Out") / 2, 12);
  DrawString ("Line Out");

  fSliderCD->DrawLabel();
  fSliderDAC->DrawLabel();
  fSliderLine->DrawLabel();
  fSliderADC->DrawLabel();
  fSliderMaster->DrawLabel();
  fSliderSpeaker->DrawLabel();
  fSliderLoop->DrawLabel();
}

void
TSoundView::DrawMacWindow()
{
  SetDrawingMode (B_OP_COPY);
  SetLowColor (ViewColor());

  BPoint where = fInputMIC->Frame().LeftTop() - BPoint (20, 16);
  BRect input_rect (where, where + BPoint (108, 121));
  DrawBox (input_rect, "Source");

  where = fSliderADC->fLSliderView->Frame().LeftTop() - BPoint (13, 12);
  BRect gain_rect (where, where + BPoint (53, 166));
  DrawBox (gain_rect, "Input", true);

  where = fSliderDAC->fLSliderView->Frame().LeftTop() - BPoint (13, 12);
  BRect dac_rect (where, where + BPoint (53, 166));
  DrawBox (dac_rect, "Output", true);

  where = fSliderSpeaker->fSliderView->Frame().LeftTop() - BPoint (13, 12);
  BRect speaker_rect (where, where + BPoint (53, 166));
  DrawBox (speaker_rect, "Speaker", true);

  BPoint head (gain_rect.left - 1,
			   (gain_rect.top + gain_rect.bottom) / 2);
  BPoint tail (input_rect.right + 2, head.y);
  DrawArrow (tail, head, 5);

  /*
  head.Set ((gain_rect.left + gain_rect.right) / 2, gain_rect.bottom + 36);
  tail.Set (head.x, gain_rect.bottom + 2);
  DrawArrow (tail, head, 5);
  MovePenBy (1 - StringWidth ("ADC") / 2, 12);
  DrawString ("ADC");

  head.Set (gain_rect.left - 1,
			(gain_rect.top + gain_rect.bottom) / 2);
  tail.Set (input_rect.right + 2, head.y);
  DrawArrow (tail, head, 5);

  head.Set ((input_rect.left + input_rect.right) / 2, input_rect.bottom + 16);
  tail.Set (head.x, input_rect.bottom + 2);
  DrawArrow (tail, head, 5);

  tail.y = head.y + 16;
  head.y = input_rect.bottom + 50;
  DrawArrow (tail, head, 5);
  MovePenBy (1 - StringWidth ("DAC") / 2, 12);
  DrawString ("DAC");

  head.Set (speaker_rect.left - 1,
			(speaker_rect.top + speaker_rect.bottom) / 2);
  tail.Set (dac_rect.right + 2, head.y);
  DrawArrow (tail, head, 5);
  */

  head.Set ((speaker_rect.left + speaker_rect.right) / 2,
			speaker_rect.bottom + 8);
  tail.Set (head.x, speaker_rect.bottom + 2);
  StrokeLine (tail, head);
  BRect dest (head.x - 11, head.y + 1, head.x + 12, head.y + 16);
  BRect source (dest);
  source.OffsetTo (74, 1);
  DrawBitmap (fSchematic, source, dest);
  //  MovePenBy (1 - StringWidth ("Speaker") / 2, 27);
  //  DrawString ("Speaker");

  tail.Set ((dac_rect.left + dac_rect.right) / 2,
			dac_rect.bottom + 2);
  head.Set (tail.x, dac_rect.bottom + 8);
  StrokeLine (tail, head);
  dest.Set (head.x - 10, head.y + 1, head.x + 10, head.y + 18);
  source = dest;
  source.OffsetTo (38, 0);
  DrawBitmap (fSchematic, source, dest);
  //  MovePenBy (1 - StringWidth ("Phones") / 2, 27);
  //  DrawString ("Phones");

  //  fSliderCD->DrawLabel();
  fSliderDAC->DrawLabel();
  //  fSliderLine->DrawLabel();
  fSliderADC->DrawLabel();
  //  fSliderMaster->DrawLabel();
  fSliderSpeaker->DrawLabel();
  //  fSliderLoop->DrawLabel();

  tail.Set ((gain_rect.right + dac_rect.left) / 2 - 1, 8);
  head.Set (tail.x, Frame().bottom - 8);
  SetHighColor (184, 184, 184);
  StrokeLine (tail, head);
  tail.x += 1;
  head.x += 1;
  SetHighColor (255, 255, 255);
  StrokeLine (tail, head);

  //SetDrawingMode(B_OP_COPY);
  //DrawBitmap(fSchematic, BPoint(0, 0));
}


char schematic[] = {0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x15,
0xa,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x15,0xb,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x0,0x15,0x15,
0x16,0x15,0x15,0x15,0x16,0x0,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x15,0x16,0x15,0x15,0x15,0x16,0x15,0x15,0x15,0x16,0x15,0x15,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x0,0x0,0x16,0x15,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0xf,0x15,0x0,
0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x15,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x15,0x15,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x15,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x16,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x15,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x0,0x0,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x0,0x1d,0x1d,
0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,
0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x0,0x1d,0x1d,0x0,0x15,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x0,0x15,0x0,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x1d,
0x0,0x0,0x0,0x0,0x1d,0x1d,0x0,0x0,0x0,0x0,0x1d,0x1d,0x0,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x15,0x1d,0x0,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x0,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x0,0x15,0x15,0x15,0x0,0x1d,0x1d,0x0,0x15,
0x15,0x1d,0x0,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,
0x0,0x0,0x1d,0x0,0x0,0x0,0x1d,0x1d,0x1d,0x0,0x0,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x0,0x1d,0x0,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x0,0x1d,0x1d,0x0,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x0,0x15,0x15,
0x15,0x16,0x0,0x1d,0x1d,0x0,0x15,0x16,0x15,0x1d,0x0,0x1d,0x1d,0x0,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x0,0x1d,0x1d,0x0,0x1d,
0x0,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x0,0x1d,0x1d,0x0,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x0,0x1d,0x0,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x0,0x1d,0x1d,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1d,0x1d,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,
0x0,0x1d,0x0,0x1d,0x1d,0x0,0x1d,0x0,0x0,0x0,0x0,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x0,0x1d,0x0,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x0,0x1d,0x1d,0x0,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x0,0x1d,0x1d,0x0,0x1d,0x0,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x0,0x1d,0x1d,0x0,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x1d,0x1d,0x0,0x1d,0x0,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,
0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x0,0x0,0x0,0x1d,0x1d,0x0,
0x1d,0x0,0x1d,0x1d,0x0,0x1d,0x1d,0x0,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x0,0x1d,0x0,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,
0x1d,0x0,0x15,0x15,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x16,0x15,0x15,0x15,0x16,0x15,0x15,0x15,0x16,0x15,
0x15,0x15,0x16,0x15,0x15,0x15,0x16,0x15,0x15,0x15,0x16,0x15,0x15,0x15,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x16,0x0,0x15,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x0,0x0,0x16,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,0x1d,
0x1d,0x1d,0x1d,0x1d,0x1d};
