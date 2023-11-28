#ifndef COLOR_PICKER_H
#define COLOR_PICKER_H

#include <stdio.h>

#include <Application.h>
#include <View.h>
#include <ColorControl.h>
#include <Control.h>
#include <StringView.h>

#include <ToolTipHook.h>

#include "ToolPicker.h"

enum {
	msg_fore_color = 128,
	msg_back_color,
	
	// private
	msg_mod_color,
	msg_new_color
};

class BSlider;
class TColorControl;

class TColorSwatch : public BControl, public BToolTipable {
public:
			TColorSwatch(BRect r, rgb_color c, BMessage *msg,
						 const char* tip = "" );
			~TColorSwatch();
			
			void Draw(BRect updateRect);
			
			status_t	Invoke(BMessage *msg = NULL);

			void MessageReceived(BMessage *msg);
			void MouseDown(BPoint pt);
			
			void KeyDown(const char *bytes, int32 n);
			
			rgb_color Color();
			void SetColor(rgb_color c);

			bool Selected();
			void SetSelected(bool state);
			
private:
			bool		fSelected;
			rgb_color	fColor;
};

class TColorPicker : public BView, public BInvoker {
public:
			TColorPicker(BRect r, rgb_color foreColor,
				rgb_color backColor);
			~TColorPicker();
			
			void AttachedToWindow();
			void Draw(BRect updateRect);
			void MessageReceived(BMessage *msg);
			void MouseMoved(BPoint where, uint32 code,
				const BMessage *a_message);
			
			void GetPreferredSize(float *w, float *h);
			float PrefixWidth() const;
			
			void Colors(rgb_color *fore, rgb_color *back);
			void SetColors(rgb_color fore, rgb_color back);
			void SetForeColor(rgb_color c, bool report=false);
			void SetBackColor(rgb_color c, bool report=false);
			
			// For use by TColorSwatch.
			void SetColorFor(TColorSwatch* who, rgb_color c, bool report=false);
			
			bool Selection();
			void ChangeSelection(TColorSwatch* selectedIcon);
			
private:
			bool			fSelected;
			
			TColorControl	*fColors;
//			TColorControl	*fColors;
			
			rgb_color		fForeColor;
			BStringView		*fForeColorLabel;
			TColorSwatch	*fForeColorBtn;
			
			rgb_color		fBackColor;
			BStringView		*fBackColorLabel;
			TColorSwatch	*fBackColorBtn;
			
			BSlider*		fAlphaSlider;
};

#endif
