#ifndef UCONTROL_H
#define UCONTROL_H

#define USE_CHANNEL_SLIDER 0

#if USE_CHANNEL_SLIDER
#include <ChannelSlider.h>
#else
#include <Slider.h>
#endif

#include <Button.h>
#include <PictureButton.h>
#include <CheckBox.h>
#include <MenuField.h>

struct UView : BView
{
  UView(BRect frame, char* name, int fd);

  virtual struct BMessage* NewMsg(int32 address, int32 value = -1,
								  uint32 what = B_CONTROL_MODIFIED);
  virtual struct BMessage* NewMenuMsg(int32 address, int32 value);

  virtual struct UKnob* AddKnob(BPoint, char*, int32, rgb_color);
  virtual struct USlider* AddSlider(BRect, char*, int32, char*);
  virtual struct UCheckBox* AddCheckBox(BRect, char*, int32);
  virtual struct ULED* AddLED(BPoint, char*, int32);
  virtual struct UMenu* AddMenu(BRect, char*, int32, BMenu*);
  virtual struct UButton* AddButton(BPoint, char*, int32);

  int fd;
};

struct ULED : BPictureButton, UControl
{
  ULED(BRect r, char* name, int fd, int32 address,
	   BMessage* msg, BPicture* off, BPicture* on);

  void ReceiveValue(int32 value);
  void SendValue(BMessage* msg);
};

struct UCheckBox : BCheckBox, UControl
{
  UCheckBox(BRect r, char* name, int fd, int32 address, BMessage* msg)
	: BCheckBox(r, name, name, msg, B_FOLLOW_TOP | B_FOLLOW_H_CENTER),
	  UControl(this, fd, address) {}

  void SendValue(BMessage* msg);
};

#if USE_CHANNEL_SLIDER
struct USlider : BChannelSlider, UControl
{
  USlider(BRect r, char* name, int fd, int32 address)
	: BChannelSlider(r, name, "", NULL, B_VERTICAL, 1,
					 B_FOLLOW_TOP | B_FOLLOW_H_CENTER),
	  UControl(this, fd, address) {}
};
#else
struct USlider : BSlider, UControl
{
  USlider(BRect r, char* name, int fd, int32 address)
	: BSlider(r, name, "", NULL, 0, 127, B_VERTICAL, B_BLOCK_THUMB,
			  B_FOLLOW_TOP | B_FOLLOW_H_CENTER),
	  UControl(this, fd, address) {}
};
#endif

struct UMenu : BMenuField, UControl
{
  UMenu(BRect r, char* name, int fd, int32 address, BMenu* menu)
	: BMenuField(r, name, "", menu), UControl(NULL, fd, address) {}

  void ReceiveValue(int32 v);
  int32 GetValue();
};

struct UButton : BButton, UControl
{
  UButton(BRect r, char* name, int fd, int32 address, BMessage* msg)
	: BButton(r, name, name, msg), UControl(this, fd, address) {}

  void ReceiveValue(int32 v);
  void SendValue(BMessage* msg);
  int32 GetValue();
};

#endif
