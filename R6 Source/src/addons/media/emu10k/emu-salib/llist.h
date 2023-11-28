//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: llist.h
//
// Author: Michael Preston
//
// Description: Generic linked list package.
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  -----------------------------------------
// Michael Preston     Dec 17, 1996  Moved FuncOrderedLinkedList::Merge() to
//                                   llist.cpp
// Michael Preston     Aug  5, 1996  Changed template functions to macros.
// Michael Preston     Jul 17, 1996  Changed ccons and removed alloc.
// Michael Preston     Jun 26, 1996  Fixed bug with delete.
// Michael Preston     Jun 19, 1996  Removed dest and opdel parameters and
//                                   added dealloc parameter instead.
// Michael Preston     Jun 18, 1996  Added RemoveCurItem().
// Michael Preston     Jun 14, 1996  Provide option to not delete list items.
// Michael Preston     Jun 13, 1996  Initial import to CVS.
// Michael Preston     Jun  7, 1996  Rewrote to remove template code.
// Michael Preston     Mar  1, 1996  Initial development.
//
//*****************************************************************************

#ifndef __LLIST_H
#define __LLIST_H

// Include files

#include "datatype.h"
#include "win_mem.h"

// Enumeration lists

typedef enum {InsertFront, InsertBeforeCurPos, InsertAfterCurPos, InsertEnd}
   InsertLoc;
typedef enum {DupInsertBefore, DupInsertAfter, DupReplace, DupIgnore} DupType;

// Generic functions

#ifndef MAX
#  define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#  define MIN(a, b)(((a) < (b)) ? (a) : (b))
#endif

#ifndef COMP
#  define COMP(x, y) (((x) < (y)) ? -1 : ((x) == (y) ? 0 : 1))
#endif

#ifndef SWAP
#  define SWAP(X, a, b) \
 { X tmp; \
   tmp = a; \
   a = b; \
   b = tmp; }
#endif

class LinkedListItem
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   LinkedListItem(void *newitm) : itm(newitm) {}
   ~LinkedListItem() {}

   void *itm;
   LinkedListItem *next;
};

//*****************************************************************************
//
// Class: LinkedList
//
// Description:
//    Generic linked list class.  This class can be used to navigate
// through a list, and as a base class for other linked lists.  No data
// can be inserted into a list of this class.  Only used for navigation.
//
//*****************************************************************************
class LinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   LinkedList(void (*deallocarg)(void *), BOOL bDeletearg=TRUE)
      {dealloc = deallocarg;
       bDelete = bDeletearg;
       list = pos = last = NULL;
#      ifdef DEBUG_LLIST
          wNumItems = 0;
#      endif
      }
   ~LinkedList() {DeleteAll();}

//*****************************************************************************
//
// Function: First
//
// Description:
//    Set the current position to the first item in the list.  If a
// parameter is specified, sets the current position to the wPos-1
// item in the list (i.e. starts at 0).
//
// Parameters: wPos - position to move to (optional)
//
// Return Value: (none)
//
//*****************************************************************************
   void First() {if (list != NULL) pos = list;}
   void First(int wPos);

//*****************************************************************************
//
// Function: Next
//
// Description:
//    Set the current position to the next item in the list.  If a
// parameter is specified, moves ahead wPos items.
//
// Parameters: wPos - number of items to move ahead (optional)
//
// Return Value: (none)
//
//*****************************************************************************
   void Next()
   {
      if ((pos != NULL) && (pos->next != list))
         pos = pos->next;
      else
         pos = NULL;
   }
   void Next(int wPos);

//*****************************************************************************
//
// Function: Last
//
// Description:
//    Set the current position to the last item in the list.
//
// Parameters: (none)
//
// Return Value: (none)
//
//*****************************************************************************
   void Last()
   {
      if (list != NULL)
         pos = last;
   }

//*****************************************************************************
//
// Function: GetNumItems
//
// Description:
//    Returns the number of items in the list.
//
// Parameters: (none)
//
// Return Value: number of items in the list
//
//*****************************************************************************
   WORD GetNumItems();

//*****************************************************************************
//
// Function: GetCurItem
//
// Description:
//    Return a pointer to the current item in the list.
//
// Parameters: (none)
//
// Return Value: pointer to current item
//
//*****************************************************************************
   void *GetCurItem()
   {
      if (pos != NULL)
         return pos->next->itm;
      else
         return NULL;
   }

//*****************************************************************************
//
// Function: EndOfList
//
// Description:
//    Check to see if the end of the list has been reached.
//
// Parameters: (none)
//
// Return Value: TRUE if at end of list, FALSE otherwise
//
//*****************************************************************************
   BOOL EndOfList()
   {
      return (pos == NULL);
   }

//*****************************************************************************
//
// Function: IsEmpty
//
// Description:
//    Check to see if the list is empty.
//
// Parameters: (none)
//
// Return Value: TRUE if list is empty, FALSE otherwise
//
//*****************************************************************************
   BOOL IsEmpty() const
   {
      return (list == NULL);
   }

   protected:

   void DeleteAll();

   void (*dealloc)(void *);
   BOOL bDelete;
   LinkedListItem *list;
   LinkedListItem *pos;
   LinkedListItem *last;

#  ifdef DEBUG_LLIST
      WORD wNumItems;
#  endif
};

//*****************************************************************************
//
// Class: UnorderedLinkedList
//
// Description:
//    Generic unordered linked list.  Same as LinkedList, but data can be
// inserted, deleted, etc.
//
//*****************************************************************************
class UnorderedLinkedList : protected LinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   UnorderedLinkedList(void (*deallocarg)(void *),
                       void *(*cconsarg)(void *),
                       BOOL bDeletearg=TRUE) :
      LinkedList(deallocarg, bDeletearg)
      {ccons = cconsarg;}
   UnorderedLinkedList(const UnorderedLinkedList& l);

//*****************************************************************************
//
// Function: operator+=
//
// Description:
//    Add a copy of a linked list.
//
// Parameters: l - linked list to add
//
// Return Value: Reference to linked list
//
//*****************************************************************************
   UnorderedLinkedList& operator+=(const UnorderedLinkedList& l);

//*****************************************************************************
//
// Function: operator=
//
// Description:
//    Replace the linked list with a copy of another linked list.
//
// Parameters: l - linked list to assign
//
// Return Value: Reference to linked list
//
//*****************************************************************************
   UnorderedLinkedList& operator=(const UnorderedLinkedList& l);

//*****************************************************************************
//
// Function: operator+
//
// Description:
//    Combine copies of two linked lists.
//
// Parameters: l - linked list to add
//
// Return Value: Combined linked list
//
//*****************************************************************************
   UnorderedLinkedList operator+(const UnorderedLinkedList& l);

//*****************************************************************************
//
// Function: Swap
//
// Description:
//    Swap the linked list with another linked list.
//
// Parameters: l - linked list to swap with
//
// Return Value: (none)
//
//*****************************************************************************
   void Swap(UnorderedLinkedList& l);

//*****************************************************************************
//
// Function: Copy
//
// Description:
//    Creates a copy of the linked list.
//
// Parameters: (none)
//
// Return Value: copy of linked list
//
//*****************************************************************************
   UnorderedLinkedList Copy() {return *this;}

//*****************************************************************************
//
// Function: Merge
//
// Description:
//    Merges the linked list with another linked list.  The list being merged
// is returned empty.
//
// Parameters: l - linked list to merge with
//
// Return Value: (none)
//
//*****************************************************************************
   void Merge(UnorderedLinkedList& l);

//*****************************************************************************
//
// Function: Insert
//
// Description:
//    Insert a pointer to a new item into the linked list.
//
// Parameters: newitm - item to insert
//             loc - location to insert
//
// Return Value: (none)
//
//*****************************************************************************
   void Insert(void *newitm, InsertLoc loc=InsertEnd);

//*****************************************************************************
//
// Function: DeleteCurItem
//
// Description:
//    Deletes the current item in the linked list.
//
// Parameters: (none)
//
// Return Value: (none)
//
//*****************************************************************************
   void DeleteCurItem();

//*****************************************************************************
//
// Function: RemoveCurItem
//
// Description:
//    Return a pointer to the current item in the list and remove it from the
// list.
//
// Parameters: (none)
//
// Return Value: pointer to current item
//
//*****************************************************************************
   void *RemoveCurItem()
   {
      void *tmpitm;

//      if ((pos != NULL) && bDelete)
      if (pos != NULL)
      {
         tmpitm = pos->next->itm;
         pos->next->itm = NULL;
         DeleteCurItem();
         return tmpitm;
      }
      else
         return NULL;
   }

//*****************************************************************************
//
// Function: DeleteList
//
// Description:
//    Deletes all the items in the linked list.
//
// Parameters: (none)
//
// Return Value: (none)
//
//*****************************************************************************
   void DeleteList() {DeleteAll(); pos = last = list = NULL;}

   void First() {LinkedList::First();}
   void First(int wPos) {LinkedList::First(wPos);}
   void Next() {LinkedList::Next();}
   void Next(int wPos) {LinkedList::Next(wPos);}
   void Last() {LinkedList::Last();}
   WORD GetNumItems() {return LinkedList::GetNumItems();}
   void* GetCurItem() {return LinkedList::GetCurItem();}
   BOOL EndOfList() {return LinkedList::EndOfList();}
   BOOL IsEmpty() {return LinkedList::IsEmpty();}
   BOOL IsEmpty() const {return LinkedList::IsEmpty();}

   protected:

   void *(*ccons)(void *);
};

//*****************************************************************************
//
// Class: FuncOrderedLinkedList
//
// Description:
//    Generic linked list ordered by a comparison function.  The comparison
// function must take two items and return -1, 0, or 1 if the first item is
// less than, equal to, or greater than the second item, respectively.  If
// a duplicate item handling parameter is not specified, the default is
// DupInsert.  Duplicate item handling: DupInsert - always insert into list
// even if it is a duplicate, DupReplace - replace duplicates with the one
// being inserted, DupIgnore - ignore duplicates being inserted.
// 
//*****************************************************************************
class FuncOrderedLinkedList : protected UnorderedLinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   typedef int (*ordertype)(void *, void *);

   FuncOrderedLinkedList(void (*deallocarg)(void *),
                         void *(*cconsarg)(void *), ordertype order,
                         DupType dup=DupInsertAfter, BOOL bDeletearg=TRUE) :
      UnorderedLinkedList(deallocarg, cconsarg, bDeletearg)
      {sortorder = order; sortdup = dup;}
   FuncOrderedLinkedList(const FuncOrderedLinkedList& l) :
      UnorderedLinkedList(l.dealloc, l.ccons)
   {
      sortdup = l.sortdup;
      sortorder = l.sortorder;
      (*(UnorderedLinkedList *)this) = l;
   }

//*****************************************************************************
//
// Function: operator+=
//
// Description:
//    Add a copy of a linked list.
//
// Parameters: l - linked list to add
//
// Return Value: Reference to linked list
//
//*****************************************************************************
   FuncOrderedLinkedList& operator+=(const FuncOrderedLinkedList& l);

//*****************************************************************************
//
// Function: operator=
//
// Description:
//    Replace the linked list with a copy of another linked list.
//
// Parameters: l - linked list to assign
//
// Return Value: Reference to linked list
//
//*****************************************************************************
   FuncOrderedLinkedList& operator=(const FuncOrderedLinkedList& l)
   {
      sortdup = l.sortdup;
      sortorder = l.sortorder;
      (*(UnorderedLinkedList *)this) = l;
      return *this;
   }

//*****************************************************************************
//
// Function: operator+
//
// Description:
//    Combine copies of two linked lists.
//
// Parameters: l - linked list to add
//
// Return Value: Combined linked list
//
//*****************************************************************************
   FuncOrderedLinkedList operator+(const FuncOrderedLinkedList& l)
   {
      FuncOrderedLinkedList tmp(dealloc, ccons, sortorder);

      tmp = *this;
      tmp += l;
      return tmp;
   }

//*****************************************************************************
//
// Function: Swap
//
// Description:
//    Swap the linked list with another linked list.
//
// Parameters: l - linked list to swap with
//
// Return Value: (none)
//
//*****************************************************************************
   void Swap(FuncOrderedLinkedList& l);

//*****************************************************************************
//
// Function: Copy
//
// Description:
//    Creates a copy of the linked list.
//
// Parameters: (none)
//
// Return Value: copy of linked list
//
//*****************************************************************************
   FuncOrderedLinkedList Copy() {return *this;}

//*****************************************************************************
//
// Function: Merge
//
// Description:
//    Merges the linked list with another linked list.  The list being merged
// is returned empty.
//
// Parameters: l - linked list to merge with
//
// Return Value: (none)
//
//*****************************************************************************
   void Merge(FuncOrderedLinkedList& l);

//*****************************************************************************
//
// Function: Insert
//
// Description:
//    Insert a pointer to a new item into the linked list.  Data is
// automatically inserted at the proper location to keep the linked list
// ordered.
//
// Parameters: newitm - item to insert
//
// Return Value: (none)
//
//*****************************************************************************
   void Insert(void *newitm);

//*****************************************************************************
//
// Function: DeleteCurItem
//
// Description:
//    Deletes the current item in the linked list.
//
// Parameters: (none)
//
// Return Value: (none)
//
//*****************************************************************************
   void DeleteCurItem() {UnorderedLinkedList::DeleteCurItem();}

//*****************************************************************************
//
// Function: RemoveCurItem
//
// Description:
//    Return a pointer to the current item in the list and remove it from the
// list.
//
// Parameters: (none)
//
// Return Value: pointer to current item
//
//*****************************************************************************
   void *RemoveCurItem() {return UnorderedLinkedList::RemoveCurItem();}

//*****************************************************************************
//
// Function: DeleteList
//
// Description:
//    Deletes all the items in the linked list.
//
// Parameters: (none)
//
// Return Value: (none)
//
//*****************************************************************************
   void DeleteList() {UnorderedLinkedList::DeleteList();}

   void First() {UnorderedLinkedList::First();}
   void First(int wPos) {UnorderedLinkedList::First(wPos);}
   void Next() {UnorderedLinkedList::Next();}
   void Next(int wPos) {UnorderedLinkedList::Next(wPos);}
   void Last() {UnorderedLinkedList::Last();}
   WORD GetNumItems() {return UnorderedLinkedList::GetNumItems();}
   void* GetCurItem() {return UnorderedLinkedList::GetCurItem();}
   BOOL EndOfList() {return UnorderedLinkedList::EndOfList();}
   BOOL IsEmpty() {return UnorderedLinkedList::IsEmpty();}
   BOOL IsEmpty() const {return UnorderedLinkedList::IsEmpty();}

//*****************************************************************************
//
// Function: SetOrder
//
// Description:
//    Switch to a new comparison function.  The linked list is automatically
// resorted.  If a new duplicate item handling parameter is not specified,
// the current one is used.
//
// Parameters: order - new comparison function to order the linked list with
//             dup - duplicate item handling
//
// Return Value: (none)
//
//*****************************************************************************
   void SetOrder(ordertype order)
      {sortorder = order; Sort();}
   void SetOrder(ordertype order, DupType dup)
      {sortorder = order; sortdup = dup; Sort();}

//*****************************************************************************
//
// Function: SetDuplicates
//
// Description:
//    Changes the duplicate item handing for the linked list.
//
// Parameters: dup - duplicate item handling
//
// Return Value: (none)
//
//*****************************************************************************
   void SetDuplicates(DupType dup)
      {if (((sortdup == DupInsertBefore) || (sortdup == DupInsertAfter)) &&
           ((dup == DupReplace) || (dup == DupIgnore)))
       {
          sortdup = dup;
          Sort();
       }
       else
          sortdup = dup;}

   private:

   void Sort();

   DupType sortdup;
   ordertype sortorder;
};

//*****************************************************************************
//
// Class: FindFuncClass
//
// Description:
//    Generic search functions to find items in a linked list by using a
// comparison function.  The comparison function must take an item and a
// match item and return 1 if they match, and 0 if they dont.
//
//*****************************************************************************
class FindFuncClass
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

//*****************************************************************************
//
// Function: Find
//
// Description:
//    Find an item in a linked list.  The current position of the list is set
// to the location of the item found.  If the item is not in the list, the
// current position of the list will be at the end of the list.
//
// Parameters: l - linked list to search
//             itm - match item to search for
//             match - comparison function
//
// Return Value: (none)
//
//*****************************************************************************
   static void Find(LinkedList& l, void *itm, int (*match)(void *, void *))
      {l.First(); FindMain(l, itm, match);}

//*****************************************************************************
//
// Function: FindNext
//
// Description:
//    Find the next item in a linked list.  The current position of the list
// is set to the location of the item found.  If the item is not in the list,
// the current position of the list will be at the end of the list.
//
// Parameters: l - linked list to search
//             itm - match item to search for
//             match - comparison function
//
// Return Value: (none)
//
//*****************************************************************************
   static void FindNext(LinkedList& l, void *itm, int (*match)(void *, void *))
      {l.Next(); FindMain(l, itm, match);}

   private:

   static void FindMain(LinkedList& l, void *itm,
                        int (*match)(void *, void *));
};

#endif
