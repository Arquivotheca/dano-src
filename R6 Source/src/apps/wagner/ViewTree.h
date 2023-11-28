// ViewTree.h -- Class for iterating through a window's views
// by Allen Brunson  March 8, 2001

#ifndef _VIEWTREE_H                                // If file not defined
#define _VIEWTREE_H                                // Start it now

#include <View.h>
#include <Window.h>


/****************************************************************************/
/*                                                                          */
/***  ViewTree class                                                      ***/
/*                                                                          */
/****************************************************************************/

class ViewTree                                     // Begin ViewTree
  {
    //*
    //***  Private data
    //*
    
    private:
    
    enum GENERAL                                   // General defines
      {
        listSize = 20,                             // Default view list size
      };
      
    struct VIEWINFO                                // Info on one view
      {
        BRect   rect;                              // Frame rect
        BView*  view;                              // View it belongs to
      };  
      
    BView*    fCurrent;                            // The current view
    BList     fViewList;                           // Master view list
    BWindow*  fWindow;                             // Our window
    bool      fWrap;                               // Wrap around at the end?
    
    
    //*
    //***  Public function prototypes
    //*
    
    public:
    
                   ViewTree(BWindow* window,       // Constructor
                    BView* current = NULL,
                    bool wrap = true);
                    
    virtual        ~ViewTree(void);                // Destructor
                   
    BView*         Current(void);                  // Gets current view
    BView*         Next(void);                     // To next view
    BView*         Prev(void);                     // To previous view
    void           SetCurrent(BView* view);        // Reset current view

    
    //*
    //***  Private function prototypes
    //*
    
    private:
    
    static int     CompareViews(const void* item1, // Comparison for sort
                    const void* item2);
                    
    BView*         FirstChild(BView* view);        // Gets first child
    BView*         FirstViewInWindow(void);        // The very first one
    
    static bool    IsNavigable(BView* view);       // Is this view navigable?
    
    BView*         LastChild(BView* view);         // Last child of view
    BView*         LastView(BView* view);          // Last descendant
    BView*         LastViewInWindow(void);         // The very last one
    
    int32          ListChildren(BView* view,       // All children, sorted
                    BList* list);
    static void    ListClear(BList* list);         // Removes all list items
    static int32   ListIndex(BView* view,          // Which item is this?
                    BList* list);
    int32          ListSibling(BView* view,        // All siblings, sorted
                    BList* list);
    int32          ListWindow(BList* list);        // Window's kids, sorted
    
    VIEWINFO*      MakeInfo(BView* view);          // Creates VIEWINFO struct
    
    BView*         NextSibling(BView* view);       // Next positional sibling
    BView*         PrevSibling(BView* view);       // Prev positional sibling
                    
    static int32   Sort(BList* list);              // Sorts view list
  };                                               // End ViewTree
  
  
/****************************************************************************/
/*                                                                          */
/***  ViewTree class                                                      ***/
/*                                                                          */
/****************************************************************************


Overview
--------

This class was inspired by BTraverseViews.  The thing that didn't work for
me in that particular implementation was that it goes to the next or
previous view based on the order in which the views were added to the window.
This one orders views based on their position in the window.

*/

#endif                                             // End _VIEWTREE_H
