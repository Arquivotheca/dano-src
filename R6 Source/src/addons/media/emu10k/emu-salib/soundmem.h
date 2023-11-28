//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: soundmem.h
//
// Author: Michael Preston
//
// Description: Sound memory manager data structures
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  -----------------------------------------
// Michael Preston     Oct  4, 1996  Added sample bits enum and SoundDataLoad
//                                   class for reading sample data.
// Michael Preston     Aug 23, 1996  Store sample location and size in bytes.
// Michael Preston     Jul 24, 1996  Added fileNameList.
// Michael Preston     Jul 17, 1996  Made changes to linked lists.
// Michael Preston     Jul  2, 1996  Cast linked list pointers.
// Michael Preston     Jun 20, 1996  Added 2nd ref count.
// Michael Preston     Jun 18, 1996  Added ref count and SoundMemory struct.
// Michael Preston     Jun 13, 1996  Initial import to CVS.
// Michael Preston     Mar  1, 1996  Initial development.
//
//*****************************************************************************

#ifndef __SOUNDMEM_H
#define __SOUNDMEM_H

#include "datatype.h"
#include "qftypes.h"
#include "llist.h"
#include "emufilio.h"

enum SoundDataType {
   sdtChannelMask      = 0x7f,
   sdt16BitMask        = 0x80
};

class SoundData
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   WORD wAllocatedRefCount;
   WORD wInUseRefCount;
   CHAR *strFileName;
   Str strSoundName;
   DWORD dwStartLocInBytes;
   DWORD dwSizeInBytes;
   DWORD dwCurrOffset;
   BYTE bySoundDataType;

   static void dealloc(void *ptr) {delete (SoundData *)ptr;}
   static void *ccons(void *ptr)
      {return new SoundData(*(SoundData *)ptr);}
};

class SoundDataLoad
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   SoundDataLoad(void *(*alloc)(), void *token);
   ~SoundDataLoad();

   void StartSoundData(SoundData *sd);
   DWORD GetSoundData(BYTE *buffer, DWORD dwNumBytes);

   EmuAbstractFileIO *io;
   BOOL b16Bit;
   BYTE byNumChannels;
   DWORD dwCurPos;
   DWORD dwEndPos;
};

class SoundDataMatch
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   CHAR *strFileName;
   Str *strSoundName;
};

// Forward declaration
int sdrefmatch(void *s, void *sm);
int sdfnmatch(void *s, void *sm);
int sdmatch(void *s, void *sm);

//*****************************************************************************
//
// Class: SoundDataList
//
// Description:
//    This class defines the structure for lists of sound header data to be
// used by the sound memory manager.
//
//*****************************************************************************
class SoundDataList : public UnorderedLinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   SoundDataList() : UnorderedLinkedList(SoundData::dealloc, SoundData::ccons)
      {}

   void Insert(SoundData *newitm, InsertLoc loc=InsertEnd)
      {UnorderedLinkedList::Insert(newitm, loc);}
   void Insert(SoundData &newitm, InsertLoc loc=InsertEnd)
      {SoundData *tmpitm;
       tmpitm = new SoundData(newitm);
       UnorderedLinkedList::Insert(tmpitm, loc);}
   SoundData *GetCurItem()
      {return (SoundData *)UnorderedLinkedList::GetCurItem();}
   SoundData *RemoveCurItem()
      {return (SoundData *)UnorderedLinkedList::RemoveCurItem();}

   void Find(BOOL bNoRefs)
      {BOOL bTmp = bNoRefs;
       FindFuncClass::Find(*(LinkedList *)this, (void *)&bTmp, sdrefmatch);}
   void Find(Str *strFileName)
      {FindFuncClass::Find(*(LinkedList *)this, (void *)strFileName,
                           sdfnmatch);}
   void Find(SoundDataMatch *sm)
      {FindFuncClass::Find(*(LinkedList *)this, (void *)sm, sdmatch);}
   void FindNext(BOOL bNoRefs)
      {BOOL bTmp = bNoRefs;
       FindFuncClass::FindNext(*(LinkedList *)this, (void *)&bTmp,
                               sdrefmatch);}
   void FindNext(Str *strFileName)
      {FindFuncClass::FindNext(*(LinkedList *)this, (void *)strFileName,
                               sdfnmatch);}
   void FindNext(SoundDataMatch *sm)
      {FindFuncClass::FindNext(*(LinkedList *)this, (void *)sm, sdmatch);}
};

class SoundMemory
{
   public:

   void UpdateLists();

   SoundDataList SoundList;
   SoundDataList ActionList;
   StrList fileNameList;
};

#endif
