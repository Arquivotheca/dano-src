#include <MenuItem.h>
#include <PopUpMenu.h>

#include "knob.h"
#include "midi.h"
#include "window.h"

UView::UView(BRect frame, char* name, int fd)
  : BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW), fd(fd)
{
}

BMessage*
UView::NewMsg(int32 address, int32 value, uint32 what)
{
  BMessage* msg = new BMessage(what);
  msg->AddInt32("fd", fd);
  if (address) {
	msg->AddInt32("address", address);
	if (what == U_MENU_MODIFIED)
	  msg->AddInt32("value", value);
  }
  return msg;
}

BMessage*
UView::NewMenuMsg(int32 address, int32 value)
{
  return NewMsg(address, value, U_MENU_MODIFIED);
}

UKnob*
UView::AddKnob(BPoint where, char* name, int32 address, rgb_color color)
{
  UKnob* knob = new UKnob(where, name, fd, address, NewMsg(address), color);
  knob->SetValue((address & 0xfffffff0) == 0x404000 ? 100 : 64);
  knob->SetTarget(be_app);
  AddChild(knob);
  return knob;
}

USlider*
UView::AddSlider(BRect r, char* name, int32 address, char* label)
{
  USlider* slider = new USlider(r, name, fd, address);
  slider->SetModificationMessage(NewMsg(address));
#if USE_CHANNEL_SLIDER
  slider->SetLimits(0, 127);
  slider->SetLimitLabels(label, "");
#else
  slider->SetLimitLabels("", label);
#endif
  slider->SetValue(100);
  slider->SetTarget(be_app);
  AddChild(slider);
  return slider;
}

UCheckBox*
UView::AddCheckBox(BRect r, char* name, int32 address)
{
  UCheckBox* box = new UCheckBox(r, name, fd, address, NewMsg(address));
  box->SetValue(address == 0x401206 || address == 0x401406);
  box->SetTarget(be_app);
  AddChild(box);
  return box;
}

UMenu*
UView::AddMenu(BRect r, char* name, int32 address, BMenu* menu)
{
  UMenu* umenu = new UMenu(r, name, fd, address, menu);
  menu->SetTargetForItems(be_app);
  umenu->SetDivider(0);
  AddChild(umenu);
  return umenu;
}

ULED*
UView::AddLED(BPoint where, char* name , int32 address)
{
  BRect r(0, 0, 9, 9);
  BeginPicture(new BPicture);
  SetHighColor(120, 0, 0);
  FillEllipse(r);
  SetHighColor(WHITE);
  StrokeArc(BPoint(4, 4), 2, 2, 90, 90);
  BPicture* off = EndPicture();

  BeginPicture(new BPicture);
  SetHighColor(255, 0, 50);
  FillEllipse(r);
  SetHighColor(WHITE);
  StrokeArc(BPoint(4, 4), 2, 2, 90, 90);
  BPicture* on = EndPicture();

  r.OffsetTo(where);
  ULED* led = new ULED(r, name, fd, address,
					   new BMessage(B_CONTROL_MODIFIED), off, on);
  ((MixWindow*) Window())->leds[address & 7] = led;
  led->SetTarget(be_app);
  AddChild(led);
  delete off;
  delete on;
  return led;
}

UButton*
UView::AddButton(BPoint where, char* name, int32 address)
{
  UButton* b = new UButton(BRect(where, where + BPoint(24, 20)),
						   name, fd, address,
						   NewMsg(address, 0, U_BUTTON_PRESSED));
  b->SetTarget(be_app);
  AddChild(b);
  return b;
}

void
UMenu::ReceiveValue(int32 v)
{
  BMenu* menu = Menu();
  for (int i = 0;; i++) {
	BMenuItem* item = menu->ItemAt(i);
	if (!item)
	  break;
	BMessage* msg = item->Message();
	int32 value;
	if (msg->FindInt32("value", &value) == B_OK)
	  if (value == v) {
		if (!item->IsMarked() && LockLooper()) {
		  item->SetMarked(true);
		  Invalidate();
		  UnlockLooper();
		}
		break;
	  }
  }
  if (address == 0x404000)
	((MixWindow*) Window())->SetOperatingMode(v);
  /*
  if (address == 0x401000)
	((MixWindow*) Window())->SetInputMode(v);
  */
}

int32
UMenu::GetValue()
{
  int32 value = 0;
  BMenuItem* item = Menu()->FindMarked();
  if (item)
	item->Message()->FindInt32("value", &value);
  return value;
}

ULED::ULED(BRect r, char* name, int fd, int32 address,
		   BMessage* msg, BPicture* off, BPicture* on)
  : BPictureButton(r, name, off, on, msg, B_TWO_STATE_BUTTON,
				   B_FOLLOW_TOP | B_FOLLOW_H_CENTER),
	UControl(this, fd, address)
{
  SetViewColor(GRAY);
}

void
ULED::ReceiveValue(int32 value)
{
  if (value != Value())
	if (LockLooper()) {
	  SetValue(value);
	  if (value)
		((MixWindow*) Window())->EnableEffect(address & 7);
	  Invalidate();
	  UnlockLooper();
	}
}

void
ULED::SendValue(BMessage* msg)
{
  if (Value()) {
	MixWindow* win = (MixWindow*) Window();
	UTopView* view = win->top;
	int32 n = address & 7;
	BMenuItem* item = view->FXMenu(n)->Menu()->FindMarked();
	int32 effect;
	if (item && item->Message()->FindInt32("value", &effect) == B_OK)
	  DT1(fd, 0x400000 + (n << 8), effect);
	win->EnableEffect(n);
  }	
  UControl::SendValue(msg);
}

void
UButton::ReceiveValue(int32 v)
{
  if (address == 0x401000) {
	MixWindow* win = my_app->FindWindow(fd);
	if (win)
	  win->SetInputMode(v);
  }
}

void
UButton::SendValue(BMessage* msg)
{
  int32 fd;
  int32 address;
  MixWindow* win = (MixWindow*) Window();

  if (win)
	if (msg->FindInt32("fd", &fd) == B_OK)
	  if (msg->FindInt32("address", &address) == B_OK)
		if (address == 0x401000) {
		  // input mode button
		  win->SetInputMode((win->input_mode + 1) % 3);
		  DT1(fd, address, win->input_mode);
		}
}

int32
UButton::GetValue()
{
  if (address == 0x401000) {
	MixWindow* win = my_app->FindWindow(fd);
	if (win)
	  return win->input_mode;
  }
  return control->Value();
}

void
UCheckBox::SendValue(BMessage* msg)
{
  // radio button behavior for solo controls
  if ((address & 0xfffff8ff) == 0x401007 && Value())
	for (int32 a = 0x401107; a <= 0x401407; a += 0x100)
	  if (address == a)
		DT1(fd, a, true);
	  else {
		DT1(fd, a, false);
		RQ1(fd, a);
	  }
  else
	UControl::SendValue(msg);
}
