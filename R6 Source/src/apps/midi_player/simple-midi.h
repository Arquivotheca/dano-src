//*****************************************************************************
//
//	File:			simple-midi.h
//
//	Description:	Simple General Midi Player
//
//	Copyright 1996, Be Incorporated. All Rights Reserved.
//
//*****************************************************************************

#include <Application.h>
#include <StringView.h>
#include <Synth.h>
#include <MidiSynthFile.h>
#include <MidiPort.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <CheckBox.h>

#include <stdio.h>
#include <math.h>

#define SCOPE

#define APP_NAME "Simple Midi Player"

#define M_OPEN_FILE			'open'
#define M_PLAY				'play'
#define M_STOP				'stop'
#define M_ENABLE_SCOPE		'scop'
#define M_REVERB			'rvrb'
#define M_VOLUME			'volu'
#define M_Q22				'q22k'
#define M_Q44				'q44k'
#define M_LIVE_OFF			'livx'
#define M_LIVE_ON			'live'

struct MidiPlayApp : BApplication
{
  MidiPlayApp();
  ~MidiPlayApp();

  void RefsReceived(BMessage* message);
  void ArgvReceived(long argc, char** argv);
  void MessageReceived(BMessage* message);
  bool QuitRequested() {return TRUE;};
  void AboutRequested();

  void StatusReport(char* report);
  void StartSong(entry_ref* ref);
  void StopSong();
  void SetSampleRate(long sample_rate);

  struct MidiPlayWindow* win;
  BMidiSynthFile* song;
  BMidiSynth* liveSynth;
  BMidiPort* livePort;
  bool instrumentsLoaded;

private:
	typedef BApplication inherited;
};

struct MidiPlayWindow : public BWindow
{
  MidiPlayWindow(BPoint where);

  void MessageReceived(BMessage* msg);
  bool QuitRequested() {be_app->PostMessage(B_QUIT_REQUESTED); return TRUE;};
  void StatusLine(char* line);

  struct ScopeView* scope;
  BCheckBox* scopeEnable;
  BMenuField* liveField;
};

#ifdef SCOPE

struct ScopeView : public BView
{
  ScopeView(BRect frame, char* name)
	: BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW) {};

  void AttachedToWindow();
  void Draw(BRect update);
  void Start();
  void Stop();
  void SetStatusLine(char* line);

  thread_id drawingThread;
  BStringView* statusLine;
  BRect bounds;
};

#endif
