//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: qfwav.h
//
// Author: Michael Preston
//
// Description: Code specific for WAV files.
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  -----------------------------------------
// Michael Preston     Oct  4, 1996  Initial import to CVS.
// Michael Preston     Sep  4, 1996  Copied from qfsf2.h
//
//*****************************************************************************

#ifndef __QFWAV_H
#define __QFWAV_H

// Include files

#include "datatype.h"
#include "win_mem.h"
#include "omega.h"
#include "qfsfread.h"
#include "sfenum.h"
#include "quickfnt.h"
#include "qftypes.h"

typedef enum
{
   WAVsmLoop         = 0x1, // 0 - non-looping sample, 1 - looping sample
   WAVsmNoContLoop   = 0x2, // 0 - continuously looping sample,
                           // 1 - non-continuously looping sample
   WAVsmStereo       = 0x4000, // 0 - not stereo, 1 - stereo
   WAVsmMasterStereo = 0x8000 // 0 - doesn't control pitch for stereo pair
                           // 1 - controls pitch for stereo pair
} WAVSampleModeDefs;

typedef enum
{
   WAVForceKeyVel,
   WAVMinValue,
   WAVMaxValue,
   WAVFilterFc,
   WAVScaleTuning,
   WAVSampleRate,
   WAVRootKey,
   WAVEndAddrs,
   WAVSampleModes,
   WAVPanEffectsSend,
   WAVInitialPitch,
//
   WAVLastGen
} WAVGenerator;

#define numWAVGens WAVLastGen

class WAVData
{
   public:

   DWORD dwEndAddrs;
   DWORD dwSampleModes;
   LONG  lPanEffectsSend;
   DWORD dwSampleRate;
};

class QFWAVReader : public QFSFReaderBase
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   QFWAVReader(RIFFParser* newRIFF);
   ~QFWAVReader();

   static BOOL CheckFileType(RIFFParser* newRIFF);
   static QFSFReaderBase* Alloc(RIFFParser* newRIFF)
      {return (CheckFileType(newRIFF) ? new QFWAVReader(newRIFF) : NULL);}

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

   void SetBankInfo();
   void AddFileAndBankToLists(QuickFont& qf);

   DWORD dwDataPos;
   RIFFPosition DataPos;

   DWORD dwCurPos;
   DWORD dwEndPos;
   BOOL b16Bit;
   BYTE byNumChannels;
   QFBankInfo bankInfo;
#ifndef __NO_MOD_CTRL
   static BOOL bDefModTableInitialized;
   static ModulatorTable defModTable;
#endif
};

#endif
