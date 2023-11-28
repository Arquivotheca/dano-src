//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: qfsf2.cpp
//
// Author: Michael Preston
//
// Description: Code specific for SoundFont 2.0 files.
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  -----------------------------------------
// Michael Preston     Aug  4, 1997  Cleaned up code.  Added more robust error
//                                   trapping.
// Michael Preston     Oct  4, 1996  Added sample bits field.  Added code to
//                                   dynamically register file type.
// Michael Preston     Aug 23, 1996  Store sample location and size in bytes.
// Michael Preston     Aug 23, 1996  Store location of SMPL chunk in lSmplPos.
// Michael Preston     Aug 13, 1996  Fixed bug in GetPresetRanges() and
//                                   GetInstRanges() - wasn't clearing
//                                   globalmodl before each split.
// Michael Preston     Aug  8, 1996  Compute default articulation data only
//                                   once instead of for every vector.
// Michael Preston     Aug  6, 1996  Revised to convert modulator destinations
//                                   and amounts.
// Michael Preston     Aug  5, 1996  Replaced min(), max(), and comp()
//                                   functions with MIN(), MAX(), and COMP()
//                                   macros.  Replaced memset() with
//                                   PTRMEMSET() for pointer arrays.
// Michael Preston     Aug  2, 1996  Improved load performance.
// Michael Preston     Jul 30, 1996  Moved sound data checking to
//                                   GetSampleRanges() from AddRange().
// Michael Preston     Jul 29, 1996  Changed preset, inst, and sample lists to
//                                   arrays.  Replaced FindChunk() calls with
//                                   GetCurPosition() and SetCurPosition().
// Michael Preston     Jul 25, 1996  Began adding error trapping.  Added
//                                   full modulator support.
// Michael Preston     Jul 24, 1996  Added compile flags for mods.  Added
//                                   call back function support.
// Michael Preston     Jul 17, 1996  Changed load functions to use index
//                                   number instead of name.  Moved code only
//                                   needed for qfsf2.cpp.  Improved bank load
//                                   time.
// Michael Preston     Jul 11, 1996  Changed CheckFileType() to accept 'sfbk'
//                                   or 'SFBK'.
// Michael Preston     Jul 10, 1996  Added AddFileAndBankToLists() and moved
//                                   file and bank names to QFPreset.
// Michael Preston     Jul  8, 1996  Added GetBankInfo().
// Michael Preston     Jun 24, 1996  Added support for SoundMemory and
//                                   SparseArray.
// Michael Preston     Jun 13, 1996  Initial import to CVS.
// Michael Preston     Mar  1, 1996  Initial development.
//
//*****************************************************************************

// Include files

#include <string.h>

#include "qfsf2.h"
#include "llist.h"
#include "riffprsr.h"
#include "qfreg.h"

// Define sizes for RIFF chunks

#define NAMESIZE 20
#define BAGLEN 4
#define MODLEN 10
#define GENLEN 4
#define PHDRLEN 38
#define INSTLEN 22
#define SHDRLEN 46

// Type definitions

#ifdef __BYTE_COHERENT
struct stByteRange
{
   BYTE byLow;
   BYTE byHigh;
};

#elif defined (__BYTE_INCOHERENT)
struct stByteRange
{
   BYTE byHigh;
   BYTE byLow;
};
#endif

typedef union
{
   struct stByteRange stRange; // Sometimes the generator is an int, and
   SHORT shAmount;          //   and sometimes it is an stRange
} unGenAmount;

class sfBankVersion
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   sfBankVersion() {}
   sfBankVersion(WORD maj, WORD min) {wMajor = maj; wMinor = min;}
   ~sfBankVersion() {}

   WORD wMajor;
   WORD wMinor;
};

class sfGen
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void *operator new[](size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}
   void operator delete[](void *ptr) {DeleteAlloc(ptr);}

   WORD wGenOper;
   WORD wGenAmt;

   static void dealloc(void *ptr) {delete (sfGen *)ptr;}
   static void *ccons(void *ptr)
      {return new sfGen(*(sfGen *)ptr);}
   static int match(void *gen, void *genm)
      {return (((sfGen *)gen)->wGenOper == *((WORD *)genm));}
   static int order(void *gen1, void *gen2)
      {return (COMP(((sfGen *)gen1)->wGenOper, ((sfGen *)gen2)->wGenOper));}
};

class sfGenList : public FuncOrderedLinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   sfGenList() :
      FuncOrderedLinkedList(sfGen::dealloc, sfGen::ccons, sfGen::order,
                            DupReplace) {}

   void Insert(sfGen* newitm)
      {FuncOrderedLinkedList::Insert(newitm);}
   void Insert(sfGen& newitm)
      {sfGen *tmpitm;
       tmpitm = new sfGen(newitm);
       FuncOrderedLinkedList::Insert(tmpitm);}
   sfGen *GetCurItem()
      {return (sfGen *)FuncOrderedLinkedList::GetCurItem();}

   void Find(WORD wKey)
      {WORD wTmpKey = wKey;
       FindFuncClass::Find(*(LinkedList *)this, (void *)&wTmpKey, sfGen::match);}
   void FindNext(WORD wKey)
      {WORD wTmpKey = wKey;
       FindFuncClass::FindNext(*(LinkedList *)this, (void *)&wTmpKey,
                               sfGen::match);}
};

#ifndef __NO_MOD_CTRL
class sfModMatch
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   WORD wModSrcOper;
   WORD wModDestOper;
   WORD wModAmtSrcOper;
};

class sfMod
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void *operator new[](size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}
   void operator delete[](void *ptr) {DeleteAlloc(ptr);}

   WORD wModSrcOper;
   WORD wModDestOper;
   SHORT shAmount;
   WORD wModAmtSrcOper;
   WORD wModTransOper;

   static void dealloc(void *ptr) {delete (sfMod *)ptr;}
   static void *ccons(void *ptr)
      {return new sfMod(*(sfMod *)ptr);}
   static int modmatch(void *m, void *mm)
      {return (((((((sfMod *)m)->wModSrcOper&0xff) == 0) &&
                ((((sfModMatch *)mm)->wModSrcOper&0xff) == 0)) || 
          (((sfMod *)m)->wModSrcOper == ((sfModMatch *)mm)->wModSrcOper)) &&
         (((sfMod *)m)->wModDestOper == ((sfModMatch *)mm)->wModDestOper) &&
         (((sfMod *)m)->wModAmtSrcOper == ((sfModMatch *)mm)->wModAmtSrcOper));}
   static int modorder(void *m1, void *m2)
      {int tmp = COMP(((sfMod *)m1)->wModSrcOper, ((sfMod *)m2)->wModSrcOper);
       if (tmp != 0)
          return tmp;
       else
       {
          tmp = COMP(((sfMod *)m1)->wModDestOper,
                     ((sfMod *)m2)->wModDestOper);
          if (tmp != 0)
             return tmp;
          else
             return COMP(((sfMod *)m1)->wModAmtSrcOper,
                         ((sfMod *)m2)->wModAmtSrcOper);
       }
      }
};

class sfModList : public FuncOrderedLinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   sfModList() : FuncOrderedLinkedList(sfMod::dealloc, sfMod::ccons,
                                       sfMod::modorder, DupReplace) {}

   void Insert(sfMod* newitm)
      {FuncOrderedLinkedList::Insert(newitm);}
   void Insert(sfMod& newitm)
      {sfMod *tmpitm;
       tmpitm = new sfMod(newitm);
       FuncOrderedLinkedList::Insert(tmpitm);}
   sfMod *GetCurItem()
      {return (sfMod *)FuncOrderedLinkedList::GetCurItem();}

   void Find(sfModMatch *sfmm)
     {FindFuncClass::Find(*(LinkedList *)this, (void *)sfmm,
                          sfMod::modmatch);}
   void FindNext(sfModMatch *sfmm)
     {FindFuncClass::FindNext(*(LinkedList *)this, sfmm,
                              sfMod::modmatch);}
};
#endif

class sfBag {
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void *operator new[](size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}
   void operator delete[](void *ptr) {DeleteAlloc(ptr);}

   WORD wGenNdx;
   WORD wModNdx;

   static void dealloc(void *ptr) {delete (sfBag *)ptr;}
   static void *ccons(void *ptr)
      {return new sfBag(*(sfBag *)ptr);}
};

class sfBagList : public UnorderedLinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   sfBagList() : UnorderedLinkedList(sfBag::dealloc, sfBag::ccons) {}

   void Insert(sfBag* newitm, InsertLoc loc=InsertEnd)
      {UnorderedLinkedList::Insert(newitm, loc);}
   void Insert(sfBag& newitm, InsertLoc loc=InsertEnd)
      {sfBag *tmpitm;
       tmpitm = new sfBag(newitm);
       UnorderedLinkedList::Insert(tmpitm, loc);}
   sfBag *GetCurItem() 
      {return (sfBag *)UnorderedLinkedList::GetCurItem();}
};

class sfPreset {
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   Str strPresetName;
   WORD wPresetNum;
   WORD wBankNum;
   WORD wBagNdx;
   DWORD dwLibrary;
   DWORD dwGenre;
   DWORD dwMorphology;

   static void dealloc(void *ptr) {delete (sfPreset *)ptr;}
   static void *ccons(void *ptr)
      {return new sfPreset(*(sfPreset *)ptr);}
};

class sfPresetList : public UnorderedLinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   sfPresetList() : UnorderedLinkedList(sfPreset::dealloc, sfPreset::ccons) {}

   void Insert(sfPreset* newitm, InsertLoc loc=InsertEnd)
      {UnorderedLinkedList::Insert(newitm, loc);}
   void Insert(sfPreset& newitm, InsertLoc loc=InsertEnd)
      {sfPreset *tmpitm;
       tmpitm = new sfPreset(newitm);
       UnorderedLinkedList::Insert(tmpitm, loc);}
   sfPreset *GetCurItem()
      {return (sfPreset *)UnorderedLinkedList::GetCurItem();}
   sfPreset *RemoveCurItem()
      {return (sfPreset *)UnorderedLinkedList::RemoveCurItem();}
};

class sfInst {
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   Str strInstName;
   WORD wBagNdx;

   static void dealloc(void *ptr) {delete (sfInst *)ptr;}
   static void *ccons(void *ptr)
      {return new sfInst(*(sfInst *)ptr);}
};

class sfInstList : public UnorderedLinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   sfInstList() : UnorderedLinkedList(sfInst::dealloc, sfInst::ccons) {}

   void Insert(sfInst *newitm, InsertLoc loc=InsertEnd)
      {UnorderedLinkedList::Insert(newitm, loc);}
   void Insert(sfInst &newitm, InsertLoc loc=InsertEnd)
      {sfInst *tmpitm;
       tmpitm = new sfInst(newitm);
       UnorderedLinkedList::Insert(tmpitm, loc);}
   sfInst *GetCurItem()
      {return (sfInst *)UnorderedLinkedList::GetCurItem();}
   sfInst *RemoveCurItem()
      {return (sfInst *)UnorderedLinkedList::RemoveCurItem();}
};

class sfSampleList : public UnorderedLinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   sfSampleList() : UnorderedLinkedList(sfSample::dealloc, sfSample::ccons) {}

   void Insert(sfSample *newitm, InsertLoc loc=InsertEnd)
      {UnorderedLinkedList::Insert(newitm, loc);}
   void Insert(sfSample &newitm, InsertLoc loc=InsertEnd)
      {sfSample *tmpitm;
       tmpitm = new sfSample(newitm);
       UnorderedLinkedList::Insert(tmpitm, loc);}
   sfSample *GetCurItem()
      {return (sfSample *)UnorderedLinkedList::GetCurItem();}
   sfSample *RemoveCurItem()
      {return (sfSample *)UnorderedLinkedList::RemoveCurItem();}
};

class Range
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   Range() {byKeyLow = byVelLow = 0;
            byKeyHigh = byVelHigh = 127;
            wNumGens = 0;
            genarr = NULL;
#  ifndef __NO_MOD_CTRL
            wNumMods = 0;
            modarr = NULL;
#  endif
           }
   ~Range() {if (genarr != NULL) delete [] genarr;
#  ifndef __NO_MOD_CTRL
             if (modarr != NULL) delete [] modarr;
#  endif
            }
   Range(Range& rng)
      {byKeyLow = rng.byKeyLow;
       byKeyHigh = rng.byKeyHigh;
       byVelLow = rng.byVelLow;
       byVelHigh = rng.byVelHigh;
       wNumGens = rng.wNumGens;
       genarr = new sfGen[wNumGens];
       memcpy(genarr, rng.genarr, wNumGens*sizeof(sfGen));
#  ifndef __NO_MOD_CTRL
       wNumMods = rng.wNumMods;
       modarr = new sfMod[wNumMods];
       memcpy(modarr, rng.modarr, wNumMods*sizeof(sfMod));
#  endif
       wIndex = rng.wIndex;}

   BYTE byKeyLow;
   BYTE byKeyHigh;
   BYTE byVelLow;
   BYTE byVelHigh;
   WORD wNumGens;
   sfGen *genarr;
#  ifndef __NO_MOD_CTRL
   WORD wNumMods;
   sfMod *modarr;
#  endif
   WORD wIndex;
   BOOL bStereo;  // Only used for inst ranges

   static void dealloc(void *ptr) {delete (Range *)ptr;}
   static void *ccons(void *ptr)
      {return new Range(*(Range *)ptr);}
};

class RangeList : public UnorderedLinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   RangeList() : UnorderedLinkedList(Range::dealloc, Range::ccons) {}

   void Insert(Range *newitm, InsertLoc loc=InsertEnd)
      {UnorderedLinkedList::Insert(newitm, loc);}
   void Insert(Range &newitm, InsertLoc loc=InsertEnd)
      {Range *tmpitm;
       tmpitm = new Range(newitm);
       UnorderedLinkedList::Insert(tmpitm, loc);}
   Range *GetCurItem()
      {return (Range *)UnorderedLinkedList::GetCurItem();}
};

class sfPresetRange
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   sfPresetRange() {preset = NULL;}
   ~sfPresetRange() {if (preset != NULL) delete preset;}
   sfPresetRange(sfPresetRange& sfpr) : range(sfpr.range)
      {if (sfpr.preset != NULL)
          preset = new sfPreset(*sfpr.preset);
       /*wIndex = sfpr.wIndex;*/}

   sfPreset *preset;
//   WORD wIndex;
   RangeList range;

   static void dealloc(void *ptr) {delete (sfPresetRange *)ptr;}
   static void *ccons(void *ptr)
      {return new sfPresetRange(*(sfPresetRange *)ptr);}
};

class sfInstRange
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   sfInstRange() {inst = NULL;}
   ~sfInstRange() {if (inst != NULL) delete inst;}
   sfInstRange(sfInstRange& sfir) : range(sfir.range)
      {if (sfir.inst != NULL)
          inst = new sfInst(*sfir.inst);
       wRefCount = sfir.wRefCount;}

   sfInst *inst;
   RangeList range;
   WORD wRefCount;

   static void dealloc(void *ptr) {delete (sfInstRange *)ptr;}
   static void *ccons(void *ptr)
      {return new sfInstRange(*(sfInstRange *)ptr);}
};

class sfSampleRange
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   sfSampleRange() {sample = NULL; snddata = NULL;}
   ~sfSampleRange() {if (sample != NULL) delete sample;}
   sfSampleRange(sfSampleRange& sfsr)
      {if (sfsr.sample != NULL)
          sample = new sfSample(*sfsr.sample);
       wIndex = sfsr.wIndex;
       wRefCount = sfsr.wRefCount;}

   sfSample *sample;
   SoundData *snddata;
   WORD wIndex;
   WORD wRefCount;

   static void dealloc(void *ptr) {delete (sfSampleRange *)ptr;}
   static void *ccons(void *ptr)
      {return new sfSampleRange(*(sfSampleRange *)ptr);}
};

class SampleLinkInfo
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   SampleLinkInfo() {}
   SampleLinkInfo(WORD wSampleIndexArg, WORD wSampleLinkArg,
                  BOOL bFoundBothArg)
   {wSampleIndex = wSampleIndexArg;
    wSampleLink = wSampleLinkArg;
    bFoundBoth = bFoundBothArg;}

   WORD wSampleIndex;
   WORD wSampleLink;
   BOOL bFoundBoth;

   static void dealloc(void *ptr) {delete (SampleLinkInfo *)ptr;}
   static void *ccons(void *ptr)
      {return new SampleLinkInfo(*(SampleLinkInfo *)ptr);}
   static int match(void *sli, void *slim)
      {return (((SampleLinkInfo *)sli)->wSampleIndex == *((WORD *)slim));}
};

class SampleLinkInfoList : public UnorderedLinkedList
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   SampleLinkInfoList() : UnorderedLinkedList(SampleLinkInfo::dealloc,
                                              SampleLinkInfo::ccons) {}

   void Insert(SampleLinkInfo *newitm, InsertLoc loc=InsertEnd)
      {UnorderedLinkedList::Insert(newitm, loc);}
   void Insert(SampleLinkInfo &newitm, InsertLoc loc=InsertEnd)
      {SampleLinkInfo *tmpitm;
       tmpitm = new SampleLinkInfo(newitm);
       UnorderedLinkedList::Insert(tmpitm, loc);}
   SampleLinkInfo *GetCurItem()
      {return (SampleLinkInfo *)UnorderedLinkedList::GetCurItem();}

   void Find(WORD wIndex)
      {WORD wTmpIndex = wIndex;
       FindFuncClass::Find(*(LinkedList *)this, (void *)&wTmpIndex,
                           SampleLinkInfo::match);}
   void FindNext(WORD wIndex)
      {WORD wTmpIndex = wIndex;
       FindFuncClass::FindNext(*(LinkedList *)this, (void *)&wTmpIndex,
                               SampleLinkInfo::match);}
};

// Static variables

BOOL QFSF2Reader::bDefModTableInitialized=FALSE;
ModulatorTable QFSF2Reader::defModTable;
WORD wCurInst = 0;

SHORT SF2Default[numSF2Gens] =
   {0,      // startAddrsOffset
    0,      // endAddrsOffset
    0,      // startloopAddrsOffset
    0,      // endloopAddrsOffset
    0,      // startAddrsCoarseOffset
    0,      // modLfoToPitch
    0,      // vibLfoToPitch
    0,      // modEnvToPitch
    13500,  // initialFilterFc
    0,      // initialFilterQ
    0,      // modLfoToFilterFc
    0,      // modEnvToFilterFc
    0,      // endAddrsCoarseOffset
    0,      // modLfoToVolume
    0,      // unused0
    0,      // chorusEffectsSend
    0,      // reverbEffectsSend
    0,      // pan
    0,      // unused1
    0,      // unused2
    0,      // unused3
    -12000, // delayModLfo
    0,      // freqModLfo
    -12000, // delayVibLfo
    0,      // freqVibLfo
    -12000, // delayModEnv
    -12000, // attackModEnv
    -12000, // holdModEnv
    -12000, // decayModEnv
    0,      // sustainModEnv
    -12000, // releaseModEnv
    0,      // keynumToModEnvHold
    0,      // keynumToModEnvDecay
    -12000, // delayVolEnv
    -12000, // attackVolEnv
    -12000, // holdVolEnv
    -12000, // decayVolEnv
    0,      // sustainVolEnv
    -12000, // releaseVolEnv
    0,      // keynumToVolEnvHold
    0,      // keynumToVolEnvDecay
    -1,     // instrument
    0,      // nop
    0,      // keyRange
    0,      // velRange
    0,      // startloopAddrsCoarseOffset
    -1,     // keynum
    -1,     // velocity
    0,      // initialAttenuation
    0,      // keyTuning
    0,      // endLoopAddrsCoarseOffset
    0,      // coarseTune
    0,      // fineTune
    -1,     // sampleId
    0,      // sampleModes
    0,      // unused4
    100,    // scaleTuning
    0,      // exclusiveClass
    -1,     // overridingRootKey
    0,      // unused5
    0       // endOper
   };

QFSF2Reader::QFSF2Reader(RIFFParser* newRIFF) :
   QFSFReaderBase(newRIFF)
{
   ClearError();
   GetNumCks();
   if (IsBad()) return;
   SetBankInfo();
   if (IsBad()) return;
   GetROMid();
}

QFSF2Reader::~QFSF2Reader()
{
}

BOOL QFSF2Reader::CheckFileType(RIFFParser* newRIFF)
{
   SWAP_WORD_BYTE_INCOH_DECLARATIONS;
   WORD ver;

   newRIFF->FindChunk(MAKE_ID("RIFF"));
   if ((newRIFF->GetChunkListToken() != MAKE_ID("sfbk")) && 
       (newRIFF->GetChunkListToken() != MAKE_ID("SFBK")))
      return FALSE;
   newRIFF->FindChunk(MAKE_ID("ifil"));
   newRIFF->StartDataStream();
   newRIFF->ReadDataStream(&ver, sizeof(WORD));
   newRIFF->StopDataStream();
   SWAP_WORD_BYTE_INCOH_ONLY(ver);
   return (ver == 2);
}

void QFSF2Reader::GetPresetInfo(QFPresetInfoList& piList)
{
   sfPresetList tmpphdr;
   QFPresetInfo *tmp;

   piList.DeleteList();
   GetPhdr(tmpphdr, 0, wNumPhdr);
   for (tmpphdr.First(); !tmpphdr.EndOfList(); tmpphdr.Next())
   {
      tmp = new QFPresetInfo;
      tmp->strPresetName = tmpphdr.GetCurItem()->strPresetName;
      tmp->dest.midiBankIndex.midiBankByValue.midiBankCC0 =
          (BYTE)tmpphdr.GetCurItem()->wBankNum;
      tmp->dest.midiBankIndex.midiBankByValue.midiBankCC32 = 0;
      tmp->dest.midiProgramIndex = tmpphdr.GetCurItem()->wPresetNum;
      piList.Insert(tmp);
   }
   piList.First();
}

void QFSF2Reader::GetInstInfo(QFSoundInfoList& siList)
{
   sfInstList tmpinst;
   QFSoundInfo *tmp;

   siList.DeleteList();
   GetInst(tmpinst, 0, wNumInst);
   for (tmpinst.First(); !tmpinst.EndOfList(); tmpinst.Next())
   {
      tmp = new QFSoundInfo;
      tmp->strSoundName = tmpinst.GetCurItem()->strInstName;
      siList.Insert(tmp);
   }
   siList.First();
}

void QFSF2Reader::GetSoundInfo(QFSoundInfoList& siList)
{
   sfSampleList tmpshdr;
   QFSoundInfo *tmp;

   siList.DeleteList();
   GetShdr(tmpshdr, 0, wNumShdr);
   for (tmpshdr.First(); !tmpshdr.EndOfList(); tmpshdr.Next())
   {
      tmp = new QFSoundInfo;
      tmp->strSoundName = tmpshdr.GetCurItem()->strSampleName;
      siList.Insert(tmp);
   }
   siList.First();
}

void QFSF2Reader::LoadBank(QuickFont& qf, WORD wBankNum, qfPresetState state,
                           WORD wInstanceNum)
{
   QFBankList tmp;
   WORD i;

   LoadSetup();
   if (IsBad())
      return;

   AddFileAndBankToLists(qf);
   if (IsBad())
      goto LoadBankError;

   GetPresetRanges(state, *qf.sndmem, enltBankLoad | wBankNum);
   if (IsBad())
      goto LoadBankError;
   CheckInstStereoPairs();

   qf.list.Swap(tmp);
   currPercent = 50;
   for (i = 0; (i < wNumPhdr) && IsOK(); i++)
   {
      ReadPresetData(qf, TRUE, i, wBankNum, i, state, wInstanceNum);
      currPercent = 50 + 50*i/wNumPhdr;
      if ((callBackFunction != NULL) &&
          (currPercent >= lastPercent+callBackPercent))
      {
         while (lastPercent+callBackPercent <= currPercent)
         {
            lastPercent += callBackPercent;
         }
         if (!callBackFunction(emuCallBackReading, currPercent))
            SetError(QF_READ_FILE);
      }
   }
   if (IsBad())
   {
      qf.list.Swap(tmp);
      goto LoadBankError;
   }


   // If we loaded the bank succesfully, merge the new presets with the
   // existing bank
   if (wBankNum == 0)
   {
      // If loading into bank 0, merge presets in bank 0 together.  We
      // don't want to shadow banks for bank 0
      tmp.Find(0);
      qf.list.Find(0);
      if (!tmp.EndOfList() && !qf.list.EndOfList())
      {
         qf.list.GetCurItem()->preset.Merge(tmp.GetCurItem()->preset);
         tmp.DeleteCurItem();
      }
   }
   qf.list.Merge(tmp);

   qf.sndmem->ActionList.Merge(snddatalist);
#  ifdef __USE_ARRAYCACHE
      ArrayCache::ReadOptimize();
#  endif
   currPercent = 100;
   if ((callBackFunction != NULL) && IsOK() &&
       (!callBackFunction(emuCallBackReading, currPercent)))
      SetError(QF_READ_FILE);

LoadBankError:
   LoadCleanup();
}

void QFSF2Reader::LoadPreset(QuickFont& qf, WORD wPresetNdx, WORD wBankNum,
                             WORD wPresetNum, qfPresetState state,
                             WORD wInstanceNum)
{
   LoadSetup();
   if (IsBad())
      return;

   AddFileAndBankToLists(qf);
   if (IsBad())
      goto LoadPresetError;

   GetPresetRanges(state, *qf.sndmem, wPresetNdx);
   if (IsBad())
      goto LoadPresetError;
   CheckInstStereoPairs();

   currPercent = 50;
   ReadPresetData(qf, FALSE, 0, wBankNum, wPresetNum, state, wInstanceNum);
   if (IsBad())
      goto LoadPresetError;

   qf.sndmem->ActionList.Merge(snddatalist);
#  ifdef __USE_ARRAYCACHE
      ArrayCache::ReadOptimize();
#  endif
   currPercent = 100;
   if ((callBackFunction != NULL) &&
       (!callBackFunction(emuCallBackReading, currPercent)))
      SetError(QF_READ_FILE);

LoadPresetError:
   LoadCleanup();
}

void QFSF2Reader::LoadInst(QuickFont& qf, WORD wInstNdx, WORD wBankNum,
                           WORD wPresetNum, qfPresetState state,
                           WORD wInstanceNum)
{
   LoadSetup();
   if (IsBad())
      return;

   AddFileAndBankToLists(qf);
   if (IsBad())
      goto LoadInstError;

   GetInstRanges(state, *qf.sndmem, wInstNdx);
   if (IsBad())
      goto LoadInstError;
   CheckInstStereoPairs();

   currPercent = 50;
   ReadPresetData(qf, FALSE, 0, wBankNum, wPresetNum, state,
                  wInstanceNum);
   if (IsBad())
      goto LoadInstError;

   qf.sndmem->ActionList.Merge(snddatalist);
#  ifdef __USE_ARRAYCACHE
      ArrayCache::ReadOptimize();
#  endif
   currPercent = 100;
   if ((callBackFunction != NULL) &&
       (!callBackFunction(emuCallBackReading, currPercent)))
      SetError(QF_READ_FILE);

LoadInstError:
   LoadCleanup();
}

void QFSF2Reader::LoadSound(QuickFont& qf, WORD wSoundNdx, WORD wBankNum,
                            WORD wPresetNum, qfPresetState state,
                            WORD wInstanceNum)
{
   LoadSetup();
   if (IsBad())
      return;

   AddFileAndBankToLists(qf);
   if (IsBad())
      goto LoadSoundError;

   GetSampleRanges(state, *qf.sndmem, wSoundNdx);
   if (IsBad())
      goto LoadSoundError;
   CheckInstStereoPairs();

   currPercent = 50;
   ReadPresetData(qf, FALSE, 0, wBankNum, wPresetNum, state, wInstanceNum);
   if (IsBad())
      goto LoadSoundError;

   qf.sndmem->ActionList.Merge(snddatalist);
#  ifdef __USE_ARRAYCACHE
      ArrayCache::ReadOptimize();
#  endif
   currPercent = 100;
   if ((callBackFunction != NULL) &&
       (!callBackFunction(emuCallBackReading, currPercent)))
      SetError(QF_READ_FILE);

LoadSoundError:
   LoadCleanup();
}

void QFSF2Reader::CheckInstStereoPairs()
{
   WORD i;
   RangeList *tmpInstRangeList;
   sfSampleRange *tmpSampleRange;
   SampleLinkInfoList sliList;

   for (i = 0; i < wNumInst; i++)
   {
      if (instarr[i] != NULL)
      {
         sliList.DeleteList();
         tmpInstRangeList = &instarr[i]->range;
         for (tmpInstRangeList->First(); !tmpInstRangeList->EndOfList();
              tmpInstRangeList->Next())
         {
            tmpSampleRange = samplearr[tmpInstRangeList->GetCurItem()->wIndex];
            if ((tmpSampleRange != NULL) &&
                ((tmpSampleRange->sample->wSampleType & rightSample) ||
                 (tmpSampleRange->sample->wSampleType & leftSample)))
            {
               sliList.Find(tmpSampleRange->sample->wSampleLink);
               if (sliList.EndOfList())
                  sliList.Insert(new SampleLinkInfo(tmpSampleRange->wIndex,
                                       tmpSampleRange->sample->wSampleLink,
                                       FALSE));
               else
                  sliList.GetCurItem()->bFoundBoth = TRUE;
            }
         }

         for (tmpInstRangeList->First(); !tmpInstRangeList->EndOfList();
              tmpInstRangeList->Next())
         {
            tmpSampleRange = samplearr[tmpInstRangeList->GetCurItem()->wIndex];
            if (tmpSampleRange != NULL)
            {
               sliList.Find(tmpSampleRange->wIndex);
               if (sliList.EndOfList())
                  sliList.Find(tmpSampleRange->sample->wSampleLink);
               tmpInstRangeList->GetCurItem()->bStereo =
                  !sliList.EndOfList() && sliList.GetCurItem()->bFoundBoth;
            }
            else
               tmpInstRangeList->GetCurItem()->bStereo = FALSE;
         }
      }
   }
}

void QFSF2Reader::LoadSetup()
{
   WORD i;

   ClearError();
   if (trans == NULL)
   {
      SetError(QF_NO_TRANSLATOR);
      return;
   }
   trans->SetFileType("SF2");
   if (!bDefModTableInitialized)
   {
      SetDefaultModulators(&defModTable);
      bDefModTableInitialized = TRUE;
   }

   currPercent = lastPercent = 0;
   presetarr = NULL;
   instarr = NULL;
   samplearr = NULL;

   presetarr = (sfPresetRange **)NewAlloc(sizeof(sfPresetRange*)*wNumPhdr);
   if (presetarr == NULL)
      goto LoadSetupError;
   for (i = 0; i < wNumPhdr; i++)
      presetarr[i] = NULL;

   instarr = (sfInstRange**)NewAlloc(sizeof(sfInstRange*)*wNumInst);
   if (instarr == NULL)
      goto LoadSetupError;
   for (i = 0; i < wNumInst; i++)
      instarr[i] = NULL;

   samplearr = (sfSampleRange **)NewAlloc(sizeof(sfSampleRange *)*wNumShdr);
   if (samplearr == NULL)
      goto LoadSetupError;
   for (i = 0; i < wNumShdr; i++)
      samplearr[i] = NULL;

   snddatalist.DeleteList();
   return;

LoadSetupError:
   if (presetarr != NULL)
      DeleteAlloc(presetarr);
   if (instarr != NULL)
      DeleteAlloc(instarr);
   SetError(QF_ALLOCATE_MEM);
}

void QFSF2Reader::LoadCleanup()
{
   WORD i;

   if (presetarr != NULL)
   {
      for (i = 0; i < wNumPhdr; i++)
         if (presetarr[i] != NULL)
            delete presetarr[i];
      DeleteAlloc(presetarr);
   } 

   if (instarr != NULL)
   {
      for (i = 0; i < wNumInst; i++)
         if (instarr[i] != NULL)
            delete instarr[i];
      DeleteAlloc(instarr);
   } 

   if (samplearr != NULL)
   {
      for (i = 0; i < wNumShdr; i++)
         if (samplearr[i] != NULL)
            delete samplearr[i];
      DeleteAlloc(samplearr);
   } 
}

void QFSF2Reader::GetPresetRanges(qfPresetState state, SoundMemory& sndmem,
                                  DWORD dwLoadType)
{
   SWAP_WORD_BYTE_INCOH_DECLARATIONS;
   sfPresetList phdrlist;
   sfBag *pbagarray;
   sfGen *pgenarray;
   SHORT genarr[numSF2Gens], globalgenarr[numSF2Gens];
   Range *tmprange;
   WORD i, j;
   WORD wBagNdx1, wBagNdx2;
   WORD wGenNdx, wNumGen;
   WORD wNumBags, wTmp;
   BOOL bGlobal;
#  ifndef __NO_MOD_CTRL
   sfMod *pmodarray;
   sfModList modl, globalmodl;
   WORD wModNdx, wNumMod;
#  endif

   // Set pointers to NULL so cleanup doesn't try to delete them
   ClearError();
   pbagarray = NULL;
   pgenarray = NULL;
#  ifndef __NO_MOD_CTRL
      pmodarray = NULL;
#  endif
   tmprange = NULL;

   // Get the list of presets to load
   if (dwLoadType & enltBankLoad)
   {
      GetPhdr(phdrlist, 0, wNumPhdr);
      if (IsBad())
         return;
      wNumBags = wNumPbag;
   }
   else
   {
      GetPhdr(phdrlist, (WORD)(dwLoadType & enltMask), 2);
      if (IsBad())
         return;
      phdrlist.First();
      wBagNdx1 = phdrlist.GetCurItem()->wBagNdx;
      phdrlist.Next();
      wNumBags = (phdrlist.EndOfList() ? wNumPbag :
                  phdrlist.GetCurItem()->wBagNdx) - wBagNdx1 + 1;
   }

   // Get the PBAGs for the presets we want to load
   if ((pbagarray = new sfBag[wNumBags]) == NULL)
      goto GetPresetRangesAllocateError;

   tRIFF->SetCurPosition(PbagPos);
   tRIFF->StartDataStream();
   if (!(dwLoadType & enltBankLoad))
      tRIFF->SeekDataStream((DWORD)wBagNdx1*BAGLEN);
   tRIFF->ReadDataStream(pbagarray, (DWORD)wNumBags*BAGLEN);
   tRIFF->StopDataStream();

   // Only loop for byte swapping if we need to
#  ifdef __BYTE_INCOHERENT
      for (j = 0; j < wNumBags; j++)
      {
         SWAP_WORD_BYTE_INCOH_ONLY(pbagarray[j].wGenNdx);
         SWAP_WORD_BYTE_INCOH_ONLY(pbagarray[j].wModNdx);
      }
#  endif

   for (phdrlist.First(), i = 0;
        i < ((dwLoadType & enltBankLoad) ? wNumPhdr : 1); i++)
   {
      sfPresetRange *presetRange;

      // Don't load the preset if we're pushing a preset up into
      // precussive space
      if ((dwLoadType & enltBankLoad) &&
          (phdrlist.GetCurItem()->wBankNum < 128) &&
          ((dwLoadType & enltMask) < 128) &&
          ((phdrlist.GetCurItem()->wBankNum + (dwLoadType & enltMask)) > 127))
      {
         phdrlist.DeleteCurItem();
         continue;
      }

      if ((presetRange = new sfPresetRange) == NULL)
         goto GetPresetRangesAllocateError;
      presetarr[i] = presetRange;

      // Store the preset and get the indices to the PBAGs
      presetRange->preset = phdrlist.RemoveCurItem();
      wBagNdx1 = (dwLoadType & enltBankLoad) ? presetRange->preset->wBagNdx : 0;
      wBagNdx2 = (phdrlist.EndOfList() ? wNumPbag :
                  phdrlist.GetCurItem()->wBagNdx) -
                 ((dwLoadType & enltBankLoad) ? 0 : presetRange->preset->wBagNdx);
      // Check for invalid PBAG indices
      if ((wBagNdx1 > wBagNdx2) || (wBagNdx1 > wNumPbag) ||
          (wBagNdx2 > wNumPbag))
         goto GetPresetRangesReadError;
      else if (wBagNdx1 < wBagNdx2)
      {
         // Get the generator and (possibly) modulator indices for the
         // entire preset
         wGenNdx = pbagarray[wBagNdx1].wGenNdx;
         if (wGenNdx > wNumPgen)
            goto GetPresetRangesReadError;
#        ifndef __NO_MOD_CTRL
            wModNdx = pbagarray[wBagNdx1].wModNdx;
            if (wModNdx > wNumPmod)
               goto GetPresetRangesReadError;
#        endif
         if (wBagNdx2 < wNumPbag)
         {
            if ((pbagarray[wBagNdx2].wGenNdx < wGenNdx) ||
                (pbagarray[wBagNdx2].wGenNdx > wNumPgen))
               goto GetPresetRangesReadError;
            wNumGen = pbagarray[wBagNdx2].wGenNdx - wGenNdx;
#           ifndef __NO_MOD_CTRL
               if ((pbagarray[wBagNdx2].wModNdx < wModNdx) ||
                   (pbagarray[wBagNdx2].wModNdx > wNumPmod))
                  goto GetPresetRangesReadError;
               wNumMod = pbagarray[wBagNdx2].wModNdx - wModNdx;
#           endif
         }
         else
         {
            wNumGen = wNumPgen - wGenNdx;
#           ifndef __NO_MOD_CTRL
               wNumMod = wNumPmod - wModNdx;
#           endif
         }

         // Store the generators
         if ((pgenarray = new sfGen[wNumGen]) == NULL)
            goto GetPresetRangesAllocateError;

         tRIFF->SetCurPosition(PgenPos);
         tRIFF->StartDataStream();
         tRIFF->SeekDataStream((DWORD)wGenNdx*GENLEN);
         tRIFF->ReadDataStream(pgenarray, (DWORD)wNumGen*GENLEN);
         tRIFF->StopDataStream();

#        ifndef __NO_MOD_CTRL
         // Store the modulators
            if (((pmodarray = new sfMod[wNumMod]) == NULL) && (wNumMod > 0))
               goto GetPresetRangesAllocateError;

            tRIFF->SetCurPosition(PmodPos);
            tRIFF->StartDataStream();
            tRIFF->SeekDataStream((DWORD)wModNdx*MODLEN);
            tRIFF->ReadDataStream(pmodarray, (DWORD)wNumMod*MODLEN);
            tRIFF->StopDataStream();
#        endif

         // Only loop for byte swapping if we need to
#        ifdef __BYTE_INCOHERENT
            for (j = 0; j < wNumGen; j++)
            {
               SWAP_WORD_BYTE_INCOH_ONLY(pgenarray[j].wGenOper);
               SWAP_WORD_BYTE_INCOH_ONLY(pgenarray[j].wGenAmt);
            }
#           ifndef __NO_MOD_CTRL
               for (j = 0; j < wNumMod; j++)
               {
                  SWAP_WORD_BYTE_INCOH_ONLY(pmodarray[j].wModSrcOper);
                  SWAP_WORD_BYTE_INCOH_ONLY(pmodarray[j].wModDestOper);
                  SWAP_WORD_BYTE_INCOH_ONLY(pmodarray[j].shAmount);
                  SWAP_WORD_BYTE_INCOH_ONLY(pmodarray[j].wModAmtSrcOper);
                  SWAP_WORD_BYTE_INCOH_ONLY(pmodarray[j].wModTransOper);
               }
#           endif
#        endif

         // Set default generators
         bGlobal = FALSE;
         memset(genarr, 0, numSF2Gens*sizeof(SHORT));
         genarr[instrument] = SF2Default[instrument];
#  ifndef __NO_MOD_CTRL
            globalmodl.DeleteList();
#  endif
         wTmp = ((dwLoadType & enltBankLoad) ? presetRange->preset->wBagNdx : 0);
         // Get generators and modulators for first zone
         GetPresetGenModList(pbagarray, pgenarray, wNumGen,
#  ifndef __NO_MOD_CTRL
                             pmodarray, wNumMod,
#  endif
                             wBagNdx1, wTmp, genarr
#  ifndef __NO_MOD_CTRL
                             , modl
#  endif
                             );
         if (IsBad())
            goto GetPresetRangesError;

         // Check for global zone
         if (genarr[instrument] == SF2Default[instrument])
         {
            memcpy(globalgenarr, genarr, numSF2Gens*sizeof(SHORT));
#           ifndef __NO_MOD_CTRL
               globalmodl.Swap(modl);
#           endif
            wBagNdx1++;
            bGlobal = TRUE;
         }
         else
         {
            memset(globalgenarr, 0, numSF2Gens*sizeof(SHORT));
            globalgenarr[instrument] = SF2Default[instrument];
         }

         // Get generators and modulators for each zone in the preset
         for (j = wBagNdx1; (j < wBagNdx2) && IsOK(); j++)
         {
            // Check to see if we already processed the global zone or if
            // there really was one
            if (bGlobal || (j != wBagNdx1))
            {
               // Get the generators and modulators for the current zone
               memcpy(genarr, globalgenarr, numSF2Gens*sizeof(SHORT));
               wTmp = ((dwLoadType & enltBankLoad) ? presetRange->preset->wBagNdx : 0);
               if (wTmp > wNumPbag)
                  goto GetPresetRangesReadError;
               GetPresetGenModList(pbagarray, pgenarray, wNumGen,
#  ifndef __NO_MOD_CTRL
                                   pmodarray, wNumMod,
#  endif
                                   j, wTmp, genarr
#  ifndef __NO_MOD_CTRL
                                   , modl
#  endif
                                   );
               if (IsBad())
                  goto GetPresetRangesError;
            }
            if ((tmprange = new Range) == NULL)
               goto GetPresetRangesAllocateError;

#           ifndef __NO_MOD_CTRL
               modl.SetDuplicates(DupIgnore);
               modl += globalmodl;
#           endif
            // Get the generators and modulators for this zone out of the
            // array and place them in the range
            GetRangeInfo(*tmprange, genarr,
#  ifndef __NO_MOD_CTRL
                         modl,
#  endif
                         instrument);
            if (IsBad())
               goto GetPresetRangesError;
            presetRange->range.Insert(tmprange);
            tmprange = NULL;
         }

         delete [] pgenarray;
         pgenarray = NULL;
#        ifndef __NO_MOD_CTRL
            delete [] pmodarray;
            pmodarray = NULL;
#        endif
      }
      // Update the callback if necessary
      currPercent = ((dwLoadType & enltBankLoad) ? (13*i/wNumPhdr) : 13);
      if ((callBackFunction != NULL) &&
          (currPercent >= lastPercent+callBackPercent))
      {
         while (lastPercent+callBackPercent <= currPercent)
            lastPercent += callBackPercent;
         if (!callBackFunction(emuCallBackReading, currPercent))
            goto GetPresetRangesReadError;
      }
   }

   delete [] pbagarray;
   pbagarray = NULL;

   // Now process the instruments
   GetInstRanges(state, sndmem, enltBankLoad);
   return;

   // Error handling and cleanup
GetPresetRangesAllocateError:
   SetError(QF_ALLOCATE_MEM);
   goto GetPresetRangesError;

GetPresetRangesReadError:
   SetError(QF_READ_FILE);

GetPresetRangesError:
   if (pbagarray != NULL)
      delete [] pbagarray;
   if (pgenarray != NULL)
      delete [] pgenarray;
#  ifndef __NO_MOD_CTRL
      if (pmodarray != NULL)
         delete [] pmodarray;
#  endif
   if (tmprange != NULL)
      delete tmprange;
}

void QFSF2Reader::GetInstRanges(qfPresetState state, SoundMemory& sndmem,
                                DWORD dwLoadType)
{
   SWAP_WORD_BYTE_INCOH_DECLARATIONS;
   sfInstList instlist;
   sfBag *ibagarray;
   sfGen *igenarray;
   SHORT genarr[numSF2Gens], globalgenarr[numSF2Gens];
   Range *tmprange;
   RangeList *rangelist;
   WORD i, j;
   WORD wBagNdx1, wBagNdx2;
   WORD *refarray;
   WORD wGenNdx, wNumGen;
   WORD wNumBags, wTmp;
   BOOL bGlobal;
#  ifndef __NO_MOD_CTRL
   sfMod *imodarray;
   sfModList modl, globalmodl;
   WORD wModNdx, wNumMod;
#  endif

   // Set pointers to NULL so cleanup doesn't try to delete them
   ClearError();
   ibagarray = NULL;
   igenarray = NULL;
#  ifndef __NO_MOD_CTRL
      imodarray = NULL;
#  endif
   tmprange = NULL;
   refarray = NULL;

   // If loading the entire bank, get all the instruments
   if (dwLoadType & enltBankLoad)
   {
      // Store reference count for each instrument
      if ((refarray = (WORD *)NewAlloc(wNumInst*sizeof(WORD))) == NULL)
         goto GetInstRangesAllocateError;
      memset(refarray, 0, wNumInst*sizeof(WORD));
      for (i = 0; i < wNumPhdr; i++)
      {
         // Only check presets that are being loaded
         if (presetarr[i] != NULL)
         {
            rangelist = &presetarr[i]->range;
            // Update the reference count for the instrument
            for (rangelist->First(); !rangelist->EndOfList(); rangelist->Next())
            {
               if ((wTmp = rangelist->GetCurItem()->wIndex) >= wNumInst)
                  goto GetInstRangesReadError;
               refarray[wTmp]++;
            }
         }
      }
      // Get the list of instruments
      GetInst(instlist, 0, wNumInst);
      if (IsBad())
         goto GetInstRangesError;
      wNumBags = wNumIbag;
   }
   // If only loading a single instrument, just get what we need
   else
   {
      // Get the list of instruments
      GetInst(instlist, (WORD)(dwLoadType & enltMask), 2);
      if (IsBad())
         return;
      instlist.First();

      // Create a dummy preset that points to the instrument
      if ((presetarr[0] = new sfPresetRange) == NULL)
         goto GetInstRangesAllocateError;
      if ((presetarr[0]->preset = new sfPreset) == NULL)
         goto GetInstRangesAllocateError;
      presetarr[0]->preset->strPresetName = instlist.GetCurItem()->strInstName;
      if ((tmprange = new Range) == NULL)
         goto GetInstRangesAllocateError;
      tmprange->wIndex = 0;
      presetarr[0]->range.Insert(tmprange);
      wBagNdx1 = instlist.GetCurItem()->wBagNdx;
      instlist.Next();
      wNumBags = (instlist.EndOfList() ? wNumIbag :
                  instlist.GetCurItem()->wBagNdx) - wBagNdx1 + 1;
   }

   // Get the IBAGs for the instruments we want to load
   if ((ibagarray = new sfBag[wNumBags]) == NULL)
      goto GetInstRangesAllocateError;
   tRIFF->SetCurPosition(IbagPos);
   tRIFF->StartDataStream();
   if (!(dwLoadType & enltBankLoad))
      tRIFF->SeekDataStream((DWORD)wBagNdx1*BAGLEN);
   tRIFF->ReadDataStream(ibagarray, (DWORD)wNumBags*BAGLEN);
   tRIFF->StopDataStream();

   // Only loop for byte swapping if we need to
#  ifdef __BYTE_INCOHERENT
      for (j = 0; j < wNumBags; j++)
      {
         SWAP_WORD_BYTE_INCOH_ONLY(ibagarray[j].wGenNdx);
         SWAP_WORD_BYTE_INCOH_ONLY(ibagarray[j].wModNdx);
      }
#  endif

   for (instlist.First(), i = 0;
        i < ((dwLoadType & enltBankLoad) ? wNumInst : 1); i++)
   {
      sfInstRange *instRange;

      // Only process the instruments we want to load
      if (!(dwLoadType & enltBankLoad) || (refarray[i] > 0))
      {
         // Store the instrument and get the indices to the IBAGs
         if ((instRange = new sfInstRange) == NULL)
            goto GetInstRangesAllocateError;
         instarr[i] = instRange;

         instRange->inst = instlist.RemoveCurItem();
         instRange->wRefCount = (dwLoadType & enltBankLoad) ? refarray[i] : 1;
         wBagNdx1 = (dwLoadType & enltBankLoad) ? instRange->inst->wBagNdx : 0;
         wBagNdx2 = (instlist.EndOfList() ? wNumIbag :
                     instlist.GetCurItem()->wBagNdx) -
                    ((dwLoadType & enltBankLoad) ? 0 : instRange->inst->wBagNdx);
         // Check for invalid IBAG indices
         if ((wBagNdx1 > wBagNdx2) || (wBagNdx1 > wNumIbag) ||
             (wBagNdx2 > wNumIbag))
            goto GetInstRangesReadError;
         else if (wBagNdx1 < wBagNdx2)
         {
            // Get the generator and (possibly) modulator indices for the
            // entire instrument
            wGenNdx = ibagarray[wBagNdx1].wGenNdx;
            if (wGenNdx > wNumIgen)
               goto GetInstRangesReadError;
#           ifndef __NO_MOD_CTRL
               wModNdx = ibagarray[wBagNdx1].wModNdx;
               if (wModNdx > wNumImod)
                  goto GetInstRangesReadError;
#           endif
            if (wBagNdx2 < wNumIbag)
            {
               if ((ibagarray[wBagNdx2].wGenNdx < wGenNdx) ||
                   (ibagarray[wBagNdx2].wGenNdx > wNumIgen))
                  goto GetInstRangesReadError;
               wNumGen = ibagarray[wBagNdx2].wGenNdx - wGenNdx;
#              ifndef __NO_MOD_CTRL
                  if ((ibagarray[wBagNdx2].wModNdx < wModNdx) ||
                      (ibagarray[wBagNdx2].wModNdx > wNumImod))
                     goto GetInstRangesReadError;
                  wNumMod = ibagarray[wBagNdx2].wModNdx - wModNdx;
#              endif
            }
            else
            {
               wNumGen = wNumIgen - wGenNdx;
#              ifndef __NO_MOD_CTRL
                  wNumMod = wNumImod - wModNdx;
#              endif
            }

            // Store the generators
            if ((igenarray = new sfGen[wNumGen]) == NULL)
               goto GetInstRangesAllocateError;

            tRIFF->SetCurPosition(IgenPos);
            tRIFF->StartDataStream();
            tRIFF->SeekDataStream((DWORD)wGenNdx*GENLEN);
            tRIFF->ReadDataStream(igenarray, (DWORD)wNumGen*GENLEN);
            tRIFF->StopDataStream();

#           ifndef __NO_MOD_CTRL
            // Store the modulators
               if (((imodarray = new sfMod[wNumMod]) == NULL) && (wNumMod > 0))
                  goto GetInstRangesAllocateError;

               tRIFF->SetCurPosition(ImodPos);
               tRIFF->StartDataStream();
               tRIFF->SeekDataStream((DWORD)wModNdx*MODLEN);
               tRIFF->ReadDataStream(imodarray, (DWORD)wNumMod*MODLEN);
               tRIFF->StopDataStream();
#           endif

            // Only loop for byte swapping if we need to
#           ifdef __BYTE_INCOHERENT
               for (j = 0; j < wNumGen; j++)
               {
                  SWAP_WORD_BYTE_INCOH_ONLY(igenarray[j].wGenOper);
                  SWAP_WORD_BYTE_INCOH_ONLY(igenarray[j].wGenAmt);
               }
#              ifndef __NO_MOD_CTRL
                  for (j = 0; j < wNumMod; j++)
                  {
                     SWAP_WORD_BYTE_INCOH_ONLY(imodarray[j].wModSrcOper);
                     SWAP_WORD_BYTE_INCOH_ONLY(imodarray[j].wModDestOper);
                     SWAP_WORD_BYTE_INCOH_ONLY(imodarray[j].shAmount);
                     SWAP_WORD_BYTE_INCOH_ONLY(imodarray[j].wModAmtSrcOper);
                     SWAP_WORD_BYTE_INCOH_ONLY(imodarray[j].wModTransOper);
                  }
#              endif
#           endif

            // Set default generators
            bGlobal = FALSE;
            memcpy(genarr, SF2Default, numSF2Gens*sizeof(SHORT));
#  ifndef __NO_MOD_CTRL
            globalmodl.DeleteList();
#  endif
            wTmp = ((dwLoadType & enltBankLoad) ? instRange->inst->wBagNdx : 0);
            // Get generators and modulators for first zone
            GetInstGenModList(ibagarray, igenarray, wNumGen,
#  ifndef __NO_MOD_CTRL
                              imodarray, wNumMod,
#  endif
                              wBagNdx1, wTmp, genarr
#  ifndef __NO_MOD_CTRL
                              , modl
#  endif
                              );
            if (IsBad())
               goto GetInstRangesError;

            // Check for global zone
            if (genarr[sampleId] == SF2Default[sampleId])
            {
               memcpy(globalgenarr, genarr, numSF2Gens*sizeof(SHORT));
#              ifndef __NO_MOD_CTRL
                  globalmodl.Swap(modl);
#              endif
               wBagNdx1++;
               bGlobal = TRUE;
            }
            else
               memcpy(globalgenarr, SF2Default, numSF2Gens*sizeof(SHORT));

            // Get generators and modulators for each zone in the instrument
            for (j = wBagNdx1; (j < wBagNdx2) && IsOK(); j++)
            {
               // Check to see if we already processed the global zone or if
               // there really was one
               if (bGlobal || (j != wBagNdx1))
               {
                  // Get the generators and modulators for the current zone
                  memcpy(genarr, globalgenarr, numSF2Gens*sizeof(SHORT));
                  wTmp = ((dwLoadType & enltBankLoad) ? instRange->inst->wBagNdx : 0);
                  if (wTmp > wNumIbag)
                     goto GetInstRangesReadError;
                  GetInstGenModList(ibagarray, igenarray, wNumGen,
#  ifndef __NO_MOD_CTRL
                                    imodarray, wNumMod,
#  endif
                                    j, wTmp, genarr
#  ifndef __NO_MOD_CTRL
                                    , modl
#  endif
                                    );
                  if (IsBad())
                     goto GetInstRangesError;
               }
               if ((tmprange = new Range) == NULL)
                  goto GetInstRangesAllocateError;
#              ifndef __NO_MOD_CTRL
                  modl.SetDuplicates(DupIgnore);
                  modl += globalmodl;
#              endif
               // Get the generators and modulators for this zone out of the
               // array and place them in the range
               GetRangeInfo(*tmprange, genarr,
#              ifndef __NO_MOD_CTRL
                            modl,
#              endif
                            sampleId);
               if (IsBad())
                  goto GetInstRangesError;
               instRange->range.Insert(tmprange);
               tmprange = NULL;
            }

            delete [] igenarray;
            igenarray = NULL;
#           ifndef __NO_MOD_CTRL
               delete [] imodarray;
               imodarray = NULL;
#           endif
         }
      }
      else
         instlist.DeleteCurItem();

      // Update the callback if necessary
      currPercent = 13 + ((dwLoadType & enltBankLoad) ? (25*i/wNumInst) : 25);
      if ((callBackFunction != NULL) &&
          (currPercent >= lastPercent+callBackPercent))
      {
         while (lastPercent+callBackPercent <= currPercent)
            lastPercent += callBackPercent;
         if (!callBackFunction(emuCallBackReading, currPercent))
            goto GetInstRangesReadError;
      }
   }

   delete [] ibagarray;
   ibagarray = NULL;
   if (refarray != NULL)
      DeleteAlloc(refarray);
   refarray = NULL;
   
   // Now process the samples
   GetSampleRanges(state, sndmem, enltBankLoad);
   return;

   // Error handling and cleanup
GetInstRangesAllocateError:
   SetError(QF_ALLOCATE_MEM);
   goto GetInstRangesError;

GetInstRangesReadError:
   SetError(QF_READ_FILE);

GetInstRangesError:
   if (ibagarray != NULL)
      delete [] ibagarray;
   if (igenarray != NULL)
      delete [] igenarray;
#  ifndef __NO_MOD_CTRL
      if (imodarray != NULL)
         delete [] imodarray;
#  endif
   if (tmprange != NULL)
      delete tmprange;
   if (refarray != NULL)
      DeleteAlloc(refarray);
}

void QFSF2Reader::GetSampleRanges(qfPresetState state, SoundMemory& sndmem,
                                  DWORD dwLoadType)
{
   sfSampleList shdrlist;
   SoundDataMatch tmpsdm;
   Range *tmprange, *presetRange, *instRange;
   RangeList *prangelist, *irangelist;
   WORD i, *refarray, wNumSamplesUsed, wTmp;
   BOOL bLoadAllSamples;
   QFRange tmpQFRange;

   // Set pointers to NULL so cleanup doesn't try to delete them
   ClearError();
   refarray = NULL;
   tmprange = NULL;

   // If loading the entire bank, store reference count for each sample
   if (dwLoadType & enltBankLoad)
   {
      if ((refarray = (WORD *)NewAlloc(wNumShdr*sizeof(WORD))) == NULL)
         goto GetSampleRangesAllocateError;
      memset(refarray, 0, wNumShdr*sizeof(WORD));
      wNumSamplesUsed = 0;
      for (i = 0; i < wNumPhdr; i++)
      {
         // Only check presets that are being loaded
         if (presetarr[i] != NULL)
         {
            prangelist = &presetarr[i]->range;
            for (prangelist->First(); !prangelist->EndOfList();
                 prangelist->Next())
            {
               presetRange = prangelist->GetCurItem();
               // Only check instruments that are being loaded
               if (presetRange->wIndex >= wNumInst)
                  goto GetSampleRangesReadError;
               if (instarr[presetRange->wIndex] != NULL)
               {
                  irangelist = &instarr[presetRange->wIndex]->range;
                  for (irangelist->First(); !irangelist->EndOfList();
                       irangelist->Next())
                  {
                     instRange = irangelist->GetCurItem();
                     SetQFRange(&tmpQFRange, *presetRange, *instRange);
                     // Update the reference count for the sample
                     if ((tmpQFRange.byKeyLow <= tmpQFRange.byKeyHigh) &&
                         (tmpQFRange.byVelLow <= tmpQFRange.byVelHigh))
                     {
                        if ((wTmp = irangelist->GetCurItem()->wIndex) >= wNumShdr)
                           goto GetSampleRangesReadError;
                        if (refarray[wTmp]++ == 0)
                           wNumSamplesUsed++;
                     }
                  }
               }
            }
         }
      }
   }
   // If only loading a single sample, just get what we need
   else
   {
      // Get the sample
      GetShdr(shdrlist, (WORD)(dwLoadType & enltMask), 1);
      if (IsBad())
         return;

      // Create a dummy preset and instrument that point to the sample
      if ((presetarr[0] = new sfPresetRange) == NULL)
         goto GetSampleRangesAllocateError;
      if ((presetarr[0]->preset = new sfPreset) == NULL)
         goto GetSampleRangesAllocateError;
      presetarr[0]->preset->strPresetName = shdrlist.GetCurItem()->strSampleName;
      if ((tmprange = new Range) == NULL)
         goto GetSampleRangesAllocateError;
      tmprange->wIndex = 0;
      presetarr[0]->range.Insert(tmprange);
      tmprange = NULL;

      if ((instarr[0] = new sfInstRange) == NULL)
         goto GetSampleRangesAllocateError;
      if ((instarr[0]->inst = new sfInst) == NULL)
         goto GetSampleRangesAllocateError;
      instarr[0]->inst->strInstName = shdrlist.GetCurItem()->strSampleName;
      if ((tmprange = new Range) == NULL)
         goto GetSampleRangesAllocateError;
      tmprange->wIndex = 0;
      instarr[0]->range.Insert(tmprange);
      tmprange = NULL;
   }

   // Get all the sample headers if more than half of them are being loaded
   bLoadAllSamples = ((dwLoadType & enltBankLoad) && (wNumSamplesUsed >= wNumShdr/2) ||
                      !(dwLoadType & enltBankLoad));
   if (bLoadAllSamples && (dwLoadType & enltBankLoad))
   {
      // Get the list of samples
      GetShdr(shdrlist, 0, wNumShdr);
      if (IsBad())
         goto GetSampleRangesError;
   }

   for (i = 0; i < ((dwLoadType & enltBankLoad) ? wNumShdr : 1); i++)
   {
      // Only process the samples we want to load
      if (!(dwLoadType & enltBankLoad) || (refarray[i] > 0))
      {
         sfSampleRange *sampleRange;

         if ((sampleRange = new sfSampleRange) == NULL)
            goto GetSampleRangesAllocateError;
         samplearr[i] = sampleRange;

         // Get the sample header if we're haven't already gotten them all
         if (!bLoadAllSamples)
         {
            GetShdr(shdrlist, i, 1);
            if (IsBad())
               goto GetSampleRangesError;
         }
         sampleRange->sample = shdrlist.RemoveCurItem();
         // Check for valid sample header info
         if (sampleRange->sample->dwEnd <= sampleRange->sample->dwStart)
             goto GetSampleRangesReadError;
         if (sampleRange->sample->dwEndLoop > sampleRange->sample->dwEnd)
             sampleRange->sample->dwEndLoop = sampleRange->sample->dwEnd;
         if ((sampleRange->sample->dwStartLoop < sampleRange->sample->dwStart) ||
             (sampleRange->sample->dwStartLoop > sampleRange->sample->dwEndLoop))
             sampleRange->sample->dwStartLoop = sampleRange->sample->dwStart;

         sampleRange->wIndex = (dwLoadType & enltBankLoad) ? i :
                                                 (WORD)(dwLoadType & enltMask);
         sampleRange->wRefCount = (dwLoadType & enltBankLoad) ? refarray[i] : 1;
         // If loading a ROM sample, check for the correct ROM id
         if (sampleRange->sample->wSampleType & ROMSample)
         {
            if (strROMid != strCurrROMid)
               goto GetSampleRangesROMError;
         }
         else
         {
            // Check to see if the sample is already loaded
            sndmem.fileNameList.Find(strFileName);
            tmpsdm.strFileName = (CHAR *)(*sndmem.fileNameList.GetCurItem());
            tmpsdm.strSoundName = &sampleRange->sample->strSampleName;
            sndmem.SoundList.Find(&tmpsdm);
            if ((sampleRange->snddata =
                 sndmem.SoundList.GetCurItem()) != NULL)
            {
               if ((sampleRange->snddata->wInUseRefCount == 0) &&
                   (state != qfpsSamplesNotAvailable))
               {
                  sndmem.SoundList.RemoveCurItem();
                  sndmem.ActionList.Insert(sampleRange->snddata);
               }
               sampleRange->snddata->wAllocatedRefCount += 
                  (dwLoadType & enltBankLoad) ? refarray[i] : 1;
               sampleRange->snddata->wInUseRefCount +=
                  ((state != qfpsSamplesNotAvailable) ?
                     ((dwLoadType & enltBankLoad) ? refarray[i] : 1) : 0);
            }
            else
            {
               // If not, create a new one
               if ((sampleRange->snddata = new SoundData) == NULL)
                  goto GetSampleRangesAllocateError;
               sampleRange->snddata->wAllocatedRefCount =
                                  (dwLoadType & enltBankLoad) ? refarray[i] : 1;
               sampleRange->snddata->wInUseRefCount =
                  ((state != qfpsSamplesNotAvailable) ?
                     ((dwLoadType & enltBankLoad) ? refarray[i] : 1) : 0);
               sampleRange->snddata->strFileName = tmpsdm.strFileName;
               sampleRange->snddata->strSoundName =
                                        sampleRange->sample->strSampleName;
               sampleRange->snddata->dwStartLocInBytes =
                                  sampleRange->sample->dwStart*2 + lSmplPos;
               sampleRange->snddata->dwSizeInBytes =
                  (sampleRange->sample->dwEnd -
                   sampleRange->sample->dwStart)*2;
               // Hack for A3 workaround
//               sampleRange->snddata->dwCurrOffset = 0;
               if ((sampleRange->sample->dwEndLoop -
                    sampleRange->sample->dwStartLoop) >= 0x800)
                  sampleRange->snddata->dwCurrOffset = 
                      (sampleRange->sample->dwStartLoop -
                       sampleRange->sample->dwStart)*2;
               else
                   sampleRange->snddata->dwCurrOffset = 0;
               sampleRange->snddata->bySoundDataType = sdt16BitMask;
               snddatalist.Insert(sampleRange->snddata);
            }
         }
      }
      else
         shdrlist.DeleteCurItem();

      // Update the callback if necessary
      currPercent = 38 + ((dwLoadType & enltBankLoad) ? (12*i/wNumShdr) : 12);
      if ((callBackFunction != NULL) &&
          (currPercent >= lastPercent+callBackPercent))
      {
         while (lastPercent+callBackPercent <= currPercent)
         {
            lastPercent += callBackPercent;
         }
         if (!callBackFunction(emuCallBackReading, currPercent))
            goto GetSampleRangesReadError;
      }
   }

   if (refarray != NULL)
      DeleteAlloc(refarray);
   refarray = NULL;
   return;

   // Error handling and cleanup
GetSampleRangesAllocateError:
   SetError(QF_ALLOCATE_MEM);
   goto GetSampleRangesError;

GetSampleRangesROMError:
   SetError(QF_WRONG_ROM_ID);
   goto GetSampleRangesError;

GetSampleRangesReadError:
   SetError(QF_READ_FILE);

GetSampleRangesError:
   if (tmprange != NULL)
      delete tmprange;
   if (refarray != NULL)
      DeleteAlloc(refarray);
}

void QFSF2Reader::GetPresetGenModList(sfBag *pbagarray, sfGen *pgenarray,
                                      WORD wNumGen,
#  ifndef __NO_MOD_CTRL
                                      sfMod *pmodarray, WORD wNumMod,
#  endif
                                      WORD wBagNdx,
                                      WORD wBagOffset, SHORT genarr[]
#  ifndef __NO_MOD_CTRL
                                      , sfModList& modl
#  endif
                                      )
{
   WORD wGenNdx1, wGenNdx2, i;
#  ifndef __NO_MOD_CTRL
   WORD wModNdx1, wModNdx2;
   sfMod *tmpmod;
#  endif

   ClearError();
   if (pbagarray[wBagNdx].wGenNdx < pbagarray[wBagOffset].wGenNdx)
      goto GetPresetGenModListReadError;
   wGenNdx1 = pbagarray[wBagNdx].wGenNdx - pbagarray[wBagOffset].wGenNdx;
#  ifndef __NO_MOD_CTRL
      if (pbagarray[wBagNdx].wModNdx < pbagarray[wBagOffset].wModNdx)
         goto GetPresetGenModListReadError;
      wModNdx1 = pbagarray[wBagNdx].wModNdx - pbagarray[wBagOffset].wModNdx;
#  endif
   if (wBagNdx+1 < wNumPbag)
   {
      if (pbagarray[wBagNdx+1].wGenNdx < pbagarray[wBagOffset].wGenNdx)
         goto GetPresetGenModListReadError;
      wGenNdx2 = pbagarray[wBagNdx+1].wGenNdx - pbagarray[wBagOffset].wGenNdx;
#  ifndef __NO_MOD_CTRL
      if (pbagarray[wBagNdx+1].wModNdx < pbagarray[wBagOffset].wModNdx)
         goto GetPresetGenModListReadError;
      wModNdx2 = pbagarray[wBagNdx+1].wModNdx - pbagarray[wBagOffset].wModNdx;
#  endif
   }
   else
   {
      if (wNumPgen < pbagarray[wBagOffset].wGenNdx)
         goto GetPresetGenModListReadError;
      wGenNdx2 = wNumPgen - pbagarray[wBagOffset].wGenNdx;
#  ifndef __NO_MOD_CTRL
      if (wNumPmod < pbagarray[wBagOffset].wModNdx)
         goto GetPresetGenModListReadError;
      wModNdx2 = wNumPmod - pbagarray[wBagOffset].wModNdx;
#  endif
   }

   if ((wGenNdx1 > wGenNdx2) || (wGenNdx1 > wNumGen) || (wGenNdx2 > wNumGen))
      goto GetPresetGenModListReadError;
#  ifndef __NO_MOD_CTRL
   if ((wModNdx1 > wModNdx2) || (wModNdx1 > wNumMod) || (wModNdx2 > wNumMod))
      goto GetPresetGenModListReadError;
#  endif

   for (i = wGenNdx1; i < wGenNdx2; i++)
      switch (pgenarray[i].wGenOper) { // Check for invalid generators
      case startAddrsOffset:
      case endAddrsOffset:
      case startloopAddrsOffset:
      case endloopAddrsOffset:
      case startAddrsCoarseOffset:
      case endAddrsCoarseOffset:
      case startloopAddrsCoarseOffset:
      case keynum:
      case velocity:
      case endloopAddrsCoarseOffset:
      case sampleModes:
      case scaleTuning:
      case exclusiveClass:
      case overridingRootKey:
         break;
      default:
         if (pgenarray[i].wGenOper < numSF2Gens)
            genarr[pgenarray[i].wGenOper] = pgenarray[i].wGenAmt;
      }
#  ifndef __NO_MOD_CTRL
      modl.DeleteList();
      for (i = wModNdx1; i < wModNdx2; i++)
      {
         if ((tmpmod = new sfMod) == NULL)
         {
            SetError(QF_ALLOCATE_MEM);
            return;
         }
         tmpmod->wModSrcOper = pmodarray[i].wModSrcOper;
         tmpmod->wModDestOper = pmodarray[i].wModDestOper;
         tmpmod->shAmount = pmodarray[i].shAmount;
         tmpmod->wModAmtSrcOper = pmodarray[i].wModAmtSrcOper;
         tmpmod->wModTransOper = pmodarray[i].wModTransOper;
         modl.Insert(tmpmod);
      }
#  endif
   return;

GetPresetGenModListReadError:
   SetError(QF_READ_FILE);
}

void QFSF2Reader::GetInstGenModList(sfBag *ibagarray, sfGen *igenarray,
                                    WORD wNumGen,
#  ifndef __NO_MOD_CTRL
                                    sfMod *imodarray, WORD wNumMod,
#  endif
                                    WORD wBagNdx,
                                    WORD wBagOffset, SHORT genarr[]
#  ifndef __NO_MOD_CTRL
                                    , sfModList& modl
#  endif
                                    )
{
   WORD wGenNdx1, wGenNdx2, i;
#  ifndef __NO_MOD_CTRL
   WORD wModNdx1, wModNdx2;
   sfMod *tmpmod;
#  endif

   ClearError();
   if (ibagarray[wBagNdx].wGenNdx < ibagarray[wBagOffset].wGenNdx)
      goto GetInstGenModListReadError;
   wGenNdx1 = ibagarray[wBagNdx].wGenNdx - ibagarray[wBagOffset].wGenNdx;
#  ifndef __NO_MOD_CTRL
      if (ibagarray[wBagNdx].wModNdx < ibagarray[wBagOffset].wModNdx)
         goto GetInstGenModListReadError;
      wModNdx1 = ibagarray[wBagNdx].wModNdx - ibagarray[wBagOffset].wModNdx;
#  endif
   if (wBagNdx+1 < wNumIbag)
   {
      if (ibagarray[wBagNdx+1].wGenNdx < ibagarray[wBagOffset].wGenNdx)
         goto GetInstGenModListReadError;
      wGenNdx2 = ibagarray[wBagNdx+1].wGenNdx - ibagarray[wBagOffset].wGenNdx;
#  ifndef __NO_MOD_CTRL
      if (ibagarray[wBagNdx+1].wModNdx < ibagarray[wBagOffset].wModNdx)
         goto GetInstGenModListReadError;
      wModNdx2 = ibagarray[wBagNdx+1].wModNdx - ibagarray[wBagOffset].wModNdx;
#  endif
   }
   else
   {
      if (wNumIgen < ibagarray[wBagOffset].wGenNdx)
         goto GetInstGenModListReadError;
      wGenNdx2 = wNumIgen - ibagarray[wBagOffset].wGenNdx;
#  ifndef __NO_MOD_CTRL
      if (wNumImod < ibagarray[wBagOffset].wModNdx)
         goto GetInstGenModListReadError;
      wModNdx2 = wNumImod - ibagarray[wBagOffset].wModNdx;
#  endif
   }

   if ((wGenNdx1 > wGenNdx2) || (wGenNdx1 > wNumGen) || (wGenNdx2 > wNumGen))
      goto GetInstGenModListReadError;
#  ifndef __NO_MOD_CTRL
   if ((wModNdx1 > wModNdx2) || (wModNdx1 > wNumMod) || (wModNdx2 > wNumMod))
      goto GetInstGenModListReadError;
#  endif

   for (i = wGenNdx1; i < wGenNdx2; i++)
      if (igenarray[i].wGenOper < numSF2Gens)
         genarr[igenarray[i].wGenOper] = igenarray[i].wGenAmt;
#  ifndef __NO_MOD_CTRL
      modl.DeleteList();
      for (i = wModNdx1; i < wModNdx2; i++)
      {
         if ((tmpmod = new sfMod) == NULL)
         {
            SetError(QF_ALLOCATE_MEM);
            return;
         }
         tmpmod->wModSrcOper = imodarray[i].wModSrcOper;
         tmpmod->wModDestOper = imodarray[i].wModDestOper;
         tmpmod->shAmount = imodarray[i].shAmount;
         tmpmod->wModAmtSrcOper = imodarray[i].wModAmtSrcOper;
         tmpmod->wModTransOper = imodarray[i].wModTransOper;
         modl.Insert(tmpmod);
      }
#  endif
   return;

GetInstGenModListReadError:
   SetError(QF_READ_FILE);
}

void QFSF2Reader::SetKeyVelRanges(QFPreset *preset, QFRangeList *qfrl)
{
   QFKeyVelRange *tmpKVRange;
   QFKeyVelRangeList keylist, vellist;
   QFRange *tmpRange;

   for (qfrl->First(); !qfrl->EndOfList(); qfrl->Next())
   {
      tmpRange = qfrl->GetCurItem();
      tmpKVRange = new QFKeyVelRange;
      tmpKVRange->byLow = tmpRange->byKeyLow;
      tmpKVRange->byHigh = tmpRange->byKeyHigh;
      keylist.Insert(tmpKVRange);
      tmpKVRange = new QFKeyVelRange;
      tmpKVRange->byLow = tmpRange->byVelLow;
      tmpKVRange->byHigh = tmpRange->byVelHigh;
      vellist.Insert(tmpKVRange);
   }

   if (keylist.GetNumItems() < vellist.GetNumItems())
   {
      preset->enkv = enkvKeyRange;
      preset->kvrange.Swap(keylist);
      for (qfrl->First(); !qfrl->EndOfList();)
      {
         tmpRange = qfrl->RemoveCurItem();
         for (preset->kvrange.First(); !preset->kvrange.EndOfList();
              preset->kvrange.Next())
         {
            tmpKVRange = preset->kvrange.GetCurItem();
            if ((tmpKVRange->byLow == tmpRange->byKeyLow) &&
                (tmpKVRange->byHigh == tmpRange->byKeyHigh))
            {
               tmpKVRange->range.Insert(tmpRange);
               break;
            }
         }
      }
   }
   else
   {
      preset->enkv = enkvVelRange;
      preset->kvrange.Swap(vellist);
      for (qfrl->First(); !qfrl->EndOfList();)
      {
         tmpRange = qfrl->RemoveCurItem();
         for (preset->kvrange.First(); !preset->kvrange.EndOfList();
              preset->kvrange.Next())
         {
            tmpKVRange = preset->kvrange.GetCurItem();
            if ((tmpKVRange->byLow == tmpRange->byVelLow) &&
                (tmpKVRange->byHigh == tmpRange->byVelHigh))
            {
               tmpKVRange->range.Insert(tmpRange);
               break;
            }
         }
      }
   }
}

void QFSF2Reader::ReadPresetData(QuickFont& qf, BOOL bBankLoad,
                                 WORD wPresetNdx, WORD wBankNum,
                                 WORD wPresetNum, qfPresetState state,
                                 WORD wInstanceNum)
{
   QFBank *tmpbank;
   QFPreset *tmppreset;
   sfPresetRange *tmpPresetRange;
   sfGenList genl, globalgenl;
#  ifndef __NO_MOD_CTRL
   sfModList modl, globalmodl;
#  endif
   WORD wTmpBankNum;
   BOOL bInsertBank, bValidPreset=FALSE;
   QFRangeList qfrl;

   ClearError();
   if ((tmpPresetRange = presetarr[wPresetNdx]) == NULL)
       return;

   wTmpBankNum = (bBankLoad ?
                  tmpPresetRange->preset->wBankNum + wBankNum :
                  wBankNum);
   qf.list.Find(wTmpBankNum);
   if (qf.list.EndOfList())
   {
      if ((tmpbank = new QFBank) == NULL)
      {
         SetError(QF_ALLOCATE_MEM);
         return;
      }
      tmpbank->wBankNum = wTmpBankNum;
      tmpbank->wParentBankNum = wBankNum;
      bInsertBank = TRUE;
   }
   else
   {
      tmpbank = qf.list.GetCurItem();
      bInsertBank = FALSE;
   }
   if ((tmppreset = new QFPreset) == NULL)
   {
      if (bInsertBank)
         delete tmpbank;
      SetError(QF_ALLOCATE_MEM);
      return;
   }
   tmppreset->strPresetName = tmpPresetRange->preset->strPresetName;
   qf.sndmem->fileNameList.Find(strFileName);
   tmppreset->strFileName = (CHAR *)(*qf.sndmem->fileNameList.GetCurItem());
   qf.bankNameList.Find(bankInfo.strBankName);
   tmppreset->strBankName = (CHAR *)(*qf.bankNameList.GetCurItem());
   tmppreset->wPresetNum = (bBankLoad ? tmpPresetRange->preset->wPresetNum :
                            wPresetNum);
   tmppreset->bSelfLoaded = !bBankLoad;
   tmppreset->state = state;
   tmppreset->wInstanceNum = wInstanceNum;

   if (!tmpPresetRange->range.IsEmpty())
   {
      bValidPreset = TRUE;
      for (tmpPresetRange->range.First();
           !tmpPresetRange->range.EndOfList() && IsOK();
           tmpPresetRange->range.Next())
      {
         ReadInstData(&qfrl, qf,
                      *tmpPresetRange->range.GetCurItem());
      }
   }
   delete presetarr[wPresetNdx];
   presetarr[wPresetNdx] = NULL;

   if (IsOK() && bValidPreset)
   {
      SetKeyVelRanges(tmppreset, &qfrl);
      tmpbank->preset.Insert(tmppreset);
      if (bInsertBank)
         qf.list.Insert(tmpbank);
   }
   else
   {
      delete tmppreset;
      if (bInsertBank)
         delete tmpbank;
   }
}

void QFSF2Reader::ReadInstData(QFRangeList *qfrl, QuickFont& qf, Range& prange)
{
   sfInstList tmpinst;
   sfGenList genl, globalgenl;
#  ifndef __NO_MOD_CTRL
   sfModList modl, globalmodl;
#  endif
   QFRange *tmprange;
   sfInstRange *tmpInstRange;
   SampleLinkInfoList sliList;

   ClearError();
   tmpInstRange = instarr[prange.wIndex];

   if (!tmpInstRange->range.IsEmpty())
   {
      for (tmpInstRange->range.First();
           !tmpInstRange->range.EndOfList() && IsOK();
           tmpInstRange->range.Next())
      {
         if ((tmprange = new QFRange) == NULL)
         {
            SetError(QF_ALLOCATE_MEM);
            return;
         }
         SetQFRange(tmprange, prange, *tmpInstRange->range.GetCurItem());
         if ((tmprange->byKeyLow > tmprange->byKeyHigh) ||
             (tmprange->byVelLow > tmprange->byVelHigh))
            delete tmprange;
         else
            AddRange(qfrl, tmprange, prange,
                     *tmpInstRange->range.GetCurItem(), qf, sliList);
      }
   }
   if (--instarr[prange.wIndex]->wRefCount == 0)
   {
      delete instarr[prange.wIndex];
      instarr[prange.wIndex] = NULL;
   }

   wCurInst++;
}

void QFSF2Reader::AddRange(QFRangeList *qfrl, QFRange *tmprange, Range& prange,
                           Range& irange, QuickFont& qf,
                           SampleLinkInfoList& sliList)
{
   sfSampleList tmpshdr;
   sfSampleRange *tmpSampleRange;

   ClearError();

   tmpSampleRange = samplearr[irange.wIndex];

   trans->SetDefaultParameters();
   CombineLevels(prange, irange, tmpSampleRange->sample);
#  ifndef __NO_MOD_CTRL
      SetRangeMods(tmprange, prange, irange, qf);
      if (IsBad())
      {
         delete tmprange;
         return;
      }
#  endif

   trans->TranslateAndStore(tmprange->artdata);

   tmprange->artdata.soundptr = samplearr[irange.wIndex]->snddata;
   if ((tmpSampleRange->sample->wSampleType & rightSample) ||
       (tmpSampleRange->sample->wSampleType & leftSample))
   {
      sliList.Find(tmpSampleRange->sample->wSampleLink);
      if (sliList.EndOfList())
      {
         sliList.Insert(new SampleLinkInfo(tmpSampleRange->wIndex,
                                       tmpSampleRange->sample->wSampleLink,
                                       FALSE));
         tmprange->dwStereoLink =
            (wCurInst << 16) | tmpSampleRange->wIndex;
      }
      else
         tmprange->dwStereoLink =
            (wCurInst << 16) | tmpSampleRange->sample->wSampleLink;
   }
   else
      tmprange->dwStereoLink = 0;

   qfrl->Insert(tmprange);
   if (--samplearr[irange.wIndex]->wRefCount == 0)
   {
      delete samplearr[irange.wIndex];
      samplearr[irange.wIndex] = NULL;
   }
}

void QFSF2Reader::SetQFRange(QFRange *range, Range& prange, Range& irange)
{
   range->byKeyLow = MAX(prange.byKeyLow, irange.byKeyLow);
   range->byKeyHigh = MIN(prange.byKeyHigh, irange.byKeyHigh);
   range->byVelLow = MAX(prange.byVelLow, irange.byVelLow);
   range->byVelHigh = MIN(prange.byVelHigh, irange.byVelHigh);
}

void QFSF2Reader::CombineLevels(Range& prange, Range& irange,
                                sfSample *shdrinfo)
{
   trans->SetSoundParameters((void *)shdrinfo, irange.bStereo);
   SetArtData(irange, FALSE);
   SetArtData(prange, TRUE);
   //MG
   LONG atten=trans->GetSourceParameter(initialAttenuation);
   trans->SetParameter(initialAttenuation, (atten*4)/10, FALSE);
}

#ifndef __NO_MOD_CTRL
void QFSF2Reader::SetDefaultModulators(ModulatorTable *modTable)
{
	  // Scale tune 100 cents
	  modTable->NewModulator(ssKeyNumber, syPositiveBipolar,
                             trans->GetDestination(scaleTuning), 6400, stLinear,
                             ssEndSupportedSources, syEndSupportedSourceTypes);

	  // Velocity to volume
	  modTable->NewModulator(ssKeyOnVelocity, syNegativeUnipolar, slConcave,
                 trans->GetDestination(initialAttenuation), 960, stLinear,
                 ssEndSupportedSources, syEndSupportedSourceTypes, slLinear);
	  
	  // Velocity to filter cutoff
	  modTable->NewModulator(ssKeyOnVelocity, syNegativeUnipolar, slLinear,
                 trans->GetDestination(initialFilterFc), -2400, stLinear,
                 ssKeyOnVelocity, syNegativeUnipolar, slSwitch);
	  
	  // Pitch wheel
	  modTable->NewModulator(ssPitchWheel, syPositiveBipolar,
                 trans->GetDestination(fineTune), 12700, stLinear,
                 ssPitchBendSensitivity, syPositiveUnipolar);
	  
	  // Channel pressure
	  modTable->NewModulator(ssChanPressure, syPositiveUnipolar,
                 trans->GetDestination(vibLfoToPitch), 50, stLinear,
                 ssEndSupportedSources, syEndSupportedSourceTypes);
	  
	  // Mod wheel
	  modTable->NewModulator(ssCC1, syPositiveUnipolar,
                 trans->GetDestination(vibLfoToPitch), 50, stLinear,
                 ssEndSupportedSources, syEndSupportedSourceTypes);
	  
	  // Volume
	  modTable->NewModulator(ssCC7, syNegativeUnipolar, slConcave,
                 trans->GetDestination(initialAttenuation), 960, stLinear,
                 ssEndSupportedSources, syEndSupportedSourceTypes, slLinear);
	  
	  // Pan (A, B=1-A)
	  modTable->NewModulator(ssCC10, syPositiveBipolar,
                 trans->GetDestination(pan), 500, stLinear,
                 ssEndSupportedSources, syEndSupportedSourceTypes);
	  modTable->NewModulator(ssCC10, syPositiveBipolar,
                 trans->GetDestination(pan)+1, 500, stLinear,
                 ssEndSupportedSources, syEndSupportedSourceTypes);
	  
	  // Expression
	  modTable->NewModulator(ssCC11, syNegativeUnipolar, slConcave,
                 trans->GetDestination(initialAttenuation), 960, stLinear,
                 ssEndSupportedSources, syEndSupportedSourceTypes, slLinear);
	  
	  // Reverb (D)
	  modTable->NewModulator(ssCC91, syPositiveUnipolar,
                 trans->GetDestination(reverbEffectsSend), 200, stLinear,
                 ssEndSupportedSources, syEndSupportedSourceTypes);
	  
	  // Chorus (C)
	  modTable->NewModulator(ssCC93, syPositiveUnipolar,
                 trans->GetDestination(chorusEffectsSend), 200, stLinear,
                 ssEndSupportedSources, syEndSupportedSourceTypes);
	  
	  // Sound Controller: Harmonic Content
	  // modTable->NewModulator(ssCC71, syPositiveBipolar, sepInitialFilterQ, 220, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes);
	  
	  // Sound Controller: Release
	  // modTable->NewModulator(ssCC72, syPositiveBipolar, sepReleaseModEnv, 14400, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes);
	  
	  // Sound Controller: Attack
	  // modTable->NewModulator(ssCC73, syPositiveBipolar, sepAttackModEnv, 14400, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes);
	  
	  // Sound Controller: Brightness
	  // modTable->NewModulator(ssCC74, syPositiveBipolar, sepInitialFilterFc, -14400, stLinear, ssEndSupportedSources, syEndSupportedSourceTypes);
}

void QFSF2Reader::SetRangeMods(QFRange *tmprange, Range& prange,
                               Range& irange, QuickFont& qf)
{
   ModulatorTable *tmpmod;
   WORD i;
   enSupportedSources src, asrc;
   enSupportedSourceType srctype, asrctype;

   ClearError();
   if ((prange.wNumMods == 0) && (irange.wNumMods == 0) &&
       (trans->GetSourceParameter(scaleTuning) == SF2Default[scaleTuning]) &&
       (trans->GetSourceParameter(keynumToVolEnvHold) == SF2Default[keynumToVolEnvHold]) &&
       (trans->GetSourceParameter(keynumToVolEnvDecay) == SF2Default[keynumToVolEnvDecay]))
   {
      tmprange->artdata.modtable = &defModTable;
      defModTable.AddRef();
   }
   else
   {
      if ((tmpmod = new ModulatorTable) == NULL)
      {
         SetError(QF_ALLOCATE_MEM);
         return;
      }
      SetDefaultModulators(tmpmod);
      if (trans->GetSourceParameter(scaleTuning) != SF2Default[scaleTuning])
         tmpmod->NewModulator(ssKeyNumber, syPositiveBipolar,
                           trans->GetDestination(scaleTuning),
                           (LONG)trans->GetSourceParameter(scaleTuning)*64, stLinear,
                           ssEndSupportedSources, syEndSupportedSourceTypes,
                           FALSE);
      if (trans->GetSourceParameter(keynumToVolEnvHold) != SF2Default[keynumToVolEnvHold])
         tmpmod->NewModulator(ssKeyNumber, syNegativeBipolar,
                           trans->GetDestination(keynumToVolEnvHold),
                           (LONG)trans->GetSourceParameter(keynumToVolEnvHold)*64, stLinear,
                           ssEndSupportedSources, syEndSupportedSourceTypes,
                           FALSE);
      if (trans->GetSourceParameter(keynumToVolEnvDecay) != SF2Default[keynumToVolEnvDecay])
         tmpmod->NewModulator(ssKeyNumber, syNegativeBipolar,
                           trans->GetDestination(keynumToVolEnvDecay),
                           (LONG)trans->GetSourceParameter(keynumToVolEnvDecay)*64, stLinear,
                           ssEndSupportedSources, syEndSupportedSourceTypes,
                           FALSE);
      for (i = 0; i < irange.wNumMods; i++)
      {
         ConvertModSource(irange.modarr[i].wModSrcOper, src, srctype);
         ConvertModSource(irange.modarr[i].wModAmtSrcOper, asrc, asrctype);

         LONG lAmount=(LONG)irange.modarr[i].shAmount;
         BOOL bAdd=FALSE, bFirstKeyTune=FALSE, bFirstPitchTune=FALSE;
         switch (irange.modarr[i].wModDestOper) {
            case startAddrsCoarseOffset:
            case endAddrsCoarseOffset:
            case startloopAddrsCoarseOffset:
            case endloopAddrsCoarseOffset:
               lAmount *= 32768;
            // break left out ON PURPOSE
            case startAddrsOffset:
            case endAddrsOffset:
            case startloopAddrsOffset:
            case endloopAddrsOffset:
               bAdd = TRUE;
               break;
            case coarseTune:
               lAmount *= 100;
               // break left out ON PURPOSE
            case fineTune:
               if ((src == ssPitchWheel) && (srctype == syPositiveBipolar) &&
                   ((enSupportedTransforms)irange.modarr[i].wModTransOper == stLinear) &&
                   (asrc == ssPitchBendSensitivity) && (asrctype == syPositiveUnipolar))
               {
                  bAdd = bFirstPitchTune;
                  bFirstPitchTune = TRUE;
               }
               else if ((src == ssKeyNumber) && (srctype == syPositiveBipolar) &&
                        ((enSupportedTransforms)irange.modarr[i].wModTransOper == stLinear) &&
                        (asrc == ssEndSupportedSources) && (asrctype == syEndSupportedSourceTypes))
               {
                  bAdd = bFirstKeyTune;
                  bFirstKeyTune = TRUE;
               }
               else
                  bAdd = TRUE;
               break;
            default:
               break;
         }

         // MP - I GUARANTEE that I have already eliminated duplicate modulators.
         //      I further guarantee that default modulators (which need to be
         //      overridden) do not have the above properties.  So... these are
         //      always additive for the above modulators. The additive nature
         //      makes it so the sfModTable will combine coarse/fine data
         //      correctly.
         
         // Well, that isn't exactly true.  As it turns out, the pitch wheel
         // is routed to fine tuning.  So, the first time we add a modulator
         // for that destination, we make it overwrite the existing one.  Then,
         // for any others, we add on to the ones we've already added.  MRP

         tmpmod->NewModulator(src, srctype,
            trans->GetDestination(irange.modarr[i].wModDestOper), lAmount,
            (enSupportedTransforms)irange.modarr[i].wModTransOper,
            asrc, asrctype, bAdd);
         if (irange.modarr[i].wModDestOper == pan)
         {
            tmpmod->NewModulator(src, srctype,
               trans->GetDestination(pan)+1, lAmount,
               (enSupportedTransforms)irange.modarr[i].wModTransOper,
               asrc, asrctype, bAdd);
         }
      }
      for (i = 0; i < prange.wNumMods; i++)
      {
         ConvertModSource(prange.modarr[i].wModSrcOper, src, srctype);
         ConvertModSource(prange.modarr[i].wModAmtSrcOper, asrc, asrctype);
         LONG lAmount=(LONG)prange.modarr[i].shAmount;
         switch (prange.modarr[i].wModDestOper) { 
            case coarseTune:
               lAmount *= 100;
               break;
            default:
               break;
         }
         tmpmod->NewModulator(src, srctype,
            trans->GetDestination(prange.modarr[i].wModDestOper), lAmount,
            (enSupportedTransforms)prange.modarr[i].wModTransOper,
            asrc, asrctype, TRUE);
         if (prange.modarr[i].wModDestOper == pan)
         {
            tmpmod->NewModulator(src, srctype,
               trans->GetDestination(pan)+1, lAmount,
               (enSupportedTransforms)prange.modarr[i].wModTransOper,
               asrc, asrctype, TRUE);
         }
      }
      tmprange->artdata.modtable = tmpmod;
   }
}

void QFSF2Reader::ConvertModSource(WORD wModSrcOper, enSupportedSources& src,
                                   enSupportedSourceType& type)
{
   type = (enSupportedSourceType)(wModSrcOper >> 8);
   if (wModSrcOper & sfModCCMask)
      switch (wModSrcOper & sfModCCNumMask) {
         case 1:
            src = ssCC1;
            break;
		 case 2:
			 src = ssCC2;
			 break;
         case 7:
            src = ssCC7;
            break;
         case 10:
            src = ssCC10;
            break;
         case 11:
            src = ssCC11;
            break;
         case 21:
            src = ssCC21;
            break;
         case 22:
            src = ssCC22;
            break;
         case 23:
			 src = ssCC23;
			 break;
		 case 24:
            src = ssCC24;
            break;
         case 91:
            src = ssCC91;
            break;
         case 93:
            src = ssCC93;
            break;
         default:
            src = ssEndSupportedSources;
            type = syEndSupportedSourceTypes;
            break;
      }
   else
      switch (wModSrcOper & sfModMask) {
         case sfModKeyOnVelocity:
            src = ssKeyOnVelocity;
            break;
         case sfModKeyNumber:
            src = ssKeyNumber;
            break;
         case sfModChanPressure:
            src = ssChanPressure;
            break;
         case sfModPitchWheel:
            src = ssPitchWheel;
            break;
         case sfModPitchBendSens:
            src = ssPitchBendSensitivity;
            break;
         case sfModNoControler:
         case sfModPolyPressure:
         default:
            src = ssEndSupportedSources;
            type = syEndSupportedSourceTypes;
            break;
   }
}

#endif

void QFSF2Reader::SetArtData(Range& rng, BOOL bAdd)
{
   WORD i;
   sfGen *tmpgen;

   for (i = 0; i < rng.wNumGens; i++)
   {
      tmpgen = &rng.genarr[i];
      trans->SetParameter(tmpgen->wGenOper, (LONG)((SHORT)tmpgen->wGenAmt), bAdd);
   }
}

void QFSF2Reader::GetRangeInfo(Range& rng, SHORT genarr[],
#  ifndef __NO_MOD_CTRL
                               sfModList& modl,
#  endif
                               WORD wGenOper)
{
   unGenAmount rangedata;
   WORD i, wNumGenDiffs;

   ClearError();
   if (genarr[keyRange] != SF2Default[keyRange])
   {
      rangedata.shAmount = genarr[keyRange];
      rng.byKeyLow = rangedata.stRange.byLow;
      rng.byKeyHigh = rangedata.stRange.byHigh;
      genarr[keyRange] = SF2Default[keyRange];
   }

   if (genarr[velRange] != SF2Default[velRange])
   {
      rangedata.shAmount = genarr[velRange];
      rng.byVelLow = rangedata.stRange.byLow;
      rng.byVelHigh = rangedata.stRange.byHigh;
      genarr[velRange] = SF2Default[velRange];
   }

   if (genarr[wGenOper] == SF2Default[wGenOper])
   {
      SetError(QF_READ_FILE);
      return;
   }
   rng.wIndex = genarr[wGenOper];
   genarr[wGenOper] = ((wGenOper == sampleId) ? SF2Default[wGenOper] : 0);

#  ifndef __NO_MOD_CTRL
      rng.wNumMods = modl.GetNumItems();
      if (((rng.modarr = new sfMod[rng.wNumMods]) == NULL) &&
          (rng.wNumMods > 0))
      {
         SetError(QF_ALLOCATE_MEM);
         return;
      }
      for (i = 0, modl.First(); !modl.EndOfList(); modl.DeleteCurItem(), i++)
      {
         rng.modarr[i].wModSrcOper = modl.GetCurItem()->wModSrcOper;
         rng.modarr[i].wModDestOper = modl.GetCurItem()->wModDestOper;
         rng.modarr[i].shAmount = modl.GetCurItem()->shAmount;
         rng.modarr[i].wModAmtSrcOper = modl.GetCurItem()->wModAmtSrcOper;
         rng.modarr[i].wModTransOper = modl.GetCurItem()->wModTransOper;
      }
#  endif
   for (i = wNumGenDiffs = 0; i < numSF2Gens; i++)
      if (genarr[i] != ((wGenOper == sampleId) ? SF2Default[i] : 0))
         wNumGenDiffs++;
   rng.wNumGens = wNumGenDiffs;
   if (((rng.genarr = new sfGen[wNumGenDiffs]) == NULL) &&
       (wNumGenDiffs > 0))
   {
      SetError(QF_ALLOCATE_MEM);
      return;
   }
   for (i = wNumGenDiffs = 0; i < numSF2Gens; i++)
   {
      if (genarr[i] != ((wGenOper == sampleId) ? SF2Default[i] : 0))
      {
         rng.genarr[wNumGenDiffs].wGenOper = i;
         rng.genarr[wNumGenDiffs++].wGenAmt = genarr[i];
      }
   }
}

BOOL QFSF2Reader::GlobalLayer(sfGenList& genl, WORD wGenOper)
{
   genl.Find(wGenOper);
   return (genl.GetCurItem() == NULL);
}

//*****************************************************************************
//
// Function: GetPhdr
//
// Description:
//    This function returns a pointer to the data for the idx'th PHDR (preset)
// subchunk.
//
// Parameters: idx - index to PHDR subchunk (starts at 0)
//
// Return Value: pointer to PHDR data
//
//*****************************************************************************
void QFSF2Reader::GetPhdr(sfPresetList& ret, WORD idx, WORD num)
{
   SWAP_WORD_BYTE_INCOH_DECLARATIONS;
   SWAP_DWORD_BYTE_INCOH_DECLARATIONS;
   sfPreset *tmp;
   CHAR tmpstr[NAMESIZE+1];
   WORD i, tmpnum;
   BYTE *bytearray;

   ClearError();
   ret.DeleteList();
   if ((idx >= wNumPhdr) || (num == 0))
      return;
   tmpnum = (idx+num > wNumPhdr) ? (wNumPhdr-idx) : num;
   tRIFF->SetCurPosition(PhdrPos);

   tRIFF->StartDataStream();
   tRIFF->SeekDataStream((DWORD)idx*PHDRLEN);
   if ((bytearray = (BYTE *)NewAlloc((DWORD)tmpnum*PHDRLEN*sizeof(BYTE))) == NULL)
   {
      SetError(QF_ALLOCATE_MEM);
      return;
   }
   tRIFF->ReadDataStream(bytearray, (DWORD)tmpnum*PHDRLEN);
   tRIFF->StopDataStream();
   for (i = 0; i < tmpnum; i++)
   {
      if ((tmp = new sfPreset) == NULL)
      {
         DeleteAlloc(bytearray);
         SetError(QF_ALLOCATE_MEM);
         return;
      }
      memcpy(tmpstr, bytearray+(DWORD)i*PHDRLEN, NAMESIZE);
      tmpstr[NAMESIZE] = '\0';
      tmp->strPresetName = tmpstr;
      memcpy(&tmp->wPresetNum, bytearray+(DWORD)i*PHDRLEN+NAMESIZE,
             PHDRLEN-NAMESIZE);

      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wPresetNum);
      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wBankNum);
      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wBagNdx);
      SWAP_DWORD_BYTE_INCOH_ONLY(tmp->dwLibrary);
      SWAP_DWORD_BYTE_INCOH_ONLY(tmp->dwGenre);
      SWAP_DWORD_BYTE_INCOH_ONLY(tmp->dwMorphology);

      ret.Insert(tmp);
   }
   DeleteAlloc(bytearray);
   ret.First();
}

//*****************************************************************************
//
// Function: GetPbag
//
// Description:
//    This function returns a pointer to the data for the idx'th PBAG (layer)
// subchunk.
//
// Parameters: idx - index to PBAG subchunk (starts at 0)
//
// Return Value: pointer to PBAG data
//
//*****************************************************************************
void QFSF2Reader::GetPbag(sfBagList& ret, WORD idx, WORD num)
{
   SWAP_WORD_BYTE_INCOH_DECLARATIONS;
   sfBag *tmp;
   WORD i, tmpnum;
   BYTE *bytearray;

   ClearError();
   ret.DeleteList();
   if ((idx >= wNumPbag) || (num == 0))
      return;
   tmpnum = (idx+num > wNumPbag) ? (wNumPbag-idx) : num;
   tRIFF->SetCurPosition(PbagPos);

   tRIFF->StartDataStream();
   tRIFF->SeekDataStream((DWORD)idx*BAGLEN);
   if ((bytearray = (BYTE *)NewAlloc((DWORD)tmpnum*BAGLEN*sizeof(BYTE))) == NULL)
   {
      SetError(QF_ALLOCATE_MEM);
      return;
   }
   tRIFF->ReadDataStream(bytearray, (DWORD)tmpnum*BAGLEN);
   tRIFF->StopDataStream();
   for (i = 0; i < tmpnum; i++)
   {
      if ((tmp = new sfBag) == NULL)
      {
         DeleteAlloc(bytearray);
         SetError(QF_ALLOCATE_MEM);
         return;
      }
      memcpy(&tmp->wGenNdx, bytearray+(DWORD)i*BAGLEN, BAGLEN);

      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wGenNdx);
      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wModNdx);

      ret.Insert(tmp);
   }
   DeleteAlloc(bytearray);
   ret.First();
}

//*****************************************************************************
//
// Function: GetPmod
//
// Description:
//    This function returns a pointer to the data for the idx'th PMOD (preset
// modulator) subchunk.  More than one PMOD data structure can be returned
// by specifying the number of PMODs to get as the second parameter.  This
// parameter is optional.
//
// Parameters: idx - index to PMOD subchunk (starts at 0)
//             num - number of PMODs to get (defaults to 1)
//
// Return Value: pointer to PMOD data
//
//*****************************************************************************
#ifndef __NO_MOD_CTRL
void QFSF2Reader::GetPmod(sfModList& ret, WORD idx, WORD num)
{
   SWAP_WORD_BYTE_INCOH_DECLARATIONS;
   sfMod *tmp;
   WORD i, tmpnum;
   BYTE *bytearray;

   ClearError();
   ret.DeleteList();
   if ((idx >= wNumPmod) || (num == 0))
      return;
   tmpnum = (idx+num > wNumPmod) ? (wNumPmod-idx) : num;
   tRIFF->SetCurPosition(PmodPos);

   tRIFF->StartDataStream();
   tRIFF->SeekDataStream((DWORD)idx*MODLEN);
   if ((bytearray = (BYTE *)NewAlloc((DWORD)tmpnum*MODLEN*sizeof(BYTE))) == NULL)
   {
      SetError(QF_ALLOCATE_MEM);
      return;
   }
   tRIFF->ReadDataStream(bytearray, (DWORD)tmpnum*MODLEN);
   tRIFF->StopDataStream();
   for (i = 0; i < tmpnum; i++)
   {
      if ((tmp = new sfMod) == NULL)
      {
         DeleteAlloc(bytearray);
         SetError(QF_ALLOCATE_MEM);
         return;
      }
      memcpy(&tmp->wModSrcOper, bytearray+(DWORD)i*MODLEN, MODLEN);

      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wModSrcOper);
      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wModDestOper);
      SWAP_WORD_BYTE_INCOH_ONLY(tmp->shAmount);
      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wModAmtSrcOper);
      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wModTransOper);

      ret.Insert(tmp);
   }
   DeleteAlloc(bytearray);
   ret.First();
}
#endif
//*****************************************************************************
//
// Function: GetPgen
//
// Description:
//    This function returns a pointer to the data for the idx'th PGEN (preset
// generator) subchunk.  More than one PGEN data structure can be returned
// by specifying the number of PGENs to get as the second parameter.  This
// parameter is optional.
//
// Parameters: idx - index to PGEN subchunk (starts at 0)
//             num - number of PGENs to get (defaults to 1)
//
// Return Value: pointer to PGEN data
//
//*****************************************************************************
void QFSF2Reader::GetPgen(sfGenList& ret, WORD idx, WORD num)
{
   SWAP_WORD_BYTE_INCOH_DECLARATIONS;
   sfGen *tmp;
   WORD i, tmpnum;
   BYTE *bytearray;

   ClearError();
   ret.DeleteList();
   if ((idx >= wNumPgen) || (num == 0))
      return;
   tmpnum = (idx+num > wNumPgen) ? (wNumPgen-idx) : num;
   tRIFF->SetCurPosition(PgenPos);

   tRIFF->StartDataStream();
   tRIFF->SeekDataStream((DWORD)idx*GENLEN);
   if ((bytearray = (BYTE *)NewAlloc((DWORD)tmpnum*GENLEN*sizeof(BYTE))) == NULL)
   {
      SetError(QF_ALLOCATE_MEM);
      return;
   }
   tRIFF->ReadDataStream(bytearray, (DWORD)tmpnum*GENLEN);
   tRIFF->StopDataStream();
   for (i = 0; i < tmpnum; i++)
   {
      if ((tmp = new sfGen) == NULL)
      {
         DeleteAlloc(bytearray);
         SetError(QF_ALLOCATE_MEM);
         return;
      }
      memcpy(&tmp->wGenOper, bytearray+(DWORD)i*GENLEN, GENLEN);

      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wGenOper);
      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wGenAmt);

      ret.Insert(tmp);
   }
   DeleteAlloc(bytearray);
   ret.First();
}

//*****************************************************************************
//
// Function: GetInst
//
// Description:
//    This function returns a pointer to the data for the idx'th INST
// (instrument) subchunk.
//
// Parameters: idx - index to INST subchunk (starts at 0)
//
// Return Value: pointer to INST data
//
//*****************************************************************************
void QFSF2Reader::GetInst(sfInstList& ret, WORD idx, WORD num)
{
   SWAP_WORD_BYTE_INCOH_DECLARATIONS;
   sfInst *tmp;
   CHAR tmpstr[NAMESIZE+1];
   WORD i, tmpnum;
   BYTE *bytearray;

   ClearError();
   ret.DeleteList();
   if ((idx >= wNumInst) || (num == 0))
      return;
   tmpnum = (idx+num > wNumInst) ? (wNumInst-idx) : num;
   tRIFF->SetCurPosition(InstPos);

   tRIFF->StartDataStream();
   tRIFF->SeekDataStream((DWORD)idx*INSTLEN);
   if ((bytearray = (BYTE *)NewAlloc((DWORD)tmpnum*INSTLEN*sizeof(BYTE))) == NULL)
   {
      SetError(QF_ALLOCATE_MEM);
      return;
   }
   tRIFF->ReadDataStream(bytearray, (DWORD)tmpnum*INSTLEN);
   tRIFF->StopDataStream();
   for (i = 0; i < tmpnum; i++)
   {
      if ((tmp = new sfInst) == NULL)
      {
         DeleteAlloc(bytearray);
         SetError(QF_ALLOCATE_MEM);
         return;
      }
      memcpy(tmpstr, bytearray+(DWORD)i*INSTLEN, NAMESIZE);
      tmpstr[NAMESIZE] = '\0';
      tmp->strInstName = tmpstr;
      memcpy(&tmp->wBagNdx, bytearray+(DWORD)i*INSTLEN+NAMESIZE,
             INSTLEN-NAMESIZE);

      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wBagNdx);

      ret.Insert(tmp);
   }
   DeleteAlloc(bytearray);
   ret.First();
}

//*****************************************************************************
//
// Function: GetIbag
//
// Description:
//    This function returns a pointer to the data for the idx'th IBAG (split)
// subchunk.
//
// Parameters: idx - index to IBAG subchunk (starts at 0)
//
// Return Value: pointer to IBAG data
//
//*****************************************************************************
void QFSF2Reader::GetIbag(sfBagList& ret, WORD idx, WORD num)
{
   SWAP_WORD_BYTE_INCOH_DECLARATIONS;
   sfBag *tmp;
   WORD i, tmpnum;
   BYTE *bytearray;

   ClearError();
   ret.DeleteList();
   if ((idx >= wNumIbag) || (num == 0))
      return;
   tmpnum = (idx+num > wNumIbag) ? (wNumIbag-idx) : num;
   tRIFF->SetCurPosition(IbagPos);

   tRIFF->StartDataStream();
   tRIFF->SeekDataStream((DWORD)idx*BAGLEN);
   if ((bytearray = (BYTE *)NewAlloc((DWORD)tmpnum*BAGLEN*sizeof(BYTE))) == NULL)
   {
      SetError(QF_ALLOCATE_MEM);
      return;
   }
   tRIFF->ReadDataStream(bytearray, (DWORD)tmpnum*BAGLEN);
   tRIFF->StopDataStream();
   for (i = 0; i < tmpnum; i++)
   {
      if ((tmp = new sfBag) == NULL)
      {
         DeleteAlloc(bytearray);
         SetError(QF_ALLOCATE_MEM);
         return;
      }
      memcpy(&tmp->wGenNdx, bytearray+(DWORD)i*BAGLEN, BAGLEN);

      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wGenNdx);
      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wModNdx);

      ret.Insert(tmp);
   }
   DeleteAlloc(bytearray);
   ret.First();
}

//*****************************************************************************
//
// Function: GetImod
//
// Description:
//    This function returns a pointer to the data for the idx'th IMOD
// (instrument modulator) subchunk.  More than one IMOD data structure can be
// returned by specifying the number of IMODs to get as the second parameter.
// This parameter is optional.
//
// Parameters: idx - index to IMOD subchunk (starts at 0)
//             num - number of IMODs to get (defaults to 1)
//
// Return Value: pointer to IMOD data
//
//*****************************************************************************
#ifndef __NO_MOD_CTRL
void QFSF2Reader::GetImod(sfModList& ret, WORD idx, WORD num)
{
   SWAP_WORD_BYTE_INCOH_DECLARATIONS;
   sfMod *tmp;
   WORD i, tmpnum;
   BYTE *bytearray;

   ClearError();
   ret.DeleteList();
   if ((idx >= wNumImod) || (num == 0))
      return;
   tmpnum = (idx+num > wNumImod) ? (wNumImod-idx) : num;
   tRIFF->SetCurPosition(ImodPos);

   tRIFF->StartDataStream();
   tRIFF->SeekDataStream((DWORD)idx*MODLEN);
   if ((bytearray = (BYTE *)NewAlloc((DWORD)tmpnum*MODLEN*sizeof(BYTE))) == NULL)
   {
      SetError(QF_ALLOCATE_MEM);
      return;
   }
   tRIFF->ReadDataStream(bytearray, (DWORD)tmpnum*MODLEN);
   tRIFF->StopDataStream();
   for (i = 0; i < tmpnum; i++)
   {
      if ((tmp = new sfMod) == NULL)
      {
         DeleteAlloc(bytearray);
         SetError(QF_ALLOCATE_MEM);
         return;
      }
      memcpy(&tmp->wModSrcOper, bytearray+(DWORD)i*MODLEN, MODLEN);

      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wModSrcOper);
      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wModDestOper);
      SWAP_WORD_BYTE_INCOH_ONLY(tmp->shAmount);
      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wModAmtSrcOper);
      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wModTransOper);

      ret.Insert(tmp);
   }
   DeleteAlloc(bytearray);
   ret.First();
}
#endif
//*****************************************************************************
//
// Function: GetIgen
//
// Description:
//    This function returns a pointer to the data for the idx'th IGEN
// (instrument generator) subchunk.  More than one IGEN data structure can be
// returned by specifying the number of IGENs to get as the second parameter.
// This parameter is optional.
//
// Parameters: idx - index to IGEN subchunk (starts at 0)
//             num - number of IGENs to get (defaults to 1)
//
// Return Value: pointer to IGEN data
//
//*****************************************************************************
void QFSF2Reader::GetIgen(sfGenList& ret, WORD idx, WORD num)
{
   SWAP_WORD_BYTE_INCOH_DECLARATIONS;
   sfGen *tmp;
   WORD i, tmpnum;
   BYTE *bytearray;

   ClearError();
   ret.DeleteList();

   if ((idx >= wNumIgen) || (num == 0))
      return;
   tmpnum = (idx+num > wNumIgen) ? (wNumIgen-idx) : num;
   tRIFF->SetCurPosition(IgenPos);

   tRIFF->StartDataStream();
   tRIFF->SeekDataStream((DWORD)idx*GENLEN);
   if ((bytearray = (BYTE *)NewAlloc((DWORD)tmpnum*GENLEN*sizeof(BYTE))) == NULL)
   {
      SetError(QF_ALLOCATE_MEM);
      return;
   }
   tRIFF->ReadDataStream(bytearray, (DWORD)tmpnum*GENLEN);
   tRIFF->StopDataStream();
   for (i = 0; i < tmpnum; i++)
   {
      if ((tmp = new sfGen) == NULL)
      {
         DeleteAlloc(bytearray);
         SetError(QF_ALLOCATE_MEM);
         return;
      }
      memcpy(&tmp->wGenOper, bytearray+(DWORD)i*GENLEN, GENLEN);

      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wGenOper);
      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wGenAmt);

      ret.Insert(tmp);
   }
   DeleteAlloc(bytearray);
   ret.First();
}

//*****************************************************************************
//
// Function: GetShdr
//
// Description:
//    This function returns a pointer to the data for the idx'th SHDR (sample
// header) subchunk.
//
// Parameters: idx - index to SHDR subchunk (starts at 0)
//
// Return Value: pointer to SHDR data
//
//*****************************************************************************
void QFSF2Reader::GetShdr(sfSampleList& ret, WORD idx, WORD num)
{
   SWAP_WORD_BYTE_INCOH_DECLARATIONS;
   SWAP_DWORD_BYTE_INCOH_DECLARATIONS;
   sfSample *tmp;
   CHAR tmpstr[NAMESIZE+1];
   WORD i, tmpnum;
   BYTE *bytearray;

   ClearError();
   ret.DeleteList();
   if ((idx >= wNumShdr) || (num == 0))
      return;
   tmpnum = (idx+num > wNumShdr) ? (wNumShdr-idx) : num;
   tRIFF->SetCurPosition(ShdrPos);

   tRIFF->StartDataStream();
   tRIFF->SeekDataStream((DWORD)idx*SHDRLEN);
   if ((bytearray = (BYTE *)NewAlloc((DWORD)tmpnum*SHDRLEN*sizeof(BYTE))) == NULL)
   {
      SetError(QF_ALLOCATE_MEM);
      return;
   }
   tRIFF->ReadDataStream(bytearray, (DWORD)tmpnum*SHDRLEN);
   tRIFF->StopDataStream();
   for (i = 0; i < tmpnum; i++)
   {
      if ((tmp = new sfSample) == NULL)
      {
         DeleteAlloc(bytearray);
         SetError(QF_ALLOCATE_MEM);
         return;
      }
      memcpy(tmpstr, bytearray+(DWORD)i*SHDRLEN, NAMESIZE);
      tmpstr[NAMESIZE] = '\0';
      tmp->strSampleName = tmpstr;
      memcpy(&tmp->dwStart, bytearray+(DWORD)i*SHDRLEN+NAMESIZE,
             SHDRLEN-NAMESIZE);

      SWAP_DWORD_BYTE_INCOH_ONLY(tmp->dwStart);
      SWAP_DWORD_BYTE_INCOH_ONLY(tmp->dwEnd);
      SWAP_DWORD_BYTE_INCOH_ONLY(tmp->dwStartLoop);
      SWAP_DWORD_BYTE_INCOH_ONLY(tmp->dwEndLoop);
      SWAP_DWORD_BYTE_INCOH_ONLY(tmp->dwSampleRate);
      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wSampleLink);
      SWAP_WORD_BYTE_INCOH_ONLY(tmp->wSampleType);

      ret.Insert(tmp);
   }
   DeleteAlloc(bytearray);
   ret.First();
}

//*****************************************************************************
//
// Function: GetNumCks
//
// Description:
//    This function finds the number of items in each subchunk of the SoundFont
// file.  It is called whenever a new SoundFont file is opened.
//
// Parameters: (none)
//
// Return Value: (none)
//
//*****************************************************************************
void QFSF2Reader::GetNumCks()
{
   DWORD chunkSize;

   tRIFF->Reset();

   tRIFF->FindChunk(MAKE_ID("pdta"), TRUE);
   if (tRIFF->IsBad()) goto gncICantBelieveIHaveAGoToError;

   tRIFF->FindChunk(MAKE_ID("phdr"), TRUE);
   if (tRIFF->IsBad()) goto gncICantBelieveIHaveAGoToError;
   tRIFF->GetCurPosition(PhdrPos);
   chunkSize=tRIFF->GetChunkSize();
   if (chunkSize<PHDRLEN) 
      wNumPhdr=0;
   else 
      wNumPhdr = ((WORD)(tRIFF->GetChunkSize()/PHDRLEN) - 1);

   tRIFF->FindChunk(MAKE_ID("pbag"), TRUE);
   if (tRIFF->IsBad()) goto gncICantBelieveIHaveAGoToError;
   tRIFF->GetCurPosition(PbagPos);
   chunkSize=tRIFF->GetChunkSize();
   if (chunkSize<BAGLEN) 
      wNumPbag=0;
   else 
      wNumPbag = ((WORD)(tRIFF->GetChunkSize()/BAGLEN) - 1);

#  ifndef __NO_MOD_CTRL
   tRIFF->FindChunk(MAKE_ID("pmod"), TRUE);
   if (tRIFF->IsBad()) goto gncICantBelieveIHaveAGoToError;
   tRIFF->GetCurPosition(PmodPos);
   chunkSize=tRIFF->GetChunkSize();
   if (chunkSize<MODLEN) 
      wNumPmod=0;
   else 
      wNumPmod = ((WORD)(tRIFF->GetChunkSize()/MODLEN) - 1);
#  endif

   tRIFF->FindChunk(MAKE_ID("pgen"), TRUE);
   if (tRIFF->IsBad()) goto gncICantBelieveIHaveAGoToError;
   tRIFF->GetCurPosition(PgenPos);
   chunkSize=tRIFF->GetChunkSize();
   if (chunkSize<GENLEN) 
      wNumPgen=0;
   else 
      wNumPgen = ((WORD)(tRIFF->GetChunkSize()/GENLEN) - 1);

   tRIFF->FindChunk(MAKE_ID("inst"), TRUE);
   if (tRIFF->IsBad()) goto gncICantBelieveIHaveAGoToError;
   tRIFF->GetCurPosition(InstPos);
   chunkSize=tRIFF->GetChunkSize();
   if (chunkSize<INSTLEN) 
      wNumInst=0;
   else 
      wNumInst = ((WORD)(tRIFF->GetChunkSize()/INSTLEN) - 1);

   tRIFF->FindChunk(MAKE_ID("ibag"), TRUE);
   if (tRIFF->IsBad()) goto gncICantBelieveIHaveAGoToError;
   tRIFF->GetCurPosition(IbagPos);
   chunkSize=tRIFF->GetChunkSize();
   if (chunkSize<BAGLEN) 
      wNumIbag=0;
   else 
      wNumIbag = ((WORD)(tRIFF->GetChunkSize()/BAGLEN) - 1);

#  ifndef __NO_MOD_CTRL
   tRIFF->FindChunk(MAKE_ID("imod"), TRUE);
   if (tRIFF->IsBad()) goto gncICantBelieveIHaveAGoToError;
   tRIFF->GetCurPosition(ImodPos);
   chunkSize=tRIFF->GetChunkSize();
   if (chunkSize<MODLEN) 
      wNumImod=0;
   else 
      wNumImod = ((WORD)(tRIFF->GetChunkSize()/MODLEN) - 1);
#  endif

   tRIFF->FindChunk(MAKE_ID("igen"), TRUE);
   if (tRIFF->IsBad()) goto gncICantBelieveIHaveAGoToError;
   tRIFF->GetCurPosition(IgenPos);
   chunkSize=tRIFF->GetChunkSize();
   if (chunkSize<GENLEN) 
      wNumIgen=0;
   else 
      wNumIgen = ((WORD)(tRIFF->GetChunkSize()/GENLEN) - 1);

   tRIFF->FindChunk(MAKE_ID("shdr"), TRUE);
   if (tRIFF->IsBad()) goto gncICantBelieveIHaveAGoToError;
   tRIFF->GetCurPosition(ShdrPos);
   chunkSize=tRIFF->GetChunkSize();
   if (chunkSize<SHDRLEN) 
      wNumShdr=0;
   else 
      wNumShdr = ((WORD)(tRIFF->GetChunkSize()/SHDRLEN) - 1);

   tRIFF->Reset();
   lSmplPos = 0;
   tRIFF->FindChunk(MAKE_ID("sdta"), TRUE);
   if (tRIFF->IsBad()) goto gncICantBelieveIHaveAGoToError;

   tRIFF->FindChunk(MAKE_ID("smpl"), TRUE);
   if (!tRIFF->IsBad())
   {
      tRIFF->GetCurPosition(SmplPos);
      lSmplPos = tRIFF->RIFFTell();
   }
   return;

   gncICantBelieveIHaveAGoToError:
   PropogateError(tRIFF);
   return;
}

sfBankVersion QFSF2Reader::GetVersion()
{
   SWAP_WORD_BYTE_INCOH_DECLARATIONS;
   sfBankVersion ret;

   tRIFF->FindChunk(MAKE_ID("iver"));
   tRIFF->StartDataStream();
   tRIFF->ReadDataStream(&ret.wMajor, sizeof(WORD));
   tRIFF->ReadDataStream(&ret.wMinor, sizeof(WORD));
   tRIFF->StopDataStream();

   SWAP_WORD_BYTE_INCOH_ONLY(ret.wMajor);
   SWAP_WORD_BYTE_INCOH_ONLY(ret.wMinor);

   return ret;
}

void QFSF2Reader::SetBankInfo()
{
   CHAR *str;

   ClearError();
   tRIFF->FindChunk(MAKE_ID("INAM"));
   if ((str = (CHAR *)NewAlloc(tRIFF->GetChunkSize()*sizeof(CHAR))) == NULL)
   {
      SetError(QF_ALLOCATE_MEM);
      return;
   }
   tRIFF->StartDataStream();
   tRIFF->ReadDataStream(str, tRIFF->GetChunkSize());
   tRIFF->StopDataStream();
   bankInfo.strBankName = str;
   DeleteAlloc(str);
   bankInfo.wNumPresets = wNumPhdr;
   bankInfo.wNumInsts = wNumInst;
   bankInfo.wNumSounds = wNumShdr;
   tRIFF->FindChunk(MAKE_ID("smpl"));
   bankInfo.dwTotalSoundSize = tRIFF->GetChunkSize();
}

void QFSF2Reader::AddFileAndBankToLists(QuickFont& qf)
{
   Str *tmpstr;

   ClearError();
   qf.sndmem->fileNameList.Find(strFileName);
   if (qf.sndmem->fileNameList.EndOfList())
   {
      if ((tmpstr = new Str(strFileName)) == NULL)
      {
         SetError(QF_ALLOCATE_MEM);
         return;
      }
      qf.sndmem->fileNameList.Insert(tmpstr);
   }
   qf.bankNameList.Find(bankInfo.strBankName);
   if (qf.bankNameList.EndOfList())
   {
      if ((tmpstr = new Str(bankInfo.strBankName)) == NULL)
      {
         SetError(QF_ALLOCATE_MEM);
         return;
      }
      qf.bankNameList.Insert(tmpstr);
   }
}

void QFSF2Reader::GetROMid()
{
   CHAR *str;
   DWORD dwIromSize;

   ClearError();
   strCurrROMid = "";
   tRIFF->FindChunk(MAKE_ID("irom"));
   if (tRIFF->GetError() == RIFF_FINDERROR)
      return;
   dwIromSize = tRIFF->GetChunkSize();
   if ((str = (CHAR *)NewAlloc(dwIromSize*sizeof(CHAR)+1)) == NULL)
   {
      SetError(QF_ALLOCATE_MEM);
      return;
   }
   tRIFF->StartDataStream();
   tRIFF->ReadDataStream(str, dwIromSize);
   tRIFF->StopDataStream();
   str[dwIromSize] = '\0';
   strCurrROMid = str;
   DeleteAlloc(str);
}

///////////////////////////////////////////////////////////////////////////////
// Code to dynamically insert SF2 code into list of available file types

class QFSF2ReaderDyn
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   QFSF2ReaderDyn()
   {sri = new SoundRegInfo;
    sri->strFileType = "SoundFont 2.0";
    sri->strFileExtension = "SF2";
    sri->alloc = QFSF2Reader::Alloc;
    if (sriList == NULL)
       sriList = new SoundRegInfoList;
    sriList->Insert(sri);}
   ~QFSF2ReaderDyn()
   {for (sriList->First(); !sriList->EndOfList(); sriList->Next())
       if (sriList->GetCurItem() == sri)
          sriList->DeleteCurItem();
    if (sriList->IsEmpty())
    {
       delete sriList;
       sriList = NULL;
    }}

   SoundRegInfo *sri;
};

QFSF2ReaderDyn dummySF2;
