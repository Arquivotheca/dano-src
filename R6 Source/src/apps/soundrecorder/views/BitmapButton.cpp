#include <stdio.h>
#include <Bitmap.h>
#include <Window.h>

#include "BitmapButton.h"
#include "DrawingTidbits.h"

BitmapButton::BitmapButton( 
	BPoint where, 
	const char *name, 
	const void *bitmap,
	int32 width,
	int32 height,
	BMessage *message,
	uint32 resizeMask,
	uint32 flags )
	: BButton( BRect( where.x, where.y, where.x+width-1, where.y+height-1 ), name, "", message, resizeMask, flags )
{
	this->height = height;
	this->width = width;
	
	bounds.Set( 0, 0, width-1, height-1 );
	
	size = height*width;
	
	normalBM = new BBitmap( bounds, B_CMAP8 );
	normalBM->SetBits( bitmap, size, 0, B_CMAP8 );
	
	highlightBM = new BBitmap( bounds, B_CMAP8 );
	highlightBM->SetBits( ((char *)bitmap)+size, size, 0, B_CMAP8 );
	
	disabledBM = new BBitmap( bounds, B_CMAP8 );
	disabledBM->SetBits( ((char *)bitmap)+2*size, size, 0, B_CMAP8 );
	
	SetViewColor( B_TRANSPARENT_COLOR );
}

BitmapButton::~BitmapButton( void )
{
	delete normalBM;
	delete highlightBM;
	delete disabledBM;
}
		
void BitmapButton::Draw( BRect updateRect )
{
	SetDrawingMode(B_OP_COPY);
	if( !IsEnabled() )
		DrawBitmap( disabledBM, bounds );
	else if( Value() )
		DrawBitmap( highlightBM, bounds );
	else
		DrawBitmap( normalBM, bounds );
}


void BitmapButton::GetPreferredSize( float *width, float *height )
{
	*width = float(this->width-1);
	*height = float(this->height-1);
}

// override AttachedToWindow so that the BControl version doesn't change the
// viewcolor out from under us.
void
BitmapButton::AttachedToWindow()
{
	// if target/looper wasn't set then default to window
	if (!Messenger().IsValid()) {
		SetTarget(Window());
	}   
	ResizeToPreferred();                                           
}

// ***
// BitmapCheckBox
// ***

BitmapCheckBox::BitmapCheckBox( 
	BPoint where, 
	const char *name, 
	const void *bitmap,
	int32 width,
	int32 height,
	BMessage *message,
	uint32 resizeMask,
	uint32 flags )
	: BControl( BRect( where.x, where.y, where.x+width-1, where.y+height-1 ), name, "", message, resizeMask, flags )
{
	this->height = height;
	this->width = width;
	
	bounds.Set( 0, 0, width-1, height-1 );
	
	size = height*width;
	
	normalBM = new BBitmap( bounds, B_CMAP8 );
	normalBM->SetBits( bitmap, size, 0, B_CMAP8 );
	
	highlightBM = new BBitmap( bounds, B_CMAP8 );
	highlightBM->SetBits( ((char *)bitmap)+size, size, 0, B_CMAP8 );
	
	checkedBM = new BBitmap( bounds, B_CMAP8 );
	checkedBM->SetBits( ((char *)bitmap)+2*size, size, 0, B_CMAP8 );
	
	highlightCheckedBM = new BBitmap( bounds, B_CMAP8 );
	highlightCheckedBM->SetBits( ((char *)bitmap)+3*size, size, 0, B_CMAP8 );
	
	disabledBM = new BBitmap( bounds, B_CMAP8 );
	disabledBM->SetBits( ((char *)bitmap)+4*size, size, 0, B_CMAP8 );
	
	disabledCheckedBM = new BBitmap( bounds, B_CMAP8 );
	disabledCheckedBM->SetBits( ((char *)bitmap)+5*size, size, 0, B_CMAP8 );
	
	fPressing = false;
	SetValue( 0 );
	SetViewColor( B_TRANSPARENT_COLOR );
	
	
}

BitmapCheckBox::~BitmapCheckBox( void )
{
	delete normalBM;
	delete highlightBM;
	delete disabledBM;
	delete checkedBM;
	delete highlightCheckedBM;
	delete disabledCheckedBM;
}
		
// make sure that we redraw after the keyboard changes our state
void 
BitmapCheckBox::KeyDown(const char *bytes, int32 numBytes)
{
	BControl::KeyDown(bytes, numBytes);
	Invalidate();
}


void
BitmapCheckBox::Draw(BRect)
{
	DrawInState(fCurrState);
}

void
BitmapCheckBox::GetPreferredSize(float *width, float *height)
{
	*width = float(this->width-1);
	*height = float(this->height-1);
}


void 
BitmapCheckBox::DrawInState(BitmapCheckBox::State state)
{
	SetDrawingMode(B_OP_COPY);
	BBitmap *bmap = NULL;
	if( !IsEnabled() )
	{
		if( Value() )
			bmap = disabledCheckedBM;
		else
			bmap = disabledBM;
	}
	else
	{
		switch (state) {
			case kNormal:
				bmap = normalBM;
				break;
			case kHighlight:
				bmap = highlightBM;
				break;
			case kChecked:
				bmap = checkedBM;
				break;
			case kHighlightChecked:
				bmap = highlightCheckedBM;
				break;
			default:
				bmap = normalBM;
				break;
		}
	}
	DrawBitmap(bmap, BPoint(0, 0));
}

BitmapCheckBox::State 
BitmapCheckBox::StateForPosition(BPoint mousePos)
{
	BRect bounds(Bounds());

	State state;
	
	if( bounds.Contains( mousePos ) )
	{
		if( Value() )
			state = kHighlightChecked;
		else
			state = kHighlight;
	}
	else
	{
		if( Value() )
			state = kChecked;
		else
			state = kNormal;
	}

	return state;
}

// make sure that fCurrState stays consistent
void 
BitmapCheckBox::SetValue(int32 value)
{
	fCurrState = (value != 0) ? kChecked : kNormal;
	BControl::SetValue(value);
}


void BitmapCheckBox::MouseMoved( BPoint where, uint32 code, const BMessage *msg )
{
	if( fPressing ) {
		State state = StateForPosition(where);
		if (state != fCurrState) {
			fCurrState = state;
			Invalidate();			
		}
	}
}

void BitmapCheckBox::MouseUp( BPoint where )
{
	fPressing = false;
	SetMouseEventMask( 0, 0 );
	State state = StateForPosition(where);
	if (state != fStateWhenPressed) {
		SetValue(!Value());
		Invoke();
	}
}

void 
BitmapCheckBox::MouseDown(BPoint point)
{
	if (!IsEnabled())
		return;
	
	SetMouseEventMask( B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS | B_SUSPEND_VIEW_FOCUS | B_NO_POINTER_HISTORY );
	fPressing = true;
	fClickPoint = point;
	fStateWhenPressed = fCurrState;
	if( Value() )
		fCurrState = kHighlightChecked;
	else
		fCurrState = kHighlight;
	Invalidate();
}

// override AttachedToWindow so that the BControl version doesn't change the
// viewcolor out from under us.
void
BitmapCheckBox::AttachedToWindow()
{
	// if target/looper wasn't set then default to window
	if (!Messenger().IsValid()) {
		SetTarget(Window());
	}            
	SetViewColor( Parent()->ViewColor() );     
	ReplaceTransparentColor(normalBM, Parent()->ViewColor());
	ReplaceTransparentColor(highlightBM, Parent()->ViewColor());
	ReplaceTransparentColor(checkedBM, Parent()->ViewColor());
	ReplaceTransparentColor(highlightCheckedBM, Parent()->ViewColor());
	ReplaceTransparentColor(disabledBM, Parent()->ViewColor());
	ReplaceTransparentColor(disabledCheckedBM, Parent()->ViewColor());               
}
