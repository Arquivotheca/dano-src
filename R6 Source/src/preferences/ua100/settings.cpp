#include "MenuItem.h"

#include "midi.h"
#include "ua100.h"
#include "settings.h"
#include "window.h"

static int32 init_misc[] = {
  // PAN
  0x401001, 0x401002,
  // SEND
  0x401100, 0x401200, 0x401300, 0x401400,
  0x401102, 0x401202, 0x401302, 0x401402,
  // SUB and MAIN
  0x641104, 0x641204, 0x641304, 0x640404,
  0x641105, 0x641205, 0x641305, 0x640405,
  // MUTE and SOLO
  0x1106, 0x1206, 0x1306, 0x1406,
  0x1107, 0x1207, 0x1307, 0x1407,
  // MASTER and REC
  0x905000, 0x905001, 0x645002, 0x640553,
  // EFX SW
  0x4001, 0x4002, 0x4003, 0x4004, 0x4005, 0x4006,
  // EFX TYPE
  0x0100, 0x0200, 0x0300, 0x0400, 0x0500, 0x0600,
  // RETURN
  0x644007, 0x644008, 0x644009, 0x64400a,
  // COPYRIGHT
  0x0001,
  0
};

static void
set(int fd, int32 address, int32 v)
{
  if (address == 0x404000)
	DT1(fd, 0x404000, v);
  my_app->ReceiveControlValue(fd, address, v);
  if ((address & 0xfffff8ff) == 0x400000) {
	MixWindow* win = my_app->FindWindow(fd);
	if (win && win->Lock()) {
	  // EFX TYPE
	  UTopView* view = win->top;
	  BMenuField* field = view->FXMenu((address >> 8) & 7);
	  BMenu* menu = field->Menu();
	  if (v == 0) {
		BMenuItem* item = menu->FindMarked();
		if (item) {
		  item->SetMarked(false);
		  field->Invalidate();
		}
	  }
	  else for (int i = 0;; i++) {
		BMenuItem* item = menu->ItemAt(i);
		if (!item)
		  break;
		BMessage* msg = item->Message();
		int32 value;
		if (msg->FindInt32("value", &value) == B_OK) {
		  if (value == v) {
			  item->SetMarked(true);
			  field->Invalidate();
			  break;
		  }
		}
	  }
	  win->Unlock();
	}
  }
}

static void
init_mode(int fd, int32 input_mode)
{
  set(fd, 0x401000, input_mode);
  for (int32* p = init_misc; *p; p++)
	set(fd, 0x400000 + (*p & 0xffff), *p >> 16);
}

void
listen(int fd)
{
  // line mode
  init_mode(fd, 1);
  // system effect 1 => HQ Reverb
  set(fd, 0x400300, 0x011);
  set(fd, 0x404005, 1);
  // FEM
  set(fd, 0x404000, 4);
}

void
use_mic1(int fd)
{
  // mic mode
  init_mode(fd, 0);
  // WAVE SELECT => Mic/Gtr
  set(fd, 0x405000, 0);
  // FEM
  set(fd, 0x404000, 4);
}

void
use_line(int fd)
{
  // line mode
  init_mode(fd, 1);
  // WAVE SELECT => Channel 1
  set(fd, 0x405000, 4);
  // FEM
  set(fd, 0x404000, 4);
}

void
eff_loop(int fd)
{
  // line mode
  init_mode(fd, 1);
  // WAVE SELECT => Channel 3
  set(fd, 0x405000, 6);
  // insertion effect 3 => Overdrive
  set(fd, 0x400300, 0x110);
  set(fd, 0x404003, 1);
  // CEM
  set(fd, 0x404000, 3);
}

void
karaoke(int fd)
{
  // mic+mic mode
  init_mode(fd, 2);
  // insertion effect 1 => Stereo EQ
  set(fd, 0x400100, 0x100);
  set(fd, 0x404001, 1);
  // system effect 1 => Chorus
  set(fd, 0x400500, 0x022);
  set(fd, 0x404005, 1);
  // system effect 2 => Reverb
  set(fd, 0x400600, 0x032);
  set(fd, 0x404006, 1);
  // CEM
  set(fd, 0x404000, 3);
}
