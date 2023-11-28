//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: qfwav.cpp
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
// Michael Preston     Sep  4, 1996  Copied from qfsf2.cpp
//
//*****************************************************************************

// Include files

#include <string.h>

#include "qfwav.h"
#include "llist.h"
#include "riffprsr.h"
#include "qfreg.h"

// Define sizes for RIFF chunks


// Type definitions

// Static variables

BOOL QFWAVReader::bDefModTableInitialized=FALSE;
ModulatorTable QFWAVReader::defModTable;

LONG WAVDefault[numWAVGens] =
   {0xffff,     // WAVForceKeyVel
    -12000,     // WAVMinValue
    12000,      // WAVMaxValue
    13500,      // WAVFilterFc
    100,        // WAVScaleTuning
    44100,      // WAVSampleRate
    60,         // WAVRootKey
    0,          // WAVEndAddrs
    0,          // WAVSampleModes
    0,          // WAVPanEffectsSend
    0           // WAVInitialPitch
   };

QFWAVReader::QFWAVReader(RIFFParser* newRIFF) :
   QFSFReaderBase(newRIFF)
{
   tRIFF->FindChunk(MAKE_ID("data"));
   tRIFF->GetCurPosition(DataPos);
   dwDataPos = tRIFF->RIFFTell();

   ClearError();
   SetBankInfo();
   if (IsBad()) return;
}

QFWAVReader::~QFWAVReader()
{
}

BOOL QFWAVReader::CheckFileType(RIFFParser* newRIFF)
{
   SWAP_WORD_BYTE_INCOH_DECLARATIONS;
   WORD wFormat;

   newRIFF->FindChunk(MAKE_ID("RIFF"));
   if (newRIFF->GetChunkListToken() != MAKE_ID("WAVE"))
      return FALSE;
   newRIFF->FindChunk(MAKE_ID("fmt "));
   newRIFF->StartDataStream();
   newRIFF->ReadDataStream(&wFormat, sizeof(WORD));
   newRIFF->StopDataStream();
   SWAP_WORD_BYTE_INCOH_ONLY(wFormat);
   return (wFormat == 1);
}

void QFWAVReader::GetPresetInfo(QFPresetInfoList& piList)
{
   QFPresetInfo *tmp;

   piList.DeleteList();
   tmp = new QFPresetInfo;
   tmp->strPresetName = bankInfo.strBankName;
   tmp->dest.midiBankIndex.midiBankByValue.midiBankCC0 = 0;
   tmp->dest.midiBankIndex.midiBankByValue.midiBankCC32 = 0;
   tmp->dest.midiProgramIndex = 0;
   piList.Insert(tmp);
   piList.First();
}

void QFWAVReader::GetInstInfo(QFSoundInfoList& siList)
{
   QFSoundInfo *tmp;

   siList.DeleteList();
   tmp = new QFSoundInfo;
   tmp->strSoundName = bankInfo.strBankName;
   siList.Insert(tmp);
   siList.First();
}

void QFWAVReader::GetSoundInfo(QFSoundInfoList& siList)
{
   QFSoundInfo *tmp;

   siList.DeleteList();
   tmp = new QFSoundInfo;
   tmp->strSoundName = bankInfo.strBankName;
   siList.Insert(tmp);
   siList.First();
}

void QFWAVReader::LoadBank(QuickFont& qf, WORD wBankNum, qfPresetState state,
                           WORD wInstanceNum)
{
   LoadSound(qf, 0, wBankNum, 0, state, wInstanceNum);
}

void QFWAVReader::LoadPreset(QuickFont& qf, WORD wPresetNdx, WORD wBankNum,
                             WORD wPresetNum, qfPresetState state,
                             WORD wInstanceNum)
{
   LoadSound(qf, wPresetNdx, wBankNum, wPresetNum, state, wInstanceNum);
}

void QFWAVReader::LoadInst(QuickFont& qf, WORD wInstNdx, WORD wBankNum,
                           WORD wPresetNum, qfPresetState state,
                           WORD wInstanceNum)
{
   LoadSound(qf, wInstNdx, wBankNum, wPresetNum, state, wInstanceNum);
}

void QFWAVReader::LoadSound(QuickFont& qf, WORD wSoundNdx, WORD wBankNum,
                            WORD wPresetNum, qfPresetState state,
                            WORD wInstanceNum)
{
   SWAP_DWORD_BYTE_INCOH_DECLARATIONS;
   SWAP_WORD_BYTE_INCOH_DECLARATIONS;
   QFBank *tmpbank;
   QFPreset *tmppreset;
   QFKeyVelRange *tmpKVRange;
   QFRange *tmprange;
   WORD wNumChannels, wCurChannel, wBitsPerSample, wDummy;
   DWORD dwSampleRate, dwDummy;
   SoundData *tmpsounddata;
   SoundDataList snddatalist;
   CHAR *tmpstr;
   BOOL bInsertBank;
   WAVData tmpdata;

   ClearError();
   if (trans == NULL)
   {
      SetError(QF_NO_TRANSLATOR);
      return;
   }

   trans->SetFileType("WAV");
   if (!bDefModTableInitialized)
   {
      SetDefaultModulators(&defModTable);
      bDefModTableInitialized = TRUE;
   }
   AddFileAndBankToLists(qf);
   if (IsBad())
      return;

   currPercent = lastPercent = 0;

   tRIFF->FindChunk(MAKE_ID("fmt "));
   tRIFF->StartDataStream();
   tRIFF->ReadDataStream(&wDummy, sizeof(WORD));
   tRIFF->ReadDataStream(&wNumChannels, sizeof(WORD));
   tRIFF->ReadDataStream(&dwSampleRate, sizeof(DWORD));
   tRIFF->ReadDataStream(&dwDummy, sizeof(DWORD));
   tRIFF->ReadDataStream(&wDummy, sizeof(WORD));
   tRIFF->ReadDataStream(&wBitsPerSample, sizeof(WORD));
   tRIFF->StopDataStream();
   SWAP_WORD_BYTE_INCOH_ONLY(wNumChannels);
   SWAP_DWORD_BYTE_INCOH_ONLY(dwSampleRate);
   SWAP_WORD_BYTE_INCOH_ONLY(wBitsPerSample);

   tRIFF->SetCurPosition(DataPos);

   qf.list.Find(wBankNum);
   if (qf.list.EndOfList())
   {
      if ((tmpbank = new QFBank) == NULL)
      {
         SetError(QF_ALLOCATE_MEM);
         return;
      }
      tmpbank->wBankNum = wBankNum;
      tmpbank->wParentBankNum = wBankNum;
      bInsertBank = TRUE;
   }
   else
   {
      tmpbank = qf.list.GetCurItem();
      bInsertBank = FALSE;
   }

   tmppreset = new QFPreset;
   tmppreset->strPresetName = bankInfo.strBankName;
   qf.sndmem->fileNameList.Find(strFileName);
   tmppreset->strFileName = (CHAR *)(*qf.sndmem->fileNameList.GetCurItem());
   qf.bankNameList.Find(bankInfo.strBankName);
   tmppreset->strBankName = (CHAR *)(*qf.bankNameList.GetCurItem());
   tmppreset->wPresetNum = wPresetNum;
   tmppreset->wInstanceNum = wInstanceNum;
   tmppreset->bSelfLoaded = TRUE;
   tmppreset->state = state;
   tmppreset->enkv = enkvKeyRange;

   tmpKVRange = new QFKeyVelRange;
   tmpKVRange->byLow = 0;
   tmpKVRange->byHigh = 127;

   for (wCurChannel = 0; wCurChannel < wNumChannels; wCurChannel++)
   {
      tmpsounddata = new SoundData;
      tmpsounddata->wAllocatedRefCount = 1;
      tmpsounddata->wInUseRefCount = ((state != qfpsSamplesNotAvailable) ? 1 : 0);
      qf.sndmem->fileNameList.Find(strFileName);
      tmpsounddata->strFileName =
         (CHAR *)(*qf.sndmem->fileNameList.GetCurItem());
      tmpstr = (CHAR *)NewAlloc(strlen(bankInfo.strBankName)+3);
      // Can't use sprintf in drivers
      //sprintf(tmpstr, "%s%d", bankInfo.strBankName, wCurChannel);
      strcpy(tmpstr, bankInfo.strBankName);
      wDummy = strlen(tmpstr);
      tmpstr[wDummy] = wCurChannel/10+'0';
      tmpstr[wDummy+1] = wCurChannel%10+'0';
      tmpstr[wDummy+2] = '\0';
      tmpsounddata->strSoundName = tmpstr;
      DeleteAlloc(tmpstr);
      tmpsounddata->dwStartLocInBytes = dwDataPos +
         wCurChannel*((wBitsPerSample == 16) ? 2 : 1);
      tmpsounddata->dwSizeInBytes = bankInfo.dwTotalSoundSize/wNumChannels;
      tmpsounddata->dwCurrOffset = 0;
      tmpsounddata->bySoundDataType =
         ((wBitsPerSample == 16) ? sdt16BitMask : 0) |
         ((wNumChannels-1) & sdtChannelMask);
      snddatalist.Insert(tmpsounddata);

      tmprange = new QFRange;
      tmprange->byKeyLow = tmprange->byVelLow = 0;
      tmprange->byKeyHigh = tmprange->byVelHigh = 127;

      trans->SetDefaultParameters();
      tmpdata.dwEndAddrs = bankInfo.dwTotalSoundSize/
         (((wBitsPerSample == 16) ? 2 : 1)*wNumChannels);
      if (wNumChannels > 1)
      {
         tmpdata.dwSampleModes = WAVsmStereo |
            ((wCurChannel == (wNumChannels-1)) ? WAVsmMasterStereo : 0) |
            (((wCurChannel + 1)%wNumChannels) << 16);
         tmpdata.lPanEffectsSend =
            (LONG)(wCurChannel/(wNumChannels-1)*1000)-500;
      }
      else
      {
         tmpdata.dwSampleModes = 0;
         tmpdata.lPanEffectsSend = 0;
      }
      tmpdata.dwSampleRate = dwSampleRate;
      trans->SetSoundParameters((void *)&tmpdata, wNumChannels > 1);
      trans->TranslateAndStore(tmprange->artdata);

      tmprange->artdata.soundptr = tmpsounddata;
#ifndef __NO_MOD_CTRL
      tmprange->artdata.modtable = &defModTable;
      defModTable.AddRef();
#endif
      tmpKVRange->range.Insert(tmprange);
   }
   tmppreset->kvrange.Insert(tmpKVRange);

   if (IsOK())
   {
      tmpbank->preset.Insert(tmppreset);
      if (bInsertBank)
         qf.list.Insert(tmpbank);
      qf.sndmem->ActionList.Merge(snddatalist);
   }
   else
   {
      delete tmpKVRange;
      delete tmppreset;
      if (bInsertBank)
         delete tmpbank;
      return;
   }

   currPercent = 100;
   if (callBackFunction != NULL)
      callBackFunction(emuCallBackReading, currPercent);

#  ifdef __USE_ARRAYCACHE
      ArrayCache::ReadOptimize();
#  endif
}

#ifndef __NO_MOD_CTRL
void QFWAVReader::SetDefaultModulators(ModulatorTable *modTable)
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
	  
	  // Effects Send 1 (D)
	  modTable->NewModulator(ssCC91, syPositiveUnipolar,
                 trans->GetDestination(chorusEffectsSend), 200, stLinear,
                 ssEndSupportedSources, syEndSupportedSourceTypes);
	  
	  // Effects Send 2 (C)
	  modTable->NewModulator(ssCC93, syPositiveUnipolar,
                 trans->GetDestination(reverbEffectsSend), 200, stLinear,
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
#endif

void QFWAVReader::SetBankInfo()
{
   CHAR *str;
   SHORT i;

   ClearError();
   str = (CHAR *)NewAlloc(strlen(strFileName)+1);
   strcpy(str, strFileName);
   for (i = strlen(str)-1; (i >= 0) && (str[i] != '/') &&
        (str[i] != '\\') && (str[i] != ':'); i--)
      if (str[i] == '.')
         str[i] = '\0';

   bankInfo.strBankName = str+i+1;
   DeleteAlloc(str);
   bankInfo.wNumPresets = 1;
   bankInfo.wNumInsts = 1;
   bankInfo.wNumSounds = 1;
//   tRIFF->FindChunk(MAKE_ID("data"));
   tRIFF->SetCurPosition(DataPos);
   bankInfo.dwTotalSoundSize = tRIFF->GetChunkSize();
}

void QFWAVReader::AddFileAndBankToLists(QuickFont& qf)
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

///////////////////////////////////////////////////////////////////////////////
// Code to dynamically insert WAV code into list of available file types

class QFWAVReaderDyn
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   QFWAVReaderDyn()
   {sri = new SoundRegInfo;
    sri->strFileType = "Microsoft Wave";
    sri->strFileExtension = "WAV";
    sri->alloc = QFWAVReader::Alloc;
    if (sriList == NULL)
       sriList = new SoundRegInfoList;
    sriList->Insert(sri);}
   ~QFWAVReaderDyn()
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

QFWAVReaderDyn dummyWAV;
