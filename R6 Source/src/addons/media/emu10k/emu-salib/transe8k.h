//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: transe8k.h
//
// Author: Michael Preston
//
// Description: Translators for E-mu 8000
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  -----------------------------------------
// Michael Preston     Oct 15, 1996  Initial development.
//
//*****************************************************************************

#ifndef __TRANSE8K_H
#define __TRANSE8K_H

// Include files
#include "transgen.h"
#include "stringcl.h"

//typedef enum
//{
#define qfsmLoop           0x1     // 0 - non-looping sample, 1 - looping sample
#define qfsmNoContLoop     0x2     // 0 - continuously looping sample,
                                   // 1 - non-continuously looping sample
#define qfsmLoopModesMask  0x3FFF  // Mask for loop modes portion
   
#define qfsmStereo         0x4000  // 0 - not stereo, 1 - stereo
#define qfsmMasterStereo   0x8000  // 0 - doesn't control pitch for stereo pair
                                   // 1 - controls pitch for stereo pair
#define qfsmStereoMask     0xC000  // Mask for stereo information
#define qfsmStereoLinkMask 0xFFFF0000  // Mask for stereo link field
//} qfSampleModeDefs;

struct qfKeyVel
{
   CHAR byKey;
   CHAR byVel;
};

union uqfKeyVel
{
   qfKeyVel qfkv;
   LONG lNum;
};

typedef enum
{
   qfEndAddrs,
   qfSampleModes,
   qfForceKeyVel,
   qfExclusiveClass,
   qfDelayLfo1,
   qfFreqLfo1,
   qfDelayLfo2,
   qfFreqLfo2,
   qfDelayEnv1,
   qfAttackEnv1,
   qfHoldEnv1,
   qfDecayEnv1,
   qfSustainEnv1,
   qfReleaseEnv1,
   qfDelayEnv2,
   qfAttackEnv2,
   qfHoldEnv2,
   qfDecayEnv2,
   qfSustainEnv2,
   qfReleaseEnv2,
   qfStartAddrs,
   qfStartloopAddrs,
   qfEndloopAddrs,
   qfInitialPitch,
   qfLfo1ToPitch,
   qfLfo2ToPitch,
   qfEnv1ToPitch,
   qfInitialFilterFc,
   qfInitialFilterQ,
   qfLfo1ToFilterFc,
   qfEnv1ToFilterFc,
   qfInitialVolume,
   qfLfo1ToVolume,
   qfChorusEffectsSend,
   qfReverbEffectsSend,
   qfPanLEffectsSend,
   qfPanREffectsSend,

#  ifndef __NO_MOD_CTRL
   lastEmu8000Dest,
   qfKeyNumToVolEnvHold=lastEmu8000Dest,
#  else
   qfKeyNumToVolEnvHold,
#  endif

   qfKeyNumToVolEnvDecay,
   qfKeyNumToModEnvHold,
   qfKeyNumToModEnvDecay,
   qfScaleTuning,

#  ifdef __NO_MOD_CTRL
   lastEmu8000Dest,
   qfSampleRate=lastEmu8000Dest,
#  else
   qfSampleRate,
#  endif

   qfSampleAttenuation,
   qfHoldModEnv=qfSampleAttenuation,
   qfRangeAttenuation,
   qfDecayModEnv=qfRangeAttenuation,
   qfSampleTuning,
   qfHoldVolEnv=qfSampleTuning,
   qfDecayVolEnv,
   qfRootKey,
   qfCoarseTuning,
   qfFineTuning,
   qfCoarseStartAddrs,
   qfFineStartAddrs,
   qfCoarseEndAddrs,
   qfFineEndAddrs,
   qfCoarseStartloopAddrs,
   qfFineStartloopAddrs,
   qfCoarseEndloopAddrs,
   qfFineEndloopAddrs,
   qfGlobalStartAddrs,
   qfGlobalEndAddrs,
   qfGlobalStartloopAddrs,
   qfGlobalEndloopAddrs,
//
   lastTempEmu8000Dest
} Emu8000Dest;

#define numEmu8000Dests lastEmu8000Dest
#define numTempEmu8000Dests lastTempEmu8000Dest

class SrcToDest;

class Emu8000Translator : public TranslatorBase
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   Emu8000Translator(EmuAbstractTranslatorApplicatorClass *xlatapparg);
   Emu8000Translator();
   ~Emu8000Translator();

   void SetFileType(Str strFileExtension);
   void SetDefaultParameters();
   void SetSoundParameters(void *sndparams, BOOL bStereo);
   void SetParameter(WORD wIndex, LONG lValue, BOOL bAdd);
   void SetParameterAndTranslate(WORD wIndex, LONG lValue, BOOL bAdd);
   LONG GetSourceParameter(WORD wIndex);
   LONG GetDestParameter(WORD wIndex);
   WORD GetDestination(WORD wIndex);
   void Translate();
   void TranslateAndStore(QFArtData& qfad);

   static TranslatorBase *Alloc(EmuAbstractTranslatorApplicatorClass
                                *xlatapparg)
      {return new Emu8000Translator(xlatapparg);}

   private:

   void SetupTables(void);

   LONG *arttable;
   LONG *defarttable;
   SrcToDest *conv;
   Str strFileType;
};

#endif
