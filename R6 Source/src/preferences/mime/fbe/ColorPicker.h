#include <stdio.h>

#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <ColorControl.h>
#include <Control.h>
#include <StringView.h>

#include "ToolPicker.h"
#include "utils.h"

enum {
	msg_color_pick = 128,
	msg_fore_color,
	msg_back_color,
	msg_trans_color
};

class TColorSwatch : public BControl {
public:
			TColorSwatch(BRect r, rgb_color c, BMessage *msg );
			~TColorSwatch();
			
			void Draw(BRect updateRect);
			
			status_t	Invoke(BMessage *msg = NULL);

			void MessageReceived(BMessage *msg);
			void MouseDown(BPoint pt);
			
			rgb_color Color();
			void SetColor(rgb_color c);

			bool Selected();
			void SetSelected(bool state);
			
private:
			bool		fSelected;
			rgb_color	fColor;
};

class TColorControl : public BView {
public:
	TColorControl(BPoint start, int32 initialSelection);
	~TColorControl();
		
	void Draw(BRect);

	void MessageReceived(BMessage *msg);

	void MouseDown(BPoint);
	void KeyDown(const char *bytes, int32 n);
private:
	int32 fSelected;
};

class TColorPicker : public BBox, public BInvoker {
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
			
			void Colors(rgb_color *fore, rgb_color *back);
			void SetColors(rgb_color fore, rgb_color back);
			void SetForeColor(rgb_color c);
			void SetForeColor(uint8 i);
			void SetBackColor(rgb_color c);
			void SetBackColor(uint8 i);
			
			bool Selection();
			void ChangeSelection(TColorSwatch* selectedIcon);
private:
			bool			fSelected;
			
			BColorControl	*fColors;
//			TColorControl	*fColors;
			
			rgb_color		fForeColor;
			BStringView		*fForeColorLabel;
			TColorSwatch	*fForeColorBtn;
			
			rgb_color		fBackColor;
			BStringView		*fBackColorLabel;
			TColorSwatch	*fBackColorBtn;
			
			BButton*		fTransparentBtn;
};
