//*****************************************************************************
//
//	File:			main.cpp
//
//	Description:	Simple General Midi Player
//
//	Copyright 1996, Be Incorporated. All Rights Reserved.
//
//*****************************************************************************

#include <Alert.h>
#include <FilePanel.h>
#include <stdlib.h>
#include <string.h>
#include <FindDirectory.h>
#include <Path.h>
#include <AudioStream.h>

#include "simple-midi.h"

int
main (int, char**)
{
  MidiPlayApp* app = new MidiPlayApp();
  app->Run();
  delete app;
  return 0;
}

MidiPlayApp::MidiPlayApp() : BApplication("application/x-vnd.Be-SGMP")
{
	new BSynth();
	status_t err = be_synth->LoadSynthData(B_BIG_SYNTH);
	if (err < B_NO_ERROR) {
		err = be_synth->LoadSynthData(B_LITTLE_SYNTH);
		if (err < B_NO_ERROR) {
			err = be_synth->LoadSynthData(B_TINY_SYNTH);
			if (err < B_NO_ERROR) {
				BPath path;
				char message[B_PATH_NAME_LENGTH + 100];
				if (err == B_BAD_INSTRUMENT
					|| (err & 0xfffff000) == B_STORAGE_ERROR_BASE) {
					err = find_directory(B_SYNTH_DIRECTORY, &path);
					sprintf(message, "Could not load instrument file: \n%s/%s",
						(err == B_OK ? path.Path() : ""), B_BIG_SYNTH_FILE);
				}
				else {
					BDACStream dac;
					float sr;
					if (err == B_SERVER_NOT_FOUND
						|| dac.SamplingRate(&sr) == B_SERVER_NOT_FOUND)
						sprintf(message, "Failed to connect to audio server.");
					else
						sprintf(message, "Error starting BMidiSynth: \n%s",
					strerror(err));
				}
				StatusReport(message);
				delete be_synth;
				be_synth = NULL;
				PostMessage(B_QUIT_REQUESTED);
				return;
			}
		}
	}
	be_synth->SetVoiceLimits(32, 0, 7);
	
	song = new BMidiSynthFile();
	win = new MidiPlayWindow (BPoint (120, 40));
	liveSynth = new BMidiSynth();
	livePort = new BMidiPort();
	if (livePort)
	livePort->Connect(liveSynth);
	instrumentsLoaded = false;
}

MidiPlayApp::~MidiPlayApp()
{
  Lock();
  StopSong();
  if (song) {
	delete song;
	song = NULL;
  }
  if (be_synth) {
//	be_synth->Unload();
	delete be_synth;
	be_synth = NULL;
  }
  Unlock();
}

void
MidiPlayApp::StatusReport(char* report)
{
  BAlert* alert = new BAlert(APP_NAME, report, "OK");
  if (alert)
	alert->Go();
}

void
MidiPlayApp::AboutRequested()
{
  StatusReport(APP_NAME " version 0.2\n"
			   "Drag Midi files onto this application to play.");
}

void
MidiPlayApp::StartSong(entry_ref* ref)
{
  if (song) {
	StopSong();
	if (song->LoadFile(ref) == B_NO_ERROR)
	  song->Start();
	else
	  StatusReport("Failed to load song.");
  }
}

void
MidiPlayApp::StopSong()
{
  if (song)
	song->Fade();
}

void
MidiPlayApp::RefsReceived(BMessage *msg)
{
  long n;
  ulong type;
  msg->GetInfo("refs", &type, &n);

  for (long i = 0; i < n; i++) {
	entry_ref ref;
	if (msg->FindRef("refs", i, &ref) == B_NO_ERROR)
	  StartSong(&ref);
  }
}

void
MidiPlayApp::ArgvReceived(long argc, char** argv)
{
	entry_ref ref;

	for (int i = 1; i < argc; i++) {
	  char* fname = argv[i];
	  printf("opening \"%s\"\n", fname);

	  if (get_ref_for_path(fname, &ref) == B_NO_ERROR)
		StartSong(&ref);
	  else {
		printf("Gack! can't get ref for path: %s\n", fname);
	  }
	}
}

void
MidiPlayApp::MessageReceived(BMessage *msg)
{
  switch(msg->what)	{
  case M_OPEN_FILE:
  {
	BFilePanel* panel = new BFilePanel();
	panel->Window()->SetTitle("Open Midi file");
	panel->Window()->Show();
	break;
  }

  case M_PLAY:
	if (song)
	  song->Start();
	break;
  case M_STOP:
	StopSong();
	break;
  case M_REVERB + B_REVERB_NONE:
  case M_REVERB + B_REVERB_CLOSET:
  case M_REVERB + B_REVERB_GARAGE:
  case M_REVERB + B_REVERB_BALLROOM:
  case M_REVERB + B_REVERB_CAVERN:
  case M_REVERB + B_REVERB_DUNGEON:
	if (be_synth)
	  be_synth->SetReverb((reverb_mode) (msg->what - M_REVERB));
	break;
  case M_Q22:
  case M_Q44:
	if (be_synth)
	  be_synth->SetSamplingRate(msg->what == M_Q44 ? 44100 : 22050);
	break;
  case M_VOLUME:
	/* [0.0, 1.0] => [0.125, 2.0] */
	if (be_synth) {
	  float volume;
	  if (msg->FindFloat("value", &volume) == B_NO_ERROR)
		be_synth->SetSynthVolume(pow (16.0, volume - 0.75));
	}
	break;
  case M_LIVE_OFF:
	if (livePort)
	  livePort->Close();
	break;
  case M_LIVE_ON:
  {
	if (!livePort || !liveSynth)
	  break;
	char* name;
	if (msg->FindString("port", (const char**) &name) != B_NO_ERROR)
	  break;
	if (livePort->Open(name) != B_NO_ERROR
		|| livePort->Start() != B_NO_ERROR) {
	  char report[32];
	  strcpy(report, "Failed to open ");
	  strcat(report, name);
	  StatusReport(report);
	  win->liveField->Menu()->FindItem(M_LIVE_OFF)->SetMarked(TRUE);
	  livePort->Close();
	  break;
	}
	if (!instrumentsLoaded) {
	  instrumentsLoaded = true;
	  win->StatusLine("Loading instruments...");
	  if (song)
		song->Stop();
	  liveSynth->EnableInput(true, true);
	  win->StatusLine(NULL);
	}
	break;
  }

  default:
	inherited::MessageReceived(msg);
	break;
  }
}
