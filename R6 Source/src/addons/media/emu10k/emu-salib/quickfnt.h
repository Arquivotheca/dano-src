//*****************************************************************************
//
//                              Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: quickfnt.h
//
// Author: Michael Preston
//
// Description: QuickFont data structures and code.
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  -----------------------------------------
// Michael Preston     Sep  5, 1996  Added MoveMIDIBank function.
// Michael Preston     Jul 25, 1996  Added default modulator.
// Michael Preston     Jul 24, 1996  Moved fileNameList to SoundMemory.
// Michael Preston     Jul 17, 1996  Made changes to linked lists.  Removed
//                                   unused functions.  Put ArrayCache as
//                                   default articulation data type.
// Michael Preston     Jul 10, 1996  Moved file and bank names to QFPreset.
// Michael Preston     Jul  2, 1996  Added compile flag for SparseArray.
// Michael Preston     Jul  2, 1996  Cast linked list pointers.
// Michael Preston     Jun 26, 1996  Fixed bug with QFPresetList.
// Michael Preston     Jun 24, 1996  Changed LONG* to SparseArray in QFArtData.
// Michael Preston     Jun 24, 1996  Added support for SoundMemory structure.
// Michael Preston     Jun 24, 1996  Made changed to linked lists.
// Michael Preston     Jun 13, 1996  Initial import to CVS.
// Michael Preston     Mar  1, 1996  Initial development.
//
//*****************************************************************************

#ifndef __QUICKFNT_H
#define __QUICKFNT_H

// Include files

#include "datatype.h" 
#include "win_mem.h"
#include "omega.h"

#ifndef __NO_MOD_CTRL
#include "modlist.h"
#endif

#include "soundmem.h"
#include "qftypes.h"
#include "llist.h"
#include "qfdata.h"

enum enKeyVel
{enkvKeyRange, enkvVelRange};

// Note: qfpsUnload is not actually stored as a state.  It is only used to
// specify that presets should be unloaded.
enum qfPresetState
{qfpsSamplesNotAvailable, qfpsNotAvailable, qfpsAvailable, qfpsUnload};

//*****************************************************************************
//
// Class: QFRange
//
// Description:
//    This class defines the structure for a key/velocity range.
//
//*****************************************************************************
class QFRange
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   QFRange() {}
   ~QFRange() {}

   BYTE byKeyLow;
   BYTE byKeyHigh;
   BYTE byVelLow;
   BYTE byVelHigh;
   DWORD dwStereoLink;
   QFArtData artdata;

   static void dealloc(void *ptr) {delete (QFRange *)ptr;}
   static void *ccons(void *ptr)
      {return new QFRange(*(QFRange *)ptr);}
};

//*****************************************************************************
//
// Class: QFRangeMatch
//
// Description:
//    This class defines the structure for matching a specific key/velocity
// combination to a key/velocity range.
//
//*****************************************************************************
class QFRangeMatch
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   BYTE byKey;
   BYTE byVel;
};

// Forward declarations
int rngmatch(void *rng, void *rngm);
int rngorder(void *rng1, void *rng2);

//*****************************************************************************
//
// Class: QFRangeList
//
// Description:
//    This class defines the structure for a list of key/velocity ranges.
//
//*****************************************************************************
class QFRangeList : public FuncOrderedLinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   QFRangeList() : FuncOrderedLinkedList(QFRange::dealloc, QFRange::ccons,
                                         rngorder) {}

   void Insert(QFRange *newitm)
      {FuncOrderedLinkedList::Insert(newitm);}
   void Insert(QFRange &newitm)
      {QFRange *tmpitm;
       tmpitm = new QFRange(newitm);
       FuncOrderedLinkedList::Insert(tmpitm);}
   QFRange *GetCurItem()
      {return (QFRange *)FuncOrderedLinkedList::GetCurItem();}
   QFRange *RemoveCurItem()
      {return (QFRange *)FuncOrderedLinkedList::RemoveCurItem();}

   void Find(QFRangeMatch *rngm)
     {FindFuncClass::Find(*(LinkedList *)this, (void *)rngm, rngmatch);}
   void FindNext(QFRangeMatch *rngm)
     {FindFuncClass::FindNext(*(LinkedList *)this, (void *)rngm, rngmatch);}
};

class QFKeyVelRange
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   QFKeyVelRange() {}
   ~QFKeyVelRange() {}

   BYTE byLow;
   BYTE byHigh;
   QFRangeList range;

   static void dealloc(void *ptr) {delete (QFKeyVelRange *)ptr;}
   static void *ccons(void *ptr)
      {return new QFKeyVelRange(*(QFKeyVelRange *)ptr);}
};

// Forward declarations
int kvrngorder(void *rng1, void *rng2);

class QFKeyVelRangeList : public FuncOrderedLinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   QFKeyVelRangeList() : FuncOrderedLinkedList(QFKeyVelRange::dealloc,
                                               QFKeyVelRange::ccons,
                                               kvrngorder, DupIgnore) {}

   void Insert(QFKeyVelRange *newitm)
      {FuncOrderedLinkedList::Insert(newitm);}
   void Insert(QFKeyVelRange &newitm)
      {QFKeyVelRange *tmpitm;
       tmpitm = new QFKeyVelRange(newitm);
       FuncOrderedLinkedList::Insert(tmpitm);}
   QFKeyVelRange *GetCurItem()
      {return (QFKeyVelRange *)FuncOrderedLinkedList::GetCurItem();}
};

//*****************************************************************************
//
// Class: QFPreset
//
// Description:
//    This class defines the structure for a single preset.
//
//*****************************************************************************
class QFPreset
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   QFPreset() {}
   ~QFPreset() {}

   Str strPresetName;
   CHAR *strBankName;
   CHAR *strFileName;
   WORD wPresetNum;
   WORD wInstanceNum;
   BOOL bSelfLoaded;
   qfPresetState state;
   enKeyVel enkv;
   QFKeyVelRangeList kvrange;

   static void dealloc(void *ptr) {delete (QFPreset *)ptr;}
   static void *ccons(void *ptr)
      {return new QFPreset(*(QFPreset *)ptr);}
};

// Forward declarations
int presetorder(void *preset1, void *preset2);
int presetmatch(void *p, void *pm);

//*****************************************************************************
//
// Class: QFPresetList
//
// Description:
//    This class defines the structure for a list of presets.
//
//*****************************************************************************
class QFPresetList : public FuncOrderedLinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   QFPresetList() : FuncOrderedLinkedList(QFPreset::dealloc, QFPreset::ccons,
                                          presetorder, DupInsertBefore) {}

   void Insert(QFPreset *newitm)
      {FuncOrderedLinkedList::Insert(newitm);}
   void Insert(QFPreset &newitm)
      {QFPreset *tmpitm;
       tmpitm = new QFPreset(newitm);
       FuncOrderedLinkedList::Insert(tmpitm);}
   QFPreset *GetCurItem()
      {return (QFPreset *)FuncOrderedLinkedList::GetCurItem();}

   void Find(WORD wKey)
      {WORD wTmpKey = wKey;
       FindFuncClass::Find(*(LinkedList *)this, (void *)&wTmpKey,
                           presetmatch);}
   void FindNext(WORD wKey)
      {WORD wTmpKey = wKey;
       FindFuncClass::FindNext(*(LinkedList *)this, (void *)&wTmpKey,
                               presetmatch);}
};

//*****************************************************************************
//
// Class: QFBank
//
// Description:
//    This class defines the structure for a single bank.
//
//*****************************************************************************
class QFBank
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   QFBank() {}
   ~QFBank() {}

   CHAR *GetFileName();
   CHAR *GetBankTitle();
   WORD wBankNum;
   WORD wParentBankNum;
   QFPresetList preset;

   static void dealloc(void *ptr) {delete (QFBank *)ptr;}
   static void *ccons(void *ptr)
      {return new QFBank(*(QFBank *)ptr);}
};

// Forward declarations
int bnkmatch(void *bnk, void *bnkm);
int bnkorder(void *bnk1, void *bnk2);

//*****************************************************************************
//
// Class: QFBankList
//
// Description:
//    This class defines the structure for a list of banks.
//
//*****************************************************************************
class QFBankList : public FuncOrderedLinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   QFBankList() : FuncOrderedLinkedList(QFBank::dealloc, QFBank::ccons,
                                        bnkorder) {}

   void Insert(QFBank *newitm)
      {FuncOrderedLinkedList::Insert(newitm);}
   void Insert(QFBank &newitm)
      {QFBank *tmpitm;
       tmpitm = new QFBank(newitm);
       FuncOrderedLinkedList::Insert(tmpitm);}
   QFBank *GetCurItem()
      {return (QFBank *)FuncOrderedLinkedList::GetCurItem();}
   QFBank *RemoveCurItem()
      {return (QFBank *)FuncOrderedLinkedList::RemoveCurItem();}

   void Find(WORD wKey)
      {WORD wTmpKey = wKey;
       FindFuncClass::Find(*(LinkedList *)this, (void *)&wTmpKey,
                           bnkmatch);}
   void FindNext(WORD wKey)
      {WORD wTmpKey = wKey;
       FindFuncClass::FindNext(*(LinkedList *)this, (void *)&wTmpKey,
                               bnkmatch);}
};

//*****************************************************************************
//
// Class: QuickFont
//
// Description:
//    This class defines the structure for the QuickFont data.
//
//*****************************************************************************
class QuickFont : public OmegaClass
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   QuickFont(SoundMemory *sm)
      {sndmem = sm;}
   ~QuickFont() {}

   void MoveMIDIBank(WORD wOldBankNdx, WORD wNewBankNdx);
   void SetInstanceState(WORD wInstanceNum, qfPresetState state);
   void SetBankState(WORD wBankNum, qfPresetState state);
   void SetPresetState(WORD wBankNum, WORD wPresetNum, qfPresetState state);

   QFBankList list;
   StrList bankNameList;
   SoundMemory *sndmem;

   private:

   void UpdateRefs(QFPreset& preset);
   void UpdateMemLists(SoundMemory& sm);
};

#endif
