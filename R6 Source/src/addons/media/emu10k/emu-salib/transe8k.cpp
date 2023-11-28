//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: transe8k.cpp
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

// Include files
#include <string.h>
#include "transe8k.h"
#include "xlatgen.h"
#include "qfsf2.h"
#include "qfwav.h"
#include "qfdls.h"
#include "quickfnt.h"
#include "saoutput.h"
#include "souttrns.h"

// External static variables

extern SHORT SF2Default[];
extern LONG WAVDefault[];
#ifdef DLS
extern LONG DLSDefault[];
#endif
extern LONG SAOutputDefault[];

// Internal function prototypes

typedef LONG (*scfunc)(LONG);
typedef void (*sdfunc)(LONG *, Emu8000Dest, LONG, BOOL);

class SrcToDest
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   Emu8000Dest tabledest;
   Emu8000Dest dest;
   scfunc conv;
   sdfunc apply;
};

// Generic functions

LONG NoConv(LONG value);
LONG CoarseAddrConv(LONG value);
LONG CoarseTuneConv(LONG value);
void NoApply(LONG *, Emu8000Dest, LONG, BOOL);
void DefAssign(LONG *arttable, Emu8000Dest dest, LONG value, BOOL bAdd);
void DefAdd(LONG *arttable, Emu8000Dest dest, LONG value, BOOL);
void DefOrEq(LONG *arttable, Emu8000Dest dest, LONG value, BOOL);
void ConvPan(LONG *arttable, Emu8000Dest dest, LONG value, BOOL bAdd);

// SF2 functions
void SF2Emu8000ComputeInitialPitch(LONG *arttable);
void SF2Emu8000ConvInitialPitch(LONG *arttable, Emu8000Dest dest, LONG value,
                                BOOL);
void SF2Emu8000ConvAddrs(LONG *arttable, Emu8000Dest dest, LONG value,
                         BOOL);
void SF2Emu8000ConvForceKey(LONG *arttable, Emu8000Dest dest, LONG value,
                            BOOL);
void SF2Emu8000ConvForceVel(LONG *arttable, Emu8000Dest dest, LONG value,
                            BOOL);
void SF2Emu8000ConvHoldDecay(LONG *arttable, Emu8000Dest dest, LONG value,
                             BOOL bAdd);

// DLS functions
LONG LongShortConv(LONG value);
LONG NegLongShortConv(LONG value);
LONG Conv1000Minus(LONG value);
LONG ConvScaleTuning(LONG value);
void DLSEmu8000ComputeInitialPitch(LONG *arttable);
void DLSEmu8000ConvInitialPitch(LONG *arttable, Emu8000Dest dest, LONG value,
                                BOOL);
void DLSEmu8000ConvAddrs(LONG *arttable, Emu8000Dest dest, LONG value,
                         BOOL);
void DLSEmu8000ConvInitialAttenuation(LONG *arttable, Emu8000Dest dest,
                                      LONG value, BOOL);


// Generic functions

LONG NoConv(LONG value)
{
   return value;
}

LONG CoarseAddrConv(LONG value)
{
   return value*32768;
}

LONG CoarseTuneConv(LONG value)
{
   return value*100;
}

void NoApply(LONG *, Emu8000Dest, LONG, BOOL)
{
}

void DefAssign(LONG *arttable, Emu8000Dest dest, LONG value, BOOL bAdd)
{
   if (bAdd)
      arttable[dest] += value;
   else
      arttable[dest] = value;
}

void DefAdd(LONG *arttable, Emu8000Dest dest, LONG value, BOOL)
{
   arttable[dest] += value;
}

void DefOrEq(LONG *arttable, Emu8000Dest dest, LONG value, BOOL)
{
   arttable[dest] |= value;
}

void ConvPan(LONG *arttable, Emu8000Dest dest, LONG value, BOOL bAdd)
{
   if (bAdd)
   {
      arttable[qfPanLEffectsSend] += value;
      arttable[qfPanREffectsSend] += value;
   }
   else
   {
      arttable[qfPanLEffectsSend] = value;
      arttable[qfPanREffectsSend] = value;
   }
}

// SF2 functions

void SF2Emu8000ComputeInitialPitch(LONG *arttable)
{
   arttable[qfInitialPitch] = arttable[qfCoarseTuning] + arttable[qfFineTuning] +
      arttable[qfScaleTuning]*(64-arttable[qfRootKey]);
}

void SF2Emu8000ConvInitialPitch(LONG *arttable, Emu8000Dest dest, LONG value,
                                BOOL)
{
   switch(dest) {
   case qfFineTuning:
   case qfCoarseTuning:
      arttable[dest] += value; //MP: Always additive, relative to chFineCorrection
      break;
   case qfScaleTuning:
   case qfRootKey:
      arttable[dest] = value;
      break;
   };
   SF2Emu8000ComputeInitialPitch(arttable);
}

void SF2Emu8000ConvAddrs(LONG *arttable, Emu8000Dest dest, LONG value,
                         BOOL)
{
   switch(dest) {
   case qfFineStartAddrs:
   case qfCoarseStartAddrs:
      arttable[dest] = value;
      arttable[qfStartAddrs] =
         arttable[qfCoarseStartAddrs] + arttable[qfFineStartAddrs] +
         arttable[qfGlobalStartAddrs];
      break;
   case qfFineEndAddrs:
   case qfCoarseEndAddrs:
      arttable[dest] = value;
      arttable[qfEndAddrs] =
         arttable[qfCoarseEndAddrs] + arttable[qfFineEndAddrs] +
         arttable[qfGlobalEndAddrs];
      break;
   case qfFineStartloopAddrs:
   case qfCoarseStartloopAddrs:
      arttable[dest] = value;
      arttable[qfStartloopAddrs] =
         arttable[qfCoarseStartloopAddrs] + arttable[qfFineStartloopAddrs] +
         arttable[qfGlobalStartloopAddrs];
      break;
   case qfFineEndloopAddrs:
   case qfCoarseEndloopAddrs:
      arttable[dest] = value;
      arttable[qfEndloopAddrs] =
         arttable[qfCoarseEndloopAddrs] + arttable[qfFineEndloopAddrs] +
         arttable[qfGlobalEndloopAddrs];
      break;
   };
}

void SF2Emu8000ConvForceKey(LONG *arttable, Emu8000Dest dest, LONG value,
                            BOOL)
{
   uqfKeyVel tmp;

   tmp.lNum = arttable[dest];
   tmp.qfkv.byKey = (CHAR)value;
   arttable[dest] = tmp.lNum;
}

void SF2Emu8000ConvForceVel(LONG *arttable, Emu8000Dest dest, LONG value,
                            BOOL)
{
   uqfKeyVel tmp;

   tmp.lNum = arttable[dest];
   tmp.qfkv.byVel = (CHAR)value;
   arttable[dest] = tmp.lNum;
}

void SF2Emu8000ConvHoldDecay(LONG *arttable, Emu8000Dest dest, LONG value,
                             BOOL bAdd)
{
    if (bAdd)
        arttable[dest] += value;
    else
        arttable[dest] = value;

    switch(dest) {
    case qfHoldModEnv:
    case qfKeyNumToModEnvHold:
        arttable[qfHoldEnv1] = arttable[qfHoldModEnv] -
                               4*arttable[qfKeyNumToModEnvHold];
        break;
    case qfDecayModEnv:
    case qfKeyNumToModEnvDecay:
        arttable[qfDecayEnv1] = arttable[qfDecayModEnv] -
                                4*arttable[qfKeyNumToModEnvDecay];
        break;
    case qfHoldVolEnv:
    case qfKeyNumToVolEnvHold:
        arttable[qfHoldEnv2] = arttable[qfHoldVolEnv] -
                               4*arttable[qfKeyNumToVolEnvHold];
        break;
    case qfDecayVolEnv:
    case qfKeyNumToVolEnvDecay:
        arttable[qfDecayEnv2] = arttable[qfDecayVolEnv] -
                                4*arttable[qfKeyNumToVolEnvDecay];
        break;
    };
}

// DLS functions

LONG LongShortConv(LONG value)
{
   return value/65536;
}

LONG NegLongShortConv(LONG value)
{
   return -value/65536;
}

LONG Conv1000Minus(LONG value)
{
    return 1000 - value/65536;
}

LONG ConvScaleTuning(LONG value)
{
    return value/128;
}

void DLSEmu8000ComputeInitialPitch(LONG *arttable)
{
   arttable[qfInitialPitch] = arttable[qfFineTuning] + arttable[qfSampleTuning] +
      arttable[qfScaleTuning]*(64-arttable[qfRootKey]);
}

void DLSEmu8000ConvInitialPitch(LONG *arttable, Emu8000Dest dest, LONG value,
                                BOOL)
{
   arttable[dest] = value;
   DLSEmu8000ComputeInitialPitch(arttable);
}

void DLSEmu8000ConvAddrs(LONG *arttable, Emu8000Dest dest, LONG value,
                         BOOL)
{
   arttable[dest] = value;
   switch(dest) {
   case qfFineStartAddrs:
      arttable[qfStartAddrs] = arttable[qfFineStartAddrs] +
                               arttable[qfGlobalStartAddrs];
      break;
   case qfFineEndAddrs:
      arttable[qfEndAddrs] = arttable[qfFineEndAddrs] +
                             arttable[qfGlobalEndAddrs];
      break;
   case qfFineStartloopAddrs:
      arttable[qfStartloopAddrs] = arttable[qfFineStartloopAddrs] +
                                   arttable[qfGlobalStartloopAddrs];
      break;
   case qfFineEndloopAddrs:
      arttable[qfEndloopAddrs] = arttable[qfFineEndloopAddrs] +
                                 arttable[qfGlobalEndloopAddrs];
      break;
   };
}

void DLSEmu8000ConvInitialAttenuation(LONG *arttable, Emu8000Dest dest,
                                      LONG value, BOOL)
{
   arttable[dest] = value;
   arttable[qfInitialVolume] = arttable[qfSampleAttenuation] +
                               arttable[qfRangeAttenuation];
}

// SF2 source to Emu8000 destination table

SrcToDest SF2Emu8000Conv[numSF2Gens] = {
/* startAddrsOffset           */ {qfFineStartAddrs,       qfStartAddrs,          NoConv,         SF2Emu8000ConvAddrs},
/* endAddrsOffset             */ {qfFineEndAddrs,         qfEndAddrs,            NoConv,         SF2Emu8000ConvAddrs},
/* startloopAddrsOffset       */ {qfFineStartloopAddrs,   qfStartloopAddrs,      NoConv,         SF2Emu8000ConvAddrs},
/* endloopAddrsOffset         */ {qfFineEndloopAddrs,     qfEndloopAddrs,        NoConv,         SF2Emu8000ConvAddrs},
/* startAddrsCoarseOffset     */ {qfCoarseStartAddrs,     qfStartAddrs,          CoarseAddrConv, SF2Emu8000ConvAddrs},
/* modLfoToPitch              */ {qfLfo1ToPitch,          qfLfo1ToPitch,         NoConv,         DefAssign},
/* vibLfoToPitch              */ {qfLfo2ToPitch,          qfLfo2ToPitch,         NoConv,         DefAssign},
/* modEnvToPitch              */ {qfEnv1ToPitch,          qfEnv1ToPitch,         NoConv,         DefAssign},
/* initialFilterFc            */ {qfInitialFilterFc,      qfInitialFilterFc,     NoConv,         DefAssign},
/* initialFilterQ             */ {qfInitialFilterQ,       qfInitialFilterQ,      NoConv,         DefAssign},
/* modLfoToFilterFc           */ {qfLfo1ToFilterFc,       qfLfo1ToFilterFc,      NoConv,         DefAssign},
/* modEnvToFilterFc           */ {qfEnv1ToFilterFc,       qfEnv1ToFilterFc,      NoConv,         DefAssign},
/* endAddrsCoarseOffset       */ {qfCoarseEndAddrs,       qfEndAddrs,            CoarseAddrConv, SF2Emu8000ConvAddrs},
/* modLfoToVolume             */ {qfLfo1ToVolume,         qfLfo1ToVolume,        NoConv,         DefAssign},
/* unused1                    */ {lastEmu8000Dest,        lastEmu8000Dest,       NoConv,         NoApply},
/* chorusEffectsSend          */ {qfChorusEffectsSend,    qfChorusEffectsSend,   NoConv,         DefAssign},
/* reverbEffectsSend          */ {qfReverbEffectsSend,    qfReverbEffectsSend,   NoConv,         DefAssign},
/* pan                        */ {qfPanLEffectsSend,      qfPanLEffectsSend,     NoConv,         ConvPan},
/* unused2                    */ {lastEmu8000Dest,        lastEmu8000Dest,       NoConv,         NoApply},
/* unused3                    */ {lastEmu8000Dest,        lastEmu8000Dest,       NoConv,         NoApply},
/* unused4                    */ {lastEmu8000Dest,        lastEmu8000Dest,       NoConv,         NoApply},
/* delayModLfo                */ {qfDelayLfo1,            qfDelayLfo1,           NoConv,         DefAssign},
/* freqModLfo                 */ {qfFreqLfo1,             qfFreqLfo1,            NoConv,         DefAssign},
/* delayVibLfo                */ {qfDelayLfo2,            qfDelayLfo2,           NoConv,         DefAssign},
/* freqVibLfo                 */ {qfFreqLfo2,             qfFreqLfo2,            NoConv,         DefAssign},
/* delayModEnv                */ {qfDelayEnv1,            qfDelayEnv1,           NoConv,         DefAssign},
/* attackModEnv               */ {qfAttackEnv1,           qfAttackEnv1,          NoConv,         DefAssign},
/* holdModEnv                 */ {qfHoldModEnv,           qfHoldEnv1,            NoConv,         SF2Emu8000ConvHoldDecay},
/* decayModEnv                */ {qfDecayModEnv,          qfDecayEnv1,           NoConv,         SF2Emu8000ConvHoldDecay},
/* sustainModEnv              */ {qfSustainEnv1,          qfSustainEnv1,         NoConv,         DefAssign},
/* releaseModEnv              */ {qfReleaseEnv1,          qfReleaseEnv1,         NoConv,         DefAssign},
/* keynumToModEnvHold         */ {qfKeyNumToModEnvHold,   qfHoldEnv1,            NoConv,         SF2Emu8000ConvHoldDecay},
/* keynumToModEnvDecay        */ {qfKeyNumToModEnvDecay,  qfDecayEnv1,           NoConv,         SF2Emu8000ConvHoldDecay},
/* delayVolEnv                */ {qfDelayEnv2,            qfDelayEnv2,           NoConv,         DefAssign},
/* attackVolEnv               */ {qfAttackEnv2,           qfAttackEnv2,          NoConv,         DefAssign},
/* holdVolEnv                 */ {qfHoldVolEnv,           qfHoldEnv2,            NoConv,         SF2Emu8000ConvHoldDecay},
/* decayVolEnv                */ {qfDecayVolEnv,          qfDecayEnv2,           NoConv,         SF2Emu8000ConvHoldDecay},
/* sustainVolEnv              */ {qfSustainEnv2,          qfSustainEnv2,         NoConv,         DefAssign},
/* releaseVolEnv              */ {qfReleaseEnv2,          qfReleaseEnv2,         NoConv,         DefAssign},
/* keynumToVolEnvHold         */ {qfKeyNumToVolEnvHold,   qfHoldEnv2,            NoConv,         SF2Emu8000ConvHoldDecay},
/* keynumToVolEnvDecay        */ {qfKeyNumToVolEnvDecay,  qfDecayEnv2,           NoConv,         SF2Emu8000ConvHoldDecay},
/* instrument                 */ {lastEmu8000Dest,        lastEmu8000Dest,       NoConv,         NoApply},
/* reserved1                  */ {lastEmu8000Dest,        lastEmu8000Dest,       NoConv,         NoApply},
/* keyRange                   */ {lastEmu8000Dest,        lastEmu8000Dest,       NoConv,         NoApply},
/* velRange                   */ {lastEmu8000Dest,        lastEmu8000Dest,       NoConv,         NoApply},
/* startLoopAddrsCoarseOffset */ {qfCoarseStartloopAddrs, qfStartloopAddrs,      CoarseAddrConv, SF2Emu8000ConvAddrs},
/* keynum                     */ {qfForceKeyVel,          qfForceKeyVel,         NoConv,         SF2Emu8000ConvForceKey},
/* velocity                   */ {qfForceKeyVel,          qfForceKeyVel,         NoConv,         SF2Emu8000ConvForceVel},
/* initialAttenuation         */ {qfInitialVolume,        qfInitialVolume,       NoConv,         DefAssign},
/* reserved2                  */ {lastEmu8000Dest,        lastEmu8000Dest,       NoConv,         NoApply},
/* endloopAddrsCoarseOffset   */ {qfCoarseEndloopAddrs,   qfEndloopAddrs,        CoarseAddrConv, SF2Emu8000ConvAddrs},
/* coarseTune                 */ {qfCoarseTuning,         qfInitialPitch,        CoarseTuneConv, SF2Emu8000ConvInitialPitch},
/* fineTune                   */ {qfFineTuning,           qfInitialPitch,        NoConv,         SF2Emu8000ConvInitialPitch},
/* sampleId                   */ {lastEmu8000Dest,        lastEmu8000Dest,       NoConv,         NoApply},
/* sampleModes                */ {qfSampleModes,          qfSampleModes,         NoConv,         DefOrEq},
/* reserved3                  */ {lastEmu8000Dest,        lastEmu8000Dest,       NoConv,         NoApply},
/* scaleTuning                */ {qfScaleTuning,          qfInitialPitch,        NoConv,         SF2Emu8000ConvInitialPitch},
/* exclusiveClass             */ {qfExclusiveClass,       qfExclusiveClass,      NoConv,         DefAssign},
/* overridingRootKey          */ {qfRootKey,              qfInitialPitch,        NoConv,         SF2Emu8000ConvInitialPitch},
/* unused5                    */ {lastEmu8000Dest,        lastEmu8000Dest,       NoConv,         NoApply},
/* endOper                    */ {lastEmu8000Dest,        lastEmu8000Dest,       NoConv,         NoApply}
   };

// DLS source to Emu8000 destination table

#ifdef DLS
SrcToDest DLSEmu8000Conv[dlsNumArticulators] = {
/* dlsArtModLfoFreq           */ {qfFreqLfo1,             qfFreqLfo1,            LongShortConv,  DefAssign},
/* dlsArtModLfoDelay          */ {qfDelayLfo1,            qfDelayLfo1,           LongShortConv,  DefAssign},
/* dlsArtVibLfoFreq           */ {qfFreqLfo2,             qfFreqLfo2,            LongShortConv,  DefAssign},
/* dlsArtVibLfoDelay          */ {qfDelayLfo2,            qfDelayLfo2,           LongShortConv,  DefAssign},
/* dlsArtVolEnvDelay          */ {qfDelayEnv2,            qfDelayEnv2,           LongShortConv,  DefAssign},
/* dlsArtVolEnvAttack         */ {qfAttackEnv2,           qfAttackEnv2,          LongShortConv,  DefAssign},
/* dlsArtVolEnvHold           */ {qfHoldEnv2,             qfHoldEnv2,            LongShortConv,  DefAssign},
/* dlsArtVolEnvDecay          */ {qfDecayEnv2,            qfDecayEnv2,           LongShortConv,  DefAssign},
/* dlsArtVolEnvSustain        */ {qfSustainEnv2,          qfSustainEnv2,         Conv1000Minus,  DefAssign},
/* dlsArtVolEnvRelease        */ {qfReleaseEnv2,          qfReleaseEnv2,         LongShortConv,  DefAssign},
/* dlsArtModEnvDelay          */ {qfDelayEnv1,            qfDelayEnv1,           LongShortConv,  DefAssign},
/* dlsArtModEnvAttack         */ {qfAttackEnv1,           qfAttackEnv1,          LongShortConv,  DefAssign},
/* dlsArtModEnvHold           */ {qfHoldEnv1,             qfHoldEnv1,            LongShortConv,  DefAssign},
/* dlsArtModEnvDecay          */ {qfDecayEnv1,            qfDecayEnv1,           LongShortConv,  DefAssign},
/* dlsArtModEnvSustain        */ {qfSustainEnv1,          qfSustainEnv1,         Conv1000Minus,  DefAssign},
/* dlsArtModEnvRelease        */ {qfReleaseEnv1,          qfReleaseEnv1,         LongShortConv,  DefAssign},
/* dlsArtInitialFc            */ {qfInitialFilterFc,      qfInitialFilterFc,     LongShortConv,  DefAssign},
/* dlsArtInitialFq            */ {qfInitialFilterQ,       qfInitialFilterQ,      LongShortConv,  DefAssign},
/* dlsArtModLfoToFc           */ {qfLfo1ToFilterFc,       qfLfo1ToFilterFc,      LongShortConv,  DefAssign},
/* dlsArtModEnvToFc           */ {qfEnv1ToFilterFc,       qfEnv1ToFilterFc,      LongShortConv,  DefAssign},
/* dlsArtSampleStart          */ {qfFineStartAddrs,       qfStartAddrs,          NoConv,         DLSEmu8000ConvAddrs},
/* dlsArtSampleEnd            */ {qfFineEndAddrs,         qfEndAddrs,            NoConv,         DLSEmu8000ConvAddrs},
/* dlsArtSampleStartLoop      */ {qfFineStartloopAddrs,   qfStartloopAddrs,      NoConv,         DLSEmu8000ConvAddrs},
/* dlsArtSampleEndLoop        */ {qfFineEndloopAddrs,     qfEndloopAddrs,        NoConv,         DLSEmu8000ConvAddrs},
/* dlsArtInitialAttenuation   */ {qfRangeAttenuation,     qfInitialVolume,       NegLongShortConv, DLSEmu8000ConvInitialAttenuation},
/* dlsArtModLfoToAttenuation  */ {qfLfo1ToVolume,         qfLfo1ToVolume,        NegLongShortConv, DefAssign},
/* dlsArtFineTune             */ {qfFineTuning,           qfInitialPitch,        LongShortConv,  DLSEmu8000ConvInitialPitch},
/* dlsArtVibLfoToPitch        */ {qfLfo2ToPitch,          qfLfo2ToPitch,         LongShortConv,  DefAssign},
/* dlsArtModLfoToPitch        */ {qfLfo1ToPitch,          qfLfo1ToPitch,         LongShortConv,  DefAssign},
/* dlsArtModEnvToPitch        */ {qfEnv1ToPitch,          qfEnv1ToPitch,         LongShortConv,  DefAssign},
/* dlsArtPan                  */ {qfPanLEffectsSend,      qfPanLEffectsSend,     LongShortConv,  ConvPan},
/* dlsArtChorus               */ {qfChorusEffectsSend,    qfChorusEffectsSend,   LongShortConv,  DefAssign},
/* dlsArtReverb               */ {qfReverbEffectsSend,    qfReverbEffectsSend,   LongShortConv,  DefAssign},
/* dlsArtKeyGroup             */ {qfExclusiveClass,       qfExclusiveClass,      NoConv,         DefAssign},
/* dlsArtScaleTuning          */ {qfScaleTuning,          qfInitialPitch,        ConvScaleTuning, DLSEmu8000ConvInitialPitch},
   };
#endif

// SAOutput to Emu8000 destination table
SrcToDest SAOutputEmu8000Conv[dlsNumArticulators] = {
/* saospGain                  */ {qfInitialVolume,    qfInitialVolume,    NegLongShortConv, DefAssign},
/* saospPan                   */ {qfPanLEffectsSend,  qfPanLEffectsSend,  LongShortConv,    ConvPan},
/* saospPitch                 */ {qfInitialPitch,     qfInitialPitch,     LongShortConv,    DefAssign},
/* saospFilterCutoff          */ {qfInitialFilterFc,  qfInitialFilterFc,  LongShortConv,    DefAssign},
/* saospFilterQ               */ {qfInitialFilterQ,   qfInitialFilterQ,   LongShortConv,    DefAssign},
/* saospDelayVolEnv           */ {qfDelayEnv2,        qfDelayEnv2,        LongShortConv,    DefAssign},
/* saospAttackVolEnv          */ {qfAttackEnv2,       qfAttackEnv2,       LongShortConv,    DefAssign},
/* saospHoldVolEnv            */ {qfHoldEnv2,         qfHoldEnv2,         LongShortConv,    DefAssign},
/* saospDecayVolEnv           */ {qfDecayEnv2,        qfDecayEnv2,        LongShortConv,    DefAssign},
/* saospSustainVolEnv         */ {qfSustainEnv2,      qfSustainEnv2,      Conv1000Minus,    DefAssign},
/* saospReleaseVolEnv         */ {qfReleaseEnv2,      qfReleaseEnv2,      LongShortConv,    DefAssign},
/* saospDelayModEnv           */ {qfDelayEnv1,        qfDelayEnv1,        LongShortConv,    DefAssign},
/* saospAttackModEnv          */ {qfAttackEnv1,       qfAttackEnv1,       LongShortConv,    DefAssign},
/* saospHoldModEnv            */ {qfHoldEnv1,         qfHoldEnv1,         LongShortConv,    DefAssign},
/* saospDecayModEnv           */ {qfDecayEnv1,        qfDecayEnv1,        LongShortConv,    DefAssign},
/* saospSustainModEnv         */ {qfSustainEnv1,      qfSustainEnv1,      Conv1000Minus,    DefAssign},
/* saospReleaseModEnv         */ {qfReleaseEnv1,      qfReleaseEnv1,      LongShortConv,    DefAssign},
/* saospDelayModLFO           */ {qfDelayLfo1,        qfDelayLfo1,        LongShortConv,    DefAssign},
/* saospFreqModLFO            */ {qfFreqLfo1,         qfFreqLfo1,         LongShortConv,    DefAssign},
/* saospDelayVibLFO           */ {qfDelayLfo2,        qfDelayLfo2,        LongShortConv,    DefAssign},
/* saospFreqVibLFO            */ {qfFreqLfo2,         qfFreqLfo2,         LongShortConv,    DefAssign},
/* saospModLFOToPitch         */ {qfLfo1ToPitch,      qfLfo1ToPitch,      LongShortConv,    DefAssign},
/* saospVibLFOToPitch         */ {qfLfo2ToPitch,      qfLfo2ToPitch,      LongShortConv,    DefAssign},
/* saospModLFOToFilterCutoff  */ {qfLfo1ToFilterFc,   qfLfo1ToFilterFc,   LongShortConv,    DefAssign},
/* saospModLFOToGain          */ {qfLfo1ToVolume,     qfLfo1ToVolume,     NegLongShortConv, DefAssign},
/* saospModEnvToPitch         */ {qfEnv1ToPitch,      qfEnv1ToPitch,      LongShortConv,    DefAssign},
/* saospModEnvToFilterCutoff  */ {qfEnv1ToFilterFc,   qfEnv1ToFilterFc,   LongShortConv,    DefAssign},
};

// Translator code

Emu8000Translator::Emu8000Translator(void)
{
	SetupTables();
}

Emu8000Translator::Emu8000Translator(EmuAbstractTranslatorApplicatorClass
                                     *xlatapparg) :
   TranslatorBase(xlatapparg)
{
   SetupTables();
}

void Emu8000Translator::SetupTables(void)
{
   arttable = (LONG *)NewAlloc(numTempEmu8000Dests*sizeof(LONG));
   defarttable = (LONG *)NewAlloc(numTempEmu8000Dests*sizeof(LONG));
}

Emu8000Translator::~Emu8000Translator()
{
   DeleteAlloc(arttable);
   DeleteAlloc(defarttable);
}

void Emu8000Translator::SetFileType(Str strFileExtension)
{
   strFileType = strFileExtension;
   memset(defarttable, 0, numTempEmu8000Dests*sizeof(LONG));
   if (strFileType == Str("SF2"))
   {
      WORD i;
      SrcToDest *tmp;

      for (i = 0; i < numSF2Gens; i++)
      {
         tmp = &SF2Emu8000Conv[i];
         tmp->apply(defarttable, tmp->tabledest, tmp->conv(SF2Default[i]), FALSE);
      }
      conv = SF2Emu8000Conv;
   }
   else if (strFileType == Str("WAV"))
   {
      defarttable[qfForceKeyVel] = WAVDefault[WAVForceKeyVel];
      defarttable[qfDelayLfo1] = defarttable[qfDelayLfo2] =
         defarttable[qfDelayEnv1] = defarttable[qfAttackEnv1] =
         defarttable[qfDecayEnv1] = defarttable[qfReleaseEnv1] =
         defarttable[qfDelayEnv2] = defarttable[qfAttackEnv2] =
         defarttable[qfDecayEnv2] = defarttable[qfReleaseEnv2] =
            WAVDefault[WAVMinValue];
         defarttable[qfHoldEnv1] = defarttable[qfHoldEnv2] =
            WAVDefault[WAVMaxValue];
      defarttable[qfInitialFilterFc] = WAVDefault[WAVFilterFc];
      defarttable[qfScaleTuning] = WAVDefault[WAVScaleTuning];
      defarttable[qfSampleRate] = WAVDefault[WAVSampleRate];
      defarttable[qfRootKey] = WAVDefault[WAVRootKey];
      defarttable[qfSampleModes] = WAVDefault[WAVSampleModes];
      defarttable[qfPanLEffectsSend] = WAVDefault[WAVPanEffectsSend];
      defarttable[qfPanREffectsSend] = WAVDefault[WAVPanEffectsSend];
      defarttable[qfInitialPitch] = WAVDefault[WAVInitialPitch] +
          WAVDefault[WAVScaleTuning]*(64-WAVDefault[WAVRootKey]);
      conv = SF2Emu8000Conv;
   }
   else if (strFileType == Str("SF2NRPN"))
   {
      defarttable[qfRootKey] = 64;
      conv = SF2Emu8000Conv;
   }
#ifdef DLS
   else if (strFileType == Str("DLS"))
   {
      WORD i;
      SrcToDest *tmp;
      uqfKeyVel kv;

      for (i = 0; i < dlsNumArticulators; i++)
      {
         tmp = &DLSEmu8000Conv[i];
         tmp->apply(defarttable, tmp->tabledest, tmp->conv(DLSDefault[i]), FALSE);
      }
      kv.qfkv.byKey = kv.qfkv.byVel = (CHAR)-1;
      defarttable[qfForceKeyVel] = kv.lNum;
      conv = DLSEmu8000Conv;
   }
#endif
   else if (strFileType == Str("SAOutput"))
   {
      WORD i;
      SrcToDest *tmp;

      for (i = 0; i < saospNumSynthParams; i++)
      {
         tmp = &SAOutputEmu8000Conv[i];
         tmp->apply(defarttable, tmp->tabledest, tmp->conv(SAOutputDefault[i]), FALSE);
      }
      conv = SAOutputEmu8000Conv;
   }
}

void Emu8000Translator::SetDefaultParameters()
{
   memcpy(arttable, defarttable, numTempEmu8000Dests*sizeof(LONG));
}

void Emu8000Translator::SetSoundParameters(void *sndparams, BOOL bStereo)
{
   if (strFileType == Str("SF2"))
   {
      sfSample *shdrinfo = (sfSample *)sndparams;

      if (shdrinfo->wSampleType & ROMSample)
      {
         arttable[qfStartAddrs] = arttable[qfGlobalStartAddrs] =
            shdrinfo->dwStart;
         arttable[qfEndAddrs] = arttable[qfGlobalEndAddrs] = shdrinfo->dwEnd;
         arttable[qfStartloopAddrs] = arttable[qfGlobalStartloopAddrs] =
            shdrinfo->dwStartLoop;
         arttable[qfEndloopAddrs] = arttable[qfGlobalEndloopAddrs] =
            shdrinfo->dwEndLoop;
      }
      else
      {
         arttable[qfStartAddrs] = arttable[qfGlobalStartAddrs] = 0;
         arttable[qfEndAddrs] = arttable[qfGlobalEndAddrs] =
            shdrinfo->dwEnd - shdrinfo->dwStart;
         arttable[qfStartloopAddrs] = arttable[qfGlobalStartloopAddrs] =
            shdrinfo->dwStartLoop - shdrinfo->dwStart;
         arttable[qfEndloopAddrs] = arttable[qfGlobalEndloopAddrs] =
            shdrinfo->dwEndLoop - shdrinfo->dwStart;
      }
      if ((shdrinfo->wSampleType & rightSample) && bStereo)
         arttable[qfSampleModes] = qfsmStereo | qfsmMasterStereo |
            (shdrinfo->wSampleLink << 16);
      else if ((shdrinfo->wSampleType & leftSample) && bStereo)
         arttable[qfSampleModes] = qfsmStereo |
            (shdrinfo->wSampleLink << 16);
      else
         arttable[qfSampleModes] = 0;
      arttable[qfSampleRate] = (LONG)shdrinfo->dwSampleRate;
      arttable[qfFineTuning] = (LONG)shdrinfo->chFineCorrection;
      arttable[qfRootKey] = shdrinfo->byOriginalKey;
      SF2Emu8000ComputeInitialPitch(arttable);
   }
   else if (strFileType == Str("WAV"))
   {
      WAVData *wavinfo = (WAVData *)sndparams;

      arttable[qfEndAddrs] = arttable[qfGlobalEndAddrs] = wavinfo->dwEndAddrs;
      if (bStereo)
         arttable[qfSampleModes] = wavinfo->dwSampleModes;
      else
         arttable[qfSampleModes] = 0;
      arttable[qfPanLEffectsSend] = wavinfo->lPanEffectsSend;
      arttable[qfPanREffectsSend] = wavinfo->lPanEffectsSend;
      arttable[qfSampleRate] = wavinfo->dwSampleRate;
   }
#ifdef DLS
   else if (strFileType == Str("DLS"))
   {
       DLSSampleData *sampleData = (DLSSampleData *)sndparams;

       arttable[qfRootKey] = sampleData->byUnityNote;
       arttable[qfSampleTuning] = sampleData->shFineTune;
       arttable[qfSampleAttenuation] = NegLongShortConv(sampleData->lAttenuation);
       arttable[qfInitialVolume] = arttable[qfSampleAttenuation] +
                                   arttable[qfRangeAttenuation];
       arttable[qfGlobalStartAddrs] = 0;
       arttable[qfGlobalEndAddrs] = sampleData->dwEnd;
       if ((sampleData->dwLoopStart == 0) && (sampleData->dwLoopLength == 0))
           arttable[qfSampleModes] = 0;
       else
       {
           arttable[qfSampleModes] = qfsmLoop;
           arttable[qfGlobalStartloopAddrs] = sampleData->dwLoopStart;
           arttable[qfGlobalEndloopAddrs] =
               sampleData->dwLoopStart + sampleData->dwLoopLength;
       }
       arttable[qfStartAddrs] = arttable[qfFineStartAddrs] +
                                arttable[qfGlobalStartAddrs];
       arttable[qfEndAddrs] = arttable[qfFineEndAddrs] +
                              arttable[qfGlobalEndAddrs];
       arttable[qfStartloopAddrs] = arttable[qfFineStartloopAddrs] +
                                    arttable[qfGlobalStartloopAddrs];
       arttable[qfEndloopAddrs] = arttable[qfFineEndloopAddrs] +
                                  arttable[qfGlobalEndloopAddrs];
       arttable[qfSampleRate] = sampleData->dwSamplesPerSec;
       DLSEmu8000ComputeInitialPitch(arttable);
   }
#endif
   else if (strFileType == Str("SAOutput"))
   {
       SAOutputSampleData *sampleData = (SAOutputSampleData *)sndparams;

       arttable[qfSampleRate] = sampleData->dwSamplesPerSec;
   }
}

void Emu8000Translator::SetParameter(WORD wIndex, LONG lValue, BOOL bAdd)
{
   SrcToDest *tmp = &conv[wIndex];
   tmp->apply(arttable, tmp->tabledest, tmp->conv(lValue), bAdd);
}

void Emu8000Translator::SetParameterAndTranslate(WORD wIndex, LONG lValue,
                                                 BOOL bAdd)
{
   SrcToDest *tmp = &conv[wIndex];
   tmp->apply(arttable, tmp->tabledest, tmp->conv(lValue), bAdd);
   arttable[tmp->dest] = xlatapp->transDestIndex[tmp->dest](arttable[tmp->dest]);
   if (tmp->dest == qfPanLEffectsSend)
      arttable[qfPanREffectsSend] = xlatapp->transDestIndex[qfPanREffectsSend](arttable[tmp->dest]);
// Apply sample rate to initial pitch
   if (tmp->dest == qfInitialPitch)
      arttable[qfInitialPitch] =
         xlatapp->applyDestIndex[qfInitialPitch](arttable[qfSampleRate],
         arttable[qfInitialPitch], xlatapp->transDestIndex[qfSampleRate]);
}

LONG Emu8000Translator::GetSourceParameter(WORD wIndex)
{
   return arttable[conv[wIndex].tabledest];
}

LONG Emu8000Translator::GetDestParameter(WORD wIndex)
{
   return arttable[wIndex];
}

WORD Emu8000Translator::GetDestination(WORD wIndex)
{
   return conv[wIndex].dest;
}

void Emu8000Translator::Translate()
{
   WORD i;

   if (xlatapp!=NULL)
   {
		for (i = 0; i < numEmu8000Dests; i++)
			arttable[i] = xlatapp->transDestIndex[i](arttable[i]);

// Apply sample rate to initial pitch
   arttable[qfInitialPitch] =
      xlatapp->applyDestIndex[qfInitialPitch](arttable[qfSampleRate],
      arttable[qfInitialPitch], xlatapp->transDestIndex[qfSampleRate]);

   }
}

void Emu8000Translator::TranslateAndStore(QFArtData& qfad)
{
   WORD i;

   if (xlatapp!=NULL)
   {
		for (i = 0; i < numEmu8000Dests; i++)
			arttable[i] = xlatapp->transDestIndex[i](arttable[i]);

// Apply sample rate to initial pitch
   arttable[qfInitialPitch] =
      xlatapp->applyDestIndex[qfInitialPitch](arttable[qfSampleRate],
      arttable[qfInitialPitch], xlatapp->transDestIndex[qfSampleRate]);

   }

#ifndef __USE_ARRAYCACHE
   qfad.arttable = (LONG *)NewAlloc(numEmu8000Dests*sizeof(LONG));
   for (i = 0; i < numEmu8000Dests; i++)
      qfad.arttable[i] = arttable[i];
#else
   qfad.arttable.SetArray(numEmu8000Dests, tmparttable);
#endif
}
