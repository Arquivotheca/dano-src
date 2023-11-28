#include "BitmapEditor.h"
#include "BitsContainer.h"
#include "IconBits.h"

#include <ByteOrder.h>
#include <Clipboard.h>
#include <Cursor.h>
#include <Debug.h>
#include <Entry.h>
#include <Path.h>
#include <File.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <TranslationUtils.h>
#include <Alert.h>
#include <MessageRunner.h>
#include <MessageFilter.h>

#include <ResourceEditor.h>

#include <experimental/BitmapTools.h>
#include <experimental/ColorTools.h>

#include "utils.h"

static void copy_view_state(BView* to, BView* from)
{
	to->SetHighColor(from->HighColor());
	to->SetLowColor(from->LowColor());
	to->SetViewColor(from->ViewColor());
	to->SetPenSize(from->PenSize());
	#if 0	// seems to hang...
	to->SetLineMode(from->LineCapMode(),
					from->LineJoinMode(),
					from->LineMiterLimit());
	#endif
	to->SetDrawingMode(from->DrawingMode());
	source_alpha sa;
	alpha_function af;
	from->GetBlendingMode(&sa, &af);
	to->SetBlendingMode(sa, af);
	BFont font;
	from->GetFont(&font);
	to->SetFont(&font);
}

const char *kDumpFileName = "Bitmaps.h";

static pattern marquee_pattern[8] = {{{0xf8, 0xf1, 0xe3, 0xc7, 0x8f, 0x1f, 0x3e, 0x7c}}};

static const type_code kSelectionPulse = 'spul';

TBitmapEditor::TBitmapEditor(float init_width, float init_height,
							 color_space init_cspace,
							 const char* name)
	: BHandler(name),
	  fBackgroundColor(B_TRANSPARENT_COLOR), fCurrentTool(kPencilTool),
	  fHotSpotX(-1), fHotSpotY(-1),
	  fSelectionMode(kPasteCopy), fDitherColorConversions(true),
	  fPenColor(make_color(255,10,10)), fLastPenColor(make_color(255,10,10)),
	  fSecondaryColor(B_TRANSPARENT_COLOR), fLastSecondaryColor(B_TRANSPARENT_COLOR),
	  fCurrentPattern(B_SOLID_HIGH), fLastPattern(B_SOLID_HIGH),
	  fXRadius(8.0), fYRadius(8.0), fStartAngle(45.0), fArcAngle(90.0),
	  fWorkspaceBits(0), fActualBits(0),
	  fColorAtLoc(B_TRANSPARENT_COLOR), fCurrentLoc(0, 0), fMousePosition(0, 0),
	  fButtons(0), fSwapModifier(false), fConstrainModifier(false),
	  fCursor(0),
	  fSelectionState(kNoSelection),
	  fRawSelectionBitmap(0), fSelectionBitmap(0), fSelectionAlpha(NULL),
	  fSelectionRunner(0),
	  fTracking(false), fTrackTool(0), fTrackTransientBits(0), fTrackFinalBits(0),
	  fEditing(false)
{
	memcpy(fMarqueePattern, marquee_pattern, sizeof(fMarqueePattern));
 	SetAttributes(init_width, init_height, init_cspace);
	InvalidateComposite();
}
  
TBitmapEditor::~TBitmapEditor()
{
	DeSelect();
	
	delete fWorkspaceBits;
	delete fActualBits;
}

void
TBitmapEditor::BitmapChanged(const char* what)
{
	(void)what;
	// we don't care.
}

void
TBitmapEditor::NewColorSelection(rgb_color, bool)
{
	// override this virtual for the editor to do something with the color selected
}

void
TBitmapEditor::NewHotSpot(int32, int32)
{
	// override this virtual for the editor to do something with the
	// hot spot selected
}

// --------------------------- BView Hooks ---------------------------

status_t
TBitmapEditor::DoMessageReceived(BMessage *msg, BHandler* caller)
{
	if (msg->WasDropped()) {
		// distinguish the target with the icon view name
		msg->AddString("drop", Name());
		caller->BHandler::MessageReceived(msg);
		return B_OK;
	}
	
	switch(msg->what) {
		case B_CLEAR:
			DoClear();
			return B_OK;
		case B_CUT:
			DoCut();
			return B_OK;
		case B_COPY:
			DoCopy();
			return B_OK;
		case B_PASTE:
			return DoPaste();
		case B_SELECT_ALL:
			DoSelectAll();
			return B_OK;
		case B_DESELECT_ALL:
			DeSelect();
			return B_OK;
		case kSelectionPulse:
			Pulse();
			return B_OK;
	}
	
	return B_ERROR;
}

status_t
TBitmapEditor::DoKeyDown(const char *key, int32 numBytes, const BMessage* msg)
{
	UpdateMouseState(msg);
	
	if( numBytes != 1 ) return B_ERROR;

	switch (key[0]) {
		case B_BACKSPACE:
			DoClear();
			return B_OK;
		
		case B_LEFT_ARROW:
			MoveSelection(-1,0);
			ReportInvalidComposite();
			return B_OK;
		case B_RIGHT_ARROW:
			MoveSelection(1,0);
			ReportInvalidComposite();
			return B_OK;
		case B_UP_ARROW:
			MoveSelection(0,-1);
			ReportInvalidComposite();
			return B_OK;
		case B_DOWN_ARROW:
			MoveSelection(0,1);
			ReportInvalidComposite();
			return B_OK;
	}
	
	return B_ERROR;
}

status_t
TBitmapEditor::DoMouseDown(BPoint point, const BMessage* msg)
{
	fMousePosition = point;
	UpdateMouseState(msg);
	
	if (fSelectionState != kNoSelection
			&& ( CurrentTool(fSwapModifier) != kSelectionTool
				 || !fSelectionRect.Contains(point) ) ) {
		DeSelect();
	}

	switch(CurrentTool(fSwapModifier)) {
		case kSelectionTool:
		case kEyeDropperTool:
		case kHotSpotTool:
			// Start editing if creating a new selection; otherwise,
			// continue editing of last selection.
			StartToolTracking(point);
			return B_OK;
	}
	
	const char* edname = "Edit Bitmap";
	switch(CurrentTool(fSwapModifier)) {
		case kPencilTool:			edname = "Draw Pencil";				break;
		case kEraserTool:			edname = "Erase";					break;
		case kLineTool:				edname = "Draw Line";				break;
		case kSelectionTool:		edname = "Rectangle Select";		break;
		case kLassoTool:			edname = "Lasso Select";			break;
		case kRectTool:				edname = "Draw Rectangle";			break;
		case kFilledRectTool:		edname = "Fill Rectangle";			break;
		case kRoundRectTool:		edname = "Draw Round Rectangle";	break;
		case kFilledRoundRectTool:	edname = "Fill Round Rectangle";	break;
		case kOvalTool:				edname = "Draw Oval";				break;
		case kFilledOvalTool:		edname = "Fill Oval";				break;
		case kArcTool:				edname = "Draw Arc";				break;
		case kFilledArcTool:		edname = "Fill Arc";				break;
		case kTriangleTool:			edname = "Draw Triangle";			break;
		case kFilledTriangleTool:	edname = "Fill Triangle";			break;
		case kBucketTool:			edname = "Fill Area";				break;
	}
	
	BeginEdit(edname);
	StartToolTracking(point);
	
	return B_OK;
}

status_t
TBitmapEditor::DoMouseMoved(BPoint where, uint32 /*code*/,
							const BMessage */*drop*/, const BMessage* msg)
{
	fMousePosition = where;
	UpdateMouseState(msg);
	
	if( !fTracking ) return B_ERROR;
	
	TrackTool(where);
	return B_OK;
}

status_t
TBitmapEditor::DoMouseUp(BPoint where, const BMessage* msg)
{
	UpdateMouseState(msg);
	fMousePosition = where;
	
	if( !fTracking ) return B_ERROR;
	
	TrackTool(where);
	EndToolTracking();
	
	return B_OK;
}

bool
TBitmapEditor::UpdateMouseState(const BMessage* from)
{
	bool changed = false;
	
	int32 but = 0;
	if( from->FindInt32("buttons", &but) == B_OK ) {
		if( but != fButtons ) {
			fButtons = but;
			changed = true;
		}
	}
	
	int32 modifiers = 0;
	if( from->FindInt32("modifiers", &modifiers) == B_OK ) {
		bool swap = (modifiers&B_SHIFT_KEY) ? true : false;
		if( swap != fSwapModifier ) {
			fSwapModifier = swap;
			changed = true;
		}
		bool constrain = (modifiers&B_COMMAND_KEY) ? true : false;
		if( constrain != fConstrainModifier ) {
			fConstrainModifier = constrain;
			changed = true;
		}
	}
	
	if( changed ) UpdateCursor(fMousePosition);

	return changed;
}

// ----------------------- Fat Bit Conversions -----------------------

BPoint
TBitmapEditor::FatBits2Actual(float pixelsPerPixel, BPoint where) const
{
	where.x = floor(where.x/pixelsPerPixel);
	where.y = floor(where.y/pixelsPerPixel);
	return where;
}

BRect
TBitmapEditor::FatBits2Actual(float pixelsPerPixel, BRect where) const
{
	where.left = floor(where.left/pixelsPerPixel);
	where.right = floor(where.right/pixelsPerPixel);
	where.top = floor(where.top/pixelsPerPixel);
	where.bottom = floor(where.bottom/pixelsPerPixel);
	return where;
}

BPoint
TBitmapEditor::Actual2FatBits(float pixelsPerPixel, BPoint where) const
{
	where.x = floor(where.x*pixelsPerPixel);
	where.y = floor(where.y*pixelsPerPixel);
	return where;
}

BRect
TBitmapEditor::Actual2FatBits(float pixelsPerPixel, BRect where) const
{
	where.left = floor(where.left*pixelsPerPixel);
	where.right = floor(where.right*pixelsPerPixel + (pixelsPerPixel-1));
	where.top = floor(where.top*pixelsPerPixel);
	where.bottom = floor(where.bottom*pixelsPerPixel + (pixelsPerPixel-1));
	return where;
}

// ----------------------- Drawing In Viewers -----------------------

float
TBitmapEditor::Width() const
{
	return fWorkspaceBits->Width();
}

float
TBitmapEditor::Height() const
{
	return fWorkspaceBits->Height();
}

color_space
TBitmapEditor::ColorSpace() const
{
	return fWorkspaceBits->ColorSpace();
}

BRect
TBitmapEditor::Bounds() const
{
	return BRect(0, 0, Width()-1, Height()-1);
}

const BBitmap*
TBitmapEditor::RealBitmap() const
{
	return fWorkspaceBits->Bitmap();
}

const BBitmap*
TBitmapEditor::ShownBitmap() const
{
	if( !fInvalidComposite.IsValid() ) {
		return fActualBits->Bitmap();
	}
	
	TBitmapEditor* This = const_cast<TBitmapEditor*>(this);
	This->CompositeBits(true);
	return This->fActualBits->Bitmap();
}

void
TBitmapEditor::GetFatBits(BView* into,
						  float pixelsPerPixel,
						  BPoint base, BRect region,
						  rgb_color gridColor) const
{
	into->PushState();
	
	float gridOff = gridColor.alpha != 0 ? 1 : 0;
	
	BRect in_region = region & BRect(0, 0,
									 Width()*pixelsPerPixel - gridOff,
									 Height()*pixelsPerPixel - gridOff);
	region.OffsetTo(base);
	BRect real_region = FatBits2Actual(pixelsPerPixel, in_region);
	BRect large_region = Actual2FatBits(pixelsPerPixel, real_region);
	base.x -= in_region.left - large_region.left;
	base.y -= in_region.top - large_region.top;
	in_region = large_region;
	in_region.OffsetTo(base);
	
	int32 first_x = int32(real_region.left);
	int32 last_x = int32(real_region.right) + 1;
	int32 first_y = int32(real_region.top);
	int32 last_y = int32(real_region.bottom) + 1;
	
	if( real_region.IsValid() ) {
		const BBitmap* shown = ShownBitmap();
		into->DrawBitmapAsync(shown, real_region, in_region);
	}
	if( gridColor.alpha != 0 && last_x >= first_x && last_y >= first_y ) {
		
		if( gridColor.alpha != 255 ) {
			into->SetDrawingMode(B_OP_ALPHA);
			into->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		}
		
		PRINT(("Drawing grid (%ld,%ld)-(%ld,%ld) from (%.2f,%l2f)-(%.2f-%.2f)\n",
				first_x, first_y, last_x, last_y,
				region.left, region.top, region.right, region.bottom));
				
		into->BeginLineArray(last_x-first_x + last_y-first_y + 2);
		int32 lineIndex;
		float temp;
		
		for (lineIndex = first_x; lineIndex <= last_x; lineIndex++) {
			temp = lineIndex * pixelsPerPixel - large_region.left + base.x;
			into->AddLine(	BPoint(temp, in_region.top),
							BPoint(temp, in_region.bottom),
							gridColor );
		}
		
		for (lineIndex = first_y; lineIndex <= last_y; lineIndex++) {
			temp = lineIndex * pixelsPerPixel - large_region.top + base.y;
			into->AddLine(	BPoint(in_region.left, temp),
							BPoint(in_region.right, temp),
							gridColor);
		}

		into->EndLineArray();
	}
	
	into->SetDrawingMode(B_OP_COPY);
	if( HotSpotX() >= first_x && HotSpotX() <= last_x &&
			HotSpotY() >= first_y && HotSpotY() <= last_y ) {
		into->SetHighColor(255, 0, 0);
		BPoint tl(HotSpotX() * pixelsPerPixel - large_region.left + base.x,
				  HotSpotY() * pixelsPerPixel - large_region.top + base.y);
		BRect area(tl, BPoint(tl.x+pixelsPerPixel, tl.y+pixelsPerPixel));
		area.InsetBy(floor(area.Width()/4), floor(area.Height()/4));
		into->FillEllipse(area);
	}
	
	if( gridColor.alpha != 0 ) into->SetHighColor(gridColor);
	else into->SetHighColor(255, 255, 255);
	
	PRINT(("Filling sides (%ld,%ld)-(%ld,%ld) around (%.2f,%l2f)-(%.2f-%.2f)\n",
			region.left, region.top, region.right, region.bottom,
			in_region.left, in_region.top, in_region.right, in_region.bottom));
			
	if( region.left < 0 ) {
		into->FillRect(BRect(region.left, region.top,
							 -1, region.bottom));
	}
	if( region.right > Width()*pixelsPerPixel+gridOff ) {
		into->FillRect(BRect(Width()*pixelsPerPixel+gridOff, region.top,
							 region.right, region.bottom));
	}
	if( region.top < 0 ) {
		into->FillRect(BRect(region.left, region.top,
							 region.right, -1));
	}
	if( region.bottom > Height()*pixelsPerPixel+gridOff ) {
		into->FillRect(BRect(region.left, Height()*pixelsPerPixel+gridOff,
							 region.right, region.bottom));
	}
	
	into->PopState();
}

void
TBitmapEditor::GetHilite(BView* into, BPoint base, BRect region) const
{
	const BBitmap* shown = ShownBitmap();
	
	BRect pos(region);
	pos.OffsetTo(base);
	
	into->PushState();
	into->SetDrawingMode(B_OP_COPY);
	into->DrawBitmapAsync(shown, region, pos);
	
	region.OffsetTo(base);
	rgb_color black_mix = { 0, 0, 0, 255 - (uint8)(255*2/3) };
	into->SetHighColor(black_mix);
	into->SetDrawingMode(B_OP_ALPHA);
	into->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
	into->FillRect(region);
	into->PopState();
}

const BCursor*
TBitmapEditor::CurrentCursor() const
{
	return fCursor ? fCursor : B_CURSOR_SYSTEM_DEFAULT;
}

// ----------------------- Current Editing State -----------------------

void
TBitmapEditor::SetAttributes(float width, float height,
							 color_space cspace)
{
	bool changed = false;
	
	if( !fWorkspaceBits ) {
		fWorkspaceBits = new TBitsContainer(width, height, cspace);
	} else {
		changed = fWorkspaceBits->SetAttributes(width, height, cspace,
												true, fDitherColorConversions);
	}
	
	if( !fActualBits ) {
		fActualBits = new TBitsContainer(fWorkspaceBits->Width(),
										 fWorkspaceBits->Height(),
										 fWorkspaceBits->ColorSpace());
	} else {
		fActualBits->SetAttributes(fWorkspaceBits->Width(),
									fWorkspaceBits->Height(),
									fWorkspaceBits->ColorSpace(),
									false, fDitherColorConversions);
	}
	
	if( changed ) {
		BMessage msg(T_ATTRIBUTES_CHANGED);
		msg.AddFloat("be:width", Width());
		msg.AddFloat("be:height", Height());
		msg.AddInt32("be:color_space", ColorSpace());
		SendNotices(T_ATTRIBUTES_CHANGED, &msg);
		InvalidateComposite();
	}
}

void
TBitmapEditor::SetHotSpot(int32 x, int32 y)
{
	if( x != fHotSpotX || y != fHotSpotY ) {
		InvalidateComposite(BRect(fHotSpotX, fHotSpotY, fHotSpotX, fHotSpotY), false);
		fHotSpotX = x;
		fHotSpotY = y;
		InvalidateComposite(BRect(fHotSpotX, fHotSpotY, fHotSpotX, fHotSpotY));
	}
}

int32
TBitmapEditor::HotSpotX() const
{
	return fHotSpotX;
}

int32
TBitmapEditor::HotSpotY() const
{
	return fHotSpotY;
}

void
TBitmapEditor::SetSelectionMode(paste_selection_mode mode)
{
	if (fSelectionMode != mode) {
		fSelectionMode = mode;
		if (fSelectionState != kNoSelection) {
			InvalidateComposite(fSelectionRect);
		}
	}
}

paste_selection_mode
TBitmapEditor::SelectionMode() const
{
	return fSelectionMode;
}

void
TBitmapEditor::SetDitherColorConversions(bool state)
{
	if (fDitherColorConversions != state) {
		fDitherColorConversions = state;
		delete fSelectionBitmap;
		fSelectionBitmap = NULL;
		if (fSelectionAlpha) free(fSelectionAlpha);
		fSelectionAlpha = NULL;
		if (fSelectionState != kNoSelection) {
			InvalidateComposite(fSelectionRect);
		}
	}
}

bool
TBitmapEditor::DitherColorConversions() const
{
	return fDitherColorConversions;
}

void
TBitmapEditor::SetBackgroundColor(rgb_color newBGColor)
{
	if( fBackgroundColor == newBGColor ) return;
	
	fBackgroundColor = newBGColor;
	if (fSelectionAlpha) free(fSelectionAlpha);
	fSelectionAlpha = NULL;
	InvalidateComposite();
}

rgb_color
TBitmapEditor::BackgroundColor() const
{
	return fBackgroundColor;
}

rgb_color
TBitmapEditor::PenColor()
{
	return fPenColor;
}

void
TBitmapEditor::NewPenColor(rgb_color color)
{
	rgb_color fixed = fWorkspaceBits->ConstrainColor(color);
	if( fixed != color ) {
		NewColorSelection(fixed, true);
	}
	if( fPenColor != fixed ) {
		fLastPenColor = fPenColor;
		fPenColor = fixed;
		SetPenColor();
	}
}

void
TBitmapEditor::SetPenColor()
{
	if (fWorkspaceBits->Lock()) {
		fWorkspaceBits->SetHighColor(fPenColor);
		fWorkspaceBits->Unlock();
	}
}

void
TBitmapEditor::RevertPenColor()
{
	rgb_color tmp = fLastPenColor;
	fLastPenColor = fPenColor;
	fPenColor = tmp;
	SetPenColor();
}

rgb_color
TBitmapEditor::SecondaryColor()
{
	return fSecondaryColor;
}

void
TBitmapEditor::NewSecondaryColor(rgb_color color)
{
	rgb_color fixed = fWorkspaceBits->ConstrainColor(color);
	if( fixed !=color ) {
		NewColorSelection(fixed, false);
	}
	if( fSecondaryColor != fixed ) {
		fLastSecondaryColor = fSecondaryColor;
		fSecondaryColor = fixed;
		SetSecondaryColor();
	}
}

void
TBitmapEditor::SetSecondaryColor()
{
	if (fWorkspaceBits->Lock()) {
		fWorkspaceBits->SetLowColor(fSecondaryColor);
		fWorkspaceBits->Unlock();
	}
}

void
TBitmapEditor::RevertSecondaryColor()
{
	rgb_color tmp = fLastSecondaryColor;
	fLastSecondaryColor = fSecondaryColor;
	fSecondaryColor = tmp;
	SetSecondaryColor();
}

pattern
TBitmapEditor::PenPattern()
{
	return fCurrentPattern;
}

void
TBitmapEditor::SetPenPattern(pattern p)
{
	fLastPattern = fCurrentPattern;
	fCurrentPattern = p;
}

void
TBitmapEditor::RevertPenPattern()
{
	fCurrentPattern = fLastPattern;
}

void
TBitmapEditor::Radius(float *x, float *y)
{
	*x = fXRadius;
	*y = fYRadius;
}

void
TBitmapEditor::SetRadius(float x, float y)
{
	fXRadius = x;
	fYRadius = y;
}

void
TBitmapEditor::Angles(float *start, float *arc)
{
	*start = fStartAngle;
	*arc = fArcAngle;
}

void
TBitmapEditor::SetAngles(float start, float arc)
{
	fStartAngle = start;
	fArcAngle = arc;
}

int32
TBitmapEditor::CurrentTool(bool swap)
{
	switch(fCurrentTool) {
		case kPencilTool:
			if (swap) 	return kEyeDropperTool;
			else 		return kPencilTool;
			break;
		case kBucketTool:
			if (swap) 	return kEyeDropperTool;
			else 		return fCurrentTool;
			break;
		case kEyeDropperTool:
			if (swap) 	return kPencilTool;
			else 		return kEyeDropperTool;
			break;
		case kEraserTool:
			return kEraserTool;
			break;
		case kLineTool:
			if (swap) 	return kEyeDropperTool;
			else 		return kLineTool;
			break;
		case kSelectionTool:
			if (swap) 	return kLassoTool;
			else		return kSelectionTool;
			break;
		case kLassoTool:
			if (swap)	return kSelectionTool;
			else		return kLassoTool;
			break;
		case kRectTool:
			if (swap) 	return kEyeDropperTool;
			else 		return kRectTool;
			break;
		case kFilledRectTool:
			if (swap) 	return kEyeDropperTool;
			else 		return kFilledRectTool;
			break;
		case kRoundRectTool:
			if (swap) 	return kEyeDropperTool;
			else 		return kRoundRectTool;
			break;
		case kFilledRoundRectTool:
			if (swap) 	return kEyeDropperTool;
			else 		return kFilledRoundRectTool;
			break;
		case kOvalTool:
			if (swap) 	return kEyeDropperTool;
			else 		return kOvalTool;
			break;
		case kFilledOvalTool:
			if (swap) 	return kEyeDropperTool;
			else 		return kFilledOvalTool;
			break;
		case kArcTool:
			if (swap) 	return kEyeDropperTool;
			else 		return kArcTool;
			break;
		case kFilledArcTool:
			if (swap) 	return kEyeDropperTool;
			else 		return kFilledArcTool;
			break;
		case kTriangleTool:
			if (swap) 	return kEyeDropperTool;
			else 		return kTriangleTool;
			break;
		case kFilledTriangleTool:
			if (swap) 	return kEyeDropperTool;
			else 		return kFilledTriangleTool;
			break;
		case kHotSpotTool:
			return kHotSpotTool;
		default:
			return kPencilTool;
			break;
	}
}

void
TBitmapEditor::SetCurrentTool(int32 tool)
{
	if( tool != kSelectionTool ) DeSelect();
	fCurrentTool = tool;
	UpdateCursor(CurrentMouseLoc());
}

// ----------------------------- Editing -----------------------------

void
TBitmapEditor::BeginEdit(const char* name)
{
	ASSERT(fEditing == false);
	fEditName = name;
	fEditing = true;
}

bool
TBitmapEditor::Editing() const
{
	return fEditing;
}

void
TBitmapEditor::EndEdit(bool cancel)
{
	ASSERT(fEditing == true);
	fEditing = false;
	if( !cancel ) BitmapChanged(fEditName.String());
	fEditName = "";
}

// Start an edit session that performs a selection in the workspace.
// Initialize the selection state with the given rectangle in the workspace.
// If 'clear' is true, also erase the bits at this location in the workspace.
bool
TBitmapEditor::BeginCopySelection(BRect selectionRect, bool clear)
{
	if( !BeginPasteSelection(fWorkspaceBits->Bitmap(), selectionRect,
							 selectionRect.LeftTop()) ) {
		return false;
	}
	
	if( clear ) {
		fSelectionStart = fSelectionRect;
	}
	
	return true;
}

// Start an edit session that performs a paste into the workspace.
// The selection state is set up at the given rectangle in the workspace,
// with the given bitmap.
bool
TBitmapEditor::BeginPasteSelection(const BBitmap* bits, BRect srcRect,
								   BPoint pos)
{
	if( !bits ) return false;

	BRect selectionRect(srcRect);
	selectionRect.OffsetTo(pos);
	if( !SetupSelection(selectionRect) ) return false;
	
	BRect destRect(selectionRect);
	destRect.OffsetTo(0,0);
	fRawSelectionBitmap = new BBitmap(destRect, bits->ColorSpace());
	copy_bitmap(fRawSelectionBitmap, bits, srcRect, BPoint(0, 0));
	if (fSelectionAlpha) free(fSelectionAlpha);
	fSelectionAlpha = NULL;
	return true;
}

void
TBitmapEditor::EndSelection(bool cancel)
{
	if( HaveSelection() ) {
		fSelectionState = kNoSelection;
		
		if( !cancel && fSelectionStart.IsValid() ) {
			BeginEdit("Erase Selection");
			if( fWorkspaceBits->Lock() ) {
				EraseUnderSelection(fWorkspaceBits);
				InvalidateComposite(fSelectionStart, false);
				fWorkspaceBits->Sync();
				fWorkspaceBits->Unlock();
			}
			EndEdit();
		}
		
		InvalidateComposite(fSelectionRect);
		
		delete fRawSelectionBitmap;
		fRawSelectionBitmap = 0;
		delete fSelectionBitmap;
		fSelectionBitmap = 0;
		if (fSelectionAlpha) free(fSelectionAlpha);
		fSelectionAlpha = NULL;
		fSelectionRect = BRect();
		delete fSelectionRunner;
		fSelectionRunner = 0;
		fSelectionStart = fSelectionRect = fLastSelectionRect = BRect();
	}
}

void
TBitmapEditor::DeSelect()
{
	if (HaveSelection()) {
		// 	called from single-click and when new tool is selected
		//
		if( fSelectionRect != fSelectionStart ) {
			OverlaySelection(fSelectionRect, fSelectionMode);
			InvalidateComposite(fSelectionStart, false);
		}
		fSelectionStart = BRect();
	}
	
	EndSelection();
}

void
TBitmapEditor::SetBitmap(const BBitmap* bits, BRect srcRect, bool report)
{
	if( report ) BeginEdit("Paste Bitmap");
	if (WorkspaceBits()->Lock()) {
		if( srcRect == bits->Bounds() &&
				srcRect != WorkspaceBits()->Bitmap()->Bounds() ) {
			// if the source is the entire bitmap, and the destination
			// is a different size, let's do our nice scaling.
			scale_bitmap(WorkspaceBits()->Bitmap(), bits);
		} else {
			WorkspaceBits()->PushState();
			WorkspaceBits()->SetDrawingMode(B_OP_COPY);
			WorkspaceBits()->DrawBitmap(bits, srcRect, Bounds());
			WorkspaceBits()->PopState();
			WorkspaceBits()->Sync();
		}
		WorkspaceBits()->Unlock();
		InvalidateComposite();
	}
	if( report ) EndEdit();
}

// ----------------------------- Cut And Paste -----------------------------

void
TBitmapEditor::AddCopyToClipboard()
{
	if( fRawSelectionBitmap ) {
		be_clipboard->Lock();
		be_clipboard->Clear();
		
		BMessage *message = be_clipboard->Data();
		if (!message) {
			printf("no clip msg\n");
			return;
		}
		
		BMessage embeddedBitmap;
		fRawSelectionBitmap->Archive(&embeddedBitmap);
		status_t err = message->AddMessage(kBitmapMimeType, &embeddedBitmap);
		ASSERT(err == B_OK);
		err = message->AddRect("rect", fSelectionRect);
		ASSERT(err == B_OK);

		be_clipboard->Commit();
		be_clipboard->Unlock();
				
	} else
		printf("can't lock the selection bits\n");
}

BBitmap*
TBitmapEditor::GetCopyFromMessage(const BMessage* message,
								  BRect* frame)
{
	BMessage embeddedBitmap;
	status_t err = message->FindMessage(kBitmapMimeType, &embeddedBitmap);
	if( err != B_OK ) return 0;
	
	BArchivable* a = instantiate_object(&embeddedBitmap);
	if( !a ) return 0;
	
	BBitmap* b = dynamic_cast<BBitmap*>(a);
	if( !b ) {
		delete a;
		return 0;
	}
	
	if( frame ) message->FindRect("rect", frame);
	
	return b;
}

void
TBitmapEditor::DoCut()
{
	if (fSelectionState == kHaveSelection) {
		AddCopyToClipboard();
		EndSelection();
	}
}

// 	copyselection is called at the end of the trackmouse while selecting
//	all that is done here is change the state of pasting and saving of the copy rect
void
TBitmapEditor::DoCopy()
{
	if (fSelectionState == kHaveSelection) {
		
		AddCopyToClipboard();
	}
}

bool
TBitmapEditor::CanCopy()
{
	return (fSelectionState == kHaveSelection);
}

// paste using selectionRect from Copy to Actual Bits
void
TBitmapEditor::OverlaySelection(BRect selectionRect, paste_selection_mode mode)
{
	BeginEdit("Paste Selection");
	if( fWorkspaceBits->Lock() ) {
		OverlaySelection(fWorkspaceBits, fWorkspaceBits->Bitmap(),
						 selectionRect, mode);
		fWorkspaceBits->Unlock();
	}
	EndEdit();
}

// paste using selectionRect from Copy to given bitmap
void
TBitmapEditor::OverlaySelection(BView* dest, BBitmap* destBM, BRect selectionRect,
								paste_selection_mode mode)
{
	BRect destRect(dest->Bounds());
	BRect srcRect(selectionRect);
	srcRect.OffsetTo(0,0);
	
	// Clip selection rectangle if it is outside of the destination
	// bitmap's bounds rectangle.
	if( selectionRect.left < destRect.left ) {
		srcRect.left += destRect.left - selectionRect.left;
		selectionRect.left = destRect.left;
	}
	if( selectionRect.top < destRect.top ) {
		srcRect.top += destRect.left - selectionRect.top;
		selectionRect.top = destRect.top;
	}
	if( selectionRect.right > destRect.right ) {
		srcRect.right += destRect.right - selectionRect.right;
		selectionRect.right = destRect.right;
	}
	if( selectionRect.bottom > destRect.bottom ) {
		srcRect.bottom += destRect.bottom - selectionRect.bottom;
		selectionRect.bottom = destRect.bottom;
	}
	
	EraseUnderSelection(dest);
	
	// If not completely out of bounds, draw it.
	if( srcRect.IsValid() && selectionRect.IsValid() ) {
		dest->PushState();
		
		// Do any color conversion needed.
		const BBitmap* selBitmap = fRawSelectionBitmap;
		if (DitherColorConversions() &&
				fRawSelectionBitmap->ColorSpace() !=
					fWorkspaceBits->Bitmap()->ColorSpace()) {
			if (!fSelectionBitmap ||
					fSelectionBitmap->ColorSpace() !=
						fWorkspaceBits->Bitmap()->ColorSpace()) {
				delete fSelectionBitmap;
				fSelectionBitmap = NULL;
				if (fSelectionAlpha) free(fSelectionAlpha);
				fSelectionAlpha = NULL;
				fSelectionBitmap = new BBitmap(fRawSelectionBitmap->Bounds(), 0,
											fWorkspaceBits->Bitmap()->ColorSpace());
				set_bitmap(fSelectionBitmap, fRawSelectionBitmap, true);
			}
			if (fSelectionBitmap) selBitmap = fSelectionBitmap;
		}
		
		switch (mode) {
			case kPasteAlpha:
				dest->SetDrawingMode(B_OP_ALPHA);
				dest->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
				dest->DrawBitmap(selBitmap,srcRect,selectionRect);
				break;
			case kPasteBackground:
				if (!fSelectionAlpha) {
					fSelectionAlpha = make_stencil(selBitmap, fSecondaryColor);
				}
				if (fSelectionAlpha) {
					dest->Sync();
					copy_bitmap(destBM, selBitmap,
								srcRect, selectionRect.LeftTop(),
								B_OP_ALPHA, fSelectionAlpha);
					break;
				}
				
				// If couldn't draw with alpha, fall through.
			case kPasteCopy:
				dest->SetDrawingMode(B_OP_COPY);
				dest->DrawBitmap(selBitmap,srcRect,selectionRect);
				break;
		}
		dest->PopState();
	}
}

status_t
TBitmapEditor::DoPaste()
{
	if( !be_clipboard->Lock() ) return B_ERROR;
	
	BMessage *message = be_clipboard->Data();
	status_t err = PasteFromMessage(message);
	
	be_clipboard->Unlock();
	
	return err;
}

status_t
TBitmapEditor::PasteFromMessage(const BMessage* message)
{
	BRect r;
	BBitmap* tempBits = GetCopyFromMessage(message, &r);
	if( tempBits ) {
		BPoint pos = r.LeftTop();
		r.OffsetTo(0, 0);
		BeginPasteSelection(tempBits, r, pos);
		delete tempBits;	// this was a leak, I think! -- hplus
		return B_OK;
	}
	
	return B_ERROR;
}

void
TBitmapEditor::EraseUnderSelection(BView* dest)
{
	EraseUnderSelection(dest, SecondaryColor());
}

void
TBitmapEditor::EraseUnderSelection(BView* dest, rgb_color color)
{
	if( fSelectionStart.IsValid() ) {
		dest->PushState();
		dest->SetHighColor(color);
		dest->SetDrawingMode(B_OP_COPY);
		dest->FillRect(fSelectionStart);
		dest->PopState();
	}
}

void
TBitmapEditor::DoClear()
{
	EndSelection();
}

bool
TBitmapEditor::CanClear()
{
	return HaveSelection();
}

void
TBitmapEditor::DoSelectAll()
{
	BeginCopySelection(Bounds());
}


void
TBitmapEditor::MessageReceived(BMessage * message)
{
	if( DoMessageReceived(message, this) == B_OK ) return;
	BHandler::MessageReceived(message);
}

// ----------------------------- Dumping Data -----------------------------

void
TBitmapEditor::DumpSelection(BDataIO& io, const char* name) const
{
	DumpMap((fSelectionState == kHaveSelection) ?
		fRawSelectionBitmap :
		fWorkspaceBits->Bitmap(),
			io, name);
}

void
TBitmapEditor::DumpEntireMap(BDataIO& io, const char *name) const
{
	DumpMap(fWorkspaceBits->Bitmap(), io, name);
}

static void
write_binary_text(BDataIO& io, const uint8* data, int32 length,
				  int32 bytesPerEntry, int32 maxBytesPerRow = 1024)
{
	char buffer[1024];
	const int32 kMaxColumnWidth = 16 / bytesPerEntry;
	maxBytesPerRow /= bytesPerEntry;
	int32 columnWidth = (maxBytesPerRow < kMaxColumnWidth) ? maxBytesPerRow : kMaxColumnWidth;
	
	while (length > 0) {

		io.Write("\n\t", 2);
		
		//	stream out each row, based on the number of bytes required per row
		//	padding is in the bitmap and will be streamed as 0xff
		for (int32 column = 0; column < columnWidth; column++) {
		
			// stream out individual pixel components
			switch (bytesPerEntry) {
				case 2: {
					length -= 2;
					sprintf(buffer, "0x%04x", *(int16*)data);
					data += 2;
				} break;
				case 4: {
					length -= 4;
					sprintf(buffer, "0x%08lx", *(int32*)data);
					data += 4;
				} break;
				default: {
					length--;
					sprintf(buffer, "0x%02x", *data++);
				} break;
			}
			io.Write(buffer, strlen(buffer));
			if (length > 0)
				io.Write(", ", 2);
			else
				break;
			
			//	make sure we don't walk off the end of the bits array
			if (!length)
				break;

		}
	}
}

void 
TBitmapEditor::DumpMap(const BBitmap *bitmap, BDataIO& io,
	const char *name) const
{
	char buffer[1024];
	int32 bytesPerPixel;
	const char *kColorSpaceName;	

	switch (bitmap->ColorSpace()) {
		case B_GRAY8:
			bytesPerPixel = 1;
			kColorSpaceName = "B_GRAY8";
			break;
		case B_CMAP8:
			bytesPerPixel = 1;
			kColorSpaceName = "B_CMAP8";
			break;
		case B_RGB15:
			bytesPerPixel = 2;
			kColorSpaceName = "B_RGB15";
			break;
		case B_RGBA15:
			bytesPerPixel = 2;
			kColorSpaceName = "B_RGBA15";
			break;
		case B_RGB16:
			bytesPerPixel = 2;
			kColorSpaceName = "B_RGB16";
			break;
		case B_RGB32:
			bytesPerPixel = 4;
			kColorSpaceName = "B_RGB32";
			break;
		case B_RGBA32:
			bytesPerPixel = 4;
			kColorSpaceName = "B_RGBA32";
			break;
		default:
			printf("dump: usupported ColorSpace\n");
			return;
	}
	
	// stream out the width, height and ColorSpace
	sprintf(buffer, "const int32 k%sWidth = %ld;\n", name, (int32)bitmap->Bounds().Width()+1);
	io.Write(buffer, strlen(buffer));
	sprintf(buffer, "const int32 k%sHeight = %ld;\n", name, (int32)bitmap->Bounds().Height()+1);
	io.Write(buffer, strlen(buffer));
	sprintf(buffer, "const color_space k%sColorSpace = %s;\n\n", name, kColorSpaceName);
	io.Write(buffer, strlen(buffer));

	const uint8* bits = (const uint8*)bitmap->Bits();
	const int32 kMaxColumnWidth = 16 / bytesPerPixel;
	int32 bytesPerRow = bitmap->BytesPerRow() / bytesPerPixel;
	int32 columnWidth = (bytesPerRow < kMaxColumnWidth) ? bytesPerRow : kMaxColumnWidth;
	
	const char* sizeName = "uint8";
	if (bytesPerPixel == 2) sizeName = "uint16";
	else if (bytesPerPixel == 4) sizeName = "uint32";
	
	// stream out the constant name for this array
	sprintf(buffer, "const %s k%sBits[] = {", sizeName, name);
	io.Write(buffer, strlen(buffer));

	for (int32 remaining = bitmap->BitsLength(); remaining > 0; ) {

		io.Write("\n\t", 2);
		
		//	stream out each row, based on the number of bytes required per row
		//	padding is in the bitmap and will be streamed as 0xff
		for (int32 column = 0; column < columnWidth; column++) {
		
			// stream out individual pixel components
			switch (bytesPerPixel) {
				case 2: {
					remaining -= 2;
					sprintf(buffer, "0x%04x", *(int16*)bits);
					bits += 2;
				} break;
				case 4: {
					remaining -= 4;
					sprintf(buffer, "0x%08lx", *(int32*)bits);
					bits += 4;
				} break;
				default: {
					--remaining;
					sprintf(buffer, "0x%02x", *bits++);
				} break;
			}
			io.Write(buffer, strlen(buffer));
			if (remaining > 0)
				io.Write(", ", 2);
			else
				break;
			
			//	make sure we don't walk off the end of the bits array
			if (!remaining)
				break;

		}
	}
	sprintf(buffer, "\n};\n\n");
	io.Write(buffer, strlen(buffer));
}

void
TBitmapEditor::DumpCursor(BDataIO& io, const char *name) const
{
	char buffer[1024];
	
	const BBitmap* bitmap = fWorkspaceBits->Bitmap();
	BMessage ioExt;
	ioExt.AddInt32("be:x_hotspot", HotSpotX());
	ioExt.AddInt32("be:y_hotspot", HotSpotY());
	
	size_t size=0;
	uint8* data = CursorFromBitmap(bitmap, &ioExt, &size);
	if( data ) {
		const uint8* pos = data;
		
		const int32 cursorSize = (int32)(*pos++);
		const int32 planeSize = (cursorSize*cursorSize)/8;
		sprintf(buffer, "const uint8 k%sBits[] = {\n", name);
		io.Write(buffer, strlen(buffer));
		sprintf(buffer, "\t%ld,\t\t/* cursor size */\n", cursorSize);
		io.Write(buffer, strlen(buffer));
		sprintf(buffer, "\t%ld,\t\t/* bits per pixel */\n", (int32)(*pos++));
		io.Write(buffer, strlen(buffer));
		sprintf(buffer, "\t%ld,\t\t/* vertical hot spot */\n", (int32)(*pos++));
		io.Write(buffer, strlen(buffer));
		sprintf(buffer, "\t%ld,\t\t/* horizontal hot spot */\n", (int32)(*pos++));
		io.Write(buffer, strlen(buffer));
		
		size -= 4;
		
		io.Write("\n\t/* data */", 12);
		write_binary_text(io, pos, planeSize, 1);
		pos += planeSize;
		size -= planeSize;
		
		io.Write(",\n\n\t/* mask */", 14);
		write_binary_text(io, pos, planeSize, 1);
		pos += planeSize;
		size -= planeSize;
		
		if (size > 0) {
			io.Write(",\n\n\t/* extra */", 15);
			write_binary_text(io, pos, size, 1);
		}
		
		free(data);
		
		io.Write("\n};\n", 4);
		
	} else {
		io.Write("/* Error creating cursor */\n", 28);
	}
}

TBitsContainer*
TBitmapEditor::WorkspaceBits()
{
	return fWorkspaceBits;
}

TBitsContainer*
TBitmapEditor::ActualBits()
{
	return fActualBits;
}

// ---------------------------- Selection ----------------------------

bool
TBitmapEditor::SetupSelection(BRect selectionRect)
{
	DeSelect();							// Clear out any existing selection.
	
	SetCurrentTool(kSelectionTool);	// !! need to send a msg to the toolpicker to sync
	
	// Set up basic selection state.  Make sure the new selection
	// rectangle is visible in the bitmap.
	if (selectionRect.right >= Width()) {
		selectionRect.OffsetTo(BPoint(Width()-selectionRect.Width()-1, selectionRect.top));
	}
	if (selectionRect.bottom >= Height()) {
		selectionRect.OffsetTo(BPoint(selectionRect.left, Height()-selectionRect.Height()-1));
	}
	if (selectionRect.left < 0) {
		selectionRect.OffsetTo(BPoint(0, selectionRect.top));
	}
	if (selectionRect.top < 0) {
		selectionRect.OffsetTo(BPoint(selectionRect.left, 0));
	}
	fSelectionState = kHaveSelection;
	MakeSelection(selectionRect);
	
	if( fSelectionState == kNoSelection ) return false;
	
	return true;
}

bool
TBitmapEditor::HaveSelection()
{
	return (fSelectionState == kHaveSelection);
}

//	move the selection by some offset values
//	used in keyboard movement of selection
void
TBitmapEditor::MoveSelection(int32 xOffset, int32 yOffset)
{
	if (fSelectionState == kHaveSelection) {			
		BRect destRect = fSelectionRect;

		destRect.left += xOffset;
		destRect.right += xOffset;
		destRect.top += yOffset;
		destRect.bottom += yOffset;
		
		if (destRect.left >= 0 && destRect.right < Width()
			&& destRect.top >= 0 && destRect.bottom < Height()) {
			fLastSelectionRect = fSelectionRect;
			fSelectionRect = destRect;
			PRINT(("Moving selection rectangle...\n"));
			InvalidateComposite(fLastSelectionRect, false);
			InvalidateComposite(fSelectionRect);
		}
	}
}

void
TBitmapEditor::MoveSelection(int32 cX, int32 cY, int32 lX, int32 lY)
{
	if (fSelectionState == kMovingSelection) {
		int32 w,h ;
		BRect destRect = fSelectionRect;
		//
		//	move the dest rect to the new offset
		//
		if (lY < cY) {				// moving rect down
			h = cY - lY; 
			destRect.top += h;
			destRect.bottom += h;
		} else {					// moving rect up
			h = lY - cY;
			destRect.top -= h;
			destRect.bottom -= h;
		}	
		
		if (lX < cX) {				// moving rect right
			w = cX - lX;
			destRect.left += w ;
			destRect.right += w;
		} else {					// moving rect left
			w = lX - cX;
			destRect.left -= w;
			destRect.right -= w;
		}	

		fLastSelectionRect = fSelectionRect;
		fSelectionRect = destRect;
		PRINT(("Moving selection rectangle...\n"));
		InvalidateComposite(fLastSelectionRect, false);
		InvalidateComposite(fSelectionRect);
	}
}

void
TBitmapEditor::MakeSelection(BRect r)
{
	if( fSelectionRect.IsValid() ) InvalidateComposite(fSelectionRect, false);
	
	fSelectionStart = BRect();
	fSelectionRect = MakeValidRect(r);
	fLastSelectionRect = BRect();
	
	if (fSelectionRect.Width() >= 0 && fSelectionRect.Height() >= 0) {
		InvalidateComposite(fSelectionRect);
		if( !fSelectionRunner ) {
			fSelectionRunner = new BMessageRunner(this,
												  new BMessage(kSelectionPulse),
												  100000);
		}
	} else {
		fSelectionState = kNoSelection;
	}
}

void
TBitmapEditor::Pulse(void)
{
	if (fSelectionState != kNoSelection) {

		uchar* pat = (uchar *)fMarqueePattern;
		uchar byte = pat[7];
		for (int32 i = 6; i >= 0; i--)
			pat[i + 1] = pat[i];
		pat[0] = byte;

		#if 0
		InvalidateComposite(BRect(fSelectionRect.left, fSelectionRect.top,
								  fSelectionRect.left+1, fSelectionRect.bottom));
		InvalidateComposite(BRect(fSelectionRect.left, fSelectionRect.top,
								  fSelectionRect.right, fSelectionRect.top+1));
		InvalidateComposite(BRect(fSelectionRect.right, fSelectionRect.top,
								  fSelectionRect.right-1, fSelectionRect.bottom));
		InvalidateComposite(BRect(fSelectionRect.left, fSelectionRect.bottom,
								  fSelectionRect.right, fSelectionRect.bottom-1));
		#endif
		InvalidateComposite(fSelectionRect);
	}
}

// ------------------------------ Mouse State ------------------------------

BPoint
TBitmapEditor::CurrentMouseLoc()
{
	return fCurrentLoc;
}

rgb_color
TBitmapEditor::CurrentColorAtLoc()
{
	return fColorAtLoc;
}

// ------------------------------ Cursors ------------------------------

void
TBitmapEditor::UpdateCursor(BPoint /*where*/)
{
	const BCursor* cursor = 0;
	
	switch(CurrentTool(fSwapModifier)) {
		case kSelectionTool:
			cursor = Resources().FindCursor(R_MARQUEE_CURSOR);
			break;
		case kEraserTool:
			cursor = Resources().FindCursor(R_ERASER_CURSOR);
			break;
		case kPencilTool:
			cursor = Resources().FindCursor(R_PENCIL_CURSOR);
			break;
		case kEyeDropperTool:
			cursor = Resources().FindCursor(R_DROPPER_CURSOR);
			break;
		case kBucketTool:
			cursor = Resources().FindCursor(R_BUCKET_CURSOR);
			break;
		default:
			cursor = Resources().FindCursor(R_SHAPES_CURSOR);
			break;
	}
	
	if( fCursor != cursor ) {
		fCursor = cursor;
		BMessage msg(T_CURSOR_CHANGED);
		SendNotices(T_CURSOR_CHANGED, &msg);
	}
}

// ------------------------------ Compositing ------------------------------

void
TBitmapEditor::InvalidateComposite(BRect region, bool report)
{
	PRINT(("Invalidating (%.2f,%.2f)-(%.2f,%.2f) with (%.2f,%.2f)-(%.2f,%.2f)\n",
			fInvalidComposite.left, fInvalidComposite.top,
			fInvalidComposite.right, fInvalidComposite.bottom,
			region.left, region.top, region.right, region.bottom));
	if( region.IsValid() ) {
		region = region & BRect(0, 0, Width()-1, Height()-1);
		fInvalidComposite = ExtendRects(fInvalidComposite, region);
		PRINT(("New invalid region is (%.2f,%.2f)-(%.2f,%.2f)\n",
			fInvalidComposite.left, fInvalidComposite.top,
			fInvalidComposite.right, fInvalidComposite.bottom));
	}
	if( report ) ReportInvalidComposite();
}

void
TBitmapEditor::InvalidateComposite()
{
	InvalidateComposite(Bounds());
}

void
TBitmapEditor::ReportInvalidComposite()
{
	if( fInvalidComposite.IsValid() ) {
		BMessage msg(T_BITMAP_INVALIDATED);
		msg.AddRect("be:region", fInvalidComposite);
		SendNotices(T_BITMAP_INVALIDATED, &msg);
	}
}

//	build the image of the icon from the edited bits, selection and marquee
bool
TBitmapEditor::InitialCompositing(BView* target)
{
	if( !fInvalidComposite.IsValid() ) return false;
	
	BRect actualRect = fInvalidComposite & Bounds();

	// draw the edited (workspace) to the actual, taking care of
	// any transparency.
	target->PushState();
	target->SetDrawingMode(B_OP_COPY);
	target->SetHighColor(B_TRANSPARENT_COLOR);
	target->FillRect(actualRect);
	//target->SetDrawingMode(B_OP_ALPHA);
	//target->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	target->DrawBitmap(fWorkspaceBits->Bitmap(), actualRect, actualRect);
	target->PopState();
	
	return true;
}

void
TBitmapEditor::FinalCompositing(BView* target, BBitmap* targetBM, bool show_marquee)
{
	target->PushState();
	
	//	draw the selection bits to the actual
	if ((fSelectionState == kHaveSelection) || (fSelectionState == kMovingSelection)) {
		//BRect selectionRect = fInvalidComposite & fSelectionRect;
		if( fSelectionRect.IsValid() ) {
			OverlaySelection(target, targetBM, fSelectionRect,
							 fSelectionMode);
		}
	}

	//	if applicable, draw the marquee around the selection
	if ( show_marquee && fSelectionState != kNoSelection) {
		target->SetHighColor(0,0,0,0);
		target->SetLowColor(255, 255, 255);
		target->SetPenSize(1);
		target->SetDrawingMode(B_OP_COPY);

		target->StrokeRect(fSelectionRect, *fMarqueePattern);		
	}
	
	target->PopState();
	target->Sync();
	
	blend_bitmap_color(targetBM, fBackgroundColor);
	
	fInvalidComposite = BRect();
}

//	build the image of the icon from the edited bits, selection and marquee
void
TBitmapEditor::CompositeBits(bool show_marquee)
{
	(void)show_marquee;
	
	if (fActualBits->Lock()) {
		if( !InitialCompositing(fActualBits) ) {
			fActualBits->Unlock();
			return;
		}
		
		FinalCompositing(fActualBits, fActualBits->Bitmap(), show_marquee);
		fActualBits->Sync();
		fActualBits->Unlock();
	} else
		printf("Draw - can't lock the actual bits buffer\n");
}

// --------------------------- Tools Tracking ---------------------------

void
TBitmapEditor::StartToolTracking(BPoint point)
{
	fTracking = true;
	fTrackButtons = fButtons;
	fTrackTool = -1;
	fLastX = (int32)point.x;
	fLastY = (int32)point.y;
	fStartX = fLastX;
	fStartY = fLastY;
	fPattern = PenPattern();
	Angles(&fTrackStartAngle, &fTrackArcAngle);
	Radius(&fTrackXRadius, &fTrackYRadius);
	fTrackFirstPass = true;
	fTrackMoved = false;
	
	// 	get the color index at the first click location
	//	used later on for pixel flipping with the pencil tool
	fColorAtClick = WorkspaceBits()->GetPixel(BPoint(fLastX,fLastY));
	
	if (fColorAtClick == PenColor()) {
		key_info keys;
		get_key_info(&keys);
		
		if (fConstrainModifier)				//	change only this color
			fPixelFlip = 3;
		else								//	change all pixels to bg
			fPixelFlip = 1;
	} else {
		if (fConstrainModifier)				//	change only this color
			fPixelFlip = 2;
		else
			fPixelFlip = 0;					// 	draw in pen color
	}
	
	fTrackTool = CurrentTool(fSwapModifier);
	fTrackTransientBits = ActualBits();
	fTrackTransientUpdate = BRect();
	fTrackFinalBits = WorkspaceBits();
	fTrackFinalUpdate = BRect();
	
	TrackTool(point);
}

void
TBitmapEditor::TrackTool(BPoint point)
{
	if( !fTracking ) return;
	
	//
	//	convert the point to the actual size
	//
	fCurX = (int32)point.x;
	fCurY = (int32)point.y;
	
	const bool moved = ( fCurX != fLastX || fCurY != fLastY );
	if( moved ) fTrackMoved = true;
	
	// constrain the bounds
	if (fCurX < 0) fCurX = 0;
	else if (fCurX >= Width()) fCurX = (int32)(Width()-1);
	if (fCurY < 0) fCurY = 0;
	else if (fCurY >= Height()) fCurY = (int32)(Height()-1);
	
	if (moved || fTrackFirstPass) {
		
		DrawTool(fTrackTool);
		
		fLastX = fCurX;
		fLastY = fCurY;
		fTrackFirstPass = false;
	}
};

void
TBitmapEditor::EndToolTracking()
{
	if( !fTracking ) return;
	
	// Draw the final tool change into the workspace.
	fTrackTransientBits = fTrackFinalBits;
	if( fTrackTool != kEyeDropperTool
			&& fTrackTool != kHotSpotTool ) DrawTool(fTrackTool);
	
	fTracking = false;
	
	if (fSelectionState == kSelecting) {
		// 	just finished selecting a selection rect,
		//	copy to the selection buffer
		if( fTrackMoved ) {
			BeginCopySelection(fSelectionRect);
		} else {
			fSelectionState = kHaveSelection;
			EndSelection(true);
		}
	} else if (fSelectionState == kMovingSelection) {
		// done moving selection.
		fSelectionState = kHaveSelection;
		if( !fTrackMoved ) {
			// if the mouse didn't move, perform deselection.
			DeSelect();
		}
	} else {
		fSelectionState = kNoSelection;
	}
	
	if( fTrackTool != kSelectionTool && fTrackTool != kEyeDropperTool
			&& fTrackTool != kHotSpotTool ) {
		// For all but the selection tool, releasing the mouse button
		// is the end if an edit operation.
		EndEdit();
	}
}

void
TBitmapEditor::DrawTool(int32 tool)
{
	int32 sx=fStartX;
	int32 sy=fStartY;
	int32 ex=fCurX;
	int32 ey=fCurY;
	
	// Nasty special case -- selection simply moves the selection
	// rectangle, so shouldn't do any compositing here.
	if( tool == kSelectionTool ) {
		ConstrainRect(fConstrainModifier,&sx,&sy,&ex,&ex);
		UseSelectionTool(sx,sy,ex,ey, fLastX,fLastY, fSwapModifier, fTrackFirstPass);
		return;
	}
	
	const bool dif_bits = fTrackTransientBits != fTrackFinalBits;
	
	if( !fTrackTransientBits->Lock() ) return;
	if( dif_bits && !fTrackFinalBits->Lock() ) {
		fTrackTransientBits->Unlock();
		return;
	}
	
	if( dif_bits ) fTrackTransientBits->PushState();
	fTrackFinalBits->PushState();
	
	fTrackFinalBits->SetHighColor( (fTrackButtons&B_SECONDARY_MOUSE_BUTTON)
									? fSecondaryColor : fPenColor );
	fTrackFinalBits->SetLowColor( (fTrackButtons&B_SECONDARY_MOUSE_BUTTON)
									? fPenColor : fSecondaryColor );
	
	if( dif_bits ) {
		copy_view_state(fTrackTransientBits, fTrackFinalBits);
	}
	
	// Restore any invalid portions of the composite area.
	InvalidateComposite(fTrackTransientUpdate, false);
	if( dif_bits ) {
		InitialCompositing(fTrackTransientBits);
	}
	
	fTrackTransientUpdate = BRect();
	fTrackFinalUpdate = BRect();
	
	switch(tool) {
		case kEyeDropperTool:
			UseEyeDropperTool(fCurX,fCurY);
			break;
		case kPencilTool:
			UsePencilTool(fPixelFlip, BPoint(fCurX,fCurY),
				BPoint(fLastX,fLastY), fColorAtClick);					
			break;
		case kEraserTool:
			UseEraser(fCurX,fCurY,fLastX,fLastY);
			break;
		case kLineTool:
			ConstrainRect(fConstrainModifier,&sx,&sy,&ex,&ex);
			UseLineTool(sx,sy,ex,ey, fPattern);
			break;
		case kLassoTool:
			printf("irregualar selection not implemented\n");
			break;
		case kRectTool:
			ConstrainRect(fConstrainModifier,&sx,&sy,&ex,&ex);
			UseRectTool(sx,sy,ex,ey, fPattern, false);
			break;
		case kFilledRectTool:
			ConstrainRect(fConstrainModifier,&sx,&sy,&ex,&ex);
			UseRectTool(sx,sy,ex,ey, fPattern, true);
			break;
		case kRoundRectTool:
			ConstrainRect(fConstrainModifier,&sx,&sy,&ex,&ex);
			UseRoundRectTool(sx,sy,ex,ey, fTrackXRadius, fTrackYRadius, fPattern, false);
			break;
		case kFilledRoundRectTool:
			ConstrainRect(fConstrainModifier,&sx,&sy,&ex,&ex);
			UseRoundRectTool(sx,sy,ex,ey, fTrackXRadius, fTrackYRadius, fPattern, true);
			break;
		case kOvalTool:
			ConstrainRect(fConstrainModifier,&sx,&sy,&ex,&ex);
			UseOvalTool(sx,sy,ex,ey, fPattern, false);
			break;
		case kFilledOvalTool:
			ConstrainRect(fConstrainModifier,&sx,&sy,&ex,&ex);
			UseOvalTool(sx,sy,ex,ey, fPattern, true);
			break;
		case kArcTool:
			ConstrainRect(fConstrainModifier,&sx,&sy,&ex,&ex);
			UseArcTool(sx,sy,ex,ey, fTrackStartAngle, fTrackArcAngle, fPattern, false);
			break;
		case kFilledArcTool:
			ConstrainRect(fConstrainModifier,&sx,&sy,&ex,&ex);
			UseArcTool(sx,sy,ex,ey, fTrackStartAngle, fTrackArcAngle, fPattern, true);
			break;
		case kTriangleTool:
			ConstrainRect(fConstrainModifier,&sx,&sy,&ex,&ex);
			UseTriangleTool(sx,sy,ex,ey, fPattern, false);
			break;
		case kFilledTriangleTool:
			ConstrainRect(fConstrainModifier,&sx,&sy,&ex,&ex);
			UseTriangleTool(sx,sy,ex,ey, fPattern, true);
			break;
		case kBucketTool:
			UseFillTool(fCurX, fCurY);
			break;
		case kHotSpotTool:
			UseHotSpotTool(fCurX,fCurY);
			break;
		default:
			debugger("Unknown tool.");
	}
	
	// If a change was made in the workspace, need to copy that change
	// back into the compositing area.
	if( fTrackFinalUpdate.IsValid() ) {
		InvalidateComposite(fTrackFinalUpdate, false);
		if( dif_bits ) {
			fTrackFinalBits->Sync();
			InitialCompositing(fTrackTransientBits);
		}
	}
	
	// Report new invalid region to anyone watching.
	InvalidateComposite(fTrackTransientUpdate);
	
	fTrackFinalBits->PopState();
	fTrackFinalBits->Sync();
	
	if( dif_bits ) {
		// Draw final graphics in composite.
		FinalCompositing(fTrackTransientBits, fTrackTransientBits->Bitmap(), true);
		fTrackTransientBits->PopState();
		fTrackTransientBits->Sync();
	}
	
	fTrackFinalBits->Unlock();
	if( dif_bits ) fTrackTransientBits->Unlock();
};

// ------------------------------ Tools ------------------------------

void
TBitmapEditor::UsePencilTool(short pixelFlip, BPoint loc, BPoint last,
							 rgb_color firstColor)
{
	rgb_color pen_color;
	if( pixelFlip == 1 || pixelFlip == 3 ) {
		// draw in secondary color.
		pen_color = fTrackFinalBits->LowColor();
	} else {
		// draw in primary color.
		pen_color = fTrackFinalBits->HighColor();
	}
	fTrackFinalBits->SetHighColor(pen_color);
	
	if( pixelFlip == 2 || pixelFlip == 3 ) {
		// only draw on original pixel color.
		rgb_color currColor = fTrackFinalBits->GetPixel(loc);
		if (currColor == firstColor) {
			// 	pixels can be non-contiguous, set individual pixels not runs			
			fTrackFinalBits->SetPixel(BPoint(loc.x, loc.y), pen_color);
		}
	} else {
		// draw on all pixels.
		fTrackFinalBits->StrokeLine(last, loc);
	}

	fTrackFinalUpdate = ContainingRect(last, loc);
}

void
TBitmapEditor::UseEyeDropperTool(int32 x, int32 y)
{
	rgb_color col = fTrackFinalBits->GetPixel(BPoint(x,y));

	if( fConstrainModifier ) {
		// When constraint modifier is pressed, mix color at position
		// with the current color.
		rgb_color hitcol = col;
		rgb_color oldcol = fTrackFinalBits->HighColor();
		for( int mix=1; mix<=8; mix++ ) {
			rgb_color newcol;
			const int16 total = oldcol.alpha + hitcol.alpha;
			const uint16 alphaMix = (oldcol.alpha != 0)
								  ? (mix * (255*hitcol.alpha)/total)
								  : 255*8;
			newcol.red = ( (int32(oldcol.red)*(255*8-alphaMix)) + (int32(hitcol.red)*alphaMix) ) / (8*255);
			newcol.green = ( (int32(oldcol.green)*(255*8-alphaMix)) + (int32(hitcol.green)*alphaMix) ) / (8*255);
			newcol.blue = ( (int32(oldcol.blue)*(255*8-alphaMix)) + (int32(hitcol.blue)*alphaMix) ) / (8*255);
			newcol.alpha = ( (int32(oldcol.alpha)*(8-mix)) + (int32(hitcol.alpha)*mix) ) / 8;
			//printf("Try #%d: r=%lx, g=%lx, b=%lx\n",
			//		mix, (int32)newcol.red, (int32)newcol.green, (int32)newcol.blue);
			newcol = fWorkspaceBits->ConstrainColor(newcol);
			//printf("Index is %d, last was %d\n", (int)c, (int)i);
			if( newcol != oldcol || newcol == col ) {
				col = newcol;
				break;
			}
		}
	}
	
	NewColorSelection(col, (fTrackButtons&B_SECONDARY_MOUSE_BUTTON) ? false : true);
}

void
TBitmapEditor::UseHotSpotTool(int32 x, int32 y)
{
	NewHotSpot(x, y);
}

void
TBitmapEditor::UseEraser(int32 x,int32 y, int32 last_x, int32 last_y)
{
	rgb_color tmp = fTrackFinalBits->HighColor();
	fTrackFinalBits->SetHighColor(fTrackFinalBits->LowColor());
	fTrackFinalBits->SetLowColor(tmp);
	fTrackFinalBits->StrokeLine(BPoint(x,y),BPoint(last_x,last_y));		
	fTrackFinalUpdate = ContainingRect(BPoint(x,y), BPoint(last_x,last_y));
}

void	
TBitmapEditor::UseLineTool(int32 x,int32 y, int32 last_x, int32 last_y, pattern p)
{
	fTrackTransientBits->StrokeLine(BPoint(x,y),BPoint(last_x,last_y), p);
	fTrackTransientUpdate = ContainingRect(BPoint(x,y), BPoint(last_x,last_y));
}

void
TBitmapEditor::UseSelectionTool(BRect r, int32 lX, int32 lY, bool , bool firstPass)
{
	if (fSelectionState == kNoSelection || fSelectionState == kSelecting) {
		//
		//	draw selection rect
		//
		if( r.left > r.right ) {
			float t = r.left;
			r.left = r.right;
			r.right = t;
		}
		if( r.top > r.bottom ) {
			float t = r.top;
			r.top = r.bottom;
			r.bottom = t;
		}
		fSelectionState = kSelecting;
		MakeSelection(r);
		fTrackTransientUpdate = r;
	} else if (fSelectionState == kHaveSelection || fSelectionState == kMovingSelection) {
		
		if (!fSelectionRect.Contains(r.LeftTop()) && firstPass) {
			// single click outside selection rect
			DeSelect();
		} else {
			// move the selection as user drags
			fSelectionState = kMovingSelection;
			fTrackFinalUpdate = fSelectionRect;
			MoveSelection(r.right, r.bottom, lX, lY);
		}
		
	} else {
		TRESPASS();
		fSelectionState = kNoSelection;
	}
}

void
TBitmapEditor::UseSelectionTool(int32 sX, int32 sY, int32 eX, int32 eY,
	int32 lX, int32 lY,
	bool addToSelection, bool firstPass)
{
	UseSelectionTool(BRect(sX,sY,eX,eY),lX,lY,addToSelection, firstPass);
}

void
TBitmapEditor::UseRectTool(BRect drawRect, pattern p, bool fill)
{
	if( drawRect.left > drawRect.right ) {
		float t = drawRect.left;
		drawRect.left = drawRect.right;
		drawRect.right = t;
	}
	if( drawRect.top > drawRect.bottom ) {
		float t = drawRect.top;
		drawRect.top = drawRect.bottom;
		drawRect.bottom = t;
	}
	if (fill)
		fTrackTransientBits->FillRect(drawRect, p);
	else
		fTrackTransientBits->StrokeRect(drawRect, p);

	fTrackTransientUpdate = drawRect;
}

void
TBitmapEditor::UseRectTool(int32 sX, int32 sY, int32 eX, int32 eY, pattern p,
	bool fill)
{
	UseRectTool(BRect(sX,sY,eX,eY), p, fill);
}

void
TBitmapEditor::UseRoundRectTool(BRect drawRect, float xRadius, float yRadius, pattern p, bool fill)
{
	if( drawRect.left > drawRect.right ) {
		float t = drawRect.left;
		drawRect.left = drawRect.right;
		drawRect.right = t;
	}
	if( drawRect.top > drawRect.bottom ) {
		float t = drawRect.top;
		drawRect.top = drawRect.bottom;
		drawRect.bottom = t;
	}
	if (fill)
		fTrackTransientBits->FillRoundRect(drawRect, xRadius, yRadius, p);
	else
		fTrackTransientBits->StrokeRoundRect(drawRect, xRadius, yRadius, p);
		
	fTrackTransientUpdate = drawRect;
}

void
TBitmapEditor::UseRoundRectTool(int32 sX, int32 sY, int32 eX, int32 eY,
	float xRadius, float yRadius, pattern p, bool fill)
{
	BRect r(sX,sY,eX,eY);
	UseRoundRectTool(r, xRadius, yRadius, p, fill);
}

void
TBitmapEditor::UseOvalTool(BRect drawRect, pattern p, bool fill)
{
	if( drawRect.left > drawRect.right ) {
		float t = drawRect.left;
		drawRect.left = drawRect.right;
		drawRect.right = t;
	}
	if( drawRect.top > drawRect.bottom ) {
		float t = drawRect.top;
		drawRect.top = drawRect.bottom;
		drawRect.bottom = t;
	}
	if (fill)
		fTrackTransientBits->FillEllipse(drawRect, p);
	else
		fTrackTransientBits->StrokeEllipse(drawRect, p);
		
	fTrackTransientUpdate = drawRect;
}

void
TBitmapEditor::UseOvalTool(int32 sX, int32 sY, int32 eX, int32 eY, pattern p, bool fill)
{
	BRect r(sX,sY,eX,eY);
	UseOvalTool(r, p, fill);
}
		
void
TBitmapEditor::UseArcTool(BRect drawRect, float startAngle, float arcAngle, pattern p, bool fill)
{
	if( drawRect.left > drawRect.right ) {
		float t = drawRect.left;
		drawRect.left = drawRect.right;
		drawRect.right = t;
	}
	if( drawRect.top > drawRect.bottom ) {
		float t = drawRect.top;
		drawRect.top = drawRect.bottom;
		drawRect.bottom = t;
	}
	if (fill)
		fTrackTransientBits->FillArc(drawRect, startAngle, arcAngle, p);
	else
		fTrackTransientBits->StrokeArc(drawRect, startAngle, arcAngle, p);
		
	fTrackTransientUpdate = drawRect;
}

void
TBitmapEditor::UseArcTool(int32 sX, int32 sY, int32 eX, int32 eY,
	float startAngle, float arcAngle, pattern p, bool fill)
{
	BRect r(sX,sY,eX,eY);
	UseArcTool(r, startAngle, arcAngle, p, fill);
}
			
void
TBitmapEditor::UseTriangleTool(int32 sX, int32 sY, int32 eX, int32 eY, pattern p, bool fill)
{
	BPoint p1(sX,sY);
	BPoint p2(eX,eY);
	BPoint p3(2*eX,sY);
	
	if (fill)
		fTrackTransientBits->FillTriangle(p1,p2,p3,p);
	else
		fTrackTransientBits->StrokeTriangle(p1,p2,p3,p);
	
	BRect r(ContainingRect(p1, p2));
	if( p3.x < r.left ) r.left = p3.x;
	if( p3.x > r.right ) r.right = p3.x;
	if( p3.y < r.top ) r.top = p3.y;
	if( p3.y > r.bottom ) r.bottom = p3.y;
	
	fTrackTransientUpdate = r;
}

void
TBitmapEditor::UseFillTool(int32 x, int32 y)
{
	rgb_color penColor = fTrackFinalBits->HighColor();
	fTrackFinalUpdate = fTrackFinalBits->DoFill(BPoint(x,y), penColor);
}
