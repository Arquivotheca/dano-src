//--------------------------------------------------------------------
//	
//	Slider.h
//
//	Written by: Robert Polic
//  Revised: 26-Nov-96 marc
//	
//	Copyright 1995, 1996 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef SLIDER_H
#define SLIDER_H

#include <Bitmap.h>
#include <CheckBox.h>
#include <View.h>

#define BACK_WIDTH (29 - 1)
#define BACK_HEIGHT (127 - 1)
#define THUMB_WIDTH (18 - 1)
#define THUMB_HEIGHT (24 - 1)

#define BITS_WIDTH(W) (((W + 7) & 0xfff8) - 1)

class SliderView : public BView {
public:
  SliderView (BPoint where, char* name, float value, BLooper* target, long ID);
  ~SliderView();

  void AttachedToWindow ();
  void Draw (BRect);
  void SetValue (float);
  float Value ();
  void MouseDown (BPoint);
  void KeyDown (const char* bytes, int32 numBytes);
  void MakeFocus (bool);

  long fID;

  static const float fMin;
  static const float fMax;

  static const int fBackWidth;
  static const int fBackHeight;
  static const int fThumbWidth;
  static const int fThumbHeight;
  static const BRect fBackRect;
  static const BRect fThumbRect;

  float ThumbTop ();

private:
  float fValue;
  BLooper* fTarget;

  float fXOffset;
  BBitmap* fSliderBack;
  BBitmap* fThumb;
  BView* fOffView;
  BBitmap* fOffScreen;

  typedef BView inherited;
};

#endif
