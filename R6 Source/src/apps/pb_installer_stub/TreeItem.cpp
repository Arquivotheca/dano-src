#include <Bitmap.h>
#include <List.h>
#include <string.h>
#include "TreeView.h"
#include "Utilities.h"

#define DEBUG 0

#include <Debug.h>

/*This Image is 22 pixels wide, 19 pixels high, and 8 bits deep*/
static unsigned char default_bitmap_data[] = 
{ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0x00, 0xFA, 0xFA, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0x00, 
0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x1F, 0xFA, 0xFA, 0xFA, 0xFA, 
0x1F, 0x5D, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xF9, 0x1F, 0x1F, 
0xFA, 0x1F, 0x5D, 0x5D, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xF9, 
0xF9, 0xF9, 0x1F, 0x5D, 0x5D, 0x5D, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x60, 
0x01, 0xF9, 0xF9, 0xF9, 0xF9, 0x5D, 0x5D, 0x5D, 0x00, 0x00, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 
0x60, 0x60, 0x01, 0xF9, 0xF9, 0xF9, 0xF9, 0x5D, 0x5D, 0x5D, 0x00, 
0xA3, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0x00, 0x1F, 0x60, 0x60, 0x60, 0x01, 0xF9, 0xF9, 0xF9, 0x5D, 0x5D, 
0x00, 0xA3, 0x1F, 0x2D, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0x00, 0x86, 0x1F, 0x1F, 0x60, 0x1F, 0x00, 0x00, 0xF9, 
0x5D, 0x00, 0xA3, 0x1F, 0x2D, 0x2D, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x86, 0x86, 0x86, 0x1F, 0xD5, 0x27, 
0x00, 0x00, 0x00, 0xA3, 0x1F, 0x2D, 0x2D, 0x2E, 0x00, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x86, 0x86, 0x86, 0x86, 
0xD5, 0x28, 0x01, 0xCA, 0xCA, 0xA3, 0xA3, 0x2D, 0x2D, 0x2E, 0x00, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x86, 0x86, 
0x86, 0x86, 0xD5, 0xD5, 0x00, 0xCA, 0xA3, 0xA3, 0xA3, 0x2D, 0x2D, 
0x2D, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 
0x86, 0x86, 0x86, 0x86, 0xD5, 0xD5, 0x00, 0xA3, 0xA3, 0xA3, 0xA3, 
0x2D, 0x2D, 0x2E, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0x00, 0x00, 0x86, 0x86, 0xD5, 0xD5, 0x01, 0x00, 0x00, 
0xA3, 0xA3, 0x2D, 0x2E, 0x00, 0x11, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x11, 
0x11, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x11, 0x11, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static BBitmap* default_bitmap = 0;

TreeItem::TreeItem() :
		sibling(0),
		children(0),
		childTail(0),
		parent(0),
		is_selected(false),
		was_selected(false),
		is_collapsed(false),
		is_invalid(true),
		position(0,0),
		label(0),
		BOX_WIDTH(7.0),
		HALF(BOX_WIDTH/2.0),
		BOX_INDENT(10.0),
		hilitedBitmap(0)
{
	SetIcon(0);
}

TreeItem::TreeItem( const char* name, BBitmap* bmap)
	:
		sibling(0),
		children(0),
		childTail(0),
		parent(0),
		is_selected(false),
		was_selected(false),
		is_collapsed(false),
		is_invalid(true),
		position(0,0),
		label(0),
		BOX_WIDTH(7.0),
		HALF(BOX_WIDTH/2.0),
		BOX_INDENT(10.0),
		hilitedBitmap(0)
{
	SetLabel(name);
	SetIcon(bmap);
}

TreeItem::~TreeItem()
{
	if ( parent)
	{
		// Make sure there are no dangling links
		parent->RemoveChild(this);
	}

	if ( label)
		free(label);
		
	if (hilitedBitmap)
		delete hilitedBitmap;
}

void TreeItem::DeleteChildren()
{
	while(children)
	{
		TreeItem* target = children;
		RemoveChild(target);
		target->DeleteChildren();
		delete target;
	}
}


float TreeItem::Height()
{
	// Use this to get good spacing for standard miniicons 
	return 19.0;
}

void TreeItem::SetIcon( BBitmap* newbitmap)
{
	if (newbitmap)
		bitmap = newbitmap;
	else
	{
		if ( !default_bitmap) {
			default_bitmap = new BBitmap(BRect(0,0,21,18), B_COLOR_8_BIT);
			::memcpy( default_bitmap->Bits(),
			          default_bitmap_data,
			          default_bitmap->BitsLength());
		}
		
		bitmap = default_bitmap;
	}

	// Create hilited version
	if (!hilitedBitmap)
		hilitedBitmap = new BBitmap(bitmap->Bounds(), B_COLOR_8_BIT);

	DarkenBitmap(bitmap, hilitedBitmap);

	is_invalid = true;
}

void TreeItem::SetLabel( const char* newlabel)
{
	if ( label)
		free(label);
	
	if ( newlabel)	
		label = strdup(newlabel);
	else
		label = 0;

	is_invalid = true;
}

bool TreeItem::CanHaveChildren()
{
	return true;
}

void TreeItem::AddSibling( TreeItem* item)
{
	item->sibling = sibling;
	item->parent = parent;
	sibling = item;
	
	if ( parent)
		parent->ChildAdded(item);	
}
		
void TreeItem::AddChild( TreeItem* item, int at)
{
	//if ( CanHaveChildren())
	//{
		// child list is null or first insertion
		if ( children == 0 || at == 0)
		{
			item->sibling = children;
			children = item;
			childTail = children;
		}
		else if (at == INT_MAX) {
			// adding at tail
			// item->sibling = 0;
			childTail->sibling = item;
			childTail = item;
		}
		else
		{
			TreeItem* here = children;
			while( --at && here->sibling)
				here = here->sibling;

			item->sibling = here->sibling;
			here->sibling = item;
			if (here == childTail)
				childTail = item;
		}
			
		item->parent = this;
		is_invalid = true;
	
		ChildAdded(item);
	//}
}

// Always deep...
TreeItem* TreeItem::RemoveChild( TreeItem* item)
{
	if ( children != 0)
	{
		// first item in the list
		if ( children == item)
		{
			children = item->sibling;
			item->sibling = 0;
			item->parent = 0;
			
			if ( !is_collapsed)
				ChildRemoved(item);
			
			return item;
		}
		else
		{
			TreeItem* here = children;
			
			do
			{
				TreeItem* next = here->sibling;
				if (next == item)
				{
					here->sibling = item->sibling;
					item->sibling = 0;
					item->parent = 0;
					if ( !is_collapsed)
						ChildRemoved(item);
						
					return item;
				}
				
				// recursive call, deep traverse
				if ( here->RemoveChild(item))
					return item;
					
				here = next;	
			} while(here);
		}
	}
	return 0;
}

// Pass addition notification up the tree...
// calls refresh at the root
void TreeItem::ChildAdded( TreeItem* who)
{
	if ( parent)
	{
		parent->ChildAdded( who);
	}
}

TreeView* TreeItem::Owner()
{
	return parent ? parent->Owner() : 0; 
}


// Pass removal notification up the tree...
void TreeItem::ChildRemoved( TreeItem* who)
{
	if ( parent)
	{
		parent->ChildRemoved( who);
	}
}

int TreeItem::FindSelectedChildren( BList& results)
{
	int count = 0;
	
	TreeItem* here = Children();
	
	while( here)
	{
		if (here->isSelected())
		{
			count++;
			results.AddItem(here);
		}
		
		TreeItem* next = here->sibling;
		count += here->FindSelectedChildren(results);
		here = next;
	}
	
	return count;		
}

void TreeItem::DoForAllChildren( void (TreeItem::*mf)())
{
	TreeItem* here = Children();
	
	while( here)
	{
		(here->*mf)();
		here->DoForAllChildren(mf);
		here = here->sibling;
	}		
}

void TreeItem::DoForAllChildren( void (TreeItem::*mf)( bool), bool p)
{
	TreeItem* here = Children();
	
	while( here)
	{
		(here->*mf)(p);
		here->DoForAllChildren(mf, p);
		here = here->sibling;
	}		
}

void TreeItem::DoForAllChildren( void (TreeItem::*mf)(TreeView*), TreeView* p)
{
	TreeItem* here = Children();
	
	while( here)
	{
		(here->*mf)(p);
		here->DoForAllChildren(mf, p);
		here = here->sibling;
	}		
}

TreeItem* TreeItem::FindChild( bool (TreeItem::*mf)() const)
{
	TreeItem* here = Children();
	
	while( here)
	{
		if ( (here->*mf)())
			return here;
		
		TreeItem* found = here->FindChild(mf);
			
		if( found)
			return found;
			
		here = here->sibling;
	}
	
	return 0;		
}


void TreeItem::Collapse()
{
	if (!is_collapsed)
	{
		is_collapsed = true;
		is_invalid = true;
	}

	// DoForAllChildren(&TreeItem::Select, FALSE);
}

void TreeItem::Explode()
{
	if ( is_collapsed)
	{
		is_collapsed = false;
		is_invalid = true;
	}
}

bool TreeItem::HasChildren() const
{
	return (children != 0);
}		
		
void TreeItem::Select( bool select)
{
	if ( select != is_selected)
	{ 
		is_selected = select;
		is_invalid = true;
		PRINT(("selecting %s\n",Label()));
	}
}

void TreeItem::UserSelect(bool select)
{
	if ( select != is_selected)
	{ 
		is_selected = select;
		is_invalid = true;
		PRINT(("user selecting %s\n",Label()));
	}
}

void TreeItem::SetPosition( BPoint newpos)
{
	position = newpos;
	is_invalid = true;
}

void TreeItem::Invoke()
{
}

void TreeItem::Draw(TreeView* owner, bool deep, BRect *updt)
{
#ifdef MK2
	BPoint pos = Position();
	BRect crect = BRect(pos.x,pos.y,pos.x + LONG_MAX,pos.y + Height()-1);
	
	// bad for each!
	BRect viewBounds;
	if ( updt )
		viewBounds = *updt;
	else
		viewBounds = owner->Bounds();
		
	bool draw = crect.Intersects(viewBounds);
#endif

	if (!isHidden()) {
		#ifndef MK2
		if (owner) {
		#endif
			if (HasChildren()) {
				if (is_collapsed) {
					#ifdef MK2
					if (draw)
					#endif
						DrawPlusBox(owner);
				}
				else {
					#ifdef MK2
					if (draw)
					#endif
						DrawMinusBox(owner);
					if (deep) {
						TreeItem* child = children;
						BRect viewBounds = owner->Bounds();
						
						while(child) {
							// Bail if possible
							if (child->Position().y > viewBounds.bottom)
								break;
							
							child->Draw(owner, true, updt);
							child = child->sibling;
						}
					} 
				}
			}
			#ifdef MK2
			if (draw)
			{
			#endif
				DrawIcon(owner);
				DrawLabel(owner);
			
				PRINT(("draw item\n"));	
			#ifdef MK2
			}
			#endif
			is_invalid = false;
		#ifndef MK2
		}
		else {
//			::LogPuts( "DrawItem", LOGGER_WARNING, "Draw on ownerless child!"); 	 		
		}
		#endif
	}
}

// Conditional redraw of this item (should be called
// after item level changes)
void TreeItem::Invalidate( TreeView* owner)
{
	// Only of interest if I have an owner and I am not hidden
	if ( owner && is_invalid && !isHidden())
	{
#ifdef MK2
		BRect inv_rect( 0.0, 
		                position.y, 
		                position.x + 2000.0, 
		                position.y + Height() - 1);	
#else
		BRect inv_rect( position.x, 
		                position.y, 
		                position.x + 2000.0, 
		                position.y + Height() - 1);	
#endif
		owner->Invalidate( inv_rect);
	}
}
		
void TreeItem::DrawPlusBox( TreeView* owner)
{
	BPoint center( position.x + BOX_INDENT, position.y + Height()/2.0);

	owner->FillEllipse(center, HALF, HALF);
}

void TreeItem::DrawMinusBox( TreeView* owner)
{
	BPoint center( position.x + BOX_INDENT, position.y + Height()/2.0);

	owner->FillEllipse(center, HALF - 1, HALF - 1, B_SOLID_LOW);
	owner->StrokeEllipse(center, HALF, HALF);
}

void TreeItem::DrawLabel( TreeView* owner)
{
	if (label) {
		// I hate doing these three calls every time!
		owner->SetFont(be_plain_font);
		//owner->SetFontSize(9);	

		font_height info;
		be_plain_font->GetHeight(&info);

		float text_offset = (Height() - info.ascent)/2.0 + info.ascent;

		BPoint location(position);
		float h = Height();

		location.x += owner->ItemIndent() + h/2.0 + BOX_INDENT + 4;
		location.y += text_offset;

		if ( is_selected) {
			BRect text_rect(
				location.x-1,
				location.y - (info.ascent + info.leading),
				location.x + owner->StringWidth(label),
				location.y + info.descent
			);

			owner->FillRect(text_rect);

			rgb_color hilite = {255, 255, 255, 0};
			rgb_color oldcolor = owner->HighColor();
			owner->SetHighColor(hilite);	

			owner->SetDrawingMode(B_OP_OVER);	
			owner->DrawString(label, location);
			owner->SetDrawingMode(B_OP_COPY);
			owner->SetHighColor(oldcolor);
		}
		else
			owner->DrawString(label, location);
	}
}

void TreeItem::DrawIcon( TreeView* owner)
{
	if (bitmap)
	{
		BPoint location( position);
		BRect br = bitmap->Bounds();
		float height = Height();
		
		location.x += owner->ItemIndent() + BOX_INDENT - (height / 2);

		// Adjust to center bitmap
		location.x += (height - br.Width()) / 2.0;
		location.y += (height - br.Height()) / 2.0;

		if (is_selected && hilitedBitmap)
			owner->DrawBitmapAsync(hilitedBitmap, location);
		else
			owner->DrawBitmapAsync(bitmap, location);
	}
}

int TreeItem::HitTest( TreeView* owner, BPoint where)
{
	// Perform simple reject
	if ((where.y < position.y) 
	  || (where.y > (position.y + Height()))
//	  || (where.x < position.x)
//	  || (where.x > position.x + owner->ItemIndent() + Height()/2.0 + BOX_INDENT + 4 + owner->TView()->StringWidth(Label()))
	   )
		return NOHIT;

	if (HasChildren()) {
		float edge = (position.x + owner->ItemIndent() + BOX_INDENT) - (Height() / 2.0);

		// Anything between position and indent - halfheight is considered the box...
		if ( (where.x > position.x) && (where.x < edge))
			return PLUSMINUS_BOX;
	}
	
	// Anything else must be a text box hit
	return LABEL_BOX;
} 

TreeItem* TreeItem::ItemAt(BPoint p)
{
	// Is point over my rectangle?
	if ((p.y > position.y) && (p.y < (position.y + Height())))
		return this;

	if (!is_collapsed && HasChildren()) {
		// Spin over children and hittest them
		TreeItem* child = children;

		while(child) {
			TreeItem* item = child->ItemAt(p);

			if ( item)
				return item;
			
			child = child->sibling;
		}	
	}
	
	return 0;
}		

bool TreeItem::isHidden() const
{
	TreeItem* aparent = parent;
	
	while(aparent) {
		if (aparent->is_collapsed)
			return TRUE;
			
		aparent = aparent->parent;
	}
	
	return FALSE; 	
}

TreeItem *TreeItem::Successor()
{
	// doesn't deal well if node itself is hidden
	
	// first comes expanded children
	if (children && !isCollapsed())
		return children;
		
	// then siblings
	if (sibling)
		return sibling;

	// must check the parents
	TreeItem *ancestor = parent;
	while (ancestor) {
		TreeItem *candidate = ancestor->sibling;
		if (candidate)
			return candidate;
		ancestor = ancestor->parent;
	}

	return NULL;
}

void TreeItem::AnimateBox(TreeView *owner, bool opening)
{
	owner;
	opening;
}
