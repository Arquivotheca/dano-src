#ifndef TOOL_PICKER_H
#define TOOL_PICKER_H

#include <stdio.h>

#include <Application.h>
#include <Bitmap.h>
#include <Box.h>
#include <Control.h>
#include <Window.h>

#include <ToolTipHook.h>

enum {
	msg_new_tool = 'tool'
};

enum ToolTypes {
	kSelectionTool = 0,
	kEraserTool,
	kPencilTool,
	kEyeDropperTool,
	kBucketTool,
	kLineTool,
	kRectTool,
	kFilledRectTool,
	kRoundRectTool,
	kFilledRoundRectTool,
	kOvalTool,
	kFilledOvalTool,
	kArcTool,
	kFilledArcTool,
	kTriangleTool,
	kFilledTriangleTool,
	kLassoTool,
	kHotSpotTool
};

// Selection tool modes
enum paste_selection_mode {
	kPasteCopy,
	kPasteAlpha,
	kPasteBackground
};

const int32 kToolCount = 12;
const int32 kToolWidth = 32;
const int32 kToolHeight = 32;

class TToolButton : public BView, public BToolTipable {
public:
			TToolButton(BRect frame, const BBitmap *icon,
						int32 index, const char* tip);
			~TToolButton();

			void Draw(BRect updateRect);
					
			void MessageReceived(BMessage *msg);
			void MouseDown(BPoint pt);			

			bool Selected();
			void SetSelected(bool state);
			
private:
			int32 	fIndex;
			bool	fSelected;
			const BBitmap *fIcon;
};

class TToolPicker : public BControl {
public:
						TToolPicker(BPoint pt, uint32 what, orientation o,
							int32 toolsPerLine, int32 tool=kPencilTool);
						~TToolPicker();
		
		void 			AttachedToWindow();
		void			GetPreferredSize(float *width, float *height);
		
		void 			Draw(BRect);
		void 			KeyDown(const char *bytes, int32 n);
		
		void 			AddTool(int32 which, uint32 what);
		BRect 			GetToolFrame(int32 which);
		
		void 			ChangeSelection(int32 index);
		void 			NextTool();
		void 			PrevTool();
		
		int32 			CurrentTool() const;
private:
		int32 			fToolsPerLine;
		orientation		fOrientation;
		int32			fCurrentTool;
		TToolButton*	fToolBtns[kToolCount];
};

class TToolPickerPalette : public BWindow {
public:
			TToolPickerPalette(BPoint,int32);
			~TToolPickerPalette();
			
			int32 CurrentTool() const;
			void SetTool(int32);
private:
		TToolPicker *fToolPicker;
};

#endif
