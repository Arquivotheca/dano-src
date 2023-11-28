// ViewTree.cpp -- Class for iterating through a window's views
// by Allen Brunson  March 8, 2001

#include <stdlib.h>
#include <Debug.h>
#include <View.h>
#include <Window.h>
#include "ViewTree.h"

// Un-comment the following define to get printf() output
// #define STUPID_BUSINESS_PRACTICES


/****************************************************************************/
/*                                                                          */
/***  ViewTree::ViewTree()                                                ***/
/*                                                                          */
/****************************************************************************

The class constructor.                                                      */

ViewTree::ViewTree(BWindow* window, BView* current,// Begin ViewTree()
 bool wrap): fViewList(listSize)
  {
    ASSERT(window);                                // Can't be NULL
    
    fCurrent = current;                            // Save current view
    fWindow  = window;                             // Save the window
    fWrap    = wrap;                               // Save wrap flag
  }                                                // End ViewTree()
	
	
/****************************************************************************/
/*                                                                          */
/***  ViewTree::~ViewTree()                                               ***/
/*                                                                          */
/****************************************************************************

The class destructor.                                                       */

ViewTree::~ViewTree(void)                          // Begin ~ViewTree()
  {
    ListClear(&fViewList);                         // Remove all entries
  }                                                // End ~ViewTree()
	
	
/****************************************************************************/
/*                                                                          */
/***  ViewTree::CompareViews()                                            ***/
/*                                                                          */
/****************************************************************************

This procedure compares the positions of two views to see which one should
come first in the tab order.  NOTE: If two views have the same upper left
corner -- and it DOES happen -- then you must return a consistent less or
greater response, otherwise the view order can change from one run to the
next, leading to infinite loops.                                            */

int ViewTree::CompareViews(const void* item1,      // Begin CompareViews()
 const void* item2)
  {
    VIEWINFO*  info1 = *(VIEWINFO**) item1;        // First view info
    VIEWINFO*  info2 = *(VIEWINFO**) item2;        // Second view info
    BRect      test1, test2;                       // Test rects

    
    //*
    //***  Verify input conditions
    //*
    
    ASSERT(info1 && info2);                        // Neither should be NULL
    ASSERT(info1 != info2);                        // Shouldn't be the same
      
    
    //*
    //***  If the views overlap vertically, return the farther left one
    //*
    
    test1 = info1->rect;                           // Copy first rect
    test2 = info2->rect;                           // Copy second rect
    
    test1.left  =   0;                             // Set both lefts the same
    test2.left  =   0;
    test1.right = 100;                             // Set both rights the same
    test2.right = 100;
    
    if (test1.Intersects(test2))                   // If vertical overlap
      {
        if (info1->rect.left < info2->rect.left) return -1;
        if (info2->rect.left < info1->rect.left) return 1;
      }

    
    //*
    //***  Now we're down to a simple left-to-right/top-to-bottom compare
    //*
    
    if (info1->rect.top<info2->rect.top) return -1;// If view1 further up
    if (info2->rect.top<info1->rect.top) return 1; // If view2 further up
    
    if (info1->rect.left<info2->rect.left)return -1;// If view1 further left
    if (info2->rect.left<info1->rect.left)return 1;// If view2 further left

    
    //*
    //***  Absolute worst case: compare via BView pointer address
    //*
    
    if (info1->view < info2->view)                 // NEVER return zero or
      return -1;                                   //  VERY BAD THINGS can
    else                                           //  happen
      return 1;  
  }                                                // End CompareViews()
  
  
/****************************************************************************/
/*                                                                          */
/***  ViewTree::Current()                                                 ***/
/*                                                                          */
/****************************************************************************

Gets the current view.                                                      */

BView* ViewTree::Current(void)                     // Begin Current()
  {
    return fCurrent;                               // There you go
  }                                                // End Current()
	
	
/****************************************************************************/
/*                                                                          */
/***  ViewTree::FirstChild()                                              ***/
/*                                                                          */
/****************************************************************************

This procedure finds the first child of a given view in positional order.   */

BView* ViewTree::FirstChild(BView* view)           // Begin FirstChild()
  {
    int32      count;                              // List count
    VIEWINFO*  info;                               // Item from the list
    BList      list(listSize);                     // List of views
    
    if (!view) return NULL;                        // If no view given
    
    count = ListChildren(view, &list);             // Get child list
    if (count <= 0) return NULL;                   // If no children
    
    info = (VIEWINFO*) list.ItemAt(0);             // Get first item
    return info->view;                             // There's yer view
  }                                                // End FirstChild()
  

/****************************************************************************/
/*                                                                          */
/***  ViewTree::FirstViewInWindow()                                       ***/
/*                                                                          */
/****************************************************************************

This procedure finds the very first view in the whole window.               */

BView* ViewTree::FirstViewInWindow(void)           // Beg FirstViewInWindow()
  {
    int32      count;                              // Total views
    VIEWINFO*  info;                               // View info struct
    BList      list(listSize);                     // List of views
    
    count = ListWindow(&list);                     // Get all window views
    if (count <= 0) return NULL;                   // If nothing there
    
    info = (VIEWINFO*) list.ItemAt(0);             // Get first item
    return info->view;                             // There you go
  }                                                // End FirstViewInWindow()
  

/****************************************************************************/
/*                                                                          */
/***  ViewTree::IsNavigable()                                             ***/
/*                                                                          */
/****************************************************************************

This procedure returns true if a BView should be included in tab order
comparisons, false if not.  Note that views that have children must be
included in the tab order but calling BView::CountChildren() is out of the
question because it requires a trip to the app_server, and the whole point
of having this function is to see if we can avoid calling BView::Frame(),
which also requires an app_server trip.  C'est la vie.                      */

bool ViewTree::IsNavigable(BView* view)            // Begin IsNavigable()
  {
    if (view->IsHidden())                          // If hidden
      return false;                                // Don't need it
    else                                           // All other cases
      return true;                                 // We need it
  }                                                // End IsNavigable()
  

/****************************************************************************/
/*                                                                          */
/***  ViewTree::LastChild()                                               ***/
/*                                                                          */
/****************************************************************************

This procedure finds the last child of a given view in positional order.    */

BView* ViewTree::LastChild(BView* view)            // Begin LastChild()
  {
    int32      count;                              // List count
    VIEWINFO*  info;                               // View info struct
    BList      list(listSize);                     // List of views
    
    if (!view) return NULL;                        // If no view given
    
    count = ListChildren(view, &list);             // Get child list
    if (count <= 0) return NULL;                   // If no children
    
    info = (VIEWINFO*) list.ItemAt(count - 1);     // Get last view
    return info->view;                             // Return view pointer
  }                                                // End LastChild()
  

/****************************************************************************/
/*                                                                          */
/***  ViewTree::LastView()                                                ***/
/*                                                                          */
/****************************************************************************

This procedure finds the very last descendant of the given view.  If there
are none it will return the view you started with.                          */

BView* ViewTree::LastView(BView* view)             // Begin LastView()
  {
    BView*  next = view;                           // To iterate with
    
    while (next)                                   // Loop til you can't
      {
        view = next;                               // Save this view
        next = LastChild(next);                    // To last child
      }
      
    return view;                                   // Return final view    
  }                                                // End LastView()

	
/****************************************************************************/
/*                                                                          */
/***  ViewTree::LastViewInWindow()                                        ***/
/*                                                                          */
/****************************************************************************

This procedure finds the very last view in the whole window.                */

BView* ViewTree::LastViewInWindow(void)            // Begin LastViewInWindow()
  {
    int32      count;                              // View count
    VIEWINFO*  info;                               // Info struct from list
    BList      list(listSize);                     // View list
    
    count = ListWindow(&list);                     // Get direct descendents
    if (count <= 0) return NULL;                   // If no kids
    
    info = (VIEWINFO*) list.ItemAt(count - 1);     // Take the last one
    return LastView(info->view);                   // Get its last descendant
  }                                                // End LastViewInWindow()
  

/****************************************************************************/
/*                                                                          */
/***  ViewTree::ListChildren()                                            ***/
/*                                                                          */
/****************************************************************************

This procedure creates a list of all views that are direct descendants of
a particular view and sorts them into positional order.                     */

int32 ViewTree::ListChildren(BView* view,          // Begin ListChildren()
 BList* list)
  {
    BView*     child;                              // Child view
    int32      count;                              // Total view count
    int32      i;                                  // Loop counter
    VIEWINFO*  info;                               // Item to add to list
    
    if (!list) return 0;                           // Exit if no list
    if (!view) return 0;                           // Exit if no view
    
    count = view->CountChildren();                 // Get view count
    
    for (i = 0; i < count; i++)                    // Loop the list
      {
        child = view->ChildAt(i);                  // Get next kid
        if (!child) {ASSERT(false); continue;}     // It should work
        
        info = MakeInfo(child);                    // Create new item
        if (!info) break;                          // Stop on failure
        
        list->AddItem(info);                       // In you go
      }
      
    return Sort(list);                             // Shuffle them
  }                                                // End ListChildren()
	
	
/****************************************************************************/
/*                                                                          */
/***  ViewTree::ListClear()                                               ***/
/*                                                                          */
/****************************************************************************

This procedure removes all VIEWINFO structs from a list and deletes them.   */

void ViewTree::ListClear(BList* list)              // Begin ListClear()
  {
    VIEWINFO*  info;                               // Info pointer
    
    if (!list) return;                             // If no list given
    
    while (true)                                   // Loop to remove
      {
        info = (VIEWINFO*) list->ItemAt(0);        // Get first item
        if (!info) break;                          // Stop at list end
        
        delete info;                               // Kill it
        list->RemoveItem((int32)0);                // Remove from list
      }
  }                                                // End ListClear()
	
	
/****************************************************************************/
/*                                                                          */
/***  ViewTree::ListIndex()                                               ***/
/*                                                                          */
/****************************************************************************

Given a view and a list of views this procedure will give you the view's
index.  If the view is not in the list it will return a negative number.    */

int32 ViewTree::ListIndex(BView* view, BList* list)// Begin ListIndex()
  {
    int32      count;                              // Total items
    int32      i;                                  // Loop counter
    VIEWINFO*  next;                               // Next view from list
    
    if (!list) return -1;                          // If no list
    if (!view) return -1;                          // If no view
    
    count = list->CountItems();                    // Get total items
    
    for (i = 0; i < count; i++)                    // Loop the list
      {
        next = (VIEWINFO*) list->ItemAt(i);        // Get next item
        if (!next) {ASSERT(false); continue;}      // It should work
        if (next->view == view) return i;          // If it's a match
      }
      
    return -1;                                     // Didn't find it    
  }                                                // End ListIndex()
	
	
/****************************************************************************/
/*                                                                          */
/***  ViewTree::ListSibling()                                             ***/
/*                                                                          */
/****************************************************************************

This procedure creates a list of all views that are siblings of the given
view, including the given view itself, and sorts them into positional
order.                                                                      */

int32 ViewTree::ListSibling(BView* view,           // Begin ListSibling()
 BList* list)
  {
    VIEWINFO*  info;                               // Item for the list
    BView*     next;                               // Next sibling
    
    if (!list) return 0;                           // Exit if no list
    if (!view) return 0;                           // Exit if no view
    
    info = MakeInfo(view);                         // Create first item
    if (!info) return 0;                           // Exit on failure
    
    list->AddItem(info);                           // Add this view
    
    for (next = view; ; )                          // Loop towards the front
      {
        next = next->PreviousSibling();            // Get prev sibling
        if (!next) break;                          // Stop at the beginning
        
        info = MakeInfo(next);                     // Create info struct
        if (!info) goto end;                       // Exit on failure
        
        list->AddItem(info);                       // Stick it in
      }
      
    for (next = view; ; )                          // Loop towards the back
      {
        next = next->NextSibling();                // Get next sibling
        if (!next) break;                          // Stop at the end
        
        info = MakeInfo(next);                     // Create info struct
        if (!info) goto end;                       // Exit on failure
        
        list->AddItem(info);                       // Stick it in
      }  
      
    end:                                           // Jump here to exit
    return Sort(list);                             // Shuffle them    
  }                                                // End ListSibling()
	
	
/****************************************************************************/
/*                                                                          */
/***  ViewTree::ListWindow()                                              ***/
/*                                                                          */
/****************************************************************************

This procedure creates a list of all views that are direct descendants of
the window and sorts them into positional order.                            */

int32 ViewTree::ListWindow(BList* list)            // Begin ListWindow()
  {
    int32      count;                              // Total view count
    int32      i;                                  // Loop counter
    VIEWINFO*  info;                               // View info struct
    BView*     view;                               // View from the window
    
    if (!list) return 0;                           // If no list given
    if (!fWindow) return 0;                        // If no window
    
    count = fWindow->CountChildren();              // Get view count
    
    for (i = 0; i < count; i++)                    // Loop the list
      {
        view = fWindow->ChildAt(i);                // Get next kid
        if (!view) {ASSERT(false); continue;}      // It should work
        
        info = MakeInfo(view);                     // Create info struct
        if (!info) break;                          // Stop on failure
        
        list->AddItem(info);                       // In you go
      }
      
    return Sort(list);                             // Shuffle them    
  }                                                // End ListWindow()
	
	
/****************************************************************************/
/*                                                                          */
/***  ViewTree::MakeInfo()                                                ***/
/*                                                                          */
/****************************************************************************

This procedure creates a VIEWINFO from a BView pointer to add to a view
list.                                                                       */

ViewTree::VIEWINFO* ViewTree::MakeInfo(BView* view)// Begin MakeInfo()
  {
    int32      count;                              // Items in master list
    int32      i;                                  // Loop counter
    VIEWINFO*  info;                               // Struct to return
    
    if (!view) return NULL;                        // If no view given
    count = fViewList.CountItems();                // Get master count
    
    for (i = 0; i < count; i++)                    // Loop main view list
      {
        info = (VIEWINFO*) fViewList.ItemAt(i);    // Get next item
        if (!info) {ASSERT(false); continue;}      // It should work
        
        if (info->view == view) return info;       // If already in list
      }
    
    info = new VIEWINFO();                         // Create new item
    if (!info) return NULL;                        // Exit on failure
    
    info->view = view;                             // Save view pointer
    info->rect = view->Frame();                    // Save frame rect
    
    fViewList.AddItem(info);                       // Into main list
    return info;                                   // There you go
  }                                                // End MakeInfo()
	
	
/****************************************************************************/
/*                                                                          */
/***  ViewTree::Next()                                                    ***/
/*                                                                          */
/****************************************************************************

Goes to the next view in the heirarchy.                                     */

BView* ViewTree::Next(void)                        // Begin Next()
  {
    BView*  next;                                  // Searching ...
    BView*  parent;                                // Might have to go up
    
    if (!fCurrent)                                 // If no current view
      {
        next = FirstViewInWindow();                // Get first one
        goto end;                                  // Get outta here
      }
      
    next = FirstChild(fCurrent);                   // Try child first
    if (next) goto end;                            // Exit if found
    
    next = NextSibling(fCurrent);                  // Try next sibling
    if (next) goto end;                            // Exit if found
    
    for (parent = fCurrent; !next && parent; )     // Loop to find parent
      {
        parent = parent->Parent();                 // Go up a level
        if (parent) next = NextSibling(parent);    // Get parent's sibling
      }
    
    if (next) goto end;                            // Exit if found
    
    if (fWrap) next = FirstViewInWindow();         // If wrapping, start over
    
    end:                                           // Jump here to exit
    
    #ifdef STUPID_BUSINESS_PRACTICES
    const char*  s1 = "None";                      // First view name
    const char*  s2 = s1;                          // Second view name
    
    if (fCurrent) s1 = fCurrent->Name();           // Get first view name
    if (next) s2 = next->Name();                   // Get next view name
    
    printf("Next: %p %s to %p %s\n", fCurrent, s1, next, s2);
    #endif
    
    if (next) fCurrent = next;                     // One further along
    return next;                                   // Return outcome
  }                                                // End Next()
	
	
/****************************************************************************/
/*                                                                          */
/***  ViewTree::NextSibling()                                             ***/
/*                                                                          */
/****************************************************************************

Goes to the next sibling of the current view in positional order.           */

BView* ViewTree::NextSibling(BView* view)          // Begin NextSibling()
  {
    int32      count;                              // Total siblings
    int32      index;                              // Index in list
    VIEWINFO*  info;                               // Struct from the list
    BList      list(listSize);                     // List of siblings
    
    if (!view) return NULL;                        // If no view
    
    count = ListSibling(view, &list);              // Get sibling list
    ASSERT(count >= 1);                            // Should work
    
    index = ListIndex(view, &list);                // Get this view's index
    if (index < 0) {ASSERT(false); return NULL;}   // Should be in the list
    
    if (index >= (count - 1)) return NULL;         // If at list end
    
    info = (VIEWINFO*) list.ItemAt(index + 1);     // Get next sibling
    return info->view;                             // Return view pointer
  }                                                // End NextSibling()
	
	
/****************************************************************************/
/*                                                                          */
/***  ViewTree::Prev()                                                    ***/
/*                                                                          */
/****************************************************************************

Goes to the previous view in the heirarchy.                                 */

BView* ViewTree::Prev(void)                        // Begin Prev()
  {
    BView*  prev;                                  // View to return
    
    if (!fCurrent)                                 // If nothing to start
      {
        prev = LastViewInWindow();                 // Get the last one
        goto end;                                  // Get outta here
      }
    
    prev = PrevSibling(fCurrent);                  // Try for prev sib
    
    if (prev)                                      // If found
      {
        prev = LastView(prev);                     // All the way down
        goto end;                                  // Done
      }  
    
    prev = fCurrent->Parent();                     // Try parent next
    if (prev) goto end;
    
    if (fWrap) prev = LastViewInWindow();          // If wrap turned on
    
    end:                                           // Jump here to exit
    
    #ifdef STUPID_BUSINESS_PRACTICES
    const char*  s1 = "None";                      // First view name
    const char*  s2 = s1;                          // Second view name
    
    if (fCurrent) s1 = fCurrent->Name();           // Get first view name
    if (prev) s2 = prev->Name();                   // Get second view name
    printf("Prev: %p %s to %p %s\n", fCurrent, s1, prev, s2);
    #endif
    
    if (prev) fCurrent = prev;                     // Update current
    return prev;                                   // There you go
  }                                                // End Prev()
	
	
/****************************************************************************/
/*                                                                          */
/***  ViewTree::PrevSibling()                                             ***/
/*                                                                          */
/****************************************************************************

Goes to the previous sibling of the current view in positional order.       */

BView* ViewTree::PrevSibling(BView* view)          // Begin PrevSibling()
  {
    int32      count;                              // Total siblings
    int32      index;                              // Index in list
    VIEWINFO*  info;                               // Info struct
    BList      list(listSize);                     // List of siblings
    
    if (!view) return NULL;                        // If no view
    
    count = ListSibling(view, &list);              // Get sibling list
    ASSERT(count >= 1);                            // Better be some
    
    index = ListIndex(view, &list);                // Get this view's index
    if (index <= 0) return NULL;                   // Right at front?
    
    info = (VIEWINFO*) list.ItemAt(index - 1);     // Get prev sibling
    return info->view;                             // Return the view
  }                                                // End PrevSibling()
	
	
/****************************************************************************/
/*                                                                          */
/***  ViewTree::SetCurrent()                                              ***/
/*                                                                          */
/****************************************************************************

This procedure allows you to reset the class' idea of the current view.     */

void ViewTree::SetCurrent(BView* current)          // Begin SetCurrent()
  {
    fCurrent = current;
  }                                                // End SetCurrent()
	
	
/****************************************************************************/
/*                                                                          */
/***  ViewTree::Sort()                                                    ***/
/*                                                                          */
/****************************************************************************

This procedure takes a BList of BViews and sorts them into order as they
appear onscreen, upper left views first, lower right views last.            */

int32 ViewTree::Sort(BList* list)                  // Begin Sort()
  {
    int32  count;                                  // Total items
    
    if (!list) return 0;                           // If no list given
    
    count = list->CountItems();                    // Get total views
    if (count <= 1) return count;                  // If no need to sort
    
    qsort(list->Items(), count, sizeof (void*),    // Sort 'em
     CompareViews);
     
    return count;                                  // That's how many 
  }                                                // End Sort()
	
	
