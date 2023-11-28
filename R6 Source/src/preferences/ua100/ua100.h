#ifndef UA100_H
#define UA100_H

#define WITH_LOCK(B) {LockLooper();B;Invalidate();UnlockLooper();}

#include <Application.h>
#include <Control.h>
#include <List.h>

#include <stdio.h>

#define APP_NAME "UA-100"
#define MAX_DEVS 4
#define U_MENU_MODIFIED 'umen'
#define U_BUTTON_PRESSED 'ubpr'

const rgb_color BLACK = {0, 0, 0, 0};
const rgb_color RED = {255, 0, 0, 0};
const rgb_color YELLOW = {255, 255, 0, 0};
const rgb_color GREEN = {0, 255, 0, 0};
const rgb_color BLUE = {0, 0, 255, 0};
const rgb_color LT_BLUE = {0, 180, 255, 0};
const rgb_color CYAN = {0, 255, 255, 0};
const rgb_color GRAY = {168, 168, 168, 0};
const rgb_color WHITE = {255, 255, 255, 0};

extern struct UApp* my_app;

struct UApp : BApplication
{
  UApp();

  void MessageReceived(BMessage* message);
  void ReadyToRun();
  void MenuModified(BMessage* msg);
  void ReceiveControlValue(int fd, int32 address, int32 value);
  void SendAllControls(int fd);
  struct MixWindow* FindWindow(int fd);

  BList controls;
  struct MixWindow* mix_wins[MAX_DEVS];
};

struct UControl
{
  UControl(BControl* control, int fd, int32 address);

  virtual int32 GetValue();
  virtual void ReceiveValue(int32 value);
  virtual void SendValue(BMessage* msg);

  struct BControl* control;
  int fd;
  int32 address;
};

#endif
