#ifndef UWINDOW_H
#define UWINDOW_H

#include <View.h>
#include <Window.h>

#include "ua100.h"
#include "view.h"

struct MixWindow : BWindow
{
  MixWindow(BPoint where, char* name, int fd);
  ~MixWindow();

  void UpdateOutputMenu(BMenu*);
  void SetOperatingMode(int32);
  void SetInputMode(int32);
  void EnableEffect(int32 n);

  struct Reader* reader;
  struct UTopView* top;
  struct ULED* leds[7];
  int32 operating_mode;
  int32 input_mode;
};

struct UTopView : UView
{
  UTopView(BRect frame, char* name, int fd);

  void AttachedToWindow();
  void Draw(BRect);

  struct UChannel* channels[4];
  struct UKnob* pans[2];
  struct UMenu* master_menu;
  struct UMenu* wave_menu;
  struct BMenuField* insertion_menu;
  struct BMenuField* sys_menu[2];

  BMenuField* FXMenu(int32 n) {
	return n<5 ? insertion_menu : sys_menu[n-5];
  }
};

#endif
