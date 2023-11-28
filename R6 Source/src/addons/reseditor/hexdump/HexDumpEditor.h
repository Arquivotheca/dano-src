#ifndef __HEX_DUMP_EDITOR__
#define __HEX_DUMP_EDITOR__

#include <ResourceEditor.h>

#include <MessageRunner.h>

class HexDumpFullEditor;

class HexDumpAddon : public BResourceAddon {
public:
	HexDumpAddon(const BResourceAddonArgs& args)
		:	BResourceAddon(args)
	{
	}

	virtual float QuickQuality(const BResourceItem *) const;
	virtual float PreciseQuality(const BResourceItem *) const;
	
	virtual BMiniItemEditor *MakeMiniEditor(const BResourceAddonArgs &,
		BResourceHandle, const BMessage *);
	virtual BFullItemEditor *MakeFullEditor(const BResourceAddonArgs &,
		BResourceHandle, const BMessage *);
	
	virtual status_t HandleDrop(const BMessage *);
};

class OffscreenBitmap {
	// simple offscreen for easy flicker free drawing
	// - all the drawing operations can be done identically to
	// the corresponding draws done directly in the target bitmap
public:
	OffscreenBitmap();
	OffscreenBitmap(BView *target, BRect frame);
		// frame is the portion of target that you wish to buffer
	~OffscreenBitmap();

	BView *BeginBlitting(BView *target, BRect destFrame,
		bool copyTargetViewState = true, bool erase = true);
		// pass true in <copyTargetViewState> if <target> view state
		// changed since last time we used it
	void DoneBlitting();
		// when you are done, call this to blit the result into your target
		// view

	BView *OffscreenView() const;
		// do your drawing into this

private:
	void SetToCommon(BView *, BRect);
	void SetUpState(BView *, BRect);
	
	BBitmap *bitmap;
	BView *target;
	BRect destinationFrame;
};

class HexDumpView : public BView {
public:
	HexDumpView(BRect, const char *, HexDumpFullEditor *);
	virtual ~HexDumpView();
	
	void GetPreferredSize(float &width, float &height);
	void DataChanged();

	void Select(uint32, uint32);
	void Insert(const char *, size_t, const char *undoName);
	void InsertHexDigit(uint32, const char *undoName);
	void Delete(const char *undoName);

	void MakeOffsetVisible(int32 offset);
	void MakeHexOffsetVisible(int32 offset);
	void MakeAsciiOffsetVisible(int32 offset);
	
	virtual void Draw(BRect);
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	
	virtual void MessageReceived(BMessage *message);
	
	virtual void MouseDown(BPoint);
	virtual void MouseMoved(BPoint where, uint32 code, const BMessage *message);
	virtual void MouseUp(BPoint);

	virtual void KeyDown(const char *bytes, int32 numBytes);
	
	virtual void Pulse();

	virtual	void WindowActivated(bool);
	virtual void MakeFocus(bool = true);
	
	virtual void FrameResized(float, float);
	
private:

	void TrackMouse(BPoint, const BMessage *, bool fromMouseDown = false);

	// drawing routines
	void DrawRange(int32 fromOffset, int32 toOffset, bool direct, bool eraseToEnd = false);
	void DrawLine(BView *, BPoint where, int32 offset, const void *data, size_t size);
	void DrawOffsetValue(BView *, BPoint where, int32 offset);
	void DrawDataCommon(BView *,
		float (HexDumpView::*drawFunc)(BView *, BPoint, const void *, size_t, int32, bool, bool),
		BPoint where, const void *data, size_t size, int32 offset, bool focused);
	float DrawHexRun(BView *, BPoint where, const void *data, size_t size,
		int32 absoluteOffset, bool selected, bool erase);
	float DrawAsciiRun(BView *, BPoint where, const void *data, size_t size,
		int32 absoluteOffset, bool selected, bool erase);
	void DrawRunCommon(BView *, BPoint where, const char *string, int32 length,
		float width, bool selected, bool erase);

	void DrawHexOutlineSelection(BView *);
	void DrawAsciiOutlineSelection(BView *);
	void DrawOutlineSelectionCommon(BView *, float left, float right,
		float notchLeft, float notchRight);

	void GetContentSize(float &width, float &height);

	void UpdateSizes();

	void TweakScrollBars(bool force=false);
	
	float LineHeight() const;
	int32 VisibleLines() const;
	
	bool InHexColumn(BPoint, bool exactHitOnly = false) const;
	bool InAsciiColumn(BPoint, bool exactHitOnly = false) const;
	
	int32 PointToOffset(BPoint, bool centerDivider) const;
	BPoint OffsetToHexPoint(int32) const;
	BPoint OffsetToAsciiPoint(int32) const;

	void DoMakeOffsetVisible(int32 offset, bool hexArea);
	
	void ShowCursor();
	void HideCursor();
	void InvertCursor();
	void DrawCursorAt(BPoint, bool);

	void StartPulse();
	void StopPulse();
	
	void BumpSelectionBy(int32 offset, bool startAnchor, bool endAnchor);
	void BumpSelectionTo(int32 offset, bool startAnchor, bool endAnchor);

	void GetViewColors(rgb_color* foreground, rgb_color* background,
					   bool selected=false) const;
	void SetViewColors(BView* into, bool selected=false) const;

	void Activate(bool on);

	// ------------
	
	// parent owns/holds the data
	HexDumpFullEditor *parent;
	
	// used to get a repeating heartbeat
	BMessageRunner* pulse;
	
	// cached metrics used for drawing
	font_height fontHeight;
	
	float fixedCharacterWidth;
	float offsetWidth;
	float hexRunWidth;
	float asciiRunWidth;
	
	// how many bytes do we draw in a single hex digit run
	uint32 hexDigitChunkSize;
	
	// selection state
	uint32 selectionStart;
	uint32 selectionEnd;
	// mouse tracking start point
	uint32 selectionAnchor;
	
	// current mouse position while tracking
	BPoint trackingPosition;
	
	// tracking mouse is on while we are tracking a selection
	bool trackingMouse;

	// hexColumnFocused vs. Ascii column focuset
	bool hexColumnFocused;

	// set when the scroll bars need to be adjusted
	bool needScrollBarTweak;
	
	// cursor blink state
	bigtime_t nextCursorTime;
	bool cursorOn;
		// need to keep track of this because cursor is drawn by inverting

	// state for hex digit entry
	bool expectingSecondHexDigit;
	
	OffscreenBitmap offscreen;

	typedef BView _inherited;
};

class HexDumpFullEditor : public BFullItemEditor, public BView {
public:
	HexDumpFullEditor(const BResourceAddonArgs &, BResourceHandle);

	virtual BView *View()
		{ return this; }

	virtual void DetachedFromWindow();
	virtual status_t Retarget(BResourceHandle);
	virtual void MessageReceived(BMessage *);
	virtual void MakeFocus(bool = true);
	
	// needed APIs:
	// max/min editor view size setter
	// copy/paste
	// GetPreferredSize - const

	const void *Data() const;
	size_t Size() const;

	void GetPreferredSize(float *width, float *height);

	void SetData(const char *data, size_t size, bool update = false);
	void Insert(uint32 offset, const char *data, size_t size, const char *undoName, bool update = false);
	void ModifyCharAt(uint32 offset, uchar ch, const char *undoName, bool update = false);
	void DeleteRange(uint32 from, uint32 to, const char *undoName, bool update = false);


protected:
	void PickUpDataChange(const BResourceCollection *collection,
		BResourceHandle& item, uint32 changes);

private:
	HexDumpView *hexDumpView;
	BString data;

};

#endif
