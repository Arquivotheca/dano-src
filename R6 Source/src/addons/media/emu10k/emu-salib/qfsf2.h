//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: qfsf2.h
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
// Michael Preston     Oct  4, 1996  Moved sample reading to soundmem.h.
//                                   Combined Alloc() and CheckFileType().
// Michael Preston     Aug 23, 1996  Store location of SMPL chunk in lSmplPos.
// Michael Preston     Aug  8, 1996  Compute default articulation data only
//                                   once instead of for every vector.
// Michael Preston     Aug  2, 1996  Improved load performance.
// Michael Preston     Jul 30, 1996  Moved sound data checking to
//                                   GetSampleRanges() from AddRange()
// Michael Preston     Jul 29, 1996  Changed preset, inst, and sample lists to
//                                   arrays.  Replaced FindChunk() calls with
//                                   GetCurPosition() and SetCurPosition().
// Michael Preston     Jul 25, 1996  Added full modulator support.
// Michael Preston     Jul 24, 1996  Added compile flags for mods.
// Michael Preston     Jul 22, 1996  Changed calls to OmegaClass.
// Michael Preston     Jul 17, 1996  Changed load functions to use index
//                                   number instead of name.  Moved code only
//                                   needed for qfsf2.cpp.  Improved bank load
//                                   time.
// Michael Preston     Jul 10, 1996  Added AddFileAndBankToLists().
// Michael Preston     Jul  8, 1996  Added GetBankInfo().
// Michael Preston     Jul  2, 1996  Cast linked list pointers.
// Michael Preston     Jul  1, 1996  Began load bank changes.
// Michael Preston     Jun 24, 1996  Changed QFArtData to LONG *
// Michael Preston     Jun 24, 1996  Added support for SparseArray structure.
// Michael Preston     Jun 24, 1996  Added support for SoundMemory structure.
// Michael Preston     Jun 24, 1996  Made changes to linked lists.
// Michael Preston     Jun 24, 1996  Added support for preset available flag.
// Michael Preston     May 17, 1996  Initial import to CVS.
// Michael Preston     Mar  1, 1996  Initial development.
//
//*****************************************************************************

#ifndef __QFSF2_H
#define __QFSF2_H

// Include files

#include "datatype.h"
#include "win_mem.h"
#include "omega.h"
#include "qfsfread.h"
#include "sfenum.h"
#include "quickfnt.h"
#include "qftypes.h"

#define numSF2Gens (endOper+1)

enum enLoadType {
   enltMask = 0xffff,
   enltBankLoad = 0x10000
};

class sfSample
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   Str strSampleName;
   DWORD dwStart;
   DWORD dwEnd;
   DWORD dwStartLoop;
   DWORD dwEndLoop;
   DWORD dwSampleRate;
   BYTE byOriginalKey;
   CHAR chFineCorrection;
   WORD wSampleLink;
   WORD wSampleType;

   static void dealloc(void *ptr) {delete (sfSample *)ptr;}
   static void *ccons(void *ptr)
      {return new sfSample(*(sfSample *)ptr);}
};

typedef enum
{
   monoSample=1,
   rightSample=2,
   leftSample=4,
   linkedSample=8,
   ROMSample=0x8000
} sfSampleType;

// Forward declarations
class RIFFParser;
class sfPresetRange;
class sfInstRange;
class sfSampleRange;
class sfBag;
class sfGen;
class sfGenList;
#ifndef __NO_MOD_CTRL
class sfMod;
class sfModList;
#endif
class Range;
class sfPresetList;
class sfBagList;
class sfInstList;
class sfSampleList;
class sfBankVersion;
class SampleLinkInfoList;

class QFSF2Reader : public QFSFReaderBase
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   QFSF2Reader(RIFFParser* newRIFF);
   ~QFSF2Reader();

   static BOOL CheckFileType(RIFFParser* newRIFF);
   static QFSFReaderBase* Alloc(RIFFParser* newRIFF)
      {return (CheckFileType(newRIFF) ? new QFSF2Reader(newRIFF) : NULL);}

   QFBankInfo& GetBankInfo() {return bankInfo;}
   void GetPresetInfo(QFPresetInfoList& piList);
   void GetInstInfo(QFSoundInfoList& siList);
   void GetSoundInfo(QFSoundInfoList& siList);
   void LoadBank(QuickFont& qf, WORD wBankNum, qfPresetState state,
                 WORD wInstanceNum);
   void LoadPreset(QuickFont& qf, WORD wPresetNdx, WORD wBankNum,
                   WORD wPresetNum, qfPresetState state, WORD wInstanceNum);
   void LoadInst(QuickFont& qf, WORD wInstNdx, WORD wBankNum,
                 WORD wPresetNum, qfPresetState state, WORD wInstanceNum);
   void LoadSound(QuickFont& qf, WORD wSoundNdx, WORD wBankNum,
                  WORD wPresetNum, qfPresetState state, WORD wInstanceNum);

   private:

#ifndef __NO_MOD_CTRL
   void SetDefaultModulators(ModulatorTable *modTable);
#endif

   void LoadSetup();
   void LoadCleanup();
   void GetPresetRanges(qfPresetState state,
                        SoundMemory& sndmem,
                        DWORD dwLoadType);
//                        SHORT shPresetNdx=LoadAll);
   void GetInstRanges(qfPresetState state,
                      SoundMemory& sndmem,
                      DWORD dwLoadType);
//                      SHORT shInstNdx=LoadAll);
   void GetSampleRanges(qfPresetState state,
                        SoundMemory& sndmem,
                        DWORD dwLoadType);
//                        SHORT shSampleNdx=LoadAll);
   void CheckInstStereoPairs();
   void GetPresetGenModList(sfBag *pbagarray, sfGen *pgenarray, WORD wNumGen,
#  ifndef __NO_MOD_CTRL
                            sfMod *pmodarray, WORD wNumMod,
#  endif
                            WORD wBagNdx,
                            WORD wBagOffset, SHORT genarr[]
#  ifndef __NO_MOD_CTRL
                            , sfModList& modl
#  endif
                            );
   void GetInstGenModList(sfBag *ibagarray, sfGen *igenarray, WORD wNumGen,
#  ifndef __NO_MOD_CTRL
                          sfMod *imodarray, WORD wNumMod,
#  endif
                          WORD wBagNdx, WORD wBagOffset, SHORT genarr[]
#  ifndef __NO_MOD_CTRL
                          , sfModList& modl
#  endif
                          );
   void SetKeyVelRanges(QFPreset *preset, QFRangeList *qfrl);
   void ReadPresetData(QuickFont& qf, BOOL bBankLoad, WORD wPresetNdx,
                       WORD wBankNum, WORD wPresetNum, qfPresetState state,
                       WORD wInstanceNum);
   void ReadInstData(QFRangeList *qfrl, QuickFont& qf, Range& prange);
   void AddRange(QFRangeList *qfrl, QFRange *tmprange, Range& prange,
                 Range& irange, QuickFont& qf, SampleLinkInfoList& sliList);
   void SetQFRange(QFRange *range, Range& prange, Range& irange);
   void CombineLevels(Range& prange, Range& irange, sfSample *shdrinfo);
#ifndef __NO_MOD_CTRL
   void SetRangeMods(QFRange *tmprange, Range& prange, Range& irange,
                     QuickFont& qf);
   void ConvertModSource(WORD wModSrcOper, enSupportedSources& src,
                                   enSupportedSourceType& type);
#endif
   void SetArtData(Range &, BOOL bAdd);
   void GetRangeInfo(Range& rng, SHORT genarr[],
#  ifndef __NO_MOD_CTRL
                     sfModList& modl,
#  endif
                     WORD wGenOper);
   BOOL GlobalLayer(sfGenList& genl, WORD wGenOper);

   void GetPhdr(sfPresetList& ret, WORD idx, WORD num=1);
   void GetPbag(sfBagList& ret, WORD idx, WORD num=1);
#  ifndef __NO_MOD_CTRL
   void GetPmod(sfModList& ret, WORD idx, WORD num=1);
#  endif
   void GetPgen(sfGenList& ret, WORD idx, WORD num=1);
   void GetInst(sfInstList& ret, WORD idx, WORD num=1);
   void GetIbag(sfBagList& ret, WORD idx, WORD num=1);
#  ifndef __NO_MOD_CTRL
   void GetImod(sfModList& ret, WORD idx, WORD num=1);
#  endif
   void GetIgen(sfGenList& ret, WORD idx, WORD num=1);
   void GetShdr(sfSampleList& ret, WORD idx, WORD num=1);

   void GetNumCks();
   sfBankVersion GetVersion();
   void SetBankInfo();
   void AddFileAndBankToLists(QuickFont& qf);
   void GetROMid();

   WORD wNumPhdr;
   WORD wNumPbag;
#  ifndef __NO_MOD_CTRL
   WORD wNumPmod;
#  endif
   WORD wNumPgen;
   WORD wNumInst;
   WORD wNumIbag;
#  ifndef __NO_MOD_CTRL
   WORD wNumImod;
#  endif
   WORD wNumIgen;
   WORD wNumShdr;

   RIFFPosition PhdrPos;
   RIFFPosition PbagPos;
#  ifndef __NO_MOD_CTRL
   RIFFPosition PmodPos;
#  endif
   RIFFPosition PgenPos;
   RIFFPosition InstPos;
   RIFFPosition IbagPos;
#  ifndef __NO_MOD_CTRL
   RIFFPosition ImodPos;
#  endif
   RIFFPosition IgenPos;
   RIFFPosition ShdrPos;
   RIFFPosition SmplPos;
   LONG lSmplPos;

   DWORD dwCurPos;
   DWORD dwEndPos;
   Str strCurrROMid;
   QFBankInfo bankInfo;

   sfPresetRange **presetarr;
   sfInstRange **instarr;
   sfSampleRange **samplearr;
   SoundDataList snddatalist;
#ifndef __NO_MOD_CTRL
   static BOOL bDefModTableInitialized;
   static ModulatorTable defModTable;
#endif
};

#endif
