#include "FatBitsEditor.h"
#define DEBUG 1

#include <Clipboard.h>
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

const char *kDumpFileName = "Bitmaps.h";

extern uchar	cursor_marquee[], cursor_lasso[], cursor_pencil[],
				cursor_eraser[], cursor_dropper[], cursor_bucket[],
				cursor_shapes[];

static const type_code kSelectionPulse = 'spul';

class modMessageFilter : public BMessageFilter
{
public:
	modMessageFilter(TFatBitsEditor* editor)
		: BMessageFilter(B_MODIFIERS_CHANGED),
		  fEditor(editor)
	{
	}
	~modMessageFilter()
	{
	}

	filter_result Filter(BMessage *message, BHandler **/*target*/)
	{
		if( fEditor && message ) fEditor->UpdateMouseState(message);
		return B_DISPATCH_MESSAGE;
	}
	
private:
	TFatBitsEditor* fEditor;
};

TFatBitsEditor::TFatBitsEditor(	float width,float height,
	uchar *bits, rgb_color gridColor, short pixelsPerPixel)
	:	BView(BRect(0,0,(width*pixelsPerPixel),(height*pixelsPerPixel)),
			"fat bit editor",B_FOLLOW_NONE,
			B_WILL_DRAW | B_NAVIGABLE ),
		fDirty(false), fEditing(false)
{
	fBackgroundColor = B_TRANSPARENT_32_BIT;
	fShowTransparentPixels = false;
 	SetViewColor(B_TRANSPARENT_32_BIT);
	fCurrentTool = kPencilTool;
  	
	fCurrColorMap = ColorMap();	

	fGridColor = gridColor;
	fDrawGrid = false;
	fPixelsPerPixel = pixelsPerPixel;

	fWidth = width;
	fHeight = height;

	fCurrentPattern = fLastPattern = B_SOLID_HIGH;
	fXRadius = 8.0;
	fYRadius = 8.0;
	fStartAngle = 45.0;
	fArcAngle = 90.0;
	
	fTracking = false;
	
	fUpdateRect = BRect(0,0,width-1,height-1);
	fLastFBUpdateRect = BRect(0,0,0,0);
	
	fWorkspaceBits = new TBitsContainer(BRect(0,0,width-1, height-1), bits,
		B_COLOR_8_BIT, true);
	fActualBits = new TBitsContainer(BRect(0,0,width-1, height-1), bits,
		B_COLOR_8_BIT, true);
	fUndoBits = new TBitsContainer(BRect(0,0,width-1,height-1), NULL,
		B_COLOR_8_BIT, true);
	fHiliteBits = new TBitsContainer(BRect(0,0,width-1,height-1), NULL,
		B_COLOR_8_BIT, true);
	
	fLastPenColorIndex = fPenColorIndex = IndexForColor(255,10,10);	
	fLastSecondaryColorIndex = fSecondaryColorIndex = B_TRANSPARENT_8_BIT;
	SetPenColor();
	SetSecondaryColor();
	
	fSelectionRunner = 0;
	fModifierFilter = 0;
	
	fSelectionState = kNoSelection;
	fSelectionRect = BRect(0,0,0,0);
	fSelectionBits = new TBitsContainer(BRect(0,0,width-1,height-1), NULL,
		B_COLOR_8_BIT, true);
	//
	//
	//	this one will have to be constrained to some logical limit
	//	and in coordination with the onscreen display
	//
	fFatBits = new TBitsContainer(BRect(0,0,(32 * width)-1, (32 * height)-1), NULL,
		B_COLOR_8_BIT, true);
	
	fActualSizeView = NULL;
	fHiliteView = NULL;
	
	fCanUndo = false;
}
  
TFatBitsEditor::~TFatBitsEditor()
{
	DeSelect();
	
	delete fWorkspaceBits;
	delete fActualBits;
	delete fFatBits;
	delete fUndoBits;
	delete fHiliteBits;

	delete fSelectionBits;	
}

void
TFatBitsEditor::AttachedToWindow()
{
	BView::AttachedToWindow();
	AttachObjects();
	
// 	Invalidate(BRect(0,0,fWidth*fPixelsPerPixel,fHeight*fPixelsPerPixel));
}

void
TFatBitsEditor::DetachedFromWindow()
{
	DetachObjects();
	BView::DetachedFromWindow();
}

void
TFatBitsEditor::AttachObjects()
{
	DetachObjects();
	fSelectionRunner = new BMessageRunner(this, new BMessage(kSelectionPulse),
										  100000);
	fModifierFilter = new modMessageFilter(this);
	Window()->AddCommonFilter(fModifierFilter);
}

void
TFatBitsEditor::DetachObjects()
{
	if( fModifierFilter ) {
		Window()->RemoveCommonFilter(fModifierFilter);
		delete fModifierFilter;
		fModifierFilter = 0;
	}
	delete fSelectionRunner;
	fSelectionRunner = 0;
}

void
TFatBitsEditor::GetPreferredSize(float *width, float *height)
{
	if( width ) *width = fWidth*fPixelsPerPixel + 1;
	if( height ) *height = fHeight*fPixelsPerPixel + 1;
}

void
TFatBitsEditor::BitsChanged()
{
	// we don't care.
}

BRect
TFatBitsEditor::ActualSizeRect()
{
	BRect srcRect(0,0,fWidth - 1,fHeight - 1);
	return srcRect;
}

BRect
TFatBitsEditor::FatBitsRect()
{
	BRect destRect(0,0,fWidth * fPixelsPerPixel - 1,fHeight * fPixelsPerPixel - 1);
	return destRect;
}

BRect
TFatBitsEditor::ActualBits2FatBitsRect(BRect actualRect)
{
	BRect fbRect;
	fbRect.left = actualRect.left * fPixelsPerPixel;
	fbRect.top = actualRect.top * fPixelsPerPixel;
	fbRect.right = actualRect.right * fPixelsPerPixel + (fPixelsPerPixel - 1);
	fbRect.bottom = actualRect.bottom * fPixelsPerPixel + (fPixelsPerPixel - 1);

	return fbRect;
}

BRect
TFatBitsEditor::FatBits2ActualRect(BRect fbRect)
{
	int32 temp;
	BRect actualRect;
	temp = (int32)(fbRect.left / fPixelsPerPixel);
	actualRect.left = temp;
	temp = (int32)(fbRect.top / fPixelsPerPixel);
	actualRect.top = temp;
	temp = (int32)((fbRect.right - (fPixelsPerPixel - 1)) / fPixelsPerPixel);
	actualRect.right = temp;
	temp = (int32)((fbRect.bottom - (fPixelsPerPixel - 1)) / fPixelsPerPixel);
	actualRect.bottom = temp;

	return actualRect;
}

BRect
TFatBitsEditor::ValidFatBitsRect(BRect r)
{
	int32 temp;
	BRect retRect = r;
	
	temp = (int32)(r.left / fPixelsPerPixel);
	temp *= fPixelsPerPixel;
	retRect.left = temp;
	temp = (int32)(r.top / fPixelsPerPixel);
	temp *= fPixelsPerPixel;
	retRect.top = temp;
	
	temp = (int32)(r.right / fPixelsPerPixel);
	temp *= fPixelsPerPixel;
	temp += (fPixelsPerPixel-1);	
	if (temp > (fWidth*fPixelsPerPixel-1))
		temp = (int32)(fWidth*fPixelsPerPixel-1);		
	retRect.right = temp;
	
	temp = (int32)(r.bottom / fPixelsPerPixel);
	temp *= fPixelsPerPixel;
	temp += (fPixelsPerPixel-1);
	if (temp > (fHeight*fPixelsPerPixel-1))
		temp = (int32)(fHeight*fPixelsPerPixel-1);
	retRect.bottom = temp;

	return retRect;
}

rgb_color
TFatBitsEditor::ColorForIndex(uint8 index)
{
	if( index == B_TRANSPARENT_8_BIT ) {
		return B_TRANSPARENT_COLOR;
	}
	return fCurrColorMap->color_list[index];
}

void 
TFatBitsEditor::WindowActivated(bool active)
{
	BView::WindowActivated(active);
	if (IsFocus())
		Invalidate(Bounds());
}

void
TFatBitsEditor::MakeFocus(bool focusState)
{
	BView::MakeFocus(focusState);
	Invalidate(Bounds());
}

//	most of this is pretty screwy and needs to be reevaluated for efficiency,
//	too much extra drawing is taking place
void
TFatBitsEditor::Draw(BRect fatBitsRect)
{
	BRect fbRect = ValidFatBitsRect(fatBitsRect);
	BRect actualRect = FatBits2ActualRect(fbRect);
	
	//	build the icon image, draws to actualBits with conversion of
	//	transparent pixels in workspace.
	CompositeBits(actualRect,fbRect);

	DrawActualSizeIcon(actualRect);	
	DrawHiliteIcon(actualRect);

	//	draw the fatbits version of it		
	if (Window() && LockLooper()) {
		if (fFatBits->Lock()) {
			fFatBits->DrawBitmap(fActualBits->Bitmap(),actualRect,fbRect);
			DrawGrid(Bounds());	// draws to fatbits
			fFatBits->Unlock();
		}

		fbRect.right += 1;
		fbRect.bottom += 1;
		DrawBitmap(fFatBits->Bitmap(), fbRect, fbRect);
		UnlockLooper();
	}
	
	// add the focus mark
	if (Window()->IsActive() && IsFocus())
		SetHighColor(keyboard_navigation_color());
	else
		SetHighColor(fGridColor);

	StrokeRect(Bounds());
}

extern pattern	marquee_pat[];

//	build the image of the icon from the edited bits, selection and marquee
void
TFatBitsEditor::CompositeBits(BRect actualRect, BRect)
{
	if (fActualBits->Lock()) {
		// draw the edited (workspace) to the actual, taking care of
		// any transparency.
		fActualBits->SetHighColor(fBackgroundColor);
		fActualBits->FillRect(actualRect);
		fActualBits->SetDrawingMode(B_OP_ALPHA);
		fActualBits->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		fActualBits->DrawBitmap(fWorkspaceBits->Bitmap(), actualRect, actualRect);
		
		// no more transparency drawing.
		fActualBits->SetDrawingMode(B_OP_COPY);
		
		//	draw the selection bits to the actual
		if ((fSelectionState == kHaveSelection) || (fSelectionState == kMovingSelection)) {
			OverlaySelection(fActualBits, fSelectionRect, fBackgroundColor);
		}

		//	if applicable, draw the marquee around the selection
		if ( Window()->IsActive() && fSelectionState != kNoSelection) {
			fActualBits->SetHighColor(0,0,0,0);
			fActualBits->SetLowColor(255, 255, 255);
			fActualBits->SetPenSize(1);
			fActualBits->SetDrawingMode(B_OP_COPY);

			fActualBits->StrokeRect(fSelectionRect, *marquee_pat);		
		}
			
		fActualBits->Sync();
		fActualBits->Unlock();
	} else
		printf("Draw - can't lock the actual bits buffer\n");
}

void
TFatBitsEditor::DrawSelectionBits(BRect)
{
	if ((fSelectionState == kHaveSelection) || (fSelectionState == kMovingSelection)) {
		BRect srcRect(fSelectionRect);
		srcRect.OffsetTo(0,0);
		fActualBits->DrawBitmap(fSelectionBits->Bitmap(),srcRect,fSelectionRect);				
	}
}

void
TFatBitsEditor::DrawMarquee(BRect)
{
	if (!Window()->IsActive())
		return;
		
	// ?? if the current tool is not the selection, don't show the marquee
	if (/*(CurrentTool(false) == kSelectionTool) &&*/ fSelectionState != kNoSelection) {
		rgb_color oldColor = fActualBits->HighColor();

		if (fSelectionRect.right > fWidth)
			fSelectionRect.right = fWidth;
		if (fSelectionRect.bottom > fHeight)
			fSelectionRect.bottom = fHeight;	
			
		fActualBits->SetHighColor(0,0,0,0);
		fActualBits->SetLowColor(255, 255, 255);
		fActualBits->SetPenSize(1);
		fActualBits->SetDrawingMode(B_OP_COPY);
		fActualBits->StrokeRect(fSelectionRect, *marquee_pat);		

		fActualBits->SetHighColor(oldColor);
		
		fActualBits->Sync();
	}
}


void
TFatBitsEditor::DrawActualSizeIcon(BRect actualRect)
{
	if( !fActualSizeView || !fActualSizeView->LockLooper() ) return;
	
	BRect destRect = actualRect;
	destRect.OffsetBy(fActualSizeFrame.left, fActualSizeFrame.top);
	fActualSizeView->PushState();
	fActualSizeView->SetDrawingMode(B_OP_COPY);
	fActualSizeView->DrawBitmap(fActualBits->Bitmap(),actualRect, destRect);
	fActualSizeView->PopState();
	fActualSizeView->UnlockLooper();
}

void
TFatBitsEditor::DrawHiliteIcon(BRect actualRect)
{
	if( !fHiliteView || !fHiliteView->Window() ) return;
	
	if( !fHiliteBits->Lock() ) return;
	
	// Draw the hilite bitmap -- copy actual bitmap into it,
	// then darken.
	fHiliteBits->PushState();
	fHiliteBits->SetDrawingMode(B_OP_COPY);
	fHiliteBits->DrawBitmap(fActualBits->Bitmap(), actualRect, actualRect);
	rgb_color black_mix = { 0, 0, 0, 255 - (uint8)(255*2/3) };
	fHiliteBits->SetHighColor(black_mix);
	fHiliteBits->SetDrawingMode(B_OP_ALPHA);
	fHiliteBits->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
	fHiliteBits->FillRect(actualRect);
	fHiliteBits->PopState();
	
	fHiliteBits->Sync();
	
	if (fHiliteView->LockLooper()) {
		BRect destRect(actualRect);
		destRect.OffsetBy(fHiliteFrame.left, fHiliteFrame.top);
		fHiliteView->PushState();
		fHiliteView->SetDrawingMode(B_OP_COPY);
		fHiliteView->DrawBitmap(fHiliteBits->Bitmap(), actualRect, destRect);
		fHiliteView->PopState();
		fHiliteView->UnlockLooper();
	} else
		printf("can't lock window\n");
	
	fHiliteBits->Unlock();
}

void
TFatBitsEditor::DrawGrid(BRect)
{
	float w = fWidth * fPixelsPerPixel + 1;
	float h = fHeight * fPixelsPerPixel + 1;

	if (fDrawGrid && (fPixelsPerPixel != 1)) {
		fFatBits->BeginLineArray(fWidth + fHeight + 2);
	
		int32 lineIndex=0;
		float temp;
		
		for (lineIndex = 0; lineIndex <= fWidth; lineIndex++) {
			temp = lineIndex * fPixelsPerPixel;
			fFatBits->AddLine(	BPoint(temp, 0), BPoint(temp, w), fGridColor);
		}
		
		for (lineIndex = 0; lineIndex <= fHeight; lineIndex++) {
			temp = lineIndex * fPixelsPerPixel;
			fFatBits->AddLine(	BPoint(0,temp), BPoint(h,temp), fGridColor);
		}

		fFatBits->EndLineArray();
	}

	fFatBits->Sync();
}

void
TFatBitsEditor::DrawBorder()
{
	fFatBits->StrokeRect(Bounds());
}

void 
TFatBitsEditor::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case kClear:
			DoClear();
			break;
		case B_UNDO:
			DoUndo();
			break;
		case B_CUT:
			DoCut();
			break;
		case B_COPY:
			DoCopy();
			break;
		case B_PASTE:
			DoPaste();
			break;
		case B_SELECT_ALL:
			DoSelectAll();
			break;
		case msg_deselect:
			DeSelect();
			break;
		case kSelectionPulse:
			Pulse();
			break;

		case 'dpsl':
			DumpSelection();
			break;

		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
			RefsReceived(msg);
			break;

		default:
			BView::MessageReceived(msg);
			break;
	}
}

void 
TFatBitsEditor::Pulse(void)
{
	if (fSelectionState != kNoSelection
		&& Window()->IsActive()) {

		uchar* pat = (uchar *)marquee_pat;
		uchar byte = pat[7];
		for (int32 i = 6; i >= 0; i--)
			pat[i + 1] = pat[i];
		pat[0] = byte;

		Draw(ActualBits2FatBitsRect(fSelectionRect));
	}
}

void
TFatBitsEditor::SetActualSizeView(BView *v, BRect frame)
{
	fActualSizeView = v;
	fActualSizeFrame = frame;
}

BView*
TFatBitsEditor::ActualSizeView()
{
	return fActualSizeView;
}

void
TFatBitsEditor::SetHiliteView(BView* v, BRect frame)
{
	fHiliteView = v;
	fHiliteFrame = frame;
}

BView*
TFatBitsEditor::HiliteView()
{
	return fHiliteView;
}

void
TFatBitsEditor::KeyDown(const char *key, int32 numBytes)
{
	UpdateMouseState(Window()->CurrentMessage());
	
	switch (key[0]) {
		case B_BACKSPACE:
			DoClear();
			break;
		
		case B_LEFT_ARROW:
			MoveSelection(-1,0);
			break;
		case B_RIGHT_ARROW:
			MoveSelection(1,0);
			break;
		case B_UP_ARROW:
			MoveSelection(0,-1);
			break;
		case B_DOWN_ARROW:
			MoveSelection(0,1);
			break;
		
		default:
			BView::KeyDown(key,numBytes);
			break;
	}
	
	Draw(ActualBits2FatBitsRect(fUpdateRect));
}

void
TFatBitsEditor::UpdateCursor(BPoint where)
{
	if (Window()->IsActive() && Bounds().Contains(where)) {
		uchar	*cursor;
		
		switch(CurrentTool(fSwapModifier)) {
			case kSelectionTool:
				cursor = cursor_marquee;
				break;
			case kEraserTool:
				cursor = cursor_eraser;
				break;
			case kPencilTool:
				cursor = cursor_pencil;
				break;
			case kEyeDropperTool:
				cursor = cursor_dropper;
				break;
			case kBucketTool:
				cursor = cursor_bucket;
				break;
			default:
				cursor = cursor_shapes;
				break;
		}
		be_app->SetCursor(cursor);
		
	} else
		be_app->SetCursor(B_HAND_CURSOR);
}

BPoint
TFatBitsEditor::CurrentMouseLoc()
{
	return fCurrentLoc;
}

uint8
TFatBitsEditor::CurrentColorAtLoc()
{
	return fColorIndexAtLoc;
}

bool
TFatBitsEditor::UpdateMouseState(const BMessage* from, bool forceCursor)
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
	
	if( changed || forceCursor ) UpdateCursor(fMousePosition);

	return changed;
}
	
void
TFatBitsEditor::MouseDown( BPoint point)
{
	BView::MouseDown(point);

	int32 xPosition = (int32)(point.x/fPixelsPerPixel);
	int32 yPosition = (int32)(point.y/fPixelsPerPixel);

	fMousePosition = point;
	UpdateMouseState(Window()->CurrentMessage(), true);
	
	//if( !IsFocus() ) MakeFocus();
	
	if (fSelectionState != kNoSelection
			&& CurrentTool(fSwapModifier) != kSelectionTool) {
		DeSelect();
	}

	switch(CurrentTool(fSwapModifier)) {
		case kBucketTool:
			BeginEdit();
			UseFillTool(xPosition,yPosition);
			Draw(ActualBits2FatBitsRect(fUpdateRect));
			fLastFBUpdateRect = fUpdateRect;
			EndEdit();
			break;
		case kEyeDropperTool:
			// This does not change the bitmap.
			UseEyeDropperTool(xPosition,yPosition);
			break;
		case kSelectionTool:
			// Start editing if creating a new selection; otherwise,
			// continue editing of last selection.
			StartToolTracking(point);
			break;
		default:
			BeginEdit();
			StartToolTracking(point);
	}
}

void
TFatBitsEditor::MouseMoved(BPoint where, uint32 code, const BMessage *a_message)
{
	fMousePosition = where;
	UpdateMouseState(Window()->CurrentMessage());
	
	switch (code) {
		case B_EXITED_VIEW:
			if ( (Window()->IsActive()) && (a_message == NULL) )
				be_app->SetCursor(B_HAND_CURSOR);
			break;
			
		default:
			TrackTool(where);
			UpdateCursor(where);
			BView::MouseMoved(where, code, a_message);
			break;
	}
}

void
TFatBitsEditor::MouseUp(BPoint where)
{
	UpdateMouseState(Window()->CurrentMessage());
	fMousePosition = where;
	TrackTool(where);
	EndToolTracking();
	UpdateCursor(where);
	BView::MouseUp(where);
}

void
TFatBitsEditor::StartToolTracking(BPoint point)
{
	fTracking = true;
	fTrackTool = -1;
	fLastX = (int32)(point.x/fPixelsPerPixel);
	fLastY = (int32)(point.y/fPixelsPerPixel);
	fStartX = fLastX;
	fStartY = fLastY;
	fPattern = PenPattern();
	Angles(&fTrackStartAngle, &fTrackArcAngle);
	Radius(&fTrackXRadius, &fTrackYRadius);
	fTrackFirstPass = true;
	fTrackMoved = false;
	
	// 	get the color index at the first click location
	//	used later on for pixel flipping with the pencil tool
	fIndexAtClick = IndexAtPoint(BPoint(fLastX,fLastY));
	
	if (fIndexAtClick == PenColor()) {
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
	
	SetMouseEventMask(B_POINTER_EVENTS|B_KEYBOARD_EVENTS,
					  B_LOCK_WINDOW_FOCUS|B_NO_POINTER_HISTORY);
	TrackTool(point);
}

void
TFatBitsEditor::TrackTool(BPoint point)
{
	if( !fTracking ) return;
	
	fTrackTool = CurrentTool(fSwapModifier);
	
	//
	//	convert the point to the actual size
	//
	int32 xPosition = (int32)(point.x/fPixelsPerPixel);
	int32 yPosition = (int32)(point.y/fPixelsPerPixel);
	
	const bool moved = ( xPosition != fLastX || yPosition != fLastY );
	if( moved ) fTrackMoved = true;
	
	// constrain the bounds
	if (xPosition < 0)
		xPosition = 0;
	else if (xPosition >= fWidth)
		xPosition = (int32)(fWidth-1);
	if (yPosition < 0)
		yPosition = 0;
	else if (yPosition >= fHeight)
		yPosition = (int32)(fHeight-1);
	
	if (moved
		|| ((fTrackTool == kPencilTool) && fTrackFirstPass)
		|| ((fTrackTool == kEraserTool) && fTrackFirstPass)
		|| (fTrackTool == kEyeDropperTool)
		|| (fTrackTool == kSelectionTool && fTrackFirstPass) ) {
		
		switch(fTrackTool) {
			case kPencilTool:
				UsePencilTool(fPixelFlip, BPoint(xPosition,yPosition),
					BPoint(fLastX,fLastY), fIndexAtClick);					
				break;
			case kEyeDropperTool:
				//UseEyeDropperTool(xPosition,yPosition);
				break;
			case kEraserTool:
				UseEraser(xPosition,yPosition,fLastX,fLastY);
				break;
			case kLineTool:
				DrawLine(fStartX,fStartY,xPosition,yPosition, fPattern);
				break;
			case kSelectionTool:
				ConstrainRect(fConstrainModifier,&fStartX,&fStartY,&xPosition,&yPosition);
				UseSelectionTool(fStartX,fStartY,xPosition,yPosition,
								 fLastX,fLastY, fSwapModifier, fTrackFirstPass);
				break;
			case kLassoTool:
				printf("irregualar selection not implemented\n");
				break;
			case kRectTool:
				ConstrainRect(fConstrainModifier,&fStartX,&fStartY,&xPosition,&yPosition);
				DrawRect(fStartX,fStartY,xPosition,yPosition, fPattern, false);
				break;
			case kFilledRectTool:
				ConstrainRect(fConstrainModifier,&fStartX,&fStartY,&xPosition,&yPosition);
				DrawRect(fStartX,fStartY,xPosition,yPosition, fPattern, true);
				break;
			case kRoundRectTool:
				ConstrainRect(fConstrainModifier,&fStartX,&fStartY,&xPosition,&yPosition);
				DrawRoundRect(fStartX,fStartY,xPosition,yPosition,
							  fTrackXRadius, fTrackYRadius, fPattern, false);
				break;
			case kFilledRoundRectTool:
				ConstrainRect(fConstrainModifier,&fStartX,&fStartY,&xPosition,&yPosition);
				DrawRoundRect(fStartX,fStartY,xPosition,yPosition,
							  fTrackXRadius, fTrackYRadius, fPattern, true);
				break;
			case kOvalTool:
				ConstrainRect(fConstrainModifier,&fStartX,&fStartY,&xPosition,&yPosition);
				DrawOval(fStartX,fStartY,xPosition,yPosition, fPattern, false);
				break;
			case kFilledOvalTool:
				ConstrainRect(fConstrainModifier,&fStartX,&fStartY,&xPosition,&yPosition);
				DrawOval(fStartX,fStartY,xPosition,yPosition, fPattern, true);
				break;
			case kArcTool:
				DrawArc(fStartX,fStartY,xPosition,yPosition,
						fTrackStartAngle, fTrackArcAngle, fPattern, false);
				break;
			case kFilledArcTool:
				DrawArc(fStartX,fStartY,xPosition,yPosition,
						fTrackStartAngle, fTrackArcAngle, fPattern, true);
				break;
			case kTriangleTool:
				DrawTriangle(fStartX, fStartY, xPosition, yPosition, fPattern, false);
				break;
			case kFilledTriangleTool:
				DrawTriangle(fStartX, fStartY, xPosition, yPosition, fPattern, true);
				break;
		}

		BRect invalRect = IntersectRects(fLastFBUpdateRect,fUpdateRect);
		Draw(ActualBits2FatBitsRect(invalRect));
	
		fLastFBUpdateRect = fUpdateRect;
				
		fLastX = xPosition;
		fLastY = yPosition;
		fTrackFirstPass = false;
	}
};

void
TFatBitsEditor::EndToolTracking()
{
	if( !fTracking ) return;
	
	fTracking = false;
	
	// invalidate the entire bitmap in an undo
	if (fTrackTool == kPencilTool || fTrackTool == kEraserTool)
		fUpdateRect = ActualSizeRect();

	if (fSelectionState == kSelecting) {
		// 	just finished selecting a selection rect,
		//	copy to the selection buffer
		BeginCopySelection(fSelectionRect);
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
	
	if( fTrackTool != kSelectionTool ) {
		// For all but the selection tool, releasing the mouse button
		// is the end if an edit operation.
		EndEdit();
	}
}

void
TFatBitsEditor::SetBackgroundColor(rgb_color newBGColor)
{
	if (fBackgroundColor.red == newBGColor.red
		&& fBackgroundColor.green == newBGColor.green
		&& fBackgroundColor.blue == newBGColor.blue)
		return;
	
	fBackgroundColor = newBGColor;
	Draw(Bounds());
}

rgb_color
TFatBitsEditor::BackgroundColor() const
{
	return fBackgroundColor;
}

void
TFatBitsEditor::SetTransparentBits()
{
	if (!fWorkspaceBits->Lock()) {
		TRESPASS();
		printf("bail\n");
		return;
	}

	fShowTransparentPixels = !fShowTransparentPixels;
	if (fShowTransparentPixels)
		fWorkspaceBits->SetHighColor(fBackgroundColor);
	else
		fWorkspaceBits->SetHighColor(B_TRANSPARENT_32_BIT);

	bool needToUpdate=false;
	BPoint loc;	
	for (int32 row=0 ; row<fHeight ; row++) {
		loc.y = row;
		for (int32 col=0 ; col<fWidth ; col++) {
			loc.x = col;
			uint8 index = IndexAtPoint(loc);
			
			if (!fShowTransparentPixels) {
				rgb_color currentColor = ColorForIndex(index);
				if (	currentColor.red == fBackgroundColor.red
					&& 	currentColor.green == fBackgroundColor.green
					&& 	currentColor.blue == fBackgroundColor.blue) {
						fWorkspaceBits->StrokeLine(loc, loc);
						needToUpdate = true;
				}
			} else {
				if (index == B_TRANSPARENT_8_BIT) {
					fWorkspaceBits->StrokeLine(loc, loc);
					needToUpdate = true;
				}
			}
			
		}
	}

	fWorkspaceBits->Sync();
	fWorkspaceBits->Unlock();
	
	if (needToUpdate)
		Draw(Bounds());
}

void
TFatBitsEditor::SetPixel(int32 x,int32 y, pattern p)
{
	if (!fWorkspaceBits->Lock()) {
		TRESPASS();
		return;
	}
	fSelectionState = kNoSelection;
	
	fWorkspaceBits->StrokeLine(BPoint(x,y),BPoint(x,y), p);		

	fUpdateRect = MakeValidRect(BRect(x,y,x,y));

	fWorkspaceBits->Sync();
	fWorkspaceBits->Unlock();
	fDirty = true;
}

void
TFatBitsEditor::SetPixel(int32 x,int32 y, int32 last_x, int32 last_y, pattern p)
{
	if (!fWorkspaceBits->Lock()) {
		TRESPASS();
		return;
	}
	fSelectionState = kNoSelection;
	
	fWorkspaceBits->StrokeLine(BPoint(x,y),BPoint(last_x,last_y), p);		

	fUpdateRect = MakeValidRect(BRect(x,y,last_x,last_y));

	fWorkspaceBits->Sync();
	fWorkspaceBits->Unlock();
	fDirty = true;
}

void
TFatBitsEditor::TestAndSetPixel(BPoint pt, uint8 targetIndex, uint8 penIndex,
	uchar* bits, uint32 bytesPerRow)
{
	uchar* tempBits = bits;
	uint32 numBytes = (int32)(bytesPerRow * pt.y + pt.x);
	tempBits += numBytes;
	uint8 currIndex = *tempBits;
	if (currIndex == targetIndex)				
		DoRecursiveFill( pt, targetIndex, penIndex, bits, bytesPerRow);
}

// how to get the largest surrounding rect to invalidate ?
void
TFatBitsEditor::DoRecursiveFill(BPoint pt, uint8 targetIndex, uint8 penIndex,
	uchar *bits, uint32 bytesPerRow)
{
	BPoint nextPt(pt.x,pt.y);
	uchar* tempBits = bits;
	uint32 numBytes;
		
	//	change this pixel
	numBytes = (int32)(bytesPerRow * pt.y + pt.x);
	tempBits += numBytes;
	*tempBits = penIndex;
	//
	//	left
	nextPt.x = pt.x - 1;
	if (nextPt.x >= 0)
		TestAndSetPixel(nextPt,targetIndex, penIndex, bits, bytesPerRow);

	//	right
	nextPt.x = pt.x + 1;
	if (nextPt.x < fWidth)
		TestAndSetPixel(nextPt,targetIndex, penIndex, bits, bytesPerRow);

	//	top 
	nextPt.x = pt.x; nextPt.y = pt.y - 1;
	if (nextPt.y >= 0)
		TestAndSetPixel(nextPt,targetIndex, penIndex, bits, bytesPerRow);

	//	bottom
	nextPt.y = pt.y + 1;
	if (nextPt.y < fHeight)
		TestAndSetPixel(nextPt,targetIndex, penIndex, bits, bytesPerRow);
}

void
TFatBitsEditor::UseFillTool(int32 x, int32 y)
{
	Undo();
	if (fWorkspaceBits->Lock()) {
		fSelectionState = kNoSelection;
		uint8 penColor = PenColor();
		rgb_color lastColor = fWorkspaceBits->HighColor();
		uchar* tempBits;
		uchar* bits = fWorkspaceBits->Bits();
		uint32 bytesPerRow = fWorkspaceBits->Bitmap()->BytesPerRow();
		uint8 targetIndex;		
		uint32 numBytes = bytesPerRow * y + x;

		tempBits = bits;
		tempBits += numBytes;
		targetIndex = *tempBits;

		if (penColor != targetIndex) {
			
			DoRecursiveFill(BPoint(x,y), targetIndex, penColor, bits, bytesPerRow);
		
			fUpdateRect = ActualSizeRect();		// ! not optimized
	
			fWorkspaceBits->SetHighColor(lastColor);
			fWorkspaceBits->Sync();
		}
		fWorkspaceBits->Unlock();
		
		fDirty = true;
	}
}

void
TFatBitsEditor::UseEyeDropperTool(int32 x, int32 y)
{
	if (fWorkspaceBits->Lock()) {
		uint8 i = IndexAtPosition(BPoint(x,y),fWorkspaceBits->Bits(),
			fWorkspaceBits->Bitmap()->BytesPerRow());

		if( fConstrainModifier ) {
			// When constraint modifier is pressed, mix color at position
			// with the current color.
			rgb_color hitcol = ColorForIndex(i);
			uint8 old = PenColor();
			rgb_color oldcol = ColorForIndex(old);
			if( hitcol.alpha == 255 && oldcol.alpha == 255 ) {
				// Only mix if both are non alpha -- otherwise, just
				// use the color that was picked.
				for( int mix=1; mix<=8; mix++ ) {
					rgb_color newcol;
					newcol.red = ( (int32(oldcol.red)*(8-mix)) + (int32(hitcol.red)*mix) ) / 8;
					newcol.green = ( (int32(oldcol.green)*(8-mix)) + (int32(hitcol.green)*mix) ) / 8;
					newcol.blue = ( (int32(oldcol.blue)*(8-mix)) + (int32(hitcol.blue)*mix) ) / 8;
					newcol.alpha = 255;
					//printf("Try #%d: r=%lx, g=%lx, b=%lx\n",
					//		mix, (int32)newcol.red, (int32)newcol.green, (int32)newcol.blue);
					uint8 c = IndexForColor(newcol);
					//printf("Index is %d, last was %d\n", (int)c, (int)i);
					if( c != old || c == i ) {
						i = c;
						break;
					}
				}
			}
		}
		
		NewColorSelection(i);
		
		// TTTT watch for transparent 
		fWorkspaceBits->Unlock();
	}
}

void
TFatBitsEditor::NewColorSelection(uint8)
{
	// override this virtual for the editor to do something with the color selected
	// TTTT watch for transparent 
}

void
TFatBitsEditor::UseEraser(int32 x,int32 y, int32 last_x, int32 last_y)
{
	if (fWorkspaceBits->Lock()) {
		fSelectionState = kNoSelection;
		rgb_color old = fWorkspaceBits->HighColor();
	
		fWorkspaceBits->SetHighColor(ColorForIndex(SecondaryColor()));
				
		fWorkspaceBits->StrokeLine(BPoint(x,y),BPoint(last_x,last_y));		
		
		fUpdateRect = MakeValidRect(BRect(x,y,last_x,last_y));
	
		fWorkspaceBits->Sync();
		fWorkspaceBits->SetHighColor(old);
		fWorkspaceBits->Unlock();
		fDirty = true;
	}
}

void	
TFatBitsEditor::DrawLine(int32 x,int32 y, int32 last_x, int32 last_y, pattern p)
{
	Undo();		// undo the bits so that we can track changes to the image
	if (fWorkspaceBits->Lock()) {
		fSelectionState = kNoSelection;
				
		fWorkspaceBits->StrokeLine(BPoint(x,y),BPoint(last_x,last_y), p);
		
		fUpdateRect = MakeValidRect(BRect(x,y,last_x,last_y));
		
		fWorkspaceBits->Sync();
		fWorkspaceBits->Unlock();
		fDirty = true;
	}
}

void
TFatBitsEditor::BeginEdit(bool undoable)
{
	ASSERT(fEditing == false);
	
	if (fUndoBits->Lock()) {
		fUndoBits->Bitmap()->SetBits(fWorkspaceBits->Bits(), fWorkspaceBits->BitsLength(),
			0, B_COLOR_8_BIT);

		if (HaveSelection()) {	
			BRect srcRect(fSelectionRect);
			srcRect.OffsetTo(0,0);	
			fUndoBits->DrawBitmap(fSelectionBits->Bitmap(),srcRect,fSelectionRect);
		}

		fUndoBits->Sync();
		fUndoBits->Unlock();
		fCanUndo = undoable;
		fEditing = true;
	}
}

bool
TFatBitsEditor::Editing() const
{
	return fEditing;
}

void
TFatBitsEditor::EndEdit(bool cancel)
{
	ASSERT(fEditing == true);
	fEditing = false;
	if( !cancel ) BitsChanged();		// report that something has changed.
}

// replaces last change with original source, use DoUndo to actually force a draw
void
TFatBitsEditor::Undo()
{
	// First make sure any existing selection is removed.
	DeSelect();
	
	if (fWorkspaceBits->Lock()) {
		fWorkspaceBits->DrawBitmap(fUndoBits->Bitmap(), fUpdateRect, fUpdateRect);

		fWorkspaceBits->Sync();
		fWorkspaceBits->Unlock();
	}
}

// called from external to this object
//	replaces last change from undo buffer, adds back any last selection
void
TFatBitsEditor::DoUndo()
{
	Undo();
//	if (fSelectionState == kHaveSelection) {  	// retain last selection rect
//		fSelectionRect = fLastSelectionRect;	// need to coordinate with actual bits in selection
//		CopySelection(fSelectionRect);
//	}

	Invalidate(ActualBits2FatBitsRect(fUpdateRect));
	
	fCanUndo = false;
}

bool
TFatBitsEditor::CanUndo()
{
	return fCanUndo;
}

void
TFatBitsEditor::AddCopyToClipboard()
{
	if (fSelectionBits->Lock()) {
		be_clipboard->Lock();
		be_clipboard->Clear();
		
		BMessage *message = be_clipboard->Data();
		if (!message) {
			printf("no clip msg\n");
			return;
		}
		
		BMessage *embeddedBitmap = new BMessage();
		BBitmap* bits = fSelectionBits->Bitmap();
		bits->Archive(embeddedBitmap,false);	// fucking added internal view
		status_t err = message->AddMessage(kBitmapMimeType, embeddedBitmap);
		ASSERT(err == B_OK);
		err = message->AddRect("rect", fSelectionRect);
		ASSERT(err == B_OK);

		be_clipboard->Commit();
		be_clipboard->Unlock();
				
		fSelectionBits->Unlock();

	} else
		printf("can't lock the selection bits\n");
}

BBitmap*
TFatBitsEditor::GetCopyFromClipboard(BRect* frame)	//, BBitmap** bits)
{
	if (be_clipboard->Lock() ) {
		BBitmap* bits = 0;
		BMessage *message = be_clipboard->Data();
	
		if (!message) {
			printf("no clip msg\n");
			return NULL;
		}

		BMessage embeddedBitmap;
		status_t err = message->FindMessage(kBitmapMimeType, &embeddedBitmap);
		if (!err) {
			bits = (BBitmap*) BBitmap::Instantiate(&embeddedBitmap);
			if (!bits)
				printf("bits are null\n");
		} else
			printf("can't recreate the bitmap\n");
	
		err = message->FindRect("rect", frame);
		if (err)
			printf("can't get the rect\n");
			
		be_clipboard->Unlock();
		
		return bits;
	} else
		printf("can't lock the clipboard\n");
		
	return NULL;
}

// copy the selection
// set the state so that the copy is not drawn
void
TFatBitsEditor::CutSelection(BRect)
{
}

void
TFatBitsEditor::DoCut()
{
	if (fSelectionState == kHaveSelection) {
		AddCopyToClipboard();
		EndSelection();
	}
}

bool
TFatBitsEditor::SetupSelection(BRect selectionRect)
{
	DeSelect();			// Clear out any existing selection.
	
	BeginEdit();		// Start new edit context.
	
	SetCurrentTool(kSelectionTool);	// !! need to send a msg to the toolpicker to synch
	
	// Set up basic selection state.
	fSelectionState = kHaveSelection;
	MakeSelection(selectionRect);
	if( fSelectionState == kNoSelection ) return false;
	fDirty = true;
	
	return true;
}

// Start an edit session that performs a selection in the workspace.
// Initialize the selection state with the given rectangle in the workspace.
// If 'clear' is true, also erase the bits at this location in the workspace.
void
TFatBitsEditor::BeginCopySelection(BRect selectionRect, bool clear)
{
	if( !SetupSelection(selectionRect) ) return;
	
	if (fSelectionBits->Lock()) {
		BRect destRect(selectionRect);
		destRect.OffsetTo(0,0);
		
		fSelectionBits->ResizeContainer(destRect);
		fSelectionBits->DrawBitmap(fWorkspaceBits->Bitmap(),selectionRect,destRect);
		fSelectionBits->Unlock();
		
		if( clear ) {
			EraseUnderSelection();
		}
	} else
		printf("BeginCopySelection - can't lock the copy buffer\n");
		
	Invalidate(ActualBits2FatBitsRect(fUpdateRect));
}

// Start an edit session that performs a paste into the workspace.
// The selection state is set up at the given rectangle in the workspace,
// with the given bitmap.
void
TFatBitsEditor::BeginPasteSelection(BRect selectionRect, BBitmap* bits)
{
	if( !bits ) return;

	if( !SetupSelection(selectionRect) ) return;
	
	if (fSelectionBits->Lock()) {
		BRect destRect(selectionRect);
		destRect.OffsetTo(0,0);
		fSelectionBits->ResizeContainer(destRect);
		fSelectionBits->DrawBitmap(bits,destRect,destRect);
		fSelectionBits->Sync();
		fSelectionBits->Unlock();
	} else
		printf("BeginPasteSelection - can't lock the buffer\n");
		
	Invalidate(ActualBits2FatBitsRect(fUpdateRect));
}

void
TFatBitsEditor::EndSelection(bool cancel)
{
	if( HaveSelection() ) {
		fSelectionState = kNoSelection;
		fDirty = true;
		
		// Not optimized -- mark entire bitmap as undoable.
		fUpdateRect = ActualSizeRect();
		
#if 0
		// Extend update rect to include area covered by new selection.
		if( fSelectionRect.left < fUpdateRect.left )
			fUpdateRect.left = fSelectionRect.left;
		if( fSelectionRect.top < fUpdateRect.top )
			fUpdateRect.top = fSelectionRect.top;
		if( fSelectionRect.right > fUpdateRect.right )
			fUpdateRect.right = fSelectionRect.right;
		if( fSelectionRect.bottom > fUpdateRect.bottom )
			fUpdateRect.bottom = fSelectionRect.bottom;
#endif

		Invalidate(ActualBits2FatBitsRect(fUpdateRect));
		
		EndEdit(cancel);
	}
}

void
TFatBitsEditor::DeSelect()
{
	if (HaveSelection()) {
		// 	called from single-click and when new tool is selected
		//
		OverlaySelection(fSelectionRect, B_TRANSPARENT_COLOR, B_OP_COPY);
	}
	
	EndSelection();
}

void
TFatBitsEditor::SetBitmap(BBitmap* bits, BRect srcRect, bool report)
{
	if( report ) BeginEdit();
	if (WorkspaceBits()->Lock()) {
		WorkspaceBits()->DrawBitmap(bits, srcRect, ActualSizeRect());
		WorkspaceBits()->Sync();
		WorkspaceBits()->Unlock();
		Invalidate(ActualBits2FatBitsRect(ActualSizeRect()));
	}
	if( report ) EndEdit();
}

// 	copyselection is called at the end of the trackmouse while selecting
//	all that is done here is change the state of pasting and saving of the copy rect
void
TFatBitsEditor::DoCopy()
{
	if (fSelectionState == kHaveSelection) {
		
		AddCopyToClipboard();
	}
}

bool
TFatBitsEditor::CanCopy()
{
	return (fSelectionState == kHaveSelection);
}

// paste using selectionRect from Copy to Actual Bits
void
TFatBitsEditor::OverlaySelection(BRect selectionRect,
								 rgb_color background, drawing_mode mode)
{
	OverlaySelection(fWorkspaceBits, selectionRect, background, mode);
}

inline bool operator==(rgb_color c1, rgb_color c2)
{
	return (*((uint32*)&c1)) == (*((uint32*)&c2));
}

// paste using selectionRect from Copy to given bitmap
void
TFatBitsEditor::OverlaySelection(TBitsContainer* dest, BRect selectionRect,
								 rgb_color background, drawing_mode mode)
{
	if (dest->Lock()) {
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
		
		// If not completely out of bounds, draw it.
		if( srcRect.IsValid() && selectionRect.IsValid() ) {
			dest->PushState();
			dest->SetDrawingMode(B_OP_ALPHA);
			if( ! (background.operator==(B_TRANSPARENT_COLOR)) ) {
				dest->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
				dest->SetHighColor(fBackgroundColor);
				dest->FillRect(selectionRect);
			}
			dest->SetDrawingMode(mode);
			if( mode == B_OP_ALPHA ) {
				dest->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
			}
			dest->DrawBitmap(fSelectionBits->Bitmap(),srcRect,selectionRect);
			dest->PopState();
			dest->Sync();
		}
		dest->Unlock();		
	} else
		printf("OverlaySelection - can't lock the buffer\n");
}

void
TFatBitsEditor::DoPaste()
{
	if (CanPaste()) {
		BRect r;
		BBitmap* tempBits = GetCopyFromClipboard(&r);
		if( tempBits ) {
			BeginPasteSelection(r,tempBits);
			delete tempBits;	// this was a leak, I think! -- hplus
		}
	} else
		printf("can't paste\n");
}


void
TFatBitsEditor::RefsReceived(
	BMessage * message)
{
	/* get bitmap out of file referenced by message */
	entry_ref ent;
	if (message->FindRef("refs", &ent)) {
		printf("no ref in RefsReceived()?\n");
		return;
	}
	BFile input;
	status_t err;
	if ((err = input.SetTo(&ent, O_RDONLY)) < B_OK) {
		char str[350];
		sprintf(str, "Cannot open '%s': %s.", ent.name, strerror(err));
		(new BAlert("", str, "Stop"))->Go();
		return;
	}
	BBitmap * new_bits = BTranslationUtils::GetBitmap(&input);
	
	// If no translator was able to handle it, check to see if this is
	// a flattened BBitmap archive.
	if (!new_bits) {
		BMessage archive;
		input.Seek(0, SEEK_SET);
		err = archive.Unflatten(&input);
		if( err == B_OK ) {
			BArchivable *a = instantiate_object(&archive);
			if((new_bits = dynamic_cast<BBitmap *>(a)) == 0)
				delete a;
		}
	}
	
	if (!new_bits) {
		char str[350];
		sprintf(str, "The file '%s' is not a recognized bitmap.", ent.name);
		(new BAlert("", str, "Stop"))->Go();
		return;
	}

	/* scale bitmap if it's too big */
	if (new_bits->Bounds().Width() >= fWidth || new_bits->Bounds().Height() >= fHeight) {
		float scaling_x = (fWidth-1)/new_bits->Bounds().Width();
		float scaling_y = (fHeight-1)/new_bits->Bounds().Height();
		float scaling = (scaling_x < scaling_y) ? scaling_x : scaling_y;
		BRect r(new_bits->Bounds());
		r.right = floor(r.right*scaling+.5);
		r.bottom = floor(r.bottom*scaling+.5);
		BBitmap * scaled = new BBitmap(r, B_CMAP8, true);
		BView * v = new BView(r, "", B_FOLLOW_NONE, B_WILL_DRAW);
		scaled->Lock();
		scaled->AddChild(v);
		v->DrawBitmap(new_bits, new_bits->Bounds(), r);
		v->Sync();
		scaled->RemoveChild(v);
		delete v;
		scaled->Unlock();
		delete new_bits;
		new_bits = scaled;
	}

	/* perform "paste" of data -- taken from DoPaste() */
	if( new_bits ) {
		BeginPasteSelection(new_bits->Bounds(), new_bits);
		delete new_bits;
	}
}

bool
TFatBitsEditor::CanPaste()
{
	status_t err=-1;
	if (be_clipboard->Lock() ) {
		BMessage *message = be_clipboard->Data();
		BMessage embeddedBitmap;
		
		err = message->FindMessage(kBitmapMimeType, &embeddedBitmap);
		be_clipboard->Unlock();
	}
	
	return (err == B_OK);
}

void
TFatBitsEditor::EraseUnderSelection()
{
	if (fWorkspaceBits->Lock()) {
		rgb_color oldC = fWorkspaceBits->HighColor();

		fWorkspaceBits->SetHighColor(ColorForIndex(SecondaryColor()));
		
		fWorkspaceBits->FillRect(fSelectionRect);
		
		fWorkspaceBits->Sync();
		
		fWorkspaceBits->SetHighColor(oldC);
		fWorkspaceBits->Unlock();
	} else
		printf("EraseUnderSelection - can't lock the actual bits buffer\n");
}

void
TFatBitsEditor::DoClear()
{
	EndSelection();
}

bool
TFatBitsEditor::CanClear()
{
	return HaveSelection();
}

void
TFatBitsEditor::DoSelectAll()
{
	BeginCopySelection(ActualSizeRect());
}

void
TFatBitsEditor::DumpSelection() const
{
	BPath path;
	find_directory(B_USER_DIRECTORY, &path);
	path.Append(kDumpFileName);
	
	FILE *dumpFile = fopen(path.Path(), "a+");
	fseek(dumpFile, 0, SEEK_END);
	if (!dumpFile) {
		printf("error opening dump file %s\n", path.Path());
		return;
	}

	DumpMap((fSelectionState == kHaveSelection) ?
		fSelectionBits->Bitmap() :
		fWorkspaceBits->Bitmap(),
			dumpFile, "Icon");
			
	// dump either the active seleciton or the entire workspace map
	fclose(dumpFile);
}

void
TFatBitsEditor::DumpEntireMap(FILE *file, const char *name) const
{
	DumpMap(fWorkspaceBits->Bitmap(), file, name);
}

void 
TFatBitsEditor::DumpMap(const BBitmap *bitmap, FILE *file,
	const char *name) const
{
	int32 bytesPerPixel;
	const char *kColorSpaceName;	

	switch (bitmap->ColorSpace()) {
		case B_GRAYSCALE_8_BIT:
			bytesPerPixel = 1;
			kColorSpaceName = "B_GRAYSCALE_8_BIT";
			break;
		case B_COLOR_8_BIT:
			bytesPerPixel = 1;
			kColorSpaceName = "B_COLOR_8_BIT";
			break;
		case B_RGB_16_BIT:
			bytesPerPixel = 2;
			kColorSpaceName = "B_RGB_16_BIT";
			break;
		case B_BIG_RGB_32_BIT:
			bytesPerPixel = 3;
			kColorSpaceName = "B_BIG_RGB_32_BIT";
			break;
		default:
			printf("dump: usupported ColorSpace\n");
			return;
	}
	
	// stream out the width, height and ColorSpace
	fprintf(file, "const int32 k%sWidth = %ld;\n", name, (int32)bitmap->Bounds().Width()+1);
	fprintf(file, "const int32 k%sHeight = %ld;\n", name, (int32)bitmap->Bounds().Height()+1);
	fprintf(file, "const color_space k%sColorSpace = %s;\n\n", name, kColorSpaceName);

	// stream out the constant name for this array
	fprintf(file, "const unsigned char k%sBits [] = {", name);

	const unsigned char *bits = (const unsigned char *)bitmap->Bits();
	const int32 kMaxColumnWidth = 16;
	int32 bytesPerRow = bitmap->BytesPerRow();
	int32 columnWidth = (bytesPerRow < kMaxColumnWidth) ? bytesPerRow : kMaxColumnWidth;
	
	for (int32 remaining = bitmap->BitsLength(); remaining; ) {

		fprintf(file, "\n\t");
		
		//	stream out each row, based on the number of bytes required per row
		//	padding is in the bitmap and will be streamed as 0xff
		for (int32 column = 0; column < columnWidth; column++) {
		
			// stream out individual pixel components
			for (int32 count = 0; count < bytesPerPixel; count++) {
				--remaining;
				fprintf(file, "0x%02x", *bits++);
				if (remaining)
					fprintf(file, ",");
				else
					break;
			}
			
			//	make sure we don't walk off the end of the bits array
			if (!remaining)
				break;

		}
	}
	fprintf(file, "\n};\n\n");
}

void
TFatBitsEditor::UsePencilTool(short pixelFlip, BPoint loc, BPoint last, uint8 firstIndex)
{
	if (!fWorkspaceBits->Lock()) {
		TRESPASS();
		return;
	}
	
	if( pixelFlip == 1 || pixelFlip == 3 ) {
		// draw in secondary color.
		fWorkspaceBits->SetHighColor(ColorForIndex(SecondaryColor()));
	} else {
		// draw in primary color.
		fWorkspaceBits->SetHighColor(ColorForIndex(PenColor()));
	}
	
	if( pixelFlip == 2 || pixelFlip == 3 ) {
		// only draw on original pixel color.
		uint8 currIndex = IndexAtPoint(loc);
		if (currIndex == firstIndex) {
			// 	pixels can be non-contiguous, set individual pixels not runs			
			SetPixel(loc.x,loc.y);
		}
	} else {
		// draw on all pixels.
		SetPixel(loc.x,loc.y,last.x,last.y);
	}

	fWorkspaceBits->Unlock();
}

bool
TFatBitsEditor::HaveSelection()
{
	return (fSelectionState == kHaveSelection);
}

//	move the selection by some offset values
//	used in keyboard movement of selection
void
TFatBitsEditor::MoveSelection(int32 xOffset, int32 yOffset)
{
	if (fSelectionState == kHaveSelection) {			
		BRect srcRect = fSelectionRect;
		BRect destRect = srcRect;

		destRect.left += xOffset;
		destRect.right += xOffset;
		destRect.top += yOffset;
		destRect.bottom += yOffset;
		
		if (destRect.left >= 0 && destRect.right < fWidth
			&& destRect.top >= 0 && destRect.bottom < fHeight) {
			fUpdateRect = IntersectRects(srcRect,destRect);
			
			fLastSelectionRect = fSelectionRect;
			fSelectionRect = destRect;

			fDirty = true;
		}
	}
}

void
TFatBitsEditor::MoveSelection(int32 cX, int32 cY, int32 lX, int32 lY)
{
	if (fSelectionState == kMovingSelection) {
		int32 w,h ;
		BRect srcRect = fSelectionRect;
		BRect destRect = srcRect;
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
		//
		//	only allow moves within the bounds of the drawing area !!
		//
//		if (destRect.left >= 0 && destRect.right < fWidth
//			&& destRect.top >= 0 && destRect.bottom < fHeight) {
// 	bitscontainer size should not have changed since last move, makeselection will resize
// 	thus, size of src and dest rects should equal
//	just move the destination rect and update all the rest
// 	no data needs to move

			fUpdateRect = IntersectRects(srcRect,destRect);
			
			fLastSelectionRect = fSelectionRect;
			fSelectionRect = destRect;

			fDirty = true;
//		}

	}
}

void
TFatBitsEditor::MakeSelection(BRect r)
{
	fSelectionRect = MakeValidRect(r);
	
	if (fSelectionRect.Width() > 0 && fSelectionRect.Height() > 0) {
		
		fUpdateRect = fSelectionRect;
		
	} else
		fSelectionState = kNoSelection;
}

void
TFatBitsEditor::UseSelectionTool(BRect r, int32 lX, int32 lY, bool , bool firstPass)
{
	if (fWorkspaceBits->Lock()) {
		
		if (fSelectionState == kNoSelection || fSelectionState == kSelecting) {
			//
			//	draw selection rect
			//
			fSelectionState = kSelecting;
			MakeSelection(r);
		} else if (fSelectionState == kHaveSelection || fSelectionState == kMovingSelection) {
			
			if (!fSelectionRect.Contains(r.LeftTop()) && firstPass) {
				// single click outside selection rect
				DeSelect();
			} else {
				// move the selection as user drags
				fSelectionState = kMovingSelection;
				MoveSelection(r.right, r.bottom, lX, lY);
			}
			
		} else {
			TRESPASS();
			fSelectionState = kNoSelection;
		}
		
		fWorkspaceBits->Sync();
		fWorkspaceBits->Unlock();
	}
}

void
TFatBitsEditor::UseSelectionTool(int32 sX, int32 sY, int32 eX, int32 eY,
	int32 lX, int32 lY,
	bool addToSelection, bool firstPass)
{
	UseSelectionTool(BRect(sX,sY,eX,eY),lX,lY,addToSelection, firstPass);
}

void
TFatBitsEditor::DrawRect(BRect r, pattern p, bool fill)
{
	Undo();		// undo the bits so that we can track changes to the image
	if (fWorkspaceBits->Lock()) {
		fSelectionState = kNoSelection;
		BRect drawRect = MakeValidRect(r);
		
		if (fill)
			fWorkspaceBits->FillRect(drawRect, p);
		else
			fWorkspaceBits->StrokeRect(drawRect, p);

		fUpdateRect = drawRect;

		fWorkspaceBits->Sync();
		fWorkspaceBits->Unlock();
		fDirty = true;
	}
}

void
TFatBitsEditor::DrawRect(int32 sX, int32 sY, int32 eX, int32 eY, pattern p,
	bool fill)
{
	DrawRect(BRect(sX,sY,eX,eY), p, fill);
}

void
TFatBitsEditor::DrawRoundRect(BRect r, float xRadius, float yRadius, pattern p, bool fill)
{
	Undo();		// undo the bits so that we can track changes to the image
	if (fWorkspaceBits->Lock()) {
		fSelectionState = kNoSelection;
		BRect drawRect = MakeValidRect(r);
		
		if (fill)
			fWorkspaceBits->FillRoundRect(drawRect, xRadius, yRadius, p);
		else
			fWorkspaceBits->StrokeRoundRect(drawRect, xRadius, yRadius, p);
			
		fUpdateRect = drawRect;

		fWorkspaceBits->Sync();
		fWorkspaceBits->Unlock();
		fDirty = true;
	}
}

void
TFatBitsEditor::DrawRoundRect(int32 sX, int32 sY, int32 eX, int32 eY,
	float xRadius, float yRadius, pattern p, bool fill)
{
	BRect r(sX,sY,eX,eY);
	DrawRoundRect(r, xRadius, yRadius, p, fill);
}

void
TFatBitsEditor::DrawOval(BRect r, pattern p, bool fill)
{
	Undo();		// undo the bits so that we can track changes to the image
	if (fWorkspaceBits->Lock()) {
		fSelectionState = kNoSelection;
		BRect drawRect = MakeValidRect(r);

		if (fill)
			fWorkspaceBits->FillEllipse(drawRect, p);
		else
			fWorkspaceBits->StrokeEllipse(drawRect, p);
			
		fUpdateRect = drawRect;

		fWorkspaceBits->Sync();
		fWorkspaceBits->Unlock();
		fDirty = true;
	}
}

void
TFatBitsEditor::DrawOval(int32 sX, int32 sY, int32 eX, int32 eY, pattern p, bool fill)
{
	BRect r(sX,sY,eX,eY);
	DrawOval(r, p, fill);
}
		
void
TFatBitsEditor::DrawArc(BRect r, float startAngle, float arcAngle, pattern p, bool fill)
{
	Undo();		// undo the bits so that we can track changes to the image
	if (fWorkspaceBits->Lock()) {
		fSelectionState = kNoSelection;
		BRect drawRect = MakeValidRect(r);

		if (fill)
			fWorkspaceBits->FillArc(drawRect, startAngle, arcAngle, p);
		else
			fWorkspaceBits->StrokeArc(drawRect, startAngle, arcAngle, p);
			
		fUpdateRect = drawRect;
			
		fWorkspaceBits->Sync();
		fWorkspaceBits->Unlock();
		fDirty = true;
	}
}

void
TFatBitsEditor::DrawArc(int32 sX, int32 sY, int32 eX, int32 eY,
	float startAngle, float arcAngle, pattern p, bool fill)
{
	BRect r(sX,sY,eX,eY);
	DrawArc(r, startAngle, arcAngle, p, fill);
}
			
void
TFatBitsEditor::DrawTriangle(int32 sX, int32 sY, int32 eX, int32 eY, pattern p, bool fill)
{
	BPoint p1(sX,sY);
	BPoint p2(eX,eY);
	BPoint p3(2*eX,sY);
	
	Undo();		// undo the bits so that we can track changes to the image
	if (fWorkspaceBits->Lock()) {
		fSelectionState = kNoSelection;
		BRect drawRect = MakeValidRect(BRect(sX,sY,eX,eY));

		if (fill)
			fWorkspaceBits->FillTriangle(p1,p2,p3,p);
		else
			fWorkspaceBits->StrokeTriangle(p1,p2,p3,p);
			
		fUpdateRect = drawRect;
							
		fWorkspaceBits->Sync();
		fWorkspaceBits->Unlock();
		fDirty = true;
	}
}

void
TFatBitsEditor::SetGrid(bool state)
{
	fDrawGrid = state;
	Invalidate(Bounds());
}

void
TFatBitsEditor::SetGridColor(uint8 r, uint8 g, uint8 b,uint8 a)
{
	rgb_color c;
	
	c.red = r;
	c.green = g;
	c.blue = b;
	c.alpha = a;
	
	fGridColor = c;
	
	Draw(Bounds());
}

void
TFatBitsEditor::SetGridColor(rgb_color c)
{
	fGridColor = c;
	Draw(Bounds());
}

void
TFatBitsEditor::NewBitmap(BBitmap *b)
{
	if (fWorkspaceBits->Lock()) {
		if (b)
			fWorkspaceBits->NewBitmap(b);
			
		fWorkspaceBits->Unlock();
	}
}

uint8
TFatBitsEditor::PenColor()
{
	return fPenColorIndex;
}

void
TFatBitsEditor::NewPenColor(uint8 i)
{
	if( fPenColorIndex != i ) {
		fLastPenColorIndex = fPenColorIndex;
		fPenColorIndex = i;
		SetPenColor();
	}
}

void
TFatBitsEditor::SetPenColor()
{
	if (fWorkspaceBits->Lock()) {
		fWorkspaceBits->SetHighColor(ColorForIndex(fPenColorIndex));
		fWorkspaceBits->Unlock();
	}
}

void
TFatBitsEditor::RevertPenColor()
{
	uint8 tmp = fLastPenColorIndex;
	fLastPenColorIndex = fPenColorIndex;
	fPenColorIndex = tmp;
	SetPenColor();
}

uint8
TFatBitsEditor::SecondaryColor()
{
	return fSecondaryColorIndex;
}

void
TFatBitsEditor::NewSecondaryColor(uint8 i)
{
	if( fSecondaryColorIndex != i ) {
		fLastSecondaryColorIndex = fSecondaryColorIndex;
		fSecondaryColorIndex = i;
		SetSecondaryColor();
	}
}

void
TFatBitsEditor::SetSecondaryColor()
{
	if (fWorkspaceBits->Lock()) {
		fWorkspaceBits->SetLowColor(ColorForIndex(fSecondaryColorIndex));
		fWorkspaceBits->Unlock();
	}
}

void
TFatBitsEditor::RevertSecondaryColor()
{
	uint8 tmp = fLastSecondaryColorIndex;
	fLastSecondaryColorIndex = fSecondaryColorIndex;
	fSecondaryColorIndex = tmp;
	SetSecondaryColor();
}

pattern
TFatBitsEditor::PenPattern()
{
	return fCurrentPattern;
}

void
TFatBitsEditor::SetPenPattern(pattern p)
{
	fLastPattern = fCurrentPattern;
	fCurrentPattern = p;
}

void
TFatBitsEditor::RevertPenPattern()
{
	fCurrentPattern = fLastPattern;
}

void
TFatBitsEditor::Radius(float *x, float *y)
{
	*x = fXRadius;
	*y = fYRadius;
}

void
TFatBitsEditor::SetRadius(float x, float y)
{
	fXRadius = x;
	fYRadius = y;
}

void
TFatBitsEditor::Angles(float *start, float *arc)
{
	*start = fStartAngle;
	*arc = fArcAngle;
}

void
TFatBitsEditor::SetAngles(float start, float arc)
{
	fStartAngle = start;
	fArcAngle = arc;
}

int32
TFatBitsEditor::CurrentTool(bool swap)
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
		default:
			return kPencilTool;
			break;
	}
}

void
TFatBitsEditor::SetCurrentTool(int32 tool)
{
	if( tool != kSelectionTool ) DeSelect();
	fCurrentTool = tool;
}

void
TFatBitsEditor::SetZoom(int32 factor)
{
	fPixelsPerPixel = factor;
	ResizeTo(fWidth*fPixelsPerPixel-1,fHeight*fPixelsPerPixel-1);
	
	Draw(Bounds());	
}

uint8
TFatBitsEditor::IndexAtPoint(BPoint pt)
{
	if (fWorkspaceBits->Lock()) {
		uint8 i = IndexAtPosition(pt, fWorkspaceBits->Bits(), fWorkspaceBits->Bitmap()->BytesPerRow());

		fWorkspaceBits->Unlock();
		
		return i;
	}
	
	return 0;
}

uint8
TFatBitsEditor::IndexAtPosition(BPoint pt, uchar* bits, uint32 bytesPerRow)
{
	uint8 byte=0;
	
	if (Bounds().Contains(pt)) {
	
		uint32 numBytes = (int32)(bytesPerRow * pt.y + pt.x);
		
		bits += numBytes;
		byte = *bits;
	}
				
	return byte;
}

rgb_color
TFatBitsEditor::ColorAtPosition(BPoint pt, uchar* bits, uint32 bytesPerRow)
{
	uint8 byte=0;
	uchar* temp = bits;
	
	if (Bounds().Contains(pt)) {
	
		uint32 numBytes = (int32)(bytesPerRow * pt.y + pt.x);
		
		temp += numBytes;
		byte = *temp;
	}
			
	return ColorForIndex(byte);
}

void
TFatBitsEditor::SetIndexAtPosition(BPoint pt, uchar* bits, uint32 bytesPerRow, uint8 index)
{
	uint32 numBytes = (int32)(bytesPerRow * pt.y + pt.x);
	
	bits += numBytes;
	*bits = index;
}

TBitsContainer*
TFatBitsEditor::WorkspaceBits()
{
	return fWorkspaceBits;
}

TBitsContainer*
TFatBitsEditor::ActualBits()
{
	return fActualBits;
}

//	returns the fatbits in actual size
//	without the selection
BBitmap*
TFatBitsEditor::CompositedBits()
{
	if (fActualBits->Lock()) {		
		fActualBits->DrawBitmap(fWorkspaceBits->Bitmap());

		if ((fSelectionState == kHaveSelection) || (fSelectionState == kMovingSelection)) {
			BRect srcRect(fSelectionRect);
			srcRect.OffsetTo(0,0);	
			fActualBits->DrawBitmap(fSelectionBits->Bitmap(),srcRect,fSelectionRect);				
		}
		
		fActualBits->Sync();
		fActualBits->Unlock();
	}
	
	return fActualBits->Bitmap();
}

TBitsContainer*
TFatBitsEditor::FatBits()
{
	return fFatBits;
}

TBitsContainer*
TFatBitsEditor::UndoBits()
{
	return fUndoBits;
}

TBitsContainer*
TFatBitsEditor::HiliteBits()
{
	return fHiliteBits;
}

bool 
TFatBitsEditor::Dirty() const
{
	return fDirty;
}

void
TFatBitsEditor::SetDirty(bool f)
{
	fDirty = f;
}
