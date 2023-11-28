//*****************************************************************************
//
//	File:			window.cpp
//
//	Description:	Simple General Midi Player
//
//	Copyright 1996, Be Incorporated. All Rights Reserved.
//
//*****************************************************************************

#include <Button.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Window.h>

#include "simple-midi.h"
#include "slider.h"

#define WIN_SIZE (BPoint(330, 180))
//#define WIN_SIZE (BPoint(190, 180))

MidiPlayWindow::MidiPlayWindow(BPoint where)
  : BWindow(BRect(where, where + WIN_SIZE), APP_NAME,
			B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
  BMenuItem* item;
  BRect rect;

  Lock();

  /* Parent View */
  BView* view = new BView(Bounds(), "", B_FOLLOW_ALL, B_WILL_DRAW);
  AddChild(view);
  view->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);

#ifdef SCOPE

  /* Scope View */
  rect.Set(0, 0, 128, 84);
  rect.OffsetBy(130, 15);
  scope = new ScopeView(rect, "scope");
  view->AddChild(scope);

  /* Scope CheckBox */
  rect.Set(0, 0, 70, 15);
  rect.OffsetBy(170, 105);
  scopeEnable = new BCheckBox(rect, "Scope", "Scope",
							  new BMessage(M_ENABLE_SCOPE));
  view->AddChild(scopeEnable);
  scopeEnable->SetValue(B_CONTROL_ON);
  PostMessage(M_ENABLE_SCOPE);

#endif

  /* Buttons */
  BButton* button;
  rect.Set(0, 0, 92, 20);
  rect.OffsetBy(10, 10);
  button = new BButton(rect, "Open", "Open File...", new BMessage(M_OPEN_FILE));
  button->SetTarget(be_app);
  view->AddChild(button);
  rect.OffsetBy(0, 35);
  button = new BButton(rect, "Play", "Play", new BMessage(M_PLAY));
  button->SetTarget(be_app);
  view->AddChild(button);
  rect.OffsetBy(0, 35);
  button = new BButton(rect, "Stop", "Stop", new BMessage(M_STOP));
  button->SetTarget(be_app);
  view->AddChild(button);

  /* Live Input Menu */
  rect.right += 60;
  rect.OffsetBy(0, 40);
  BPopUpMenu* live = new BPopUpMenu("Live Input:");
  live->AddItem(new BMenuItem("off", new BMessage(M_LIVE_OFF)));
  {
	BMidiPort port;
	char name[B_PATH_NAME_LENGTH];
	int i = 0;
	while (port.GetDeviceName(i++, name, sizeof(name)) == B_NO_ERROR) {
	  BMessage* msg = new BMessage(M_LIVE_ON);
	  if (msg->AddString("port", name) == B_NO_ERROR)
		live->AddItem(new BMenuItem(name, msg));
	  else
		delete msg;
	}
  }
  live->SetTargetForItems(be_app);
  liveField = new BMenuField(rect, "Live Input:", "Live Input:", live);
  liveField->SetDivider(54);
  view->AddChild(liveField);
  live->FindItem(M_LIVE_OFF)->SetMarked(TRUE);

  /* Quality Menu */
  rect.right -= 20;
  rect.OffsetBy(0, 30);
  BPopUpMenu* qual = new BPopUpMenu("Quality:");
  qual->AddItem(new BMenuItem("22kHz", new BMessage(M_Q22)));
  qual->AddItem(new BMenuItem("44kHz", new BMessage(M_Q44)));
  qual->SetTargetForItems(be_app);
  BMenuField* qualField = new BMenuField(rect, "Quality:", "Quality:", qual);
  qualField->SetDivider(54);
  view->AddChild(qualField);
  qual->FindItem(M_Q44)->SetMarked(TRUE);
  be_app->PostMessage(M_Q44);

  /* Reverb Menu */
  rect.OffsetBy(140, 0);
  BPopUpMenu* rev = new BPopUpMenu("Reverb:");
  item = new BMenuItem("None", new BMessage(M_REVERB + B_REVERB_NONE));
  rev->AddItem(item);
  item = new BMenuItem("Closet", new BMessage(M_REVERB + B_REVERB_CLOSET));
  rev->AddItem(item);
  item = new BMenuItem("Garage", new BMessage(M_REVERB + B_REVERB_GARAGE));
  rev->AddItem(item);
  item = new BMenuItem("Igor's Lab", new BMessage(M_REVERB + B_REVERB_BALLROOM));
  rev->AddItem(item);
  item = new BMenuItem("Cavern", new BMessage(M_REVERB + B_REVERB_CAVERN));
  rev->AddItem(item);
  item = new BMenuItem("Dungeon", new BMessage(M_REVERB + B_REVERB_DUNGEON));
  rev->AddItem(item);
  rev->SetTargetForItems(be_app);
  BMenuField* revField = new BMenuField(rect, "Reverb:", "Reverb:", rev);
  revField->SetDivider(50);
  view->AddChild(revField);
  rev->FindItem(M_REVERB + B_REVERB_BALLROOM)->SetMarked(TRUE);
  be_app->PostMessage(M_REVERB + B_REVERB_BALLROOM);

  /* Volume */
  SliderView* vol;
  vol = new SliderView(BPoint(285, 10), "Volume", 0.75, be_app, M_VOLUME);
  //vol = new SliderView(BPoint(190, 10), "Volume", 0.75, be_app, M_VOLUME);
  view->AddChild(vol);

  Unlock();
  Show();
  //SetDiscipline(FALSE);
}

void
MidiPlayWindow::MessageReceived (BMessage* msg)
{
  switch(msg->what) {
#ifdef SCOPE
  case M_ENABLE_SCOPE:
	if (scopeEnable->Value())
	  scope->Start();
	else
	  scope->Stop();
	break;
#endif
  case B_REFS_RECEIVED:
  case B_SIMPLE_DATA:
	if (msg->what == B_SIMPLE_DATA)
	  msg->what = B_REFS_RECEIVED;
	be_app->PostMessage(new BMessage(*msg));
	break;
  default:
	inherited::MessageReceived(msg);
	break;
  }
}

void
MidiPlayWindow::StatusLine (char* line)
{
#ifdef SCOPE
  scope->SetStatusLine(line);
#endif
}

#ifdef SCOPE

static const rgb_color leftColor = {0, 0, 255, 0};		// blue
static const rgb_color rightColor = {255, 0, 0, 0};		// red
static const rgb_color monoColor = {0, 130, 0, 0};		// green
static const rgb_color overColor = {0, 255, 255, 0};	// yellow
static const rgb_color blackColor = {0, 0, 0, 0};		// black

long
scope_thread (void* data)
{
  ScopeView* view = (ScopeView*) data;
  BWindow* win = view->Window();

  while (win->Lock()) {
	if (view->drawingThread != find_thread(NULL)) {
	  win->Unlock();
	  break;
	}
	if (!view->statusLine)
	  if (((MidiPlayWindow*) win)->scopeEnable->Value())
		view->Draw(BRect());
	win->Unlock();
	snooze(70000);
  }

  return 0;
}

void
ScopeView::AttachedToWindow() {
  SetViewColor(B_TRANSPARENT_32_BIT);
  bounds = Bounds();
}

void
ScopeView::Start()
{
  statusLine = NULL;
  drawingThread = spawn_thread (scope_thread, "scope",
								B_DISPLAY_PRIORITY, this);
  resume_thread (drawingThread);
}

void
ScopeView::Stop()
{
  drawingThread = B_BAD_THREAD_ID;
}

void
ScopeView::Draw(BRect)
{
  short int audioLeft[1024];
  short int audioRight[1024];

  if (!be_synth)
	return;
  int samples = be_synth->GetAudio(audioLeft, audioRight, 1024);
  if (samples <= 0 || samples > 1024)
	return;
  bool flat = (-1 <= audioLeft[0] && audioLeft[0] <= 1);

  int width = bounds.IntegerWidth();
  float scale = bounds.Height() / -65536.0;

  BeginLineArray(2.5 * width);
  BPoint center = bounds.LeftTop() + BPoint(0, bounds.Height() / 2);


  /* STEREO */
  for (int count = 0; count < width - 1; count++, center.x++) {
	int index = count * samples / width;
	int left1 = audioLeft[index];
	int right1 = audioRight[index];
	index = (count + 1) * samples / width;
	int left2 = audioLeft[index];
	int right2 = audioRight[index];

	if (left1 != right1 || left2 != right2) {
	  AddLine (center + BPoint(0, scale * left1),
			   center + BPoint(1, scale * left2),
			   (left2 > 32500 || left2 < -32500) ? overColor : leftColor);
	  AddLine (center + BPoint(0, scale * right1),
			   center + BPoint(1, scale * right2),
			   (right2 > 32500 || right2 < -32500) ? overColor : rightColor);
	}
	if (left2 == right2)
	  if (left2 != 0 || flat)
		if (left2 <= 32500 && left2 >= -32500)
		  AddLine (center + BPoint(0, scale * (left1 + right1) / 2),
				   center + BPoint(1, scale * left2),
				   monoColor);
  }

  SetHighColor(blackColor);
  FillRect(bounds);
  EndLineArray();
  Flush();
}

void
ScopeView::SetStatusLine (char* line)
{
  BWindow* win = Window();
  if (win && win->Lock()) {
	if (statusLine) {
	  statusLine->RemoveSelf();
	  delete statusLine;
	  statusLine = NULL;
	}
	if (line) {
	  BRect rect = Bounds();
	  rect.InsetBy(0, 35);
	  statusLine = new BStringView(rect, "status", line);
	  AddChild(statusLine);
	  statusLine->SetFontSize(12);
	  statusLine->SetViewColor(blackColor);
	  statusLine->SetHighColor(monoColor);
	  statusLine->SetAlignment(B_ALIGN_CENTER);
	}
	win->Unlock();
  }
}

#endif
