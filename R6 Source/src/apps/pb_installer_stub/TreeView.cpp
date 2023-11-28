#include <Window.h>
#include <ScrollBar.h>
#include <Rect.h>
#include "TreeView.h"
// #include "Logger.h"

#include "MyDebug.h"

class TreeRoot : public TreeItem
{
	public:
		TreeRoot() : owner(0) {}
		
		void SetOwner( TreeView* l) { owner = l; }
		void ChildAdded( TreeItem*);
		void ChildRemoved( TreeItem*);
		TreeView* Owner() { return owner; }
		
	private:
		TreeView* owner;
};

void TreeRoot::ChildAdded( TreeItem* who)
{
	who;
	if (owner)
		owner->Refresh( true);
}

void TreeRoot::ChildRemoved( TreeItem* who)
{
	if (owner)
	{
		if ( owner->Current() == who)
			owner->SetCurrent(0);
			
		owner->Refresh( true);
	}
}

/////////////////////////////////////////////////////////
//  TreeView class
/////////////////////////////////////////////////////////

TreeView::TreeView( BRect frame, 
				    const char* name,
				    const char* label,
				    BMessage* message,
				    ulong resizing_mode,
				    ulong flags
                  ) 
  : BControl( frame, name, label, message, resizing_mode, flags | B_WILL_DRAW |
  															B_FRAME_EVENTS),
    item_indent(25),
    root( new TreeRoot),
    current(0),
    dataheight(0),
    datawidth(0),
    selection_mode(MULTIPLE_SELECT),
    vscroll(0),
    hscroll(0)
{	
	root->SetOwner(this);
	
	#ifndef MK2
	RecalculateScrollbars(frame.Width(), frame.Height());
	#endif
}

TreeView::~TreeView()
{
	root->DeleteChildren();
	delete root;
}

void TreeView::Purge()
{
	// Do a deep delete of all children
	root->DeleteChildren();
	Refresh(true);	
}
		
void TreeView::Draw(BRect updateRect)
{
#ifdef MK2
	PRINT(("draw start===\n"));
	// no reason for extra function call!
	TreeItem* here = root->Children();

	while(here) {
		// check children before position?
		if (!here->HasChildren() && here->Position().y + here->Height() < updateRect.top) {
			here = here->Sibling();
			continue;
		}

		if (here->Position().y > updateRect.bottom)
			break;

		here->Draw(this, true,&updateRect);
		here = here->Sibling();
	}
	PRINT(("draw end===\n"));
#else
	Redraw(updateRect);
#endif
}

void TreeView::MouseDown(BPoint p)
{
	BMessage* msg = Window()->CurrentMessage();
	if (msg) {
		long mods = msg->FindInt32( "modifiers");
		long buttons = msg->FindInt32( "buttons");
		long clicks = msg->FindInt32( "clicks");
		ClickAt(p, mods, buttons, clicks);
	}
}
		
void TreeView::FrameResized( float width, float height)
{
	BControl::FrameResized( width, height);
	RecalculateScrollbars( width, height);
}

TreeItem* TreeView::Current()
{
	return current;
}

void TreeView::SetCurrent( TreeItem* item)
{
	if ( current != item)
	{
		TreeItem* oldcurrent = current;
		
		current = item;

		if(oldcurrent)
			oldcurrent->Invalidate(this);
					
		if(current)
			current->Invalidate(this);
	}
}

void TreeView::GetSelected( BList& list)
{
	list.MakeEmpty();
	root->FindSelectedChildren( list);
}

TreeItem* TreeView::GetSelected()
{
	return root->FindChild( &TreeItem::isSelected);	
}

bool TreeView::AnySelected()
{
	return root->FindChild( &TreeItem::isSelected) != 0;
}

float TreeView::ItemIndent()
{
	return item_indent;
}

void TreeView::SetItemIndent( float i)
{
	if ( i != item_indent)
	{
		item_indent = i;
		Refresh(true);
	}
}

void TreeView::SetScrollBars(BScrollBar* h, BScrollBar* v)
{
	hscroll = h;
	vscroll = v;
#ifdef MK2
	BRect frect = Bounds();
#else
	Window()->Lock();
	BRect frect = Frame();
	Window()->Unlock();
#endif
	v->SetSteps(19, 19 * 2);
	RecalculateScrollbars( frect.Width(), frect.Height());
}

void TreeView::Refresh( bool full)
{
	RecalculatePositions();

	if( full)
		Invalidate();
	/*
	else
		root->DoForAllChildren( &TreeItem::Invalidate, this);
	*/
}

#ifndef MK2
void TreeView::Redraw(BRect updateRect)
{	
	TreeItem* here = root->Children();

	while(here) {
		if (here->Position().y + here->Height() < updateRect.top && !here->HasChildren()) {
			here = here->Sibling();
			continue;
		}

		if (here->Position().y > updateRect.bottom)
			break;

		here->Draw(this, true);
		here = here->Sibling();
	}	
}
#endif

void TreeView::AddItem( TreeItem* item)
{
	root->AddChild( item);
}

TreeItem* TreeView::RemoveItem( TreeItem* item)
{
	return root->RemoveChild(item);
}
		
// Open up the whole tree
void TreeView::ExplodeAll()
{
	root->DoForAllChildren(&TreeItem::Explode);

	Refresh(true);
}

void TreeView::SelectAll( bool select)
{
	root->DoForAllChildren( &TreeItem::Select, select);		

	Refresh(false);
}


void TreeView::AllAttached()
{
	BRect frect = Frame();

	RecalculateScrollbars(frect.Width(), frect.Height());
}

void TreeView::RecalculatePositions()
{
	BPoint where(0, 0);
	where = RePosChildren(root, where);

	dataheight = where.y;
	
	// Now we know where the last item would go we can recalculate
	// the vertical scrollbar
	BRect frect = Frame();
	RecalculateScrollbars( frect.Width(), frect.Height());
}


void TreeView::RecalculateScrollbars( float w, float h)
{
	w;
	
	if (!vscroll)
		return;
		
	float prop = h / dataheight;
		
	if ( prop >= 1.0) {
		vscroll->SetRange(0, 0);
		vscroll->SetProportion(1.0);
	}
	else {
		vscroll->SetProportion( prop);
		vscroll->SetRange( 0, dataheight - h);
	}
}

BPoint TreeView::RePosChildren( TreeItem* item, BPoint where)
{
	TreeItem* here = item->Children();

	while( here) {
		here->SetPosition(where);
		where.y += here->Height();
		if (!(here->isCollapsed()) && here->HasChildren()) {
			where.x += ItemIndent();	
			where = RePosChildren( here, where);
			where.x -= ItemIndent(); 
		}
		here = here->Sibling();
	}

	return where;
}

#define ehkANINMATING 0

void TreeView::ClickAt( BPoint p, long modifiers, long buttons, long clicks)
{
	// Right now I am only interested in primary clicks
	if (buttons & B_PRIMARY_MOUSE_BUTTON ) {
		// Check direct children to find a possible hit
		TreeItem* candidate = 0;
		TreeItem* here = root->Children();
	
		while(here) {
			candidate = here->ItemAt(p);

			if (candidate)
				break;

			here = here->Sibling();
		}
	
		if (!candidate)
			return;

		// Found the candidate, do a more refined hittest
		int hr = candidate->HitTest(this, p); 
	
		switch(hr) {
			case TreeItem::PLUSMINUS_BOX:
			{
				bool exploding = candidate->isCollapsed();

				if (exploding) {
					candidate->Explode();
					RecalculatePositions();

					candidate->AnimateBox(this,TRUE);
#if (ehkANIMATING)
					// Redraw knob
					candidate->Draw(this, FALSE);

					// Draw new version offscreen
					BPicture* pict;
					BeginPicture(new BPicture);
					BRect bounds = Bounds();
					FillRect(bounds, B_SOLID_LOW);
					Draw(bounds);
					pict = EndPicture();
					BRect tmp = bounds;
					tmp.OffsetTo(0, 0);
					tmp.right += bounds.left;
					tmp.bottom += bounds.top;
					BBitmap* offscreen = new BBitmap(tmp, B_COLOR_8_BIT, TRUE);
					BView* offView = new BView(tmp, "offView", B_FOLLOW_NONE, B_WILL_DRAW);
					offscreen->AddChild(offView);
					offscreen->Lock();
					offView->DrawPicture(pict, BPoint(0, 0));
					offscreen->Unlock();

					// Figure height of new chunk
					long height = 0;
					TreeItem* next = candidate->Successor();
					float top = candidate->Position().y + candidate->Height();
					while (next) {
						height += next->Height();
						if (top + height > tmp.bottom)
							break;
						next = next->Sibling();
					}
					
					// Figure destination rectangle for blitting and shifting
					BRect dst = bounds;
					dst.top = candidate->Position().y + candidate->Height();
					BRect moveRect = dst;
					BRect moveRect2 = moveRect;
					moveRect2.OffsetBy(0, 1);
					height = min(height, dst.Height());
					dst.bottom = dst.top;
					
					// Figure source rectangle for blitting and shifting
					BRect src = bounds;
					src.bottom = candidate->Position().y + candidate->Height() + height;
					src.top = src.bottom;
					
					// Do it!
					for (long i = 0; i <= height; i++) {
						CopyBits(moveRect, moveRect2);
						DrawBitmap(offscreen, src, dst);
						//Sync();
						src.top--;
						dst.bottom++;
						moveRect.top++;
						moveRect2.top++;
						snooze(1000);
					}
					delete offscreen;
#endif
				}
				else {
					// Figure height of disappearing chunk
#if (ehkANIMATING)
					BRect bounds = Bounds();
					long height = 0;
					float top = candidate->Position().y + candidate->Height();
					TreeItem* next = candidate->Successor();
					while (next) {
						if (top + height > bounds.bottom) {
							height -= next->Height();
							break;
						}
						height += next->Height();
						next = next->Sibling();
					}
#endif
					candidate->AnimateBox(this,FALSE);
					candidate->Collapse();				
					RecalculatePositions();

#if (ehkANIMATING)
					// Redraw knob
					candidate->Draw(this, FALSE);
					
					// Draw new version offscreen
					BPicture* pict;
					BeginPicture(new BPicture);
					FillRect(bounds, B_SOLID_LOW);
					Draw(bounds);
					pict = EndPicture();
					BRect tmp = bounds;
					tmp.OffsetTo(0, 0);
					tmp.right += bounds.left;
					tmp.bottom += bounds.top;
					BBitmap* offscreen = new BBitmap(tmp, B_COLOR_8_BIT, TRUE);
					BView* offView = new BView(tmp, "offView", B_FOLLOW_NONE, B_WILL_DRAW);
					offscreen->AddChild(offView);
					offscreen->Lock();
					offView->DrawPicture(pict, BPoint(0, 0));
					offscreen->Unlock();

					BRect moveRect = tmp;
					moveRect.top += candidate->Position().y + candidate->Height();
					moveRect.bottom = min(moveRect.bottom, moveRect.top + height);
					BRect moveRect2 = moveRect;
					moveRect2.OffsetBy(0, -1);
					height = min(height, moveRect.Height());					

					BRect dst = bounds;
					dst.top = moveRect.bottom;
					BRect src = dst;
					src.OffsetBy(0, -height);

					// Do it!
					for (long i = 0; i <= height; i++) {
						CopyBits(moveRect, moveRect2);
						DrawBitmap(offscreen, src, dst);
						Sync();
						src.bottom++;
						dst.top--;
						moveRect.bottom--;
						moveRect2.bottom--;
						snooze(1000);
					}
					delete offscreen;
#endif
				}
#if (!ehkANIMATING)				
				// make this smarter!!!
				BRect b = Bounds();
				b.top = candidate->Position().y;
				Invalidate(b);
#endif
				break;
			}
			case TreeItem::LABEL_BOX:
			{
				if ( (clicks == 2) && (Current() == candidate) && 
						!(modifiers & B_SHIFT_KEY) &&
						!(modifiers & B_COMMAND_KEY) )
				{
					candidate->Invoke();
					Invoke();
				}
				else {
				//	if ( (selection_mode == SINGLE_SELECT) || !(modifiers & B_SHIFT_KEY))
				//		SelectAll(false);
				//	candidate->Select( TRUE);
					SetCurrent( candidate);

				//	Refresh(false);
					DoSelecting(p, modifiers);
				}
				break;
			
			}
			case TreeItem::NOHIT:
				break;		
				
			default:
				break;		
		}
	}
}

void TreeView::DoSelecting(BPoint p, long modifiers)
{
	BRect bounds;
	bounds = Bounds();

	// Do we need to deal with modifier keys?
	if (modifiers & B_SHIFT_KEY || modifiers & B_COMMAND_KEY) {
		TreeItem* next = root->Children();
		while (next) {
			next->SetWasSelected(next->isSelected());
			next = next->Successor(); 
		}
	}

	// Get a local and current mouse position
	ulong button;
	GetMouse(&p, &button);

	// Start with empty box
	BRect r(p.x, p.y, p.x, p.y);

	// Draw first rect
	SetDrawingMode(B_OP_INVERT);
	StrokeRect(r, B_MIXED_COLORS);
	SetDrawingMode(B_OP_COPY);

	bool gotFirstSel = FALSE;
	bool cSelMode;

	BPoint newPoint, oldPoint;
	oldPoint = p;
	bool scrolling = TRUE; // Make sure we go through the loop at least once
	while (button) {
		GetMouse(&newPoint, &button);
		if (newPoint != oldPoint || scrolling) {
			// Erase old rect
			SetDrawingMode(B_OP_INVERT);
			StrokeRect(r, B_MIXED_COLORS);
			SetDrawingMode(B_OP_COPY);

			// Autoscroll
			bounds = Bounds();
			if ((hscroll || vscroll) && bounds.Contains(newPoint) == FALSE) {
				float deltaX = 0;
				float deltaY = 0;
				if (newPoint.x < bounds.left)
					deltaX = newPoint.x - bounds.left;
				else if (newPoint.x > bounds.right)
					deltaX = newPoint.x - bounds.right;
				if (newPoint.y < bounds.top)
					deltaY = newPoint.y - bounds.top;
				else if (newPoint.y > bounds.bottom)
					deltaY = newPoint.y - bounds.bottom;

				if (hscroll && deltaX)
						hscroll->SetValue(hscroll->Value() + deltaX);
				
				if (vscroll && deltaY)
						vscroll->SetValue(vscroll->Value() + deltaY);

				scrolling = TRUE;
			}
			else
				scrolling = FALSE;
			
			// Protect against cross-overs
			r.left = min_c(newPoint.x, p.x);
			r.top = min_c(newPoint.y, p.y);
			r.right = max_c(newPoint.x, p.x);
			r.bottom = max_c(newPoint.y, p.y);

			// Select contained items
			// Check 'em all
			// ???Is this worth optimizing?
			TreeItem* next = root->Children();
			while (next) {
				float top = next->Position().y;
				float bottom = top + next->Height();
				if (r.top < bottom && r.bottom > top) {
					if (modifiers & B_SHIFT_KEY)
						next->UserSelect(!next->wasSelected());
					else if (modifiers & B_COMMAND_KEY) {
						if (!gotFirstSel) {
							cSelMode = !next->wasSelected();
							gotFirstSel = TRUE;
						}
						next->UserSelect(cSelMode);
						next->SetWasSelected(cSelMode);
					}
					else
						next->UserSelect(TRUE);
				}
				else {
					if (modifiers & B_SHIFT_KEY)
						next->Select(next->wasSelected());
					else if (modifiers & B_COMMAND_KEY)
						next->Select(next->wasSelected());
					else
						next->Select(FALSE);
				}

				next->Invalidate(this); // Conditional

				next = next->Successor(); 
			}

			Window()->UpdateIfNeeded();
				
			// Draw new rect
			SetDrawingMode(B_OP_INVERT);
			StrokeRect(r, B_MIXED_COLORS);
			SetDrawingMode(B_OP_COPY);
			
			Flush();

			oldPoint = newPoint;
		}
		
		snooze(25000);
	}

	// Erase last rect
	SetDrawingMode(B_OP_INVERT);
	StrokeRect(r, B_MIXED_COLORS);
	SetDrawingMode(B_OP_COPY);
}

void TreeView::SetSelectionMode( int mode)
{
	if (mode != selection_mode)
	{
		SelectAll(false);
		selection_mode = mode;
	}
}
