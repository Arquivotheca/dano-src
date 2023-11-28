//--------------------------------------------------------------------
//	
//	Slider.h
//
//	Written by: Robert Polic
//  Revised: 27-Jun-96 marc
//	
//	Copyright 1995, 1996 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef SLIDER_H
#define SLIDER_H

#ifndef _BITMAP_H
#include <Bitmap.h>
#endif
#ifndef _CHECK_BOX_H
#include <CheckBox.h>
#endif
#ifndef _VIEW_H
#include <View.h>
#endif

#define BACK_WIDTH (29 - 1)
#define BACK_HEIGHT (127 - 1)
#define THUMB_WIDTH (18 - 1)
#define THUMB_HEIGHT (24 - 1)

#define BITS_WIDTH(W) (((W + 7) & 0xfff8) - 1)

enum Half {BOTH_HALVES, LEFT_HALF, RIGHT_HALF};

class TSliderView : public BView {
public:
  TSliderView (BPoint where, Half half, char* name, float value,
			   BHandler* target, int32 ID);
  ~TSliderView();

  void AttachedToWindow ();
  void Draw (BRect);
  void SetValue (float, bool sync = FALSE);
  float Value ();
  void MouseDown (BPoint);
  void KeyDown (const char* bytes, int32 numBytes);
  void MakeFocus (bool);
  void DrawLabel (char* label);
  void Split ();

  int32 fID;
  TSliderView* fPartner;

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
  BHandler* fTarget;

  Half fHalf;
  float fXOffset;
  BBitmap* fSliderBack;
  BBitmap* fThumb;
  BView* fOffView;
  BBitmap* fOffScreen;
};

class TMonoSlider {
public:
  TMonoSlider (BPoint where, char* label, float value, BView* view,
			   int32 ID, int32 muteID = -1, bool mute = FALSE);
  virtual ~TMonoSlider();

  virtual void SetValue (float);
  virtual void SetMute (bool);
  virtual void DrawLabel ();

  TSliderView* fSliderView;
  BCheckBox* fMuteBox;
  char* fLabel;
};

class TStereoSlider {
public:
  TStereoSlider (BPoint where, char* label, float r_value, float l_value,
				 BView* view, int32 r_ID, int32 l_ID,
				 int32 muteID = -1, bool mute = FALSE);
  virtual ~TStereoSlider();
  
  virtual void SetRValue (float);
  virtual void SetLValue (float);
  virtual void SetMute (bool);
  virtual void DrawLabel ();
  
  TSliderView* fRSliderView;
  TSliderView* fLSliderView;
  BCheckBox* fMuteBox;
  char* fLabel;
};

#endif
