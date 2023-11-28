#include <experimental/DividerControl.h>
#include <experimental/ColorTools.h>

#include <Application.h>
#include <Bitmap.h>
#include <Cursor.h>
#include <Debug.h>
#include <MessageRunner.h>
#include <Window.h>

#include <math.h>

#define TWIST_PULSE_MESSAGE		'tpls'

static const int ARROW_DIAMETER	= 16;
static const int ARROW_LINES	= 5;

static const int THICKNESS		= 8;

static const int DRAGGER_THICKNESS	= 13;
static const int HALF_DRAG_THICKNESS = DRAGGER_THICKNESS / 2;
static const int GRAB_LENGTH        = 10;

// Cursors.

static const uint8 kHorizontalCursorData[] = {
	16, 		 /* cursor size */
	1, 		 /* bits per pixel */
	7, 		 /* vertical hot spot */
	7, 		 /* horizontal hot spot */

	/* data */
	0x01, 0x80, 0x02, 0x40, 0x04, 0x20, 0x08, 0x10, 0x10, 0x08, 0x0f, 0xf0, 0x7f, 0xfe, 0x80, 0x01, 
	0x80, 0x01, 0x7f, 0xfe, 0x0f, 0xf0, 0x10, 0x08, 0x08, 0x10, 0x04, 0x20, 0x02, 0x40, 0x01, 0x80,

	/* mask */
	0x01, 0x80, 0x03, 0xc0, 0x07, 0xe0, 0x0f, 0xf0, 0x1f, 0xf8, 0x0f, 0xf0, 0x7f, 0xfe, 0xff, 0xff, 
	0xff, 0xff, 0x7f, 0xfe, 0x0f, 0xf0, 0x1f, 0xf8, 0x0f, 0xf0, 0x07, 0xe0, 0x03, 0xc0, 0x01, 0x80
};

static const uint8 kVerticalCursorData[] = {
	16, 		 /* cursor size */
	1, 		 /* bits per pixel */
	7, 		 /* vertical hot spot */
	7, 		 /* horizontal hot spot */

	/* data */
	0x01, 0x80, 0x02, 0x40, 0x02, 0x40, 0x0a, 0x50, 0x16, 0x68, 0x26, 0x64, 0x46, 0x62, 0x86, 0x61, 
	0x86, 0x61, 0x46, 0x62, 0x26, 0x64, 0x16, 0x68, 0x0a, 0x50, 0x02, 0x40, 0x02, 0x40, 0x01, 0x80,

	/* mask */
	0x01, 0x80, 0x03, 0xc0, 0x03, 0xc0, 0x0b, 0xd0, 0x1f, 0xf8, 0x3f, 0xfc, 0x7f, 0xfe, 0xff, 0xff, 
	0xff, 0xff, 0x7f, 0xfe, 0x3f, 0xfc, 0x1f, 0xf8, 0x0b, 0xd0, 0x03, 0xc0, 0x03, 0xc0, 0x01, 0x80
};

#if 0
static const uint8 kVerticalCursorData[] = {
	16, 	 /* cursor size */
	1, 		 /* bits per pixel */
	7, 		 /* vertical hot spot */
	7, 		 /* horizontal hot spot */

	/* data */
	0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x09, 0x90, 0x19, 0x98, 0x39, 0x9c, 0x79, 0x9e, 
	0x79, 0x9e, 0x39, 0x9c, 0x19, 0x98, 0x09, 0x90, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00,

	/* mask */
	0x01, 0x80, 0x03, 0xc0, 0x03, 0xc0, 0x0f, 0xf0, 0x1f, 0xf8, 0x3f, 0xfc, 0x7f, 0xfe, 0xff, 0xff, 
	0xff, 0xff, 0x7f, 0xfe, 0x3f, 0xfc, 0x1f, 0xf8, 0x0f, 0xf0, 0x03, 0xc0, 0x03, 0xc0, 0x01, 0x80
};

static const uint8 kHorizontalCursorData[] = {
	16, 	 /* cursor size */
	1, 		 /* bits per pixel */
	7, 		 /* vertical hot spot */
	7, 		 /* horizontal hot spot */

	/* data */
	0x00, 0x00, 0x01, 0x80, 0x03, 0xc0, 0x07, 0xe0, 0x0f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x7f, 0xfe, 
	0x7f, 0xfe, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xf0, 0x07, 0xe0, 0x03, 0xc0, 0x01, 0x80, 0x00, 0x00,

	/* mask */
	0x01, 0x80, 0x03, 0xc0, 0x07, 0xe0, 0x0f, 0xf0, 0x1f, 0xf8, 0x1f, 0xf8, 0x7f, 0xfe, 0xff, 0xff, 
	0xff, 0xff, 0x7f, 0xfe, 0x1f, 0xf8, 0x1f, 0xf8, 0x0f, 0xf0, 0x07, 0xe0, 0x03, 0xc0, 0x01, 0x80
};
#endif

DividerControl::DividerControl(BPoint where, const char* name,
							   BMessage* message,
							   orientation o, uint32 resizeMask,
							   bool useTwister,
							   bool fillToEdge)
	: BControl(BRect(where.x, where.y, where.x+1, where.y+1),
			   name, name, message, resizeMask, B_WILL_DRAW|B_FRAME_EVENTS|B_FULL_UPDATE_ON_RESIZE),
	  fOrientation(o), fFillToEdge(fillToEdge), fTracking(false), fInitPos(0),
	  fVSplit(NULL), fHSplit(NULL), fUseTwister(useTwister), fTwisting(false),
	  fOverKnob(false), fTwistAmount(0), fTwistSpeed(0), fTwistPulse(0)
{
	float w;
	float h;
	GetPreferredSize(&w, &h);
	ResizeTo(w, h);
	
	// The bottom view is currently shown.
	SetValue(1);
	
	SetViewColor(B_TRANSPARENT_COLOR);
	SetDoubleBuffering(B_UPDATE_INVALIDATED|B_UPDATE_SCROLLED|B_UPDATE_RESIZED);
	
	fVSplit = new BCursor(kVerticalCursorData);
	fHSplit = new BCursor(kHorizontalCursorData);
}

DividerControl::~DividerControl()
{
	delete fTwistPulse;
	fTwistPulse = NULL;
	
	delete fVSplit;
	delete fHSplit;
}

void DividerControl::AttachedToWindow()
{
	inherited::AttachedToWindow();
	
	BView* parent = Parent();
	
	fBackColor = parent ? parent->ViewColor()
			   : ui_color(B_PANEL_BACKGROUND_COLOR);
	
	float narrowSize = fUseTwister ? ARROW_DIAMETER-1 : DRAGGER_THICKNESS - 1;
	
	if( fFillToEdge && parent ) {
		BRect pb = parent->Bounds();
		if( fOrientation == B_HORIZONTAL ) {
			ResizeTo(pb.Width()-Frame().left+1, narrowSize);
		} else {
			ResizeTo(narrowSize, pb.Height()-Frame().top+1);
		}
	}
	
	fTwistAmount = TargetTwist();
	fTwistSpeed = 0;
}

void DividerControl::DetachedFromWindow()
{
	delete fTwistPulse;
	fTwistPulse = NULL;
	fTwistSpeed = 0;
}

void DividerControl::FrameMoved(BPoint new_position)
{
	inherited::FrameMoved(new_position);
}

void DividerControl::MessageReceived(BMessage *msg)
{
	switch( msg->what ) {
		case TWIST_PULSE_MESSAGE:
		{
			float target = TargetTwist();
			PRINT(("Current at twist %f, target is %f\n", fTwistAmount, target));
			if( floor(fTwistAmount*100) == floor(target*100) ) {
				fTwistAmount = target;
				Draw(BRect(0, 0, ARROW_DIAMETER, ARROW_DIAMETER));
				delete fTwistPulse;
				fTwistPulse = 0;
				fTwistSpeed = 0;
				return;
			}
			float orig = fTwistAmount;
			float delta = target-fTwistAmount;
			if( delta > 0 ) {
				if( fTwistSpeed < 0 ) fTwistSpeed += delta/20;
				else if( delta > fTwistSpeed*2 ) fTwistSpeed += delta/30;
				else fTwistSpeed -= (delta/5);
			} else {
				if( fTwistSpeed > 0 ) fTwistSpeed += delta/20;
				else if( delta < fTwistSpeed*2 ) fTwistSpeed += delta/30;
				else fTwistSpeed -= (delta/5);
			}
			fTwistAmount += fTwistSpeed;
			if( delta >= 0 && fTwistAmount > target ) fTwistAmount = target;
			if( delta <= 0 && fTwistAmount < target ) fTwistAmount = target;
			PRINT(("New twist is %f, delta of %f\n", fTwistAmount, delta));
			if( floor(orig*50) != floor(fTwistAmount*50) ) {
				Draw(BRect(0, 0, ARROW_DIAMETER, ARROW_DIAMETER));
			}
		} break;
		default:
			inherited::MessageReceived(msg);
	}
}

void DividerControl::Draw(BRect updateRect)
{
	static rgb_color black = { 0, 0, 0, 255 };
	rgb_color shadow = mix_color(fBackColor, black, 229);
	rgb_color shine = { 255, 255, 255, 255 };
		
	const int32 thickness = fUseTwister ?
	                        THICKNESS/2 + int32((THICKNESS*fTwistAmount)/2+.5) :
	                        DRAGGER_THICKNESS;
	
	const float left = ARROW_DIAMETER+1;
	const float right = Bounds().right;
	const float top = (ARROW_DIAMETER-thickness)/2;
	const float bottom = top+thickness-1;
	
	const float abottom = Bounds().bottom;
	
	if( true == fUseTwister ) {
		if( (updateRect.left < left-1) && (updateRect.right > 0) ) {
			SetHighColor(fBackColor);
			FillRect(BRect(0, 0, left-1, abottom));
			
			rgb_color shineStart = fOverKnob
								 ? mix_color(fBackColor, shine, 153)
								 : mix_color(fBackColor, shadow, 102);
			rgb_color shadowStart = fOverKnob
								  ? mix_color(fBackColor, shadow, 102)
								  : mix_color(fBackColor, shine, 51);
			BPoint p1, p2, p3;
			BPoint d1, d2, d3;
			float rot = M_PI_2 + M_PI_2*fTwistAmount;
			const float radius = ARROW_DIAMETER/2;
			d1.x = cos( (2*M_PI*45/360) + rot );
			d1.y = sin( (2*M_PI*45/360) + rot );
			p1.x = radius + radius*d1.x;
			p1.y = radius + radius*d1.y;
			d2.x = cos( (2*M_PI*(180-45)/360) + rot );
			d2.y = sin( (2*M_PI*(180-45)/360) + rot );
			p2.x = radius + radius*d2.x;
			p2.y = radius + radius*d2.y;
			d3.x = cos( (2*M_PI*270/360) + rot );
			d3.y = sin( (2*M_PI*270/360) + rot );
			p3.x = radius + radius*d3.x;
			p3.y = radius + radius*d3.y;
			
			SetHighColor(mix_color(fBackColor, fOverKnob ? shadow : shine, 128));
			FillTriangle(p1, p2, p3);
			
			static uint8 mix[ARROW_LINES] = { 76, 128, 204, 255, 128 };
			BeginLineArray(ARROW_LINES*3);
			for( int32 i=0; i<ARROW_LINES; i++ ) {
				AddLine(p2, p3, mix_color(shadowStart, fOverKnob ? shine : shadow, mix[i]));
				AddLine(p3, p1, mix_color(shadowStart, fOverKnob ? shine : shadow, ((uint16)mix[i]*2)/3));
				AddLine(p1, p2, mix_color(shineStart, fOverKnob ? shadow : shine, mix[i]));
				p1 -= d1;
				p2 -= d2;
				p3 -= d3;
			}
	
		EndLineArray();
		}
		
		SetHighColor(fBackColor);
		if( top > 0 ) FillRect(BRect(left, 0, right, top-1));
		if( bottom < abottom ) FillRect(BRect(left, bottom+1, right, abottom));
		
		rgb_color shineStart = fTracking
							 ? mix_color(fBackColor, shine, 178)
							 : mix_color(fBackColor, shadow, 128);
		rgb_color shadowStart = fTracking
							  ? mix_color(fBackColor, shine, 102)
							  : mix_color(fBackColor, shadow, 229);
		BeginLineArray((thickness/2)*4);

		rgb_color col;
		for( int32 i=thickness/2-1; i>=0; i-- ) {
			col = mix_color(shineStart, fTracking ? shadow : shine,
							((i+1)*255) / (THICKNESS/2) );
			AddLine(BPoint(left, top+i), BPoint(right, top+i), col);
			AddLine(BPoint(left, top+i),
					BPoint(left-(thickness+i)/2, top+thickness/2), col);
			
			col = mix_color(shadowStart, fTracking ? shadow : shine,
							((i+1)*255) / (THICKNESS/2) );
			AddLine(BPoint(left, bottom-i), BPoint(right, bottom-i), col);
			AddLine(BPoint(left, bottom-i),
					BPoint(left-(thickness+i)/2, bottom-thickness/2), col);
		}
		EndLineArray();
	} else {
		// New style w/o twister.
		// todo: support vertical orientation.
	
		// background
		SetHighColor(fBackColor);
		FillRect(Bounds());

		float widthInset = 0;
		float heightInset = 3;
		
		if( B_VERTICAL == fOrientation ) {
			heightInset = 0;
			widthInset = 3;
		}

		BRect ref(Bounds().InsetBySelf(widthInset, heightInset));
		
		// Border.
		SetHighColor(ui_color(B_UI_CONTROL_BORDER_COLOR));
		StrokeRoundRect(ref.InsetByCopy(1, 1), 2, 2);

		// Body
		SetHighColor(ui_color(B_UI_CONTROL_HIGHLIGHT_COLOR));
		SetLowColor(ui_color(B_UI_CONTROL_BACKGROUND_COLOR));
		FillRect(ref.InsetByCopy(2, 2), B_SOLID_HIGH);

		BeginLineArray(fTracking ? 7 : 1);

		if( true == fTracking ) {
			// Pressed state
			rgb_color shadow(tint_color(ui_color(B_UI_SHADOW_COLOR), B_LIGHTEN_1_TINT));
			rgb_color shine(ui_color(B_UI_SHINE_COLOR));
			
			BPoint start(ref.left + 2, ref.bottom);
			BPoint end(ref.right - 3, start.y);

			// Bottom right shine.
			
			// bottom
			AddLine(start, end, shine);

			// right bottom angle
			start.x = end.x + 1;
			end.Set(ref.right, ref.bottom - 2);
			AddLine(start, end, shine);

			// right
			start.Set(end.x, end.y - 1);
			end.y = ref.top + 2;
			AddLine(start, end, shine);

			// Top Left Shadow
			
			// left
			start.Set(ref.left, ref.bottom - 2);
			end.Set(start.x, ref.top + 2);
			AddLine(start, end, shadow);

			// left top angle
			start.Set(ref.left + 1,end.y - 1);
			end.Set(ref.left + 2, ref.top);
			AddLine(start, end, shadow);
			
			// top
			start.Set(ref.left + 3, ref.top);
			end.Set(ref.right - 2, ref.top);
			AddLine(start, end, shadow);			
			
			SetLowColor(tint_color(LowColor(), B_DARKEN_2_TINT));
		}
				
		// Grab
		BPoint start((Bounds().Width() / 2) - (GRAB_LENGTH / 2), Bounds().Height() / 2);
		BPoint end(start.x + GRAB_LENGTH, start.y);
		
		if( B_VERTICAL == fOrientation ) {
			start.Set(Bounds().Width() / 2, (Bounds().Height() / 2) - (GRAB_LENGTH / 2));
			end.Set(start.x, start.y + GRAB_LENGTH);
		}
		
		StrokeLine(start, end, B_SOLID_LOW);

		EndLineArray();
	}
}

void DividerControl::MouseDown(BPoint where)
{
	SetMouseEventMask(B_POINTER_EVENTS,
					  B_LOCK_WINDOW_FOCUS|B_SUSPEND_VIEW_FOCUS|B_NO_POINTER_HISTORY);
	
	if( fUseTwister && (where.x <= ARROW_DIAMETER) ) {
		fOverKnob = true;
		StartTwisting();
	} else if( Value() ) {
		fTracking = true;
		fInitPoint = ConvertToScreen(where);
		fInitPos = Frame().top;
	}
	
	Invalidate(Bounds());
}

void DividerControl::MouseUp(BPoint)
{
	if( fTwisting ) {
		if( fOverKnob ) {
			SetValue(Value() ? 0 : 1);
			InvokeNotify(Message(), B_CONTROL_INVOKED);
		}
		StartTwisting();
	}
	
	be_app->SetCursor(B_CURSOR_SYSTEM_DEFAULT);
	
	fTracking = false;
	fTwisting = false;
	fOverKnob = false;
	Invalidate(Bounds());
}

void DividerControl::MouseMoved(BPoint where, uint32 /*code*/,
								const BMessage */*a_message*/)
{
	BRect r(Bounds());
	const BCursor* cur = fHSplit;

	if( fUseTwister ) {
		if( B_HORIZONTAL == fOrientation ) {
			r.left = ARROW_DIAMETER;
		} else {
			r.top = ARROW_DIAMETER;
		}
	}

	if( B_VERTICAL == fOrientation ) {
		cur = fVSplit;
	}
	
	if( r.Contains(where) || (true == fTracking) )
	{
		be_app->SetCursor(cur);
		SetViewCursor(cur);
	} else {
		be_app->SetCursor(B_CURSOR_SYSTEM_DEFAULT);
		SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
	}


	if( fTwisting ) {
		bool over = false;
		if( where.x >= 0 && where.x < ARROW_DIAMETER
				&& where.y >= 0 && where.y < ARROW_DIAMETER ) over = true;
		if( over != fOverKnob ) {
			fOverKnob = over;
			Invalidate(BRect(0, 0, ARROW_DIAMETER, ARROW_DIAMETER));
		}
		StartTwisting();
	} else if( fTracking ) {
		BView* before = PreviousSibling();
		BView* after = NextSibling();
		if( !before || !after ) return;
		
		float bPrefW;
		float bPrefH;
		float aPrefW;
		float aPrefH;
		before->GetPreferredSize(&bPrefW, &bPrefH);
		after->GetPreferredSize(&aPrefW, &aPrefH);

		BRect myFrame = Frame();
		BRect aFrame = after->Frame();

		if( B_VERTICAL == fOrientation ) {
			BRect bFrame = before->Frame();

			float width = 0;
			
			if( NULL != Parent() ) {
				width = Parent()->Bounds().Width();
			} else {
				width = Window()->Bounds().Width();
			}
			
			if( myFrame.Width() + aFrame.Width() + bFrame.Width() + 1 > width ) {
				return;
			}

			where = ConvertToParent(where);

			bFrame.right = where.x - (HALF_DRAG_THICKNESS + 1);
			if( bFrame.Width() < bPrefW ) {
				bFrame.right = bFrame.left + bPrefW;
				where.x = bFrame.right + HALF_DRAG_THICKNESS + 1;
			}
			
			aFrame.left = where.x + HALF_DRAG_THICKNESS + 1;
			if( aFrame.Width() < aPrefW ) {
				aFrame.left = aFrame.right - aPrefW;
				where.x = aFrame.left - (HALF_DRAG_THICKNESS + 1);
				bFrame.right = where.x - (HALF_DRAG_THICKNESS + 1);
			}						
			
			myFrame.OffsetTo(where.x - HALF_DRAG_THICKNESS, myFrame.top);

			Window()->BeginViewTransaction();
			MoveTo(myFrame.left, myFrame.top);
			before->ResizeTo(bFrame.Width(), bFrame.Height());
			after->MoveTo(aFrame.left, aFrame.top);
			after->ResizeTo(aFrame.Width(), aFrame.Height());
			Window()->EndViewTransaction();
 		} else {
			float bCurW = before->Bounds().Width();
			float bCurH = before->Bounds().Height();
			float aCurW = after->Bounds().Width();
			float aCurH = after->Bounds().Height();
			where = ConvertToScreen(where);
	
			float newPos = fInitPos + where.y - fInitPoint.y;
			if( newPos == Frame().top ) return;
			
			
			float delta = newPos - Frame().top;
			float orig_delta = delta;
			PRINT(("Delta is %f\n", delta));
			if( delta < 0 && (bCurH+delta) < bPrefH ) {
				delta = bPrefH - bCurH;
				PRINT(("Previous sibling out of bounds: delta now %f\n", delta));
			}
			if( delta > 0 && (aCurH-delta) < aPrefH ) {
				delta = aCurH - aPrefH;
				PRINT(("Next sibling out of bounds: delta now %f\n", delta));
			}
			if( delta < 0 && (bCurH+delta) < bPrefH ) {
				// The window is too small to fit both above and below at their
				// preferred sizes -- just take what the user selected, bounded
				// at not making either above or below view zero.
				delta = orig_delta;
				if( delta < 0 && (bCurH+delta) < 2 ) {
					delta = 2 - bCurH;
				}
				if( delta > 0 && (aCurH-delta) < 2 ) {
					delta = aCurH - 2;
				}
				PRINT(("Both of bounds: delta now %f\n", delta));
			}
			
			delta = floor(delta+.5);
			
			if( delta != 0 ) {
				Window()->BeginViewTransaction();
				MoveTo(myFrame.left, myFrame.top+delta);
				before->ResizeTo(bCurW, bCurH+delta);
				after->MoveTo(aFrame.left, aFrame.top+delta);
				after->ResizeTo(aCurW, aCurH-delta);
				Window()->EndViewTransaction();
			}
		}
	}
}

void DividerControl::GetPreferredSize(float *width, float *height)
{
	float* thickness = height;
	float* length    = width;
	
	if( fOrientation == B_VERTICAL ) {
		thickness = width;
		length    = height;
	}
	
	*length    = fUseTwister ? ARROW_DIAMETER + THICKNESS*2 - 1 : GRAB_LENGTH + 8;
	*thickness = fUseTwister ? ARROW_DIAMETER - 1               : DRAGGER_THICKNESS - 1;
}

void DividerControl::SetValue(int32 value)
{
	inherited::SetValue(value);
	fTwistAmount = TargetTwist();
}

void DividerControl::AnimateValue(int32 value)
{
	inherited::SetValue(value);
	if( Window() && !Window()->IsHidden() ) {
		StartTwisting();
	} else {
		fTwistAmount = TargetTwist();
	}
}

orientation DividerControl::Orientation()
{
	return fOrientation;
}

float DividerControl::TargetTwist() const
{
	return fOverKnob ? .5 : (Value() ? 1 : 0);
}

void DividerControl::StartTwisting()
{
	fTwisting = true;
	if( !fTwistPulse ) {
		PRINT(("Creating twist animation messenger.\n"));
		fTwistPulse = new BMessageRunner(BMessenger(this),
										 new BMessage(TWIST_PULSE_MESSAGE),
										 50*1000);
		fTwistSpeed = 0;
	}
}

