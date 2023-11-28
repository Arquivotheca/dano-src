#ifndef BITMAP_EDITOR_H
#define BITMAP_EDITOR_H

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

class BDataIO;
class BMessageFilter;
class TBitsContainer;

enum {
	T_BITMAP_INVALIDATED		= 'Tbiv',
	/*	Fields:
	**	BRect "be:region"
	**		Area of bitmap that needs to be redrawn.
	*/
	
	T_ATTRIBUTES_CHANGED		= 'Tatc',
	
	T_CURSOR_CHANGED			= 'Tcur'
};

class TBitmapEditor : public BHandler
{
public:
	TBitmapEditor(float init_width, float init_height, color_space init_cspace,
				  const char* name = "bitmap editor");
	~TBitmapEditor();
	
	// This is called whenever a change is made to the real bitmap.
	// Subclass and override to track edits of the bitmap.
	virtual void BitmapChanged(const char* what);
	
	// This is called whenever the currently selected color changes.
	virtual void NewColorSelection(rgb_color color, bool primary);
	
	// This is called whenever the cursor hot spot changes.
	virtual void NewHotSpot(int32 x, int32 y);
	
	// Hooks that viewers should call.
	
	status_t DoMessageReceived(BMessage* msg, BHandler* caller);
	status_t DoKeyDown(const char *bytes, int32 numBytes, const BMessage* msg);
	status_t DoMouseDown(BPoint point, const BMessage* msg);
	status_t DoMouseMoved(BPoint where, uint32 code,
						  const BMessage *drop, const BMessage* msg);
	status_t DoMouseUp(BPoint where, const BMessage* msg);
	
	bool UpdateMouseState(const BMessage* msg);
	
	// Fat bit conversions.
	
	BPoint FatBits2Actual(float pixelsPerPixel, BPoint where) const;
	BRect FatBits2Actual(float pixelsPerPixel, BRect where) const;
	BPoint Actual2FatBits(float pixelsPerPixel, BPoint where) const;
	BRect Actual2FatBits(float pixelsPerPixel, BRect where) const;
	
	// Drawing in viewers.
	
	float Width() const;
	float Height() const;
	color_space ColorSpace() const;
	BRect Bounds() const;
	
	const BBitmap* RealBitmap() const;
	const BBitmap* ShownBitmap() const;
	
	void GetFatBits(BView* into,
					float pixelsPerPixel,
					BPoint base, BRect region /* in fat bits space */,
					rgb_color gridColor) const;
	void GetHilite(BView* into, BPoint base, BRect region) const;
	
	const BCursor* CurrentCursor() const;
	
	// Current editing state.
	
	void SetAttributes(float width=-1, float height=-1,
					   color_space cspace=B_NO_COLOR_SPACE);
	
	void SetHotSpot(int32 x, int32 y);
	int32 HotSpotX() const;
	int32 HotSpotY() const;
	
	void SetSelectionMode(paste_selection_mode mode);
	paste_selection_mode SelectionMode() const;
	
	void SetDitherColorConversions(bool state);
	bool DitherColorConversions() const;
	
	void SetBackgroundColor(rgb_color newBGColor);
	rgb_color BackgroundColor() const;
	
	rgb_color PenColor();
	void SetPenColor();
	void RevertPenColor();
	void NewPenColor(rgb_color i);
	
	rgb_color SecondaryColor();
	void SetSecondaryColor();
	void RevertSecondaryColor();
	void NewSecondaryColor(rgb_color i);
	
	pattern PenPattern();
	void SetPenPattern(pattern p);
	void RevertPenPattern();
	
	void Radius(float *x, float *y);
	void SetRadius(float x, float y);
	
	void Angles(float *start, float *arc);
	void SetAngles(float start, float arc);
	
	int32 CurrentTool(bool swap = false);
	void SetCurrentTool(int32 tool);
	
	// Editing
	
	// Editing: Call BeginEdit() to start editing the bitmap.  Call
	// EndEdit() when finished editing, which will lock in and report
	// the changes you have made to the workspace.
	void BeginEdit(const char* name);
	bool Editing() const;
	void EndEdit(bool cancel=false);
	
	// Start a selection operation.  These are implicitly wrapped by
	// BeginEdit() and EndEdit().  The first uses a selection from the
	// given rectangle in the workspace bitmap, the second uses a
	// selection from the given bitmap.
	bool BeginCopySelection(BRect selectionRect, bool clear=true);
	bool BeginPasteSelection(const BBitmap* bitmap, BRect selection,
							 BPoint position);
	
	// Stop selecting.  This implicitly calls EndEdit().  Any current
	// selection bitmap is -not- pasted into the workspace.
	void EndSelection(bool cancel=false);
	
	// Stop selecting like above, but first paste the current selection
	// bitmap into the workspace.
	void DeSelect();
	
	// Completely change the icon bitmap.
	void SetBitmap(const BBitmap* icon, BRect srcRect, bool report=true);
	
	// Cut and paste of current selection.
	
	void AddCopyToClipboard();
	BBitmap* GetCopyFromMessage(const BMessage* message, BRect* out_frame);
	
	void DoCut();
	void DoCopy();
	bool CanCopy();
	void OverlaySelection(BRect selectionRect,
						  paste_selection_mode mode);
	void OverlaySelection(BView* dest, BBitmap* destBM, BRect selectionRect,
						  paste_selection_mode mode);
	status_t DoPaste();
	status_t PasteFromMessage(const BMessage* message);
	void EraseUnderSelection(BView* dest);
	void EraseUnderSelection(BView* dest, rgb_color color);
	void DoClear();
	bool CanClear();
	void DoSelectAll();
	
	virtual void MessageReceived(BMessage* message);
	
	// Dumping of data.
	
	void DumpSelection(BDataIO& io, const char *name) const;
	void DumpEntireMap(BDataIO& io, const char *name) const;
	void DumpMap(const BBitmap *, BDataIO& io, const char *dumpName) const;
	void DumpCursor(BDataIO& io, const char *name) const;
	
	
	// Internal interaction
	
	TBitsContainer* WorkspaceBits();
	TBitsContainer* ActualBits();

private:
	rgb_color fBackgroundColor;
	short fCurrentTool;
	
	int32 fHotSpotX, fHotSpotY;
	
	paste_selection_mode fSelectionMode;
	
	bool fDitherColorConversions;
	
	pattern fMarqueePattern[8];
	
	rgb_color fPenColor;
	rgb_color fLastPenColor;
	rgb_color fSecondaryColor;
	rgb_color fLastSecondaryColor;
	
	pattern fCurrentPattern;
	pattern fLastPattern;
	float fXRadius;
	float fYRadius;
	float fStartAngle;
	float fArcAngle;
	
	TBitsContainer *fWorkspaceBits;		// bits currently being edited
	TBitsContainer *fActualBits;		// composite of workspace, selection, marquee
	
	// Mouse state
	BPoint CurrentMouseLoc();
	rgb_color CurrentColorAtLoc();
	rgb_color fColorAtLoc;
	BPoint fCurrentLoc;
	BPoint fMousePosition;
	int32 fButtons;
	bool fSwapModifier;
	bool fConstrainModifier;
	
	// Cursor state
	void UpdateCursor(BPoint where);
	const BCursor* fCursor;
	
	// Selection state
	bool SetupSelection(BRect selectionRect);
	bool HaveSelection();
	void MoveSelection( int32 xOffset, int32 yOffset);				
	void MoveSelection( int32 cX, int32 cY, int32 lX, int32 lY);
	void MakeSelection( BRect r);
	void Pulse(void);
	enum selection_state {
		kNoSelection = 0,
		kSelecting,
		kHaveSelection,
		kMovingSelection
	};
	selection_state fSelectionState;
	BRect fSelectionStart;
	BRect fSelectionRect;
	BRect fLastSelectionRect;
	BBitmap* fRawSelectionBitmap;	// original bits being pasted into workspace
	BBitmap* fSelectionBitmap;		// converted bits being pasted into workspace
	uint8* fSelectionAlpha;			// which bits to paste?
	BMessageRunner* fSelectionRunner;

	// generation of composited bitmap
	void InvalidateComposite(BRect region, bool report=true);
	void InvalidateComposite();
	void ReportInvalidComposite();
	bool InitialCompositing(BView* target);
	void FinalCompositing(BView* target, BBitmap* targetBM, bool show_marquee);
	void CompositeBits(bool show_marquee);
	BRect fInvalidComposite;
	
	// tool tracking control and state
	void StartToolTracking(BPoint point);
	void TrackTool(BPoint point);
	void EndToolTracking();
	void DrawTool(int32 tool);
	bool fTracking;
	int32 fTrackTool;
	uint32 fTrackButtons;
	TBitsContainer* fTrackTransientBits;
	BRect fTrackTransientUpdate;
	TBitsContainer* fTrackFinalBits;
	BRect fTrackFinalUpdate;
	int32 fLastX, fLastY;
	int32 fStartX, fStartY;
	int32 fCurX, fCurY;
	pattern fPattern;
	float fTrackStartAngle, fTrackArcAngle;
	float fTrackXRadius, fTrackYRadius;
	bool fTrackFirstPass;
	bool fTrackMoved;
	rgb_color fColorAtClick;
	uint8 fPixelFlip;
	
	// tools
	void UsePencilTool(short pixelFlip, BPoint loc, BPoint last, rgb_color firstColor);
	void UseEyeDropperTool(int32 x, int32 y);
	void UseHotSpotTool(int32 x, int32 y);
	void UseEraser(int32 x,int32 y, int32 last_x, int32 last_y);
	void UseLineTool(int32 x,int32 y, int32 last_x, int32 last_y,
		pattern p = B_SOLID_HIGH);
	void UseSelectionTool(BRect r, int32 lX, int32 lY, bool addToSelection,
		bool firstPass);
	void UseSelectionTool(int32 sX, int32 sY, int32 eX, int32 eY,
		int32 lX, int32 lY,
		bool addToSelection, bool firstPass);
	void UseRectTool(BRect r, pattern p = B_SOLID_HIGH, bool fill = false);
	void UseRectTool(int32 sX, int32 sY, int32 eX, int32 eY,
		pattern p = B_SOLID_HIGH, bool fill = false);
	void UseRoundRectTool(BRect r, float xRadius, float yRadius,
		pattern p = B_SOLID_HIGH, bool fill = false);
	void UseRoundRectTool(int32 sX, int32 sY, int32 eX, int32 eY,
		float xRadius, float yRadius, pattern p = B_SOLID_HIGH,
		bool fill = false);
	void UseOvalTool(BRect r, pattern p, bool fill = false);
	void UseOvalTool(int32 sX, int32 sY, int32 eX, int32 eY,
		pattern p = B_SOLID_HIGH, bool fill = false);
	void UseArcTool(BRect r, float startAngle, float arcAngle,
		pattern p = B_SOLID_HIGH, bool fill = false);
	void UseArcTool(int32 sX, int32 sY, int32 eX, int32 eY,
		float startAngle, float arcAngle, pattern p = B_SOLID_HIGH,
		bool fill = false);
	void UseTriangleTool(int32 sX, int32 sY, int32 eX, int32 eY,
		pattern p, bool fill);
	void UseFillTool(int32 x, int32 y);
	
	bool fEditing;						// currently changing the bitmap?
	BString fEditName;					// name of current edit operation.
};

extern const char *kDumpFileName;

#endif
