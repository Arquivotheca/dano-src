#include "channel.h"
#include "knob.h"

rgb_color
channel_color(int32 n)
{
  static const rgb_color colors[4] = {
	RED, YELLOW, GREEN, LT_BLUE
  };
  return colors[n-1];
}

UChannel::UChannel(BPoint where, char* name, int fd, int32 n)
  : UView(BRect(where, where + CHANNEL_SIZE), name, fd),
	n(n), color(channel_color(n))
{
}

void
UChannel::AttachedToWindow()
{
  int32 x = n << 8;

  SetViewColor(GRAY);
  SetLowColor(GRAY);

  /*
  if (n < 3)
	AddKnob(BPoint(2, PAN_Y), "pan", 0x401000 + n, color);
  */
  AddLED(BPoint(13, INS_Y), "insert", 0x404000 + n);
  AddKnob(BPoint(2, SEND1_Y), "send1", 0x401000 + x, color);
  AddKnob(BPoint(2, SEND2_Y), "send2", 0x401002 + x, color);

  BRect r(0, SUB_Y, CHANNEL_X, SUB_Y + SUB_H);
  AddSlider(r, "sub", 0x401004 + x, "");
  r.top = MUTE_Y;
  r.bottom = MUTE_Y + BOX_H;
  r.left = BOX_I;
  AddCheckBox(r, "", 0x401006 + x);
  r.top = SOLO_Y;
  r.bottom = SOLO_Y + BOX_H;
  AddCheckBox(r, "", 0x401007 + x);

  r.top = MAIN_Y;
  r.bottom = MAIN_Y + MAIN_H;
  r.left = 0;
  static char* labels[] = {"1", "2", "3", "4"};
  AddSlider(r, "main", 0x401005 + x, labels[n-1]);
}

void
UChannel::Draw(BRect)
{
#if USE_CHANNEL_SLIDER
  char digit[4];
  digit[0] = n + '0';
  digit[1] = 0;
  SetHighColor(BLACK);
  DrawString(digit, BPoint(10, MAIN_Y + MAIN_H));
#endif
}
