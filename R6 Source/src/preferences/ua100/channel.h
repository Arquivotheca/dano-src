#ifndef UCHANNEL_H
#define UCHANNEL_H

#include "window.h"

#define CHANNEL_I (32 + 23 + 28 + 10)
#define CHANNEL_X 34
#define CHANNEL_Y (450 - 23 - 28 - 10)
#define CHANNEL_SIZE BPoint(CHANNEL_X, CHANNEL_Y)

#define BOX_I 9
#define INS_Y 0
#define SEND1_Y (INS_Y + 18)
#define SEND2_Y (SEND1_Y + KNOB_Y + 8)
#if USE_CHANNEL_SLIDER
#define SUB_Y (SEND2_Y + KNOB_Y - 20)
#define SUB_H 120
#define MUTE_Y (SUB_Y + SUB_H - 2)
#define BOX_H 20
#define SOLO_Y (MUTE_Y + BOX_H)
#define MAIN_Y (SOLO_Y + BOX_H - 20)
#define MAIN_H 180
#else
#define SUB_Y (SEND2_Y + KNOB_Y)
#define SUB_H 112
#define MUTE_Y (SUB_Y + SUB_H)
#define BOX_H 20
#define SOLO_Y (MUTE_Y + BOX_H)
#define MAIN_Y (SOLO_Y + BOX_H)
#define MAIN_H 160
#endif

extern rgb_color channel_color(int32 n);

struct UChannel : UView
{
  UChannel(BPoint where, char* name, int fd, int32 n);
  void AttachedToWindow();
  void Draw(BRect);

  int32 n;
  rgb_color color;
};

#endif
