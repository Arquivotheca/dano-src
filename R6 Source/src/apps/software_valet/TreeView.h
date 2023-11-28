#ifndef _TREEVIEW_H_
#define _TREEVIEW_H_

#include <Control.h>

class TreeView;
class TreeRoot;
 
////////////////////////////////////////////////////////////////////////////
// Base item that can be added to a TreeView
// TreeView assumes all it's objects look like this
//
// You can derive from this to get notifications, add your own data
// and overide behaviours.  We do it this way so that trees that
// contain different types of items do not have to spend time
// figuring which item is current to control different behaviours.
//
// Although the TreeView knows which items are in it, TreeItems have
// no knowledge of whether, or which, TreeView they are in.  Therefore
// several of the methods applying to a TreeItem take a TreeView as a
// parameter.
// 
////////////////////////////////////////////////////////////////////////////

// mods by MK 2/19/97
#define MK2

class TreeItem
{
	public:
		/////////////////////////////////////////////////////////////
		// Constructors and destructors
		/////////////////////////////////////////////////////////////
		TreeItem();
		TreeItem( const char* name,  // Copied in
				  BBitmap* bitmap    // NOT COPIED, HELD BY REFERENCE!
		        );
		        
		virtual ~TreeItem();

		// Delete all my children...
		void DeleteChildren();
		
		/////////////////////////////////////////////////////////////
		// The next group of functions manipulate the tree
		// None of them copy or delete the items concerned
		// *** Items are held by reference ***
		/////////////////////////////////////////////////////////////
		// Various ways to Add, added items MUST NOT have siblings
		// before the addition!
		virtual void AddChild( TreeItem* child, int at = INT_MAX);
		virtual void AddSibling( TreeItem* child);
		void AddFirstChild( TreeItem* child) { AddChild( child, 0); }
		
		// Remove may return 0 if item not found
		TreeItem* RemoveChild( TreeItem*);
		
		/////////////////////////////////////////////////////////////
		// These let you navigate down the tree
		// We assume all items potentially have children
		/////////////////////////////////////////////////////////////
		TreeItem* Sibling() { return sibling; }
		TreeItem* Children() { return children; }
		// returns the next item visible in the view in a top down fashion
		// return null if there is no successor i.e. end of the list
		TreeItem* Successor();
		
		/////////////////////////////////////////////////////////////
		// Method iterators over children...
		// I want to apply a TreeItem method to all
		// connected children (preorder).
		/////////////////////////////////////////////////////////////
		void DoForAllChildren( void (TreeItem::*mf)()); 
		void DoForAllChildren( void (TreeItem::*mf)(bool), bool p); 
		void DoForAllChildren( void (TreeItem::*mf)(TreeView*), TreeView* p);
		
		// Apply a predicate to all children, stops after first one
		// to return TRUE and returns a pointer to that child
		TreeItem* FindChild( bool (TreeItem::*mf)() const); 
		
		/////////////////////////////////////////////////////////////
		// Set/Get Icon and label
		/////////////////////////////////////////////////////////////
		virtual void SetIcon( BBitmap* newbitmap);
		const BBitmap* Icon() const { return bitmap; }
		const BBitmap* HIcon() const { return hilitedBitmap; }
		void SetLabel( const char* newtext);
		const char* Label() const { return label; }
		
		/////////////////////////////////////////////////////////////
		// Simple queries
		/////////////////////////////////////////////////////////////
		virtual bool HasChildren() const;
		virtual bool CanHaveChildren();
		bool isSelected() const { return is_selected; }
		bool wasSelected() const { return was_selected; }
		const BPoint& Position() const { return position; }
		bool isCollapsed() const { return is_collapsed;}
		bool isHidden() const;
		virtual float Height();	
		bool isInvalid() const { return is_invalid; }
		virtual TreeView* Owner();

		// Add any selected children (go deep) into the provided list
		// does NOT clear the list first
		// returns the number of items added
		int FindSelectedChildren( BList& results);
		
		/////////////////////////////////////////////////////////////
		// Hit tests
		/////////////////////////////////////////////////////////////
		
		// Enumeration for result of HitTest method, the base class
		// only uses the provided values.  A more sophisticated derived
		// class could add to this since "HitTest" returns an "int"
		enum HitResult { NOHIT, PLUSMINUS_BOX, LABEL_BOX };
		
		// Tell me what area in this object is underneath the point
		virtual int HitTest( TreeView* owner, BPoint where);
		 
		// Look for the item under the point provided, search this
		// item, then preorder recursively through all its children
		TreeItem* ItemAt(BPoint where);
		
		/////////////////////////////////////////////////////////////
		// Notifications/state changes
		/////////////////////////////////////////////////////////////
		virtual void Collapse();
		virtual void Explode();
		virtual void Select( bool select);
		virtual void UserSelect( bool select);
		virtual void SetPosition( BPoint newpos);
		virtual void SetWasSelected(bool flag) { was_selected = flag; }
		virtual void Invoke();
		virtual void ChildAdded( TreeItem*);
		virtual void ChildRemoved( TreeItem*);
		
		/////////////////////////////////////////////////////////////
		// Draw item in view provided
		//
		// If "deep" is false this just refreshes this item
		//
		// If "deep" is true this also draws the child line
		// and then recursively calls the same method for
		// any children
		/////////////////////////////////////////////////////////////

		virtual void Draw( TreeView* owner, bool deep, BRect *updt = NULL);

		virtual void AnimateBox(TreeView *owner, bool opening);
		// Conditionally Invalidate this items rectangle
		// Only do this if the item thinks it needs to be
		// redrawn (is_invalid)
		virtual void Invalidate( TreeView*);
		
	protected:
		const float BOX_WIDTH;
		const float HALF;
		const float BOX_INDENT;

		/////////////////////////////////////////////////////////////
		// Building blocks for Draw
		/////////////////////////////////////////////////////////////
		virtual void DrawPlusBox( TreeView* owner);
		virtual void DrawMinusBox( TreeView* owner);
		virtual void DrawIcon( TreeView* owner);
		virtual void DrawLabel( TreeView* owner);

		BBitmap* bitmap;  // bitmap to use for this item
		BBitmap* hilitedBitmap;  // hilited bitmap to use for this item
		
	private:
		/////////////////////////////////////////////////////////////
		// Items cannnot be assigned or copied! Why can't Be do this?
		/////////////////////////////////////////////////////////////
		TreeItem( const TreeItem&);
		TreeItem& operator=(const TreeItem&);
		
		
		/////////////////////////////////////////////////////////////
		// Items are linked via these next three
		/////////////////////////////////////////////////////////////
		TreeItem* sibling;
		TreeItem* children;
		// for fast addition of children at the end
		TreeItem* childTail;
		TreeItem* parent;
				
		/////////////////////////////////////////////////////////////
		// State variables for items
		/////////////////////////////////////////////////////////////
		bool is_selected;
		bool was_selected; // For handling selecting w/ modifier keys
		bool is_collapsed;
		bool is_invalid;
		
		/////////////////////////////////////////////////////////////
		// Other variables
		/////////////////////////////////////////////////////////////
		BPoint position;  // top-right of my rectangle
		char* label;      // text label for this item
};


////////////////////////////////////////////////////////////////////////////
// TreeView, a type of BControl
//
// This view displays a tree structured list, it contains TreeItems
//
// This is a type of BControl and it intended to be placed as a child
// of another view in much the same way as any control.
//
// Because a TreeView is intended to be able to hold lots of different types
// of TreeItems, it will be normal to not have to make a further derivation
// from this.  Look at TreeItem to see what you can do at the Item level.
//
////////////////////////////////////////////////////////////////////////////
class TreeView : public BControl
{
	public:
		// Constructor like any other BControl...
		TreeView( BRect frame, 
				  const char* name,
				  const char* label,
				  BMessage* message,
				  ulong resizing_mode,
				  ulong flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE
		);
		
		// Destructor which WILL NOT destroy any items this list refers to.
		//
		// To delete all the items refer to "Purge"
		virtual ~TreeView();
		
		// Get rid of all items (recursively) AND DELETE THEM
		virtual void Purge();
		
		virtual void Draw(BRect updaterect);
		
		virtual void MouseDown(BPoint);
	    virtual void FrameResized(float width, float height);
		
		/////////////////////////////////////////////////////////////
		// Add or remove items to/from top level of tree (yes, a tree
		// can have multiple roots).  To add/remove to lower levels
		// use methods on ListItems
		//
		// Neither of these next two copy or delete the item
		// All items are held by reference
		/////////////////////////////////////////////////////////////
		virtual void AddItem( TreeItem*);
		virtual TreeItem* RemoveItem( TreeItem* item);

		/////////////////////////////////////////////////////////////
		// Get at the root of the list
		// The root is an invisible item and should never be
		// deleted.
		// iterate over the roots siblings
		// or down into its children to get at the other children...
		// See TreeItem for those methods
		/////////////////////////////////////////////////////////////
		TreeItem* Root() { return (TreeItem*)root; }
		
		// Open up the whole tree, I wanna see it all
		virtual void ExplodeAll();
		
		// Select or deselect everything
		virtual void SelectAll( bool select);
		
		// Find the current (or focus) item.  This may have nothing to
		// do with selection, although this is often updated as selection
		// precedes.  The current item may be 0.  
		TreeItem* Current();
		virtual void SetCurrent( TreeItem* item);
		
		// Selection may be multiple, you can get a list of selected
		// items, test to see if any are selected (the test is a
		// little cheaper and may be useful to enable/disable other
		// controls, or get the first selected.
		//
		// When getting a list, the list is cleared on entry.  Be careful
		// since the list may become invalidated by other operations
		void GetSelected( BList&);
		TreeItem* GetSelected();
		bool AnySelected();

		// Control selection mode... single select means 0 or 1 can be
		// selected, multiple select means 0 or more can be selected.
		enum { SINGLE_SELECT, MULTIPLE_SELECT };	
		virtual void SetSelectionMode( int mode);
		int SelectionMode() { return selection_mode; }
		
		// When you change the list via purely item level operations
		// so the list is not aware of this, you better call this
		// to make sure the display gets updated
		// Set the flag to TRUE to get everything updated, FALSE
		// if the list layout was unaltered (selection,labels,etc.)
		virtual void Refresh( bool refresh_all);
		
		/////////////////////////////////////////////////////////////
		// Control geometry of tree
		// All the list itself controls is the amount of indentation
		// to give each item.  All the rest of the geometry is under
		// control of the TreeItem
		/////////////////////////////////////////////////////////////
		virtual float ItemIndent();
		virtual void SetItemIndent( float i);
		
		void SetScrollBars(BScrollBar* h, BScrollBar* v);
		virtual void RecalculatePositions();

	protected:
		// Used to pass off mouse clicks from child view
		void ClickAt( BPoint, long modifiers, long buttons, long clicks);

		void DoSelecting(BPoint, long modifiers);

		// Recalculate the scrollbars, current graphics view width
		// and height are passed in
		void RecalculateScrollbars( float width, float height);
		
		/////////////////////////////////////////////////////////////
		// Draw the whole list, normally only called
		// internally... you may want to override this
		/////////////////////////////////////////////////////////////
		// virtual void Redraw();

		#ifndef MK2		
		virtual void Redraw(BRect updt);
		#endif
		
		// Next two are used after a change that potentially causes
		// items to move around (addition/removal/expode...)
		virtual BPoint RePosChildren( TreeItem* item, BPoint where);
			
		virtual void AllAttached();
		
	private:
		// Guess what these do
		int selection_mode;
		BScrollBar* vscroll;
		BScrollBar* hscroll;		
		float item_indent;
		TreeRoot* root;		
		TreeItem* current;
		float dataheight;
		float datawidth;			
};

#endif
