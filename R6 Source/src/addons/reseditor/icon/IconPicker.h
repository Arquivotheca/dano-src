#ifndef ICON_PICKER_H
#define ICON_PICKER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <Control.h>

#include <String.h>

#include "FatBitDefs.h"

enum {
	T_ICON_VIEW_MOVED		= 'Tivm'
	/*	Fields:
	**	BRect "be:region"
	**		Area of bitmap that is now being edited.
	*/
};

enum {
	msg_large_icon = 'lrge',
	msg_small_icon = 'smal'
};

enum DragSource {
	kDragSourceNone = 0,
	kDragSourceLarge = 1,
	kDragSourceMini = 2,
	kDragSourceBoth = kDragSourceLarge | kDragSourceMini
};

class TBitmapEditor;

class TIconView : public BView, public BInvoker {
public:
			TIconView(BRect frame, const char *name, const char *mimeType,
					  bool hilite=false);
			~TIconView();
			
			void StartShowing(TBitmapEditor* editor);
			void SetView(BRect area);
			
			void Draw(BRect updateRect);
			
			void AllAttached();
			void MessageReceived(BMessage *msg);
			void MouseDown(BPoint pt);
			
private:
			TBitmapEditor*	fShowing;
			bool			fHilite;
			BRect			fViewArea;
			BString			fIconMimeType;
};

class TIconPicker : public BControl {
public:
			TIconPicker(BRect frame);
			~TIconPicker();
		
			void StartShowing(TBitmapEditor* primary,
							  TBitmapEditor* secondary);
							  
			void AttachedToWindow();
			void GetPreferredSize(float *width, float *height);
			
			void DrawFancy();
			void Draw(BRect updateRect);
			void FrameResized(float width, float height);
			void DrawFocusMark(bool state);

			void KeyDown(const char *bytes, int32 n);
			void MouseDown(BPoint pt);
			
			virtual	status_t Invoke(BMessage *msg = NULL);
			
			void BuildDragData(BMessage *dragMsg, DragSource source = kDragSourceBoth);
			
			TIconView* 	LargeIcon();
			TIconView* 	SmallIcon();
			TIconView*	LargeHiliteIcon();
			TIconView*	SmallHiliteIcon();
			
private:
			void AddParts(float width, float height);
			
			TBitmapEditor*	fPrimaryBitmap;
			TBitmapEditor*	fSecondaryBitmap;
			
			TIconView* 	fLargeIcon;
			TIconView*	fLargeHiliteIcon;
			TIconView*	fSmallIcon;
			TIconView*	fSmallHiliteIcon;
};

#endif
