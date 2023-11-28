#include <ToolTip.h>

#include <Message.h>
#include <MessageRunner.h>

#include <Bitmap.h>
#include <Picture.h>
#include <Screen.h>
#include <View.h>
#include <Window.h>

#include <Autolock.h>
#include <Debug.h>
#include <StreamIO.h>
#include <String.h>

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

namespace BPrivate {

// ----------------------------------------------------------------------

status_t mix_bitmaps(BBitmap* out,
					 const BBitmap* b1, const BBitmap* b2, uint8 amount)
{
	if( out->BitsLength() != b1->BitsLength() ||
		out->BitsLength() != b2->BitsLength() ||
		out->ColorSpace() != b1->ColorSpace() ||
		out->ColorSpace() != b2->ColorSpace() ) {
		TRESPASS();
		return B_BAD_VALUE;
	}
	
	switch( out->ColorSpace() ) {
		case B_RGB32:
		case B_RGBA32:
		case B_RGB24:
		case B_GRAY8:
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
		case B_RGB24_BIG:
		{
			uint8* sOut = (uint8*)out->Bits();
			uint8* eOut = sOut + out->BitsLength();
			uint8* sB1 = (uint8*)b1->Bits();
			uint8* sB2 = (uint8*)b2->Bits();
			
			while( sOut < eOut ) {
				*sOut = (uint8)( ( ((uint16)*sB1)*(255-amount)
								 + ((uint16)*sB2)*(amount)
								 ) / 255 );
				sOut++;
				sB1++;
				sB2++;
			}
		} break;
		
		case B_RGB16:
		{
			uint16* sOut = (uint16*)out->Bits();
			uint16* eOut = sOut + out->BitsLength()/2;
			uint16* sB1 = (uint16*)b1->Bits();
			uint16* sB2 = (uint16*)b2->Bits();
			
			while( sOut < eOut ) {
				const uint16 b1 = *sB1;
				const uint16 b2 = *sB2;
				uint16 b = ( (b1&0x1F)*(255-amount)
								+ (b2&0x1F)*(amount) ) / 255;
				uint16 g = ( ((b1>>5)&0x3F)*(255-amount)
								+ ((b2>>5)&0x3F)*(amount) ) / 255;
				uint16 r = ( ((b1>>11)&0x1F)*(255-amount)
								+ ((b2>>11)&0x1F)*(amount) ) / 255;
				*sOut = b | (g<<5) | (r<<11);
				sOut++;
				sB1++;
				sB2++;
			}
		} break;
		
		case B_RGB15:
		case B_RGBA15:
		{
			uint16* sOut = (uint16*)out->Bits();
			uint16* eOut = sOut + out->BitsLength()/2;
			uint16* sB1 = (uint16*)b1->Bits();
			uint16* sB2 = (uint16*)b2->Bits();
			
			while( sOut < eOut ) {
				const uint16 b1 = *sB1;
				const uint16 b2 = *sB2;
				if( b1 == B_TRANSPARENT_MAGIC_RGBA15 ) {
					*sOut = b2;
				} else if( b2 == B_TRANSPARENT_MAGIC_RGBA15 ) {
					*sOut = b1;
				} else {
					uint16 b = ( (b1&0x1F)*(255-amount)
									+ (b2&0x1F)*(amount) ) / 255;
					uint16 g = ( ((b1>>5)&0x1F)*(255-amount)
									+ ((b2>>5)&0x1F)*(amount) ) / 255;
					uint16 r = ( ((b1>>10)&0x1F)*(255-amount)
									+ ((b2>>10)&0x1F)*(amount) ) / 255;
					*sOut = b | (g<<5) | (r<<10) | 0x8000;
				}
				sOut++;
				sB1++;
				sB2++;
			}
		} break;
		
		case B_CMAP8:
		{
			uint8* sOut = (uint8*)out->Bits();
			uint8* eOut = sOut + out->BitsLength();
			uint8* sB1 = (uint8*)b1->Bits();
			uint8* sB2 = (uint8*)b2->Bits();
			
			BScreen s;
			const color_map* cm = system_colors();
			
			while( sOut < eOut ) {
				if( *sB1 == B_TRANSPARENT_MAGIC_CMAP8 ) {
					*sOut = *sB2;
				} else if( *sB2 == B_TRANSPARENT_MAGIC_CMAP8 ) {
					*sOut = *sB1;
				} else {
					rgb_color c1 = cm->color_list[*sB1];
					rgb_color c2 = cm->color_list[*sB2];
					c1.red = (uint8)( ( ((uint16)c1.red)*(255-amount)
										+ ((uint16)c2.red)*(amount)
									) / 255 );
					c1.green = (uint8)( ( ((uint16)c1.green)*(255-amount)
										+ ((uint16)c2.green)*(amount)
									) / 255 );
					c1.blue = (uint8)( ( ((uint16)c1.blue)*(255-amount)
										+ ((uint16)c2.blue)*(amount)
									) / 255 );
					*sOut = cm->index_map[
								((c1.red&0xF8)<<7)
							|	((c1.green&0xF8)<<2)
							|	((c1.blue&0xF8)>>3)
							];
				}
				sOut++;
				sB1++;
				sB2++;
			}
		} break;
		
		default:
			TRESPASS();
			return B_BAD_VALUE;
	}
	
	return B_OK;
}

status_t copy_bitmap(BBitmap* dest,
					 const BBitmap* src, BRect srcRect, BPoint destPnt)
{
	if( dest->ColorSpace() != src->ColorSpace() ) {
		TRESPASS();
		return B_BAD_VALUE;
	}
	
	size_t bytesPerPixel = 1;
	
	switch( dest->ColorSpace() ) {
		case B_RGB32:
		case B_RGBA32:
		case B_RGB32_BIG:
		case B_RGBA32_BIG:
			bytesPerPixel = 4;
			break;
			
		case B_RGB24:
		case B_RGB24_BIG:
			bytesPerPixel = 3;
			break;
			
		case B_RGB16:
		case B_RGB15:
		case B_RGBA15:
			bytesPerPixel = 2;
			break;
			
		case B_GRAY8:
		case B_CMAP8:
			bytesPerPixel = 1;
			break;
			
		default:
			TRESPASS();
			return B_BAD_VALUE;
	}
	
	if( destPnt.x < 0 ) {
		srcRect.left -= destPnt.x;
		destPnt.x = 0;
	}
	if( destPnt.y < 0 ) {
		srcRect.top -= destPnt.y;
		destPnt.y = 0;
	}
	if( srcRect.left < 0 ) {
		destPnt.x -= srcRect.left;
		srcRect.left = 0;
	}
	if( srcRect.top < 0 ) {
		destPnt.y -= srcRect.top;
		srcRect.top = 0;
	}
	
	int32 srcLineStart = int32(srcRect.left+.5)*bytesPerPixel;
	int32 destLineStart = int32(destPnt.x+.5)*bytesPerPixel;
	int32 lineBytes = int32(srcRect.right-srcRect.left+1.5)*bytesPerPixel;
	if( srcLineStart+lineBytes > src->BytesPerRow() ) {
		lineBytes = src->BytesPerRow()-srcLineStart;
	}
	if( destLineStart+lineBytes > dest->BytesPerRow() ) {
		lineBytes = dest->BytesPerRow()-destLineStart;
	}
	if( lineBytes <= 0 ) return B_OK;
	
	uint8* srcData = ((uint8*)src->Bits()) + srcLineStart
				   + int32(srcRect.top)*src->BytesPerRow();
	uint8* srcEnd = ((uint8*)src->Bits()) + src->BitsLength();
	uint8* destData = ((uint8*)dest->Bits()) + destLineStart
					+ int32(destPnt.y)*dest->BytesPerRow();
	uint8* destEnd = ((uint8*)dest->Bits()) + dest->BitsLength();
	
	while( srcData < srcEnd && destData < destEnd ) {
		memcpy(destData, srcData, lineBytes);
		srcData += src->BytesPerRow();
		destData += dest->BytesPerRow();
	}
	
	return B_OK;
}

// --------------------------- TipView ---------------------------

class TipView : public BView
{
public:
	TipView(BRect frame, const char* name, const BToolTipInfo* info,
			uint32 resizeMask = B_FOLLOW_NONE,
			uint32 flags = B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE)
		: BView(frame, name, resizeMask, flags)
	{
		BFont font(*be_plain_font);
		if( info ) {
			fTip = info->Text();
			font = *info->Font();
			SetLowColor(info->FillColor());
			SetHighColor(info->TextColor());
		}
		//font.SetSpacing(B_STRING_SPACING);
		SetFont(&font);
		SetViewColor(B_TRANSPARENT_COLOR);
	}
	
	virtual	void Draw(BRect /*updateRect*/)
	{
		static rgb_color shine = { 255, 255, 255, 255 };
		static rgb_color shadow = { 0, 0, 0, 255 };
		rgb_color text = HighColor();
		
		BRect b(Bounds());
		
		SetHighColor(shadow);
		StrokeRect(b);
		b.InsetBy(1, 1);
		
		SetHighColor(mix_color(LowColor(), shine, 128+64));
		StrokeLine(BPoint(b.left, b.bottom), BPoint(b.left, b.top));
		StrokeLine(BPoint(b.left, b.top), BPoint(b.right, b.top));
		SetHighColor(mix_color(LowColor(), shadow, 128-64));
		StrokeLine(BPoint(b.right, b.top), BPoint(b.right, b.bottom));
		StrokeLine(BPoint(b.right, b.bottom), BPoint(b.top+1, b.bottom));
		
		FillRect(BRect(b.left+1, b.top+1, b.right-1, b.bottom-1), B_SOLID_LOW);
		
		font_height fh;
		GetFontHeight(&fh);
		SetHighColor(text);
		
		DrawString(fTip.String(), BPoint(b.left+2, b.top+1+fh.ascent));
	}

	virtual void GetPreferredSize(float* width, float* height)
	{
		font_height fh;
		GetFontHeight(&fh);
		*height = floor( fh.ascent+fh.descent + 4 + .5 );
		*width = floor( StringWidth(fTip.String()) + 6 + .5 );
	}
	
	BPoint TextOrigin() const
	{
		font_height fh;
		GetFontHeight(&fh);
		return BPoint(3, floor(fh.ascent+2+.5));
	}
	
private:
	BString fTip;
};

// --------------------------- TipWindow ---------------------------

class TipWindow : public BWindow
{
public:
	TipWindow(BToolTip& owner)
		: BWindow(BRect(-100, -100, -90, -90),
				  "Tool Tip",
				  B_NO_BORDER_WINDOW_LOOK,
				  B_FLOATING_ALL_WINDOW_FEEL,
				  B_NOT_MOVABLE|B_NOT_CLOSABLE|B_NOT_ZOOMABLE
				  |B_NOT_MINIMIZABLE|B_NOT_RESIZABLE
				  |B_AVOID_FOCUS|B_NO_WORKSPACE_ACTIVATION
				  |B_WILL_ACCEPT_FIRST_CLICK|B_ASYNCHRONOUS_CONTROLS),
		  fOwner(owner),
		  fDrawer(Bounds(), "drawer", B_FOLLOW_ALL, B_WILL_DRAW),
		  fState(S_OFF),
		  fShowTime(1500*1000), fHideTime(10000*1000), fSettleTime(1000*1000),
		  fStateTimer(0), fAnim(0), fTip(0),
		  fBackPic(0), fForePic(0), fMixPic(0),
          fInline(false), fCurAlpha(0), fDestAlpha(0)
	{
		fDrawer.SetViewColor(B_TRANSPARENT_COLOR);
		AddChild(&fDrawer);
		Run();
	}
	~TipWindow()
	{
		fOwner.WindowGone(this);
		fDrawer.RemoveSelf();
		StopStateTimer();
		DestroyTip(true);
	}
	
	void ShowTip(const BMessenger& source)
	{
		BAutolock l(this);
		switch( fState ) {
			case S_OFF:
				PRINT(("ShowTip(): S_OFF to S_HOVER\n"));
				fState = S_HOVER;
				StartStateTimer(fShowTime);
				break;
			case S_HOVER:
				PRINT(("ShowTip(): S_HOVER to S_HOVER\n"));
				StartStateTimer(fShowTime);
				break;
			case S_REQUEST:
			case S_SHOWN:
			case S_SETTLE:
				PRINT(("ShowTip(): S_REQUEST or S_SHOWN or S_SETTLE to S_SETTLE\n"));
				fState = S_REQUEST;
				StartStateTimer(fHideTime);
				RequestTipInfo();
				break;
			default:
				TRESPASS();
		}
		
		fSource = source;
	}
	
	void SetToolTipInfo(BRect region, BToolTipInfo* info, bool now = false)
	{
		BAutolock l(this);
		
		if( !now && fState != S_REQUEST && fState != S_SHOWN ) {
			// If the tool tip isn't currently shown and this call
			// isn't requesting to show the tool tip right now, then
			// don't do anything.
			delete info;
			return;
		}
		
		if( fState != S_REQUEST ) {
			// If this wasn't called in response to a request, just
			// perform a show immediately.
			StartStateTimer(fHideTime);
		}
		
		fState = S_SHOWN;
		CreateTip(region, info);
	}
	
	void CursorMoved(BPoint /*where*/, BPoint delta)
	{
		if( fState != S_HOVER ) return;
		
		BAutolock l(this);
		if( delta.x > 3 || delta.x < -3 || delta.y > 3 || delta.y < -3 ) {
			PRINT(("Cursor moved enough to restart tool tip.\n"));
			StartStateTimer(fShowTime);
		}
	}
	
	void HideTip()
	{
		BAutolock l(this);
		switch( fState ) {
			case S_OFF:
				PRINT(("HideTip(): S_OFF to S_OFF\n"));
				StopStateTimer();
				return;
			case S_HOVER:
				PRINT(("HideTip(): S_HOVER to S_OFF\n"));
				fState = S_OFF;
				StopStateTimer();
				break;
			case S_REQUEST:
			case S_SHOWN:
				PRINT(("HideTip(): S_REQUEST or S_SHOWN to S_SETTLE\n"));
				fState = S_SETTLE;
				StartStateTimer(fSettleTime);
				break;
			case S_SETTLE:
				PRINT(("HideTip(): S_SETTLE to S_SETTLE\n"));
				break;
			default:
				TRESPASS();
		}
		
		DestroyTip();
	}
	
	void KillTip()
	{
		BAutolock l(this);
		switch( fState ) {
			case S_OFF:
				PRINT(("KillTip(): S_OFF to S_OFF\n"));
				StopStateTimer();
				return;
			case S_HOVER:
				PRINT(("KillTip(): S_HOVER to S_OFF\n"));
				fState = S_OFF;
				StopStateTimer();
				break;
			case S_REQUEST:
			case S_SHOWN:
			case S_SETTLE:
				PRINT(("KillTip(): S_REQUEST or S_SHOWN or S_SETTLE to S_OFF\n"));
				fState = S_OFF;
				StopStateTimer();
				break;
			default:
				TRESPASS();
		}
		
		DestroyTip(true);
	}
	
	void TransitionState()
	{
		StopStateTimer();
		
		bigtime_t next_time = 0;
		
		switch( fState ) {
			case S_OFF:
				PRINT(("Transition: S_OFF to S_OFF\n"));
				DestroyTip(true);
				break;
			case S_HOVER:
				PRINT(("Transition: S_HOVER to S_REQUEST\n"));
				RequestTipInfo();
				fState = S_REQUEST;
				next_time = fHideTime;
				break;
			case S_REQUEST:
			case S_SHOWN:
				PRINT(("Transition: S_REQUEST or S_SHOWN to S_SETTLE\n"));
				DestroyTip();
				fState = S_SETTLE;
				next_time = fSettleTime;
				break;
			case S_SETTLE:
				PRINT(("Transition: S_SETTLE to S_OFF\n"));
				DestroyTip(true);
				fState = S_OFF;
				break;
			default:
				TRESPASS();
		}
		
		if( next_time > 0 ) StartStateTimer(next_time);
	}
	
	void RequestTipInfo()
	{
		if( fSource.IsValid() ) {
			BMessage msg(B_REQUEST_TOOL_INFO);
			fSource.SendMessage(&msg);
		}
	}
	
	void StartStateTimer(bigtime_t when)
	{
		StopStateTimer();
		if( when <= 0 ) return;
		BMessage msg('puls');
		fStateTimer = new BMessageRunner(BMessenger(this), &msg, when);
	}
	
	void StopStateTimer()
	{
		delete fStateTimer;
		fStateTimer = NULL;
	}
	
	virtual void DispatchMessage(BMessage* msg, BHandler* handler)
	{
		switch( msg->what ) {
			case B_MOUSE_MOVED: {
				BPoint pnt;
				if( msg->FindPoint("where", &pnt) == B_OK ) {
					ConvertToScreen(&pnt);
					PRINT(("Checking if (%.2f,%.2f) is in (%.2f,%.2f)-(%.2f-%.2f)\n",
							pnt.x, pnt.y,
							fInlineRegion.left, fInlineRegion.top,
							fInlineRegion.right, fInlineRegion.bottom));
					if( !fInlineRegion.Contains(pnt) ) {
						if( fDestAlpha != 0.0 ) HideTip();
						return;
					}
				}
				if( fInline && fState == S_SETTLE && !IsHidden() ) {
					PRINT(("Mouse move in inline tip: showing.\n"));
					fDestAlpha = 1.0;
					fState = S_SHOWN;
					StartStateTimer(fHideTime);
					StartAnimation();
				}
			} break;
			
			case B_MOUSE_DOWN:
			case B_MOUSE_UP:
				// TO DO: Forward message to underlying window.
				KillTip();
				return;
		}
		
		inherited::DispatchMessage(msg, handler);
	}
	
	virtual void MessageReceived(BMessage* msg)
	{
		switch( msg->what ) {
			case 'anim':
				if( fDestAlpha > 0.5 ) {
					fCurAlpha += .1;
					if( fCurAlpha >= fDestAlpha ) {
						StopAnimation();
						fCurAlpha = 1.0;
						return;
					}
				} else {
					fCurAlpha -= .15;
					if( fCurAlpha <= fDestAlpha ) {
						StopAnimation();
						fCurAlpha = 0.0;
						return;
					}
				}
				
				PRINT(("Drawing with alpha=%f, tip=%p\n",
						fCurAlpha, fTip));
				if( fMixPic && fBackPic && fForePic ) {
					mix_bitmaps(fMixPic, fBackPic, fForePic,
								(uint8)(255*fCurAlpha+.5));
					fDrawer.DrawBitmap(fMixPic);
				}
				break;
				
			case 'puls':
				TransitionState();
				break;
				
			default:
				inherited::MessageReceived(msg);
				break;
		}
	}
	
	BBitmap* CreateTipBitmap(BView* from)
	{
		BScreen s(this);
		BBitmap* bm = new BBitmap(from->Bounds().OffsetToSelf(BPoint(0, 0)),
									B_BITMAP_CLEAR_TO_WHITE|B_BITMAP_ACCEPTS_VIEWS,
									s.ColorSpace(), B_ANY_BYTES_PER_ROW, s.ID());
		bm->Lock();
		bm->AddChild(from);
		from->PushState();
		from->Draw(from->Bounds());
		from->PopState();
		from->Sync();
		from->RemoveSelf();
		bm->Unlock();
		return bm;
	}
	
	void CreateTip(BRect region, BToolTipInfo* info, bool now=false)
	{
		// Grab the current screen bitmap, in case our new tip
		// overlaps it anywhere -- we can use it to recover what was
		// originally on the screen at that point.
		BBitmap* prev_pic = fBackPic;
		fBackPic = NULL;
		
		CleanupAll();
		
		/*if( info->View() ) {
			fTip = info->DetachView();
		} else*/ if( info->Text() && *info->Text() ) {
			fTip = new TipView(Bounds(), "TipView", info);
		} else {
			DestroyTip();
			delete info;
			return;
		}
		
		fInline = info->Inline();
		fInlineRegion = region;
		
		BScreen s(this);
		BRect sb(s.Frame());
		
		float w, h;
		fTip->GetPreferredSize(&w, &h);
		if( w > sb.Width() ) w = sb.Width();
		if( h > sb.Height() ) h = sb.Height();
		
		fTip->SetResizingMode(B_FOLLOW_NONE);
		
		float x = (region.left+region.right)/2 - w/2;
		float y = region.bottom + 6;
		
		TipView* tipView = NULL;
		if (info->HasOrigin() && (tipView=dynamic_cast<TipView*>(fTip)) != NULL) {
			// Tool tip should be placed with text at this
			// location.
			const BPoint tipOrigin = tipView->TextOrigin();
			x = region.left + info->Origin().x - tipOrigin.x;
			y = region.top + info->Origin().y - tipOrigin.y;
		}
		
		#if 0
		if( info->Inline() ) {
			x = region.left;
			y = region.top;
			if( w < region.Width() ) w = region.Width();
			if( h < region.Height() ) h = region.Height();
		}
		#endif
		
		if( x < sb.left ) x = sb.left;
		if( (x+w) > sb.right ) x = sb.right-w;
		if( y < sb.top ) y = sb.top;
		if( info->Inline() ) {
			if( (y+h) > sb.bottom ) y = sb.bottom-h;
		} else {
			if( (y+h) > sb.bottom ) y = region.top - h - 6;
		}
		if( y < sb.top ) y = sb.top;
		x = floor(x+.5);
		y = floor(y+.5);
		
		delete info;
		info = NULL;
		
		BRect bbnd(0, 0, w, h);
		BRect wfrm(x, y, x+w, y+h);
		fTip->ResizeTo(w, h);
		
		fBackPic = new BBitmap(bbnd, 0, s.ColorSpace(),
							   B_ANY_BYTES_PER_ROW, s.ID());
		s.ReadBitmap(fBackPic, false, &wfrm);
		if( !fBackPic ) {
			PRINT(("*** ERROR GETTING SCREEN BITMAP.\n"));
			fCurAlpha = fDestAlpha;
			StopAnimation();
			delete prev_pic;
			return;
		}
		
		// If there was still a tool tip displayed, and the new tip overlaps
		// the previous one, copy the screen bitmap we had for that area into
		// our new screen bitmap.
		if( prev_pic ) {
			if( wfrm.Intersects(fBackRegion) ) {
				BRect i = wfrm&fBackRegion;
				BPoint p(0, 0);
				if( wfrm.left < fBackRegion.left ) {
					p.x = fBackRegion.left-wfrm.left;
					i.OffsetTo(0, i.top);
				} else {
					i.OffsetTo(wfrm.left-fBackRegion.left, i.top);
				}
				if( wfrm.top < fBackRegion.top ) {
					p.y = fBackRegion.top-wfrm.top;
					i.OffsetTo(i.left, 0);
				} else {
					i.OffsetTo(i.left, wfrm.top-fBackRegion.top);
				}
				copy_bitmap(fBackPic, prev_pic, i, p);
			}
			delete prev_pic;
			prev_pic = NULL;
		}
		
		fBackRegion = wfrm;
		
		fDestAlpha = 1.0;
		
		// If the window isn't yet visible, move it now so that it
		// will show up in the correct spot.  Otherwise, we want to
		// move it after the animation starts, so that it is moved
		// with the new data.
		const bool hidden = IsHidden();
		if (hidden) {
			ResizeTo(w, h);
			MoveTo(x, y);
		}
		
		if( !now ) {
			StartAnimation();
		} else {
			fCurAlpha = 1.0;
			StopAnimation();
		}
		
		if (!hidden) {
			ResizeTo(w, h);
			MoveTo(x, y);
		}
	}
	
	void DestroyTip(bool now=false)
	{
		fDestAlpha = 0.0;
		
		if( IsHidden() ) return;
		
		if( !now ) {
			StartAnimation();
		} else {
			fCurAlpha = 0.0;
			StopAnimation();
		}
		
		if( fTip ) {
			fTip->RemoveSelf();
			delete fTip;
			fTip = 0;
		}
	}
	
	void StartAnimation()
	{
		if (fCurAlpha == fDestAlpha) {
			StopAnimation();
			return;
		}
		
		if (!fTip) return;
		
		Cleanup();
		BScreen s;
		fForePic = CreateTipBitmap(fTip);
		fMixPic = new BBitmap(fForePic->Bounds(), 0, fForePic->ColorSpace(),
							  B_ANY_BYTES_PER_ROW, s.ID());
                              
		delete fAnim;
		fAnim = NULL;
		BMessage msg('anim');
		fAnim = new BMessageRunner(BMessenger(this), &msg, 50*1000);
		if (IsHidden()) Show();
	}
	
	void StopAnimation()
	{
		delete fAnim;
		fAnim = NULL;
		
		if (fDestAlpha > 0.5) {
			if (fTip) {
				if (fTip->Window()) {
					fTip->RemoveSelf();
				}
				fDrawer.AddChild(fTip);
				if (IsHidden()) Show();
				fTip->PushState();
				fTip->Draw(Bounds());
				fTip->Sync();
				fTip->PopState();
			}
			
		} else {
			if (!IsHidden()) Hide();
			CleanupAll();
		}
	}
	
	void Cleanup()
	{
		if (fTip && fTip->Window()) {
			fTip->RemoveSelf();
		}
		delete fForePic;
		fForePic = NULL;
		delete fMixPic;
		fMixPic = NULL;
	}
	
	void CleanupAll()
	{
		Cleanup();
		if (fTip) {
			delete fTip;
			fTip = NULL;
		}
		delete fBackPic;
		fBackPic = NULL;
	}
	
private:
	typedef BWindow inherited;
	
	BToolTip& fOwner;
	BView fDrawer;

	enum tip_state {
		S_OFF,
		S_HOVER,
		S_REQUEST,
		S_SHOWN,
		S_SETTLE
	};
	
	tip_state fState;
	bigtime_t fShowTime;
	bigtime_t fHideTime;
	bigtime_t fSettleTime;
	
	BMessenger fSource;
	
	BMessageRunner* fStateTimer;
	BMessageRunner* fAnim;
	BView* fTip;
	BBitmap* fBackPic;
	BBitmap* fForePic;
	BBitmap* fMixPic;
	BRect fBackRegion;
	BRect fInlineRegion;
	bool fInline;
	float fCurAlpha;
	float fDestAlpha;
};

}	// namespace BPrivate

using namespace BPrivate;

// --------------------------- BToolTipInfo ---------------------------

BToolTipInfo::BToolTipInfo()
	: fFont(*be_plain_font), fInline(false), fHasOrigin(false), fView(0)
{
	fFillColor = ui_color(B_TOOLTIP_BACKGROUND_COLOR);
	fTextColor = ui_color(B_TOOLTIP_TEXT_COLOR);
	
	BMessage settings;
	if (get_ui_settings(&settings) == B_OK)
		settings.FindFlat(B_UI_TOOLTIP_FONT, &fFont);
}

BToolTipInfo::~BToolTipInfo()
{
	delete fView;
}

void BToolTipInfo::SetText(const char* text)
{
	fText = text;
}

const char* BToolTipInfo::Text() const
{
	return fText.String();
}

void BToolTipInfo::SetFont(const BFont* font)
{
	fFont = *font;
}

const BFont* BToolTipInfo::Font() const
{
	return &fFont;
}

void BToolTipInfo::SetFillColor(rgb_color color)
{
	fFillColor = color;
}

rgb_color BToolTipInfo::FillColor() const
{
	return fFillColor;
}

void BToolTipInfo::SetTextColor(rgb_color color)
{
	fTextColor = color;
}

rgb_color BToolTipInfo::TextColor() const
{
	return fTextColor;
}

void BToolTipInfo::SetInline(bool state)
{
	fInline = state;
}

bool BToolTipInfo::Inline() const
{
	return fInline;
}

void BToolTipInfo::SetOrigin(BPoint origin)
{
	fOrigin = origin;
	fHasOrigin = true;
}

void BToolTipInfo::ClearOrigin()
{
	fHasOrigin = false;
}

bool BToolTipInfo::HasOrigin() const
{
	return fHasOrigin;
}

BPoint BToolTipInfo::Origin() const
{
	return fOrigin;
}

void BToolTipInfo::SetView(BView* view)
{
	delete fView;
	fView = view;
}

BView* BToolTipInfo::View() const
{
	return fView;
}

BView* BToolTipInfo::DetachView()
{
	BView* v = fView;
	fView = 0;
	return v;
}

void BToolTipInfo::_ReservedToolTipInfo1() {}
void BToolTipInfo::_ReservedToolTipInfo2() {}
void BToolTipInfo::_ReservedToolTipInfo3() {}
void BToolTipInfo::_ReservedToolTipInfo4() {}
void BToolTipInfo::_ReservedToolTipInfo5() {}
void BToolTipInfo::_ReservedToolTipInfo6() {}
void BToolTipInfo::_ReservedToolTipInfo7() {}
void BToolTipInfo::_ReservedToolTipInfo8() {}
void BToolTipInfo::_ReservedToolTipInfo9() {}
void BToolTipInfo::_ReservedToolTipInfo10() {}
void BToolTipInfo::_ReservedToolTipInfo11() {}
void BToolTipInfo::_ReservedToolTipInfo12() {}
void BToolTipInfo::_ReservedToolTipInfo13() {}
void BToolTipInfo::_ReservedToolTipInfo14() {}
void BToolTipInfo::_ReservedToolTipInfo15() {}
void BToolTipInfo::_ReservedToolTipInfo16() {}

// --------------------------- BToolTip ---------------------------

BToolTip::BToolTip()
	: fTip(0)
{
}

BToolTip::~BToolTip()
{
	if (fTip && fTip->Lock()) {
		fTip->Close();
	}
}

static BToolTip global_tip;

BToolTip* BToolTip::Default()
{
	return &global_tip;
}

status_t BToolTip::ShowTip(const BMessenger& who)
{
	BAutolock l(fAccess);
	HideTip(BMessenger());
	if (who.IsValid()) {
		fCurrentOwner = who;
		Tip()->ShowTip(fCurrentOwner);
	}
	return B_OK;
}

status_t BToolTip::CursorMoved(const BMessenger& who, BPoint where, BPoint delta)
{
	BAutolock l(fAccess);
	if( fCurrentOwner != who ) return B_BAD_VALUE;
	if( fTip ) fTip->CursorMoved(where, delta);
	return B_OK;
}

BToolTipInfo* BToolTip::NewToolTipInfo() const
{
	BToolTipInfo* info = new BToolTipInfo();
	return info;
}

status_t BToolTip::SetToolTipInfo(const BMessenger& who, BRect region, BToolTipInfo* info)
{
	BAutolock l(fAccess);
	if( fCurrentOwner != who ) {
		delete info;
		return B_BAD_VALUE;
	}
	fTip->SetToolTipInfo(region, info);
	return B_OK;
}

status_t BToolTip::HideTip(const BMessenger& who)
{
	BAutolock l(fAccess);
	if( who.IsValid() && fCurrentOwner != who ) return B_BAD_VALUE;
	if( fTip ) fTip->HideTip();
	fCurrentOwner = BMessenger();
	return B_OK;
}

status_t BToolTip::KillTip(const BMessenger& who)
{
	BAutolock l(fAccess);
	if( who.IsValid() && fCurrentOwner != who ) return B_BAD_VALUE;
	if( fTip ) fTip->KillTip();
	fCurrentOwner = BMessenger();
	return B_OK;
}

status_t BToolTip::RemoveOwner(const BMessenger& who)
{
	BAutolock l(fAccess);
	if( fCurrentOwner == who ) {
		if( fTip ) fTip->HideTip();
		fCurrentOwner = BMessenger();
		return B_OK;
	}
	return B_BAD_VALUE;
}

TipWindow* BToolTip::Tip()
{
	if( !fTip ) fTip = new TipWindow(*this);
	return fTip;
}

void BToolTip::WindowGone(TipWindow* /*w*/)
{
	fTip = 0;
}

void BToolTip::_WatchMyToolTip1() {}
void BToolTip::_WatchMyToolTip2() {}
void BToolTip::_WatchMyToolTip3() {}
void BToolTip::_WatchMyToolTip4() {}
void BToolTip::_WatchMyToolTip5() {}
void BToolTip::_WatchMyToolTip6() {}
void BToolTip::_WatchMyToolTip7() {}
void BToolTip::_WatchMyToolTip8() {}
void BToolTip::_WatchMyToolTip9() {}
void BToolTip::_WatchMyToolTip10() {}
void BToolTip::_WatchMyToolTip11() {}
void BToolTip::_WatchMyToolTip12() {}
void BToolTip::_WatchMyToolTip13() {}
void BToolTip::_WatchMyToolTip14() {}
void BToolTip::_WatchMyToolTip15() {}
void BToolTip::_WatchMyToolTip16() {}
