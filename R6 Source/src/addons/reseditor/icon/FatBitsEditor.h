#ifndef FAT_BIT_EDITOR_H
#define FAT_BIT_EDITOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Bitmap.h>
#include <Screen.h>
#include <ScrollView.h>
#include <View.h>
#include <String.h>

#include "FatBitDefs.h"
#include "ToolPicker.h"

class BMessageFilter;
class BScrollView;
class TBitmapEditor;
class TBitsContainer;

class TFatBitsEditor : public BView
{
public:
	TFatBitsEditor(TBitmapEditor& editor, rgb_color gridColor,
				   float pixelsPerPixel = 8);
	~TFatBitsEditor();
	
	void AllAttached();
	void DetachedFromWindow();
	void GetPreferredSize(float *width, float *height);
	
	virtual void WindowActivated(bool active);

	void Draw(BRect updateRect);
	
	void MessageReceived(BMessage* msg);
	
	void KeyDown(const char *bytes, int32 numBytes);
	void MouseDown( BPoint point);
	void MouseMoved(BPoint where, uint32 code, const BMessage *a_message);
	void MouseUp(BPoint where);
	
	void FrameResized(float width, float height);
	
	void MakeFocus(bool state = true);
	void TargetedByScrollView(BScrollView *sv);
	
	void FixupScrollBar();
	
	void SetGridColor(rgb_color c);
	void SetGridColor(uint8 r, uint8 g, uint8 b,uint8 a=0);
	
	void SetZoom(float factor);
	float Zoom() const;
	
	void UpdateMouseState(const BMessage* from);
	
private:
	static float PreferredWidth(TBitmapEditor& editor, float pixelsPerPixel);
	static float PreferredHeight(TBitmapEditor& editor, float pixelsPerPixel);
	
	TBitmapEditor& fEditor;
	float fPixelsPerPixel;
	rgb_color fGridColor;
	BScrollView* fScrollView;
	TBitsContainer* fOffscreen;
	
	// Interactive state while connected to window.
	void AttachObjects();
	void DetachObjects();
	BMessageFilter* fModifierFilter;
};

#endif
