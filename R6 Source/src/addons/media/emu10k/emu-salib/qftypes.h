//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: qftypes.h
//
// Author: Michael Preston
//
// Description: Common type defs for QuickFont Reader and Navigator code.
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  -----------------------------------------
// Michael Preston     Oct  4, 1996  Removed file type enum list.
// Michael Preston     Jul 10, 1996  Added Find commands to StrList.
// Michael Preston     Jun 24, 1996  Made changes to linked lists.
// Michael Preston     Jun 13, 1996  Initial import to CVS.
// Michael Preston     May  1, 1996  Initial development.
//
//*****************************************************************************

#ifndef __QFTYPES_H
#define __QFTYPES_H

// Include files

#include "stringcl.h"
#include "llist.h"
#include "emuerrs.h"

class StrList : public UnorderedLinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   StrList() : UnorderedLinkedList(Str::dealloc, Str::ccons) {}

   void Insert(Str* newitm, InsertLoc loc=InsertEnd)
      {UnorderedLinkedList::Insert(newitm, loc);}
   void Insert(Str& newitm, InsertLoc loc=InsertEnd)
      {Str *tmpitm;
       tmpitm = new Str(newitm);
       UnorderedLinkedList::Insert(tmpitm, loc);}
   void Insert(CHAR *newitm, InsertLoc loc=InsertEnd)
      {Str *tmpitm;
       tmpitm = new Str(newitm);
       UnorderedLinkedList::Insert(tmpitm, loc);}
   Str *GetCurItem()
      {return (Str *)UnorderedLinkedList::GetCurItem();}

   void Find(Str *strPresetName)
      {FindFuncClass::Find(*(LinkedList *)this, (void *)strPresetName,
                           strmatch);}
   void Find(CHAR *achPresetName)
      {FindFuncClass::Find(*(LinkedList *)this, (void *)achPresetName,
                           strchmatch);}
   void FindNext(Str *strPresetName)
      {FindFuncClass::FindNext(*(LinkedList *)this, (void *)strPresetName,
                               strmatch);}
   void FindNext(CHAR *achPresetName)
      {FindFuncClass::FindNext(*(LinkedList *)this, (void *)achPresetName,
                               strchmatch);}

   private:

   static int strmatch(void *str1, void *str2)
     {return (*(Str *)str1 == *(Str *)str2);}
   static int strchmatch(void *str1, void *str2)
     {return (*(Str *)str1 == (CHAR *)str2);}
};

typedef enum {
   QF_SUCCESS = 0,
   QF_ALLOCATE_MEM=SF_MEMORYERROR,
   QF_OPEN_FILE=RIFF_OPENFILEERROR,
   QF_WRONG_ROM_ID=SF_WRONGWAVETABLE,
   QF_READ_FILE=RIFF_READFILEERROR,
   QF_RIFF=RIFF_ERROR,
   QF_UNKNOWN_FILE_TYPE=RIFF_IDERROR,
   QF_NO_TRANSLATOR=SF_ERROR
} QuickFontErrs;

enum SoundMemType
{
   SampleMem,
// Add other sound memory types here
   lastSoundMemType
};

enum SynthType
{
   Emu8000,
// Add other synth types here
   lastSynthType
};

#define numSoundMemTypes lastSoundMemType
#define numSynthTypes lastSynthType

#endif
