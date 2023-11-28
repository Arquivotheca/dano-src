//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: llist.cpp
//
// Author: Michael Preston
//
// Description: Generic linked list package.
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  ------------------------------------------
// Michael Preston     Dec 17, 1996  Fixed FuncOrderedLinkedList::Merge()
// Michael Preston     Aug  5, 1996  Replaced swap() function with SWAP() macro
// Michael Preston     Jul 17, 1996  Changed ccons and removed alloc.  Added
//                                   DEBUG compile flag for wNumItems.
// Michael Preston     Jun 26, 1996  Fixed bug with delete.
// Michael Preston     Jun 19, 1996  Removed dest and opdel parameters and
//                                   added dealloc parameter instead.
// Michael Preston     Jun 13, 1996  Initial import to CVS.
// Michael Preston     Jun  7, 1996  Rewrote to remove template code
// Michael Preston     Mar  1, 1996  Initial development.
//
//*****************************************************************************

#include "llist.h"

void LinkedList::First(int wPos)
{
   int wTmp;

   for (wTmp = 0, First(); !EndOfList() && (wTmp < wPos); wTmp++, Next());
}

void LinkedList::Next(int wPos)
{
   int wTmp;

   for (wTmp = 0; !EndOfList() && (wTmp < wPos); wTmp++, Next());
}

WORD LinkedList::GetNumItems()
{
#  ifdef DEBUG_LLIST
      return wNumItems;
#  endif

   WORD ret;

   for (ret = 0, First(); !EndOfList(); ret++, Next());
   return ret;
}

void LinkedList::DeleteAll()
{
   LinkedListItem *tmp, *tmp2;

   if (list == NULL)
      return;
   tmp = list;
   do {
      tmp2 = tmp->next;
      if (bDelete && (tmp->itm != NULL))
	 dealloc(tmp->itm);
      delete tmp;
      tmp = tmp2;
   } while (tmp != list);

#  ifdef DEBUG_LLIST
      wNumItems = 0;
#  endif
}

UnorderedLinkedList::UnorderedLinkedList(const UnorderedLinkedList& l) :
   LinkedList(l.dealloc, l.bDelete)
{
   LinkedListItem *tmppos;

   ccons = l.ccons;
   list = pos = last = NULL;

   if (!l.IsEmpty())
      for (tmppos = l.list; tmppos != NULL;
           tmppos = (tmppos->next != l.list ? tmppos->next :
                     (LinkedListItem *)NULL))
         Insert(ccons(tmppos->next->itm));
}

UnorderedLinkedList&
 UnorderedLinkedList::operator+=(const UnorderedLinkedList& l)
{
   LinkedListItem *tmppos;

   if (!l.IsEmpty())
      for (tmppos = l.list; tmppos != NULL;
           tmppos = (tmppos->next != l.list ? tmppos->next :
                     (LinkedListItem *)NULL))
         Insert(ccons(tmppos->next->itm));
   return *this;
}

UnorderedLinkedList&
 UnorderedLinkedList::operator=(const UnorderedLinkedList& l)
{
   LinkedListItem *tmppos;

   DeleteList();

   if (!l.IsEmpty())
      for (tmppos = l.list; tmppos != NULL;
           tmppos = (tmppos->next != l.list ? tmppos->next :
                     (LinkedListItem *)NULL))
         Insert(ccons(tmppos->next->itm));
   return *this;
}

UnorderedLinkedList
 UnorderedLinkedList::operator+(const UnorderedLinkedList& l)
{
   UnorderedLinkedList tmp(dealloc, ccons, bDelete);
   LinkedListItem *tmppos;

   tmp = *this;
   if (!l.IsEmpty())
      for (tmppos = l.list; tmppos != NULL;
           tmppos = (tmppos->next != l.list ? tmppos->next :
                     (LinkedListItem *)NULL))
         tmp.Insert(ccons(tmppos->next->itm));
   return tmp;
}

void UnorderedLinkedList::Swap(UnorderedLinkedList& l)
{
   SWAP(LinkedListItem *, list, l.list);
   SWAP(LinkedListItem *, pos, l.pos);
   SWAP(LinkedListItem *, last, l.last);

#  ifdef DEBUG_LLIST
      SWAP(WORD, wNumItems, l.wNumItems);
#  endif
}

void UnorderedLinkedList::Merge(UnorderedLinkedList& l)
{
   LinkedListItem *tmppos, *tmpdel;

   if (!l.IsEmpty())
   {
      tmppos = l.list;
      l.list = l.list->next;
      tmppos->next = NULL;
      for (tmppos = l.list; tmppos != NULL;)
      {
         Insert(tmppos->itm);
         tmpdel = tmppos;
         tmppos = tmppos->next;
         delete tmpdel;
      }
      l.list = l.pos = l.last = (LinkedListItem *)NULL;

#     ifdef DEBUG_LLIST
         l.wNumItems = 0;
#     endif
   }
}

void UnorderedLinkedList::Insert(void *newitm, InsertLoc loc)
{
   LinkedListItem *tmp, *tmp2;

   if (list == NULL)
   {
      list = new LinkedListItem(newitm);
      last = list->next = list;
#     ifdef DEBUG_LLIST
         wNumItems++;
#     endif
   }
   else
   {
      if (loc == InsertFront)
      {
         First();
         Insert(newitm, InsertBeforeCurPos);
      }
      else if (loc == InsertEnd)
      {
         pos = NULL;
         Insert(newitm, InsertBeforeCurPos);
      }
      else if (loc == InsertAfterCurPos)
      {
         Next();
         Insert(newitm, InsertBeforeCurPos);
      }
      else if (loc == InsertBeforeCurPos)
      {
         tmp2 = (pos == NULL) ? list : pos;
         tmp = new LinkedListItem(newitm);
         tmp->next = tmp2->next;
         tmp2->next = tmp;
         if (pos == NULL)
         {
            pos = last = list;
            list = tmp;
         }
         else if (pos == last)
            last = tmp;
#        ifdef DEBUG_LLIST
            wNumItems++;
#        endif
      }
   }
}

void UnorderedLinkedList::DeleteCurItem()
{
   LinkedListItem *tmp;

   if (pos == NULL)
      return;

#  ifdef DEBUG_LLIST
      wNumItems--;
#  endif

   if (list == list->next)
   {
      if (bDelete && (list->itm != NULL))
	 dealloc(list->itm);
      delete list;
      list = pos = last = NULL;
      return;
   }
   tmp = pos->next;
   pos->next = pos->next->next;
   if (bDelete && (tmp->itm != NULL))
      dealloc(tmp->itm);
   delete tmp;
   if (list == tmp)
   {
      list = pos;
      pos = NULL;
   }
   if (last == tmp)
      last = pos;
}

FuncOrderedLinkedList&
 FuncOrderedLinkedList::operator+=(const FuncOrderedLinkedList& l)
{
   LinkedListItem *tmppos;

   if (!l.IsEmpty())
      for (tmppos = l.list; tmppos != NULL;
           tmppos = (tmppos->next != l.list ? tmppos->next :
                     (LinkedListItem *)NULL))
         Insert(ccons(tmppos->next->itm));
   return *this;
}

void FuncOrderedLinkedList::Swap(FuncOrderedLinkedList& l)
{
   FuncOrderedLinkedList::ordertype tmporder;
   DupType tmpdup;

   (*(UnorderedLinkedList *)this).Swap(l);
   tmporder = l.sortorder;
   l.sortorder = sortorder;
   sortorder = tmporder;
   tmpdup = l.sortdup;
   l.sortdup = sortdup;
   sortdup = tmpdup;
}

void FuncOrderedLinkedList::Merge(FuncOrderedLinkedList& l)
{
   LinkedListItem *tmppos, *tmpdel;

   if (!l.IsEmpty())
   {
      tmppos = l.list;
      l.list = l.list->next;
      tmppos->next = NULL;
      for (tmppos = l.list; tmppos != NULL;)
      {
         Insert(tmppos->itm);
         tmpdel = tmppos;
         tmppos = tmppos->next;
         delete tmpdel;
      }
      l.list = l.pos = l.last = (LinkedListItem *)NULL;

#     ifdef DEBUG_LLIST
         l.wNumItems = 0;
#     endif
   }
}

void FuncOrderedLinkedList::Insert(void *newitm)
{
   First();
   if (sortdup == DupInsertBefore)
      while (!EndOfList() && (sortorder(GetCurItem(), newitm) == -1))
         Next();
   else if (sortdup == DupInsertAfter)
      while (!EndOfList() && (sortorder(GetCurItem(), newitm) != 1))
         Next();
   else
   {
      while (!EndOfList() && (sortorder(GetCurItem(), newitm) < 0))
         Next();
      if (!EndOfList() && (sortorder(GetCurItem(), newitm) == 0))
         if (sortdup == DupIgnore)
         {
//            delete newitm;
            dealloc(newitm);
            return;
         }
         else
            DeleteCurItem();
   }
   UnorderedLinkedList::Insert(newitm, InsertBeforeCurPos);
}

void FuncOrderedLinkedList::Sort()
{
   FuncOrderedLinkedList tmplist(dealloc, ccons, sortorder, sortdup);

   tmplist.Merge(*this);
   tmplist.Swap(*this);
}

void FindFuncClass::FindMain(LinkedList& l, void *itm,
                                  int (*match)(void *, void *))
{
// End of list or list is empty
   if (l.EndOfList())
      return;
// List is not empty
   do {
      if (match(l.GetCurItem(), itm))
         return;
      l.Next();
   } while (!l.EndOfList());
}
