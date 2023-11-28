//*****************************************************************************
//
//                             Copyright (c) 1998
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: qfdls.h
//
// Author: Michael Preston
//
// Description: Code specific for DLS files.
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  -----------------------------------------
// Michael Preston     Jun 17, 1998  Copied from qfwav.h
//
//*****************************************************************************

#ifndef __QFDLS_H
#define __QFDLS_H

// Include files

#include "datatype.h"
#include "win_mem.h"
#include "omega.h"
#include "qfsfread.h"
#include "sfenum.h"
#include "quickfnt.h"
#include "qftypes.h"

// Enumerations
enum dlsArticulators {
    dlsArtModLfoFreq,
    dlsArtModLfoDelay,
    dlsArtVibLfoFreq,
    dlsArtVibLfoDelay,
    dlsArtVolEnvDelay,
    dlsArtVolEnvAttack,
    dlsArtVolEnvHold,
    dlsArtVolEnvDecay,
    dlsArtVolEnvSustain,
    dlsArtVolEnvRelease,
    dlsArtModEnvDelay,
    dlsArtModEnvAttack,
    dlsArtModEnvHold,
    dlsArtModEnvDecay,
    dlsArtModEnvSustain,
    dlsArtModEnvRelease,
    dlsArtInitialFc,
    dlsArtInitialFq, // Doesn't exist yet
    dlsArtModLfoToFc,
	dlsArtModEnvToFc,
    dlsArtSampleStart,
    dlsArtSampleEnd,
    dlsArtSampleStartLoop,
    dlsArtSampleEndLoop,
    dlsArtInitialAttenuation,
    dlsArtModLfoToAttenuation,
    dlsArtFineTune,
    dlsArtVibLfoToPitch,
    dlsArtModLfoToPitch,
    dlsArtModEnvToPitch,
    dlsArtPan,
    dlsArtChorus,
    dlsArtReverb,
    dlsArtKeyGroup,
    dlsArtScaleTuning, // This is just a temporary articulator to compute pitch
//
    dlsArtEndArticulators
};

#define dlsNumArticulators dlsArtEndArticulators


class DLSSampleData
{
    public:

    void *operator new(size_t size) {return NewAlloc(size);}
    void operator delete(void *ptr) {DeleteAlloc(ptr);}

    BYTE byUnityNote;
    SHORT shFineTune;
    LONG lAttenuation;
    DWORD dwEnd;
    DWORD dwLoopStart;
    DWORD dwLoopLength;
    DWORD dwSamplesPerSec;
};

class dlsConnBlock;
class dlsConnBlockList;

class QFDLSReader : public QFSFReaderBase
{
    public:

    void *operator new(size_t size) {return NewAlloc(size);}
    void operator delete(void *ptr) {DeleteAlloc(ptr);}

    QFDLSReader(RIFFParser* newRIFF);
    ~QFDLSReader();

    static BOOL CheckFileType(RIFFParser* newRIFF);
    static QFSFReaderBase* Alloc(RIFFParser* newRIFF)
        {return (CheckFileType(newRIFF) ? new QFDLSReader(newRIFF) : NULL);}

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
    void ConvBankNumber(DWORD dwDlsBankNum, unMIDIBank *midiBankIndex);
    void SetKeyVelRanges(QFPreset *preset, QFRangeList *qfrl);
    void ReadInstData(QuickFont& qf, BOOL bBankLoad, WORD wPresetNdx,
                      WORD wBankNum, WORD wPresetNum, qfPresetState state,
                      WORD wInstanceNum);
    void ReadRegionData(QFRangeList *qfrl, dlsConnBlockList& globalDcbList,
                        BOOL bFoundRgn2);
    void ReadConnBlocks(dlsConnBlockList& dcbList);
    void SetArtData(
#ifndef __NO_MOD_CTRL
             ModulatorTable *modTable,
             BOOL& bFoundMods,
#endif
             dlsConnBlockList& dcbList);
    dlsArticulators GetArtNdx(WORD wSource, WORD wControl, WORD wDestination);
#ifndef __NO_MOD_CTRL
    enSupportedSources GetConnBlockSource(WORD wSource);
    dlsArticulators GetConnBlockDest(WORD wDestination);
    void GetConnBlockSourceType(WORD wTransform, enSupportedSourceType& sstSource,
                                enSupportedSourceStyles& sssSource, BOOL bSource);
    void GetConnBlockDestTransform(WORD wTransform,
                                   enSupportedTransforms& stDestination);
#endif
    void GetWsmpData(DLSSampleData *wsmpData);
    void GetSampleRefs();
    void ReadSampleData(SoundMemory& sndmem);
    void LoadSamples();
    void GetBankSizes();
    void SetBankInfo();
    void AddFileAndBankToLists(QuickFont& qf);

    QFBankInfo bankInfo;
    DWORD dwNumPoolCues;
    SoundData **SoundDataArr;
    DLSSampleData *SampleDataArr;
    SoundDataList snddatalist;
#ifndef __NO_MOD_CTRL
   static BOOL bDefModTableInitialized;
   static ModulatorTable defModTable;
#endif
};

#endif
