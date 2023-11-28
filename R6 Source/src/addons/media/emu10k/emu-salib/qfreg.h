//*****************************************************************************
//
//                              Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: qfreg.h
//
// Author: Michael Preston
//
// Description: QuickFont file and synthesizer registraton procedures.
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  -----------------------------------------
// Michael Preston     Oct  4, 1996  Initial import to CVS.
// Michael Preston     Sep 27, 1996  Initial development.
//
//*****************************************************************************

#ifndef __QFREG_H
#define __QFREG_H

// Include files

#include "datatype.h" 
#include "win_mem.h"
#include "omega.h"
#include "llist.h"
#include "stringcl.h"
#include "riffprsr.h"
#include "qfsfread.h"

class SoundRegInfo
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   Str strFileType;
   Str strFileExtension;
   QFSFReaderBase *(*alloc)(RIFFParser *);

   static void dealloc(void *ptr) {delete (SoundRegInfo *)ptr;}
   static void *ccons(void *ptr)
      {return new SoundRegInfo(*(SoundRegInfo *)ptr);}
};

class SoundRegInfoList : public UnorderedLinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   SoundRegInfoList() : UnorderedLinkedList(SoundRegInfo::dealloc,
					    SoundRegInfo::ccons) {}

   void Insert(SoundRegInfo* newitm, InsertLoc loc=InsertEnd)
      {UnorderedLinkedList::Insert(newitm, loc);}
   SoundRegInfo *GetCurItem()
      {return (SoundRegInfo *)UnorderedLinkedList::GetCurItem();}
};

extern SoundRegInfoList *sriList;

#endif
