#ifndef	UKNOB_H
#define	UKNOB_H

#include <Control.h>
#include "ua100.h"

#define KNOB_MIN 0
#define KNOB_MAX 127
#define DOT_DROP 4
#define KNOB_X 33
#define KNOB_Y (DOT_DROP + 24)
#define KNOB_CENTER_X 15
#define KNOB_CENTER_Y (DOT_DROP + 11)
#define KNOB_CENTER BPoint(KNOB_CENTER_X, KNOB_CENTER_Y)

struct UKnob : BControl, UControl
{
  UKnob(BPoint where, char* name, int fd, int32 address,
		BMessage* msg, rgb_color notchColor);
  ~UKnob();

  virtual void SetValue(int32 value);
  virtual void Draw(BRect updateRect);
  virtual void MouseDown(BPoint where);
  virtual void MouseMoved(BPoint where, uint32 code, const BMessage* msg);
  virtual void MouseUp(BPoint where);
  virtual void DrawDot(BPoint where);

  rgb_color	fNotchColor;
  BRect		fRect;
  BRect		fClipRect;
  BView*	fBufferView;
  struct BBitmap*	fBitmap;
  struct BBitmap*	fImageBuffer;
};

#endif
