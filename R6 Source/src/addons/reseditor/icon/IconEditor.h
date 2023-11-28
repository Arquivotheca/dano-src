#ifndef ICON_EDITOR_H
#define ICON_EDITOR_H

#include <stdio.h>

#include <Messenger.h>
#include <FilePanel.h>

#include "FatBitDefs.h"
#include "FatBitsEditor.h"
#include "ColorPicker.h"
#include "ToolPicker.h"
#include "IconPicker.h"

enum {
	msg_selection_tool = 999,
	msg_eraser_tool,
	msg_pencil_tool,
	msg_eye_tool,
	msg_fill_tool,
	msg_line_tool,
	msg_rect_tool,
	msg_frect_tool,
	msg_rrect_tool,
	msg_frrect_tool,
	msg_oval_tool,
	msg_foval_tool,
	msg_hotspot_tool,
	msg_next_tool,
	msg_prev_tool,
	msg_show_tbits,
	msg_set_bg_color,
	msg_new_bg_color,
	msg_save_colorwind_loc
};

const uint32 kDumpSelection = 'dpsl';
const uint32 kDumpIcons = 'dpic';
const uint32 kDumpCursor = 'dcur';
const uint32 kSetSelectionMode = 'ssmd';
const uint32 kToggleDithering = 'tdth';

// forward reference internal implementation.
class TCustomBitmapEditor;
class BScrollView;
class BMenuField;

class TIconEditor : public BView {
public:
	TIconEditor(BRect r, const BMessage* configuration);
	~TIconEditor();
	
	virtual void			BitmapChanged(TBitmapEditor* editor,
										  const char* what, bool mini);
	virtual void			HotSpotChanged(TBitmapEditor* editor,
										   int32 x, int32 y, bool mini);
	
	void					SetControls(BView* controls);
	BView*					Controls() const		{ return fControls; }
	
	void 					AttachedToWindow();
	void 					AllAttached();
	void					DetachedFromWindow();
	void					GetPreferredSize(float *width, float *height);
	void					LayoutViews(float width, float height);
	void					FrameResized(float width, float height);
	void					MakeFocus(bool = true);
	
	void 					AddParts();
	
	status_t				GetConfiguration(BMessage* into) const;
	status_t				SetConfiguration(const BMessage* from);
	
	// note that the semantic of these is the opposite of the above --
	// GetPrefs() changes this objects settings from the file, and
	// SetPrefs() writes the current settings into the file.
	void					GetPrefs();
	void					SetPrefs();
	
	void 					Draw(BRect updateRect);
	
	void 					KeyDown(const char *key, int32 numBytes);
	
	void 					NewSecondaryImage(const BBitmap*, BRect, bool report=true);
	void 					NewPrimaryImage(const BBitmap*, BRect, bool report=true);
	
	void					SetBitmap(const BBitmap* primary,
									  const BBitmap* secondary=0);
	void					SetPrimaryAttributes(float width=-1, float height=-1,
												 color_space cspace=B_NO_COLOR_SPACE);
	void					SetSecondaryAttributes(float width=-1, float height=-1,
												   color_space cspace=B_NO_COLOR_SPACE);
												   
	void					SetPrimaryHotSpot(int32 x, int32 y);
	int32					PrimaryHotSpotX() const;
	int32					PrimaryHotSpotY() const;
	
	void					SetSecondaryHotSpot(int32 x, int32 y);
	int32					SecondaryHotSpotX() const;
	int32					SecondaryHotSpotY() const;
	
	status_t 				FindUpdateIcon(BMessage* msg, bool which);
	void 					HandleIconDrop(BMessage *);
		
	void 					MessageReceived(BMessage *);
	void 					MouseDown(BPoint where);
	void 					MouseMoved(BPoint where, uint32 code, const BMessage *);

	void 					DumpIcons() const;
	void 					DumpCursor() const;
	
	void 					Pulse();
	
	void					SetSelectionMode(paste_selection_mode mode);
	paste_selection_mode	SelectionMode() const;
	
	void					SetDitherColorConversions(bool state);
	bool					DitherColorConversions() const;
	
	void 					DropperColorSelection(rgb_color, bool);
	
	const BBitmap* 			PrimaryImage();
	const BBitmap* 			SecondaryImage();
	
	bool 					Dirty() const;
	void 					SetDirty(bool);
	
	const TBitmapEditor* 	PrimaryBitmapEditor() const;
	TBitmapEditor* 			PrimaryBitmapEditor();
	const TBitmapEditor*	SecondaryBitmapEditor() const;
	TBitmapEditor*		 	SecondaryBitmapEditor();
	
	void 					ChooseColorWindow();

private:
	typedef BView inherited;
	friend class TCustomBitmapEditor;
	
	void					ReportBitsChange(TCustomBitmapEditor* editor,
											 const char* what);
	void					ReportHotSpotChange(TCustomBitmapEditor* editor,
												int32 x, int32 y);
	void					DeleteIcon(TCustomBitmapEditor** editor,
									   TFatBitsEditor** viewer,
									   BScrollView** scroller,
									   BMenuField** zoomer);
	
	int32 					fCurrentTool;
	paste_selection_mode	fSelectionMode;
	rgb_color 				fForeColor;
	rgb_color				fBackColor;
	rgb_color				fGridColor;
	rgb_color				fBackgroundColor;
	BPoint					fColorWindowOffsets;
	
	bool					fDitherColorConversions;
	
	BMessenger				fColorWindow;
	
	// Preferred size of this control, computed when parts are added.
	float					fPrefWidth;
	float					fPrefHeight;
	
	enum layout_style {
		UNKNOWN_LAYOUT,
		PRIMARY_LAYOUT,
		SECONDARY_LAYOUT,
		BOTH_LAYOUT
	};
	layout_style			fLayout;
	float					fPriWidth, fPriHeight;
	color_space				fPriColorSpace;
	float					fSecWidth, fSecHeight;
	color_space				fSecColorSpace;
	
	BView*					fControls;
	
	TIconPicker*			fIconPicker;
	TCustomBitmapEditor*	fPriEditor;
	TFatBitsEditor*			fPriViewer;
	BScrollView*			fPriScroller;
	BMenuField*				fPriZoomer;
	TCustomBitmapEditor*	fSecEditor;
	TFatBitsEditor*			fSecViewer;
	BScrollView*			fSecScroller;
	BMenuField*				fSecZoomer;
	
	TColorPicker*			fColorPicker;
	TToolPicker*			fToolPicker;
#if 0	
	BFilePanel *fSavePanel;
#endif
};

class TColorControl;

class TColorWindow : public BWindow {
public:
						TColorWindow(BPoint, BView*, rgb_color);
						~TColorWindow();
				
		void			MessageReceived(BMessage*);
		bool			QuitRequested();
private:
		void			ReportPosition();
		rgb_color		fOriginalColor;
		rgb_color		fColor;
		TColorControl* 	fCC;
		BMessenger 		fTarget;
};

#endif
