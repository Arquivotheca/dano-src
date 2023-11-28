//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: qfsfread.h
//
// Author: Michael Preston
//
// Description: Abstract reader base class definition.
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  -----------------------------------------
// Michael Preston     Oct  4, 1996  Moved sample reading to soundmem.h
// Michael Preston     Jul 30, 1996  Made ~QFSFReaderBase() virtual.
// Michael Preston     Jul 24, 1996  Added SetupCallBack();
// Michael Preston     Jul 22, 1996  Added OmegaClass.
// Michael Preston     Jul 17, 1996  Changed load functions to use index
//                                   number instead of name.
// Michael Preston     Jul  8, 1996  Added GetBankInfo().
// Michael Preston     Jun 24, 1996  Added bMarkSounds for SoundMemory support.
// Michael Preston     May 17, 1996  Initial import to CVS.
// Michael Preston     Mar  1, 1996  Initial development.
//
//*****************************************************************************

#ifndef __QFSFREAD_H
#define __QFSFREAD_H

#include "datatype.h"
#include "riffprsr.h"
#include "quickfnt.h"
#include "qftypes.h"
#include "transgen.h"
#include "emucllbk.h"
#include "emusc.h"

class QFBankInfo
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   Str strBankName;
   WORD wNumPresets;
   WORD wNumInsts;
   WORD wNumSounds;
   DWORD dwTotalSoundSize;
};

class QFPresetInfo
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   Str strPresetName;
   midiDest dest;

   static void dealloc(void *ptr) {delete (QFPresetInfo *)ptr;}
   static void *ccons(void *ptr)
      {return new QFPresetInfo(*(QFPresetInfo *)ptr);}
};

class QFPresetInfoList : public UnorderedLinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   QFPresetInfoList() : UnorderedLinkedList(QFPresetInfo::dealloc,
                                            QFPresetInfo::ccons) {}

   void Insert(QFPresetInfo* newitm, InsertLoc loc=InsertEnd)
      {UnorderedLinkedList::Insert(newitm, loc);}
   QFPresetInfo *GetCurItem()
      {return (QFPresetInfo *)UnorderedLinkedList::GetCurItem();}
};

class QFSoundInfo
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   Str strSoundName;

   static void dealloc(void *ptr) {delete (QFSoundInfo *)ptr;}
   static void *ccons(void *ptr)
      {return new QFSoundInfo(*(QFSoundInfo *)ptr);}
};

class QFSoundInfoList : public UnorderedLinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   QFSoundInfoList() : UnorderedLinkedList(QFSoundInfo::dealloc,
                                            QFSoundInfo::ccons) {}

   void Insert(QFSoundInfo* newitm, InsertLoc loc=InsertEnd)
      {UnorderedLinkedList::Insert(newitm, loc);}
   QFSoundInfo *GetCurItem()
      {return (QFSoundInfo *)UnorderedLinkedList::GetCurItem();}
};

class QFSFReaderBase : public OmegaClass
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   QFSFReaderBase(RIFFParser* newRIFF)
      {tRIFF = newRIFF; strFileName = tRIFF->GetFileIO()->GetPathName();
       trans = NULL; callBackFunction = NULL;}
   virtual ~QFSFReaderBase() {}

   void SetTranslator(TranslatorBase *transarg)
     {trans = transarg;}
   void SetROMid(CHAR *romid)
     {strROMid = romid;}
   void SetupCallBack(downLoadCallBack functionPtr, BYTE percent)
     {callBackFunction = functionPtr;
      callBackPercent = percent;}

   virtual QFBankInfo& GetBankInfo()=0;
   virtual void GetPresetInfo(QFPresetInfoList& piList)=0;
   virtual void GetInstInfo(QFSoundInfoList& siList)=0;
   virtual void GetSoundInfo(QFSoundInfoList& siList)=0;
   virtual void LoadBank(QuickFont& qf, WORD wBankNum, qfPresetState state,
                         WORD wInstanceNum)=0;
   virtual void LoadPreset(QuickFont& qf, WORD wPresetNdx, WORD wBankNum,
                           WORD wPresetNum, qfPresetState state,
                           WORD wInstanceNum)=0;
   virtual void LoadInst(QuickFont& qf, WORD wInstNdx, WORD wBankNum,
                         WORD wPresetNum, qfPresetState state,
                         WORD wInstanceNum)=0;
   virtual void LoadSound(QuickFont& qf, WORD wSoundNdx, WORD wBankNum,
                          WORD wPresetNum, qfPresetState state,
                          WORD wInstanceNum)=0;

   protected:

   RIFFParser* tRIFF;
   Str strFileName;
   Str strROMid;
   TranslatorBase *trans;
   downLoadCallBack callBackFunction;
   BYTE callBackPercent, currPercent, lastPercent;
};

#endif
