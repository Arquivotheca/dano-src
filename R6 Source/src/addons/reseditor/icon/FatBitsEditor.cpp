#include "FatBitsEditor.h"
#include "BitmapEditor.h"
#include "BitsContainer.h"

#include "utils.h"

#include <Debug.h>
#include <Alert.h>
#include <MessageFilter.h>

#include <ResourceEditor.h>

#include <vector>

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

TFatBitsEditor::TFatBitsEditor(TBitmapEditor& editor, rgb_color gridColor,
							   float pixelsPerPixel)
	:	BView(BRect(0, 0,
					PreferredWidth(editor,pixelsPerPixel),
					PreferredHeight(editor,pixelsPerPixel)),
			"fat bit editor",B_FOLLOW_NONE,
			B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE ),
		fEditor(editor), fPixelsPerPixel(pixelsPerPixel),
		fGridColor(gridColor), fScrollView(0), fOffscreen(0),
		fModifierFilter(0)
{
	SetViewColor(B_TRANSPARENT_COLOR);
}
  
TFatBitsEditor::~TFatBitsEditor()
{
	delete fOffscreen;
	fOffscreen = 0;
	delete fModifierFilter;
	fModifierFilter = 0;
}

float
TFatBitsEditor::PreferredWidth(TBitmapEditor& editor, float pixelsPerPixel)
{
	float w = editor.Width()*pixelsPerPixel;
	if( w > 32*8 ) w = 32*8;
	return w;
}

float
TFatBitsEditor::PreferredHeight(TBitmapEditor& editor, float pixelsPerPixel)
{
	float h = editor.Height()*pixelsPerPixel;
	if( h > 32*8 ) h = 32*8;
	return h;
}

void
TFatBitsEditor::AllAttached()
{
	BView::AllAttached();
	AttachObjects();
	FixupScrollBar();
}

void
TFatBitsEditor::DetachedFromWindow()
{
	DetachObjects();
	BView::DetachedFromWindow();
}

void
TFatBitsEditor::GetPreferredSize(float *width, float *height)
{
	*width = PreferredWidth(fEditor, fPixelsPerPixel);
	*height = PreferredHeight(fEditor, fPixelsPerPixel);
}

void 
TFatBitsEditor::WindowActivated(bool active)
{
	BView::WindowActivated(active);
	if (IsFocus() && fScrollView)
		fScrollView->Draw(Bounds());
}

void
TFatBitsEditor::Draw(BRect area)
{
	rgb_color gc = fGridColor;
	if( fPixelsPerPixel < 3 ) gc = B_TRANSPARENT_COLOR;
	BScreen s(Window());
	if( !fOffscreen ) {
		fOffscreen = new TBitsContainer(Bounds().Width()+1,
										Bounds().Height()+1,
										s.ColorSpace());
	} else {
		fOffscreen->SetAttributes(Bounds().Width()+1,
								  Bounds().Height()+1,
								  s.ColorSpace(),
								  false);
	}
	
	area = area & Bounds();
	
	if( fOffscreen->Lock() ) {
		fEditor.GetFatBits(fOffscreen, fPixelsPerPixel,
						   BPoint(area.left-Bounds().left,
						   		  area.top-Bounds().top),
						   area, gc);
		fOffscreen->Sync();
		fOffscreen->Unlock();
	}
	
	BRect src(area);
	src.OffsetBy(-Bounds().left, -Bounds().top);
	DrawBitmap(fOffscreen->Bitmap(), src, area);
}

void 
TFatBitsEditor::MessageReceived(BMessage *msg)
{
	if( fEditor.DoMessageReceived(msg, this) == B_OK ) return;
	
	bool handled = false;
	
	switch(msg->what) {
		case B_OBSERVER_NOTICE_CHANGE: {
			#if DEBUG
			PRINT(("Received notification: ")); msg->PrintToStream();
			#endif
			uint32 what = 0;
			msg->FindInt32(B_OBSERVE_WHAT_CHANGE, (int32*)&what);
			switch( what ) {
				case T_BITMAP_INVALIDATED: {
					BRect region;
					if( msg->FindRect("be:region", &region) == B_OK ) {
						region = fEditor.Actual2FatBits(fPixelsPerPixel, region);
					} else {
						region = Bounds();
					}
					if( region.IsValid() ) {
						Draw(region);
						Flush();
					}
					handled = true;
				} break;
				case T_ATTRIBUTES_CHANGED: {
					FixupScrollBar();
					Invalidate();
				} break;
				case T_CURSOR_CHANGED: {
					PRINT(("Setting view cursor to %p\n",
							fEditor.CurrentCursor()));
					SetViewCursor(fEditor.CurrentCursor());
					handled = true;
				} break;
			}
		}
	}
	
	if( !handled ) BView::MessageReceived(msg);
}

void
TFatBitsEditor::KeyDown(const char *key, int32 numBytes)
{
	if( fEditor.DoKeyDown(key, numBytes, Window()->CurrentMessage()) == B_OK ) {
		return;
	}
	
	BView::KeyDown(key,numBytes);
}

void
TFatBitsEditor::MouseDown(BPoint point)
{
	if( !IsFocus() ) MakeFocus();
	
	BPoint real_point = fEditor.FatBits2Actual(fPixelsPerPixel, point);
	if( fEditor.DoMouseDown(real_point, Window()->CurrentMessage()) == B_OK ) {
		SetMouseEventMask(B_POINTER_EVENTS|B_KEYBOARD_EVENTS,
						  B_LOCK_WINDOW_FOCUS|B_NO_POINTER_HISTORY);
		return;
	}
	
	BView::MouseDown(point);
}

void
TFatBitsEditor::MouseMoved(BPoint point, uint32 code, const BMessage *a_message)
{
	if( code == B_ENTERED_VIEW ) SetViewCursor(fEditor.CurrentCursor());
	
	BPoint real_point = fEditor.FatBits2Actual(fPixelsPerPixel, point);
	if( fEditor.DoMouseMoved(real_point, code, a_message,
							 Window()->CurrentMessage()) == B_OK ) {
		return;
	}
	
	BView::MouseMoved(point, code, a_message);
}

void
TFatBitsEditor::MouseUp(BPoint point)
{
	BPoint real_point = fEditor.FatBits2Actual(fPixelsPerPixel, point);
	if( fEditor.DoMouseUp(real_point, Window()->CurrentMessage()) == B_OK ) {
		return;
	}
	BView::MouseUp(point);
}

void
TFatBitsEditor::FrameResized(float, float)
{
	FixupScrollBar();
}

void
TFatBitsEditor::MakeFocus(bool state)
{
	if (state == IsFocus())
		return;

	BView::MakeFocus(state);

	if (fScrollView)
		fScrollView->SetBorderHighlighted(state);
}

void
TFatBitsEditor::TargetedByScrollView(BScrollView *sv)
{
	fScrollView = sv;
}

void
TFatBitsEditor::FixupScrollBar(void)
{
	BScrollBar* sv = ScrollBar(B_VERTICAL);
	BScrollBar* sh = ScrollBar(B_HORIZONTAL);
	
	if( !sv || !sh ) return;
	
	BRect bounds(Bounds());
	BRect avail(0, 0,
				fEditor.Width()*fPixelsPerPixel,
				fEditor.Height()*fPixelsPerPixel);
	
	if( avail.right < bounds.right ) avail.right = bounds.right;
	if( avail.bottom < bounds.bottom ) avail.bottom = bounds.bottom;
	
	if( sv ) {
		sv->SetRange(0, avail.Height() - bounds.Height());
		sv->SetProportion(bounds.Height() / avail.Height());
		sv->SetSteps(fPixelsPerPixel, bounds.Height());
	}
	if( sh ) {
		sh->SetRange(0, avail.Width() - bounds.Width());
		sh->SetProportion(bounds.Width() / avail.Width());
		sh->SetSteps(fPixelsPerPixel, bounds.Width());
	}
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
	
	Invalidate();
}

void
TFatBitsEditor::SetGridColor(rgb_color c)
{
	fGridColor = c;
	Invalidate();
}

void
TFatBitsEditor::SetZoom(float factor)
{
	fPixelsPerPixel = factor;
	Invalidate();
	FixupScrollBar();
}

float
TFatBitsEditor::Zoom() const
{
	return fPixelsPerPixel;
}

void
TFatBitsEditor::UpdateMouseState(const BMessage* from)
{
	if( Window() && Window()->IsActive() ) {
		fEditor.UpdateMouseState(from);
	}
}

void
TFatBitsEditor::AttachObjects()
{
	DetachObjects();
	fModifierFilter = new modMessageFilter(this);
	Window()->AddCommonFilter(fModifierFilter);
	StartWatchingAll(BMessenger(&fEditor));
}

void
TFatBitsEditor::DetachObjects()
{
	if( fModifierFilter ) {
		Window()->RemoveCommonFilter(fModifierFilter);
		delete fModifierFilter;
		fModifierFilter = 0;
	}
}
