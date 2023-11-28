#ifndef __MODLIST_H
#define __MODLIST_H

#include "llist.h"
#include "sfmodtbl.h"

//*****************************************************************************
//
// Class: ModTableList
//
// Description:
//    This class defines the structure for a list of Modulator Tables
//
//*****************************************************************************
class ModTableList : public FuncOrderedLinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   static int modtableorder(void *, void *) {return 0;}
   //static int modtablematch(void *, void *) {return 0;}

   ModTableList() : FuncOrderedLinkedList(ModulatorTable::dealloc, 
					  ModulatorTable::ccons,
					  modtableorder) {}

   void Insert(ModulatorTable *newitm)
      {FuncOrderedLinkedList::Insert(newitm);}
   void Insert(ModulatorTable &newitm)
      {ModulatorTable *tmpitm;
       tmpitm = new ModulatorTable(newitm);
       FuncOrderedLinkedList::Insert(tmpitm);}
   ModulatorTable *GetCurItem()
      {return (ModulatorTable *)FuncOrderedLinkedList::GetCurItem();}

   //void Find(ModulatorTable *rngm)
   //  {FindFuncClass::Find(*(LinkedList *)this, (void *)rngm, rngmatch);}
   //void FindNext(ModulatorTable *rngm)
   //  {FindFuncClass::FindNext(*(LinkedList *)this, (void *)rngm, rngmatch);}
};

#endif

