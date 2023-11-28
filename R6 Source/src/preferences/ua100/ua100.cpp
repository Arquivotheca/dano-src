#include <Control.h>
#include <MenuItem.h>

#include "midi.h"
#include "ua100.h"
#include "window.h"
#include "settings.h"

struct UApp* my_app;

int
main (int, char**)
{
  my_app = new UApp();
  my_app->Run();
  delete my_app;
  my_app = NULL;
  return 0;
}

UApp::UApp() : BApplication("application/x-vnd.Be-" APP_NAME)
{
}

void
UApp::ReadyToRun()
{
  char name[B_PATH_NAME_LENGTH];
  int fds[MAX_DEVS];
  int n = 0;

  for (int i = 0; i < MAX_DEVS; i++) {
	sprintf(name, "/dev/midi/ua100/%d/3", i + 1);
	fds[i] = open(name, O_RDWR);
	if (fds[i] >= 0)
	  ++n;
	mix_wins[i] = NULL;
  }

  if (n == 0)
	new MixWindow(BPoint (60, 40), APP_NAME, -1);
  else for (int i = 0; i < MAX_DEVS; i++)
	if (fds[i] >= 0)
	  if (n == 1)
		mix_wins[i] = new MixWindow(BPoint (60, 40), APP_NAME, fds[i]);
	  else {
		sprintf(name, APP_NAME " %d", i + 1);
		mix_wins[i] = new MixWindow(BPoint (60+20*i, 40+20*i), name, fds[i]);
	  }
}

MixWindow*
UApp::FindWindow(int fd)
{
  for (int i = 0; i < MAX_DEVS; i++)
	if (mix_wins[i] && mix_wins[i]->reader->fd == fd)
	  return mix_wins[i];
  return NULL;
}

void
UApp::MenuModified(BMessage* msg)
{
  int32 fd;
  int32 address;
  int32 value;

  if (msg->FindInt32("fd", &fd) == B_OK) {
	MixWindow* win = FindWindow(fd);
	if (win)
	  if (msg->FindInt32("address", &address) == B_OK)
		if (msg->FindInt32("value", &value) == B_OK)
		  if (address == 0x400100) {
			// main effect menu
			for (int n = 1; n < 7; n++)
			  if (win->leds[n]->GetValue())
				if (n < 5 || win->operating_mode == 4)
				  DT1(fd, 0x400000 + (n << 8), value);
		  }
		  else if (address == 0x404000) {
			if (value != win->operating_mode && win->Lock()) {
			  UTopView* top = win->top;
			  BMenu* menu = top->insertion_menu->Menu();
			  if (value == 4)
				menu->FindItem("High Quality Reverb")->SetMarked(true);
			  else {
				menu->FindItem("Reverb")->SetMarked(true);
				for (int i = 0; i < 2; i++)
				  top->sys_menu[i]->Menu()->FindItem("Delay")->SetMarked(true);
			  }
			  for (int i = 1; i < 7; i++) {
				DT1(win->leds[i]->fd, win->leds[i]->address, 0);
				win->leds[i]->SetValue(false);
				win->leds[i]->Invalidate();
			  }
			  win->Unlock();
			}
			DT1(fd, address, value);
		  }
		  else
			DT1(fd, address, value);
  }
}

void
UApp::MessageReceived(BMessage* msg)
{
  void* source;
  int32 fd;

  switch(msg->what)	{
  case B_CONTROL_MODIFIED:
  case U_BUTTON_PRESSED:
	if (msg->FindPointer("source", &source) == B_OK)
	  (dynamic_cast<UControl*> ((BControl*) source))->SendValue(msg);
	break;
  case U_MENU_MODIFIED:
	MenuModified(msg);
	break;
  case U_LISTEN:
	if (msg->FindInt32("fd", &fd) == B_OK)
	  listen(fd);
	break;
  case U_USE_MIC1:
	if (msg->FindInt32("fd", &fd) == B_OK)
	  use_mic1(fd);
	break;
  case U_USE_LINE:
	if (msg->FindInt32("fd", &fd) == B_OK)
	  use_line(fd);
	break;
  case U_EFF_LOOP:
	if (msg->FindInt32("fd", &fd) == B_OK)
	  eff_loop(fd);
	break;
  case U_KARAOKE:
	if (msg->FindInt32("fd", &fd) == B_OK)
	  karaoke(fd);
	break;
  default:
	BApplication::MessageReceived(msg);
	break;
  }
}

void
UApp::ReceiveControlValue(int fd, int32 address, int32 value)
{
  for (int i = 0;; i++) {
	UControl* control = (UControl*) controls.ItemAt(i);
	if (!control)
	  break;
	if (fd == control->fd)
	  if (address == control->address)
		control->ReceiveValue(value);
  }
}

void
UApp::SendAllControls(int fd)
{
  for (int i = 0;; i++) {
	UControl* control = (UControl*) controls.ItemAt(i);
	if (!control)
	  break;
	if (fd == control->fd && control->address != 0x404000)
	  DT1(fd, control->address, control->GetValue());
  }
  // EFX TYPE
  MixWindow* win = FindWindow(fd);
  if (win)
	for (int i = 1; i < 7; i++)
	  if (win->leds[i]->GetValue()) {
		BMenuField* field = (win->operating_mode == 4
							 ? win->top->insertion_menu
							 : win->top->FXMenu(i));
		BMenuItem* item = field->Menu()->FindMarked();
		if (item) {
		  int32 value;
		  if (item->Message()->FindInt32("value", &value) == B_OK)
			DT1(fd, 0x400000 + (i < 8), value);
		}
	  }
}

UControl::UControl(BControl* control, int fd, int32 address)
  : control(control), fd(fd), address(address)
{
  my_app->controls.AddItem(this);
}

int32
UControl::GetValue()
{
  return control ? control->Value() : 0;
}

void
UControl::SendValue(BMessage*)
{
  if (control)
	DT1(fd, address, control->Value());
}

void
UControl::ReceiveValue(int32 value)
{
  if (control && value != control->Value())
	if (control->LockLooper()) {
	  control->SetValue(value);
	  control->Invalidate();
	  control->UnlockLooper();
	}
}

/*
  UControls:

   ULED			BPictureButton
   UCheckBox	BCheckBox
   USlider		BSlider
   UMenu		BMenuField
   UButton		BButton
   UKnob		BControl
*/
