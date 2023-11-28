#include <string.h>
#include "xlat8000.h"
#include "transe8k.h"
#include "emu_rw.h"

#define MODULENAME "SF2 to E8K Standard"

Emu8000TransApp::Emu8000TransApp(void) : EmuAbstractTranslatorApplicatorClass(MODULENAME)
{
  transDestIndex = NULL;
  applyDestIndex = NULL;
  remveDestIndex = NULL;
  BuildTables();
}

Emu8000TransApp::~Emu8000TransApp(void)
{
  BlastTables();
}

void Emu8000TransApp::
BlastTables(void)
{
  if (transDestIndex != NULL)
    delete [] transDestIndex;
  transDestIndex = NULL;

  if (applyDestIndex != NULL)
    delete [] applyDestIndex;
  applyDestIndex = NULL;

  if (remveDestIndex != NULL)
    delete [] remveDestIndex;
  remveDestIndex = NULL;

}

void  Emu8000TransApp::
BuildTables(void)
{

  // Each table type has two forms: a Translator which converts a value
  // from SoundFont units to synthesizer units, and an Applicator which
  // does an effective "add" of a SoundFont unit value to a synthesizer
  // unit value
  BuildDestIndexTable();
}

void Emu8000TransApp::
BuildDestIndexTable()
{
  if ((transDestIndex == NULL) &&
      ((transDestIndex = new translator[LastVoiceVariable]) == NULL))
  {
    SetError(SF_MEMORYERROR);
    return;
  }

  if ((applyDestIndex == NULL) &&
      ((applyDestIndex = new applicator[LastVoiceVariable]) == NULL))
  {
    SetError(SF_MEMORYERROR);
    return;
  }

  if ((remveDestIndex == NULL) &&
      ((remveDestIndex = new precremove[LastVoiceVariable]) == NULL))
  {
    SetError(SF_MEMORYERROR);
    return;
  }


  // Build translator table
  transDestIndex[0                 ] = TransNoTranslation;
  transDestIndex[DelayLfo1         ] = TransTimeCentsToDelay;
  transDestIndex[FreqLfo1          ] = TransCentsToLFOFreq;
  transDestIndex[DelayLfo2         ] = TransTimeCentsToDelay;
  transDestIndex[FreqLfo2          ] = TransCentsToLFOFreq;
  transDestIndex[DelayEnv1         ] = TransTimeCentsToDelay;
  transDestIndex[AttackEnv1        ] = TransTimeCentsToEnvAttack;
  transDestIndex[HoldEnv1          ] = TransTimeCentsToEnvHold;
  transDestIndex[DecayEnv1         ] = TransTimeCentsToEnvDecayRelease;
  transDestIndex[SustainEnv1       ] = TransP1PercentToEnvSustain;
  transDestIndex[ReleaseEnv1       ] = TransTimeCentsToEnvDecayRelease;
  transDestIndex[DelayEnv2         ] = TransTimeCentsToDelay;
  transDestIndex[AttackEnv2        ] = TransTimeCentsToEnvAttack;
  transDestIndex[HoldEnv2          ] = TransTimeCentsToEnvHold;
  transDestIndex[DecayEnv2         ] = TransTimeCentsToEnvDecayRelease;
  transDestIndex[SustainEnv2       ] = TransP1PercentToEnvSustain;
  transDestIndex[ReleaseEnv2       ] = TransTimeCentsToEnvDecayRelease;
  transDestIndex[StartAddrs        ] = TransNoTranslation;
  transDestIndex[StartloopAddrs    ] = TransNoTranslation;
  transDestIndex[EndloopAddrs      ] = TransNoTranslation;
  transDestIndex[InitialPitch      ] = TransCentsToInitialPitch;
  transDestIndex[Lfo1ToPitch       ] = TransCentsMaxExcurToPitchMod;
  transDestIndex[Lfo2ToPitch       ] = TransCentsMaxExcurToPitchMod;
  transDestIndex[Env1ToPitch       ] = TransCentsMaxExcurToPitchMod;
  transDestIndex[InitialFilterFc   ] = TransCentsToFilterCutoff;
  transDestIndex[InitialFilterQ    ] = TransCbToFilterQ;
  transDestIndex[Lfo1ToFilterFc    ] = TransCentsMaxExcurToLFOFiltMod;
  transDestIndex[Env1ToFilterFc    ] = TransCentsMaxExcurToEnvFiltMod;
  transDestIndex[InitialVolume     ] = TransCbToInitialAtten;
  transDestIndex[Lfo1ToVolume      ] = TransCbMaxExcurToLFOVolMod;
  transDestIndex[ChorusEffectsSend ] = TransP1PercentToEffectSend;
  transDestIndex[ReverbEffectsSend ] = TransP1PercentToEffectSend;
  transDestIndex[PanEffectsSend    ] = TransP1PercentToPan;
  transDestIndex[AuxEffectsSend    ] = TransP1PercentToPan;

  // Sound data reader specific translations
  transDestIndex[qfEndAddrs           ] = TransNoTranslation;
  transDestIndex[qfForceKeyVel        ] = TransNoTranslation;
  transDestIndex[qfExclusiveClass     ] = TransNoTranslation;
  transDestIndex[qfSampleModes        ] = TransNoTranslation;
  transDestIndex[qfSampleRate         ] = TransHzToCents;

# ifdef __NO_MOD_CTRL
  transDestIndex[qfScaleTuning        ] = TransNoTranslation;
  transDestIndex[qfKeyNumToVolEnvHold ] = TransNoTranslation;
  transDestIndex[qfKeyNumToVolEnvDecay] = TransNoTranslation;
# endif


  // Build applicator table
  applyDestIndex[0                 ] = ApplySimpleAdd;
  applyDestIndex[DelayLfo1         ] = ApplyTransAdd;
  applyDestIndex[FreqLfo1          ] = ApplyTransAdd;
  applyDestIndex[DelayLfo2         ] = ApplyTransAdd;
  applyDestIndex[FreqLfo2          ] = ApplyTransAdd;
  applyDestIndex[DelayEnv1         ] = ApplyTransAdd;
  applyDestIndex[AttackEnv1        ] = ApplyTransAdd;
  applyDestIndex[HoldEnv1          ] = ApplyTransAdd;
  applyDestIndex[DecayEnv1         ] = ApplyTransAdd;
  applyDestIndex[SustainEnv1       ] = ApplyTransAdd;
  applyDestIndex[ReleaseEnv1       ] = ApplyTransAdd;
  applyDestIndex[DelayEnv2         ] = ApplyTransAdd;
  applyDestIndex[AttackEnv2        ] = ApplyTransAdd;
  applyDestIndex[HoldEnv2          ] = ApplyTransAdd;
  applyDestIndex[DecayEnv2         ] = ApplyTransAdd;
  applyDestIndex[SustainEnv2       ] = ApplyTransAdd;
  applyDestIndex[ReleaseEnv2       ] = ApplyTransAdd;
  applyDestIndex[StartAddrs        ] = ApplyTransAdd;
  applyDestIndex[StartloopAddrs    ] = ApplyTransAdd;
  applyDestIndex[EndloopAddrs      ] = ApplyTransAdd;
  applyDestIndex[InitialPitch      ] = ApplyTransAdd;
  applyDestIndex[Lfo1ToPitch       ] = ApplyTransAdd;
  applyDestIndex[Lfo2ToPitch       ] = ApplyTransAdd;
  applyDestIndex[Env1ToPitch       ] = ApplyTransAdd;
  applyDestIndex[InitialFilterFc   ] = ApplyTransAdd;
  applyDestIndex[InitialFilterQ    ] = ApplyTransAdd;
  applyDestIndex[Lfo1ToFilterFc    ] = ApplyTransAdd;
  applyDestIndex[Env1ToFilterFc    ] = ApplyTransAdd;
  applyDestIndex[InitialVolume     ] = ApplyTransAdd;
  applyDestIndex[Lfo1ToVolume      ] = ApplyTransAdd;
  applyDestIndex[ChorusEffectsSend ] = ApplyTransAdd;
  applyDestIndex[ReverbEffectsSend ] = ApplyTransAdd;
  applyDestIndex[PanEffectsSend    ] = ApplyTransAdd;
  applyDestIndex[AuxEffectsSend    ] = ApplyTransAdd;
  applyDestIndex[CurrentPitch      ] = ApplyTransAdd;
  applyDestIndex[Fraction          ] = ApplyTransAdd;
  applyDestIndex[Stereo            ] = ApplyTransAdd;
  applyDestIndex[CurrentVolume     ] = ApplyTransAdd;
  applyDestIndex[CurrentFilter     ] = ApplyTransAdd;
  applyDestIndex[FilterDelayMemory1] = ApplyTransAdd;
  applyDestIndex[FilterDelayMemory2] = ApplyTransAdd;

  // Build precision removal table
  remveDestIndex[0                 ] = RemveNoTranslation;
  remveDestIndex[DelayLfo1         ] = RemveTimeCentsToDelay;
  remveDestIndex[FreqLfo1          ] = RemveCentsToLFOFreq;
  remveDestIndex[DelayLfo2         ] = RemveTimeCentsToDelay;
  remveDestIndex[FreqLfo2          ] = RemveCentsToLFOFreq;
  remveDestIndex[DelayEnv1         ] = RemveTimeCentsToDelay;
  remveDestIndex[AttackEnv1        ] = RemveTimeCentsToEnvAttack;
  remveDestIndex[HoldEnv1          ] = RemveTimeCentsToEnvHold;
  remveDestIndex[DecayEnv1         ] = RemveTimeCentsToEnvDecayRelease;
  remveDestIndex[SustainEnv1       ] = RemveP1PercentToEnvSustain;
  remveDestIndex[ReleaseEnv1       ] = RemveTimeCentsToEnvDecayRelease;
  remveDestIndex[DelayEnv2         ] = RemveTimeCentsToDelay;
  remveDestIndex[AttackEnv2        ] = RemveTimeCentsToEnvAttack;
  remveDestIndex[HoldEnv2          ] = RemveTimeCentsToEnvHold;
  remveDestIndex[DecayEnv2         ] = RemveTimeCentsToEnvDecayRelease;
  remveDestIndex[SustainEnv2       ] = RemveP1PercentToEnvSustain;
  remveDestIndex[ReleaseEnv2       ] = RemveTimeCentsToEnvDecayRelease;
  remveDestIndex[StartAddrs        ] = RemveNoTranslation;
  remveDestIndex[StartloopAddrs    ] = RemveNoTranslation;
  remveDestIndex[EndloopAddrs      ] = RemveNoTranslation;
  remveDestIndex[InitialPitch      ] = RemveCentsToInitialPitch;
  remveDestIndex[Lfo1ToPitch       ] = RemveCentsMaxExcurToPitchMod;
  remveDestIndex[Lfo2ToPitch       ] = RemveCentsMaxExcurToPitchMod;
  remveDestIndex[Env1ToPitch       ] = RemveCentsMaxExcurToPitchMod;
  remveDestIndex[InitialFilterFc   ] = RemveCentsToFilterCutoff;
  remveDestIndex[InitialFilterQ    ] = RemveCbToFilterQ;
  remveDestIndex[Lfo1ToFilterFc    ] = RemveCentsMaxExcurToLFOFiltMod;
  remveDestIndex[Env1ToFilterFc    ] = RemveCentsMaxExcurToEnvFiltMod;
  remveDestIndex[InitialVolume     ] = RemveCbToInitialAtten;
  remveDestIndex[Lfo1ToVolume      ] = RemveCbMaxExcurToLFOVolMod;
  remveDestIndex[ChorusEffectsSend ] = RemveP1PercentToEffectSend;
  remveDestIndex[ReverbEffectsSend ] = RemveP1PercentToEffectSend;
  remveDestIndex[PanEffectsSend    ] = RemveP1PercentToPan;
  remveDestIndex[AuxEffectsSend    ] = RemveP1PercentToNPan;
  remveDestIndex[CurrentPitch      ] = RemveNoTranslation;
  remveDestIndex[Fraction          ] = RemveNoTranslation;
  remveDestIndex[Stereo            ] = RemveNoTranslation;
  remveDestIndex[CurrentVolume     ] = RemveNoTranslation;
  remveDestIndex[CurrentFilter     ] = RemveNoTranslation;
  remveDestIndex[FilterDelayMemory1] = RemveNoTranslation;
  remveDestIndex[FilterDelayMemory2] = RemveNoTranslation;
}

/*
Note also that the logMagTable could be implemented as a SHORT array
and shifted; the low order 4 bits are noise, and that the logSlopeTable
could be implemented 8 bit (CHAR), but
that the logSlopeTable multiply must be a SHORT multiply to give a 14
bit result.
*/

static const DWORD logMagTable[128] = {
0x00000, 0x02dfc, 0x05b9e, 0x088e6, 0x0b5d6, 0x0e26f, 0x10eb3, 0x13aa2,
0x1663f, 0x1918a, 0x1bc84, 0x1e72e, 0x2118b, 0x23b9a, 0x2655d, 0x28ed5,
0x2b803, 0x2e0e8, 0x30985, 0x331db, 0x359eb, 0x381b6, 0x3a93d, 0x3d081,
0x3f782, 0x41e42, 0x444c1, 0x46b01, 0x49101, 0x4b6c4, 0x4dc49, 0x50191,
0x5269e, 0x54b6f, 0x57006, 0x59463, 0x5b888, 0x5dc74, 0x60029, 0x623a7,
0x646ee, 0x66a00, 0x68cdd, 0x6af86, 0x6d1fa, 0x6f43c, 0x7164b, 0x73829,
0x759d4, 0x77b4f, 0x79c9a, 0x7bdb5, 0x7dea1, 0x7ff5e, 0x81fed, 0x8404e,
0x86082, 0x88089, 0x8a064, 0x8c014, 0x8df98, 0x8fef1, 0x91e20, 0x93d26,
0x95c01, 0x97ab4, 0x9993e, 0x9b79f, 0x9d5d9, 0x9f3ec, 0xa11d8, 0xa2f9d,
0xa4d3c, 0xa6ab5, 0xa8808, 0xaa537, 0xac241, 0xadf26, 0xafbe7, 0xb1885,
0xb3500, 0xb5157, 0xb6d8c, 0xb899f, 0xba58f, 0xbc15e, 0xbdd0c, 0xbf899,
0xc1404, 0xc2f50, 0xc4a7b, 0xc6587, 0xc8073, 0xc9b3f, 0xcb5ed, 0xcd07c,
0xceaec, 0xd053f, 0xd1f73, 0xd398a, 0xd5384, 0xd6d60, 0xd8720, 0xda0c3,
0xdba4a, 0xdd3b4, 0xded03, 0xe0636, 0xe1f4e, 0xe384a, 0xe512c, 0xe69f3,
0xe829f, 0xe9b31, 0xeb3a9, 0xecc08, 0xee44c, 0xefc78, 0xf148a, 0xf2c83,
0xf4463, 0xf5c2a, 0xf73da, 0xf8b71, 0xfa2f0, 0xfba57, 0xfd1a7, 0xfe8df};

static const SHORT logSlopeTable[128] = {
0x5c, 0x5c, 0x5b, 0x5a, 0x5a, 0x59, 0x58, 0x58,
0x57, 0x56, 0x56, 0x55, 0x55, 0x54, 0x53, 0x53,
0x52, 0x52, 0x51, 0x51, 0x50, 0x50, 0x4f, 0x4f,
0x4e, 0x4d, 0x4d, 0x4d, 0x4c, 0x4c, 0x4b, 0x4b,
0x4a, 0x4a, 0x49, 0x49, 0x48, 0x48, 0x47, 0x47,
0x47, 0x46, 0x46, 0x45, 0x45, 0x45, 0x44, 0x44,
0x43, 0x43, 0x43, 0x42, 0x42, 0x42, 0x41, 0x41,
0x41, 0x40, 0x40, 0x40, 0x3f, 0x3f, 0x3f, 0x3e,
0x3e, 0x3e, 0x3d, 0x3d, 0x3d, 0x3c, 0x3c, 0x3c,
0x3b, 0x3b, 0x3b, 0x3b, 0x3a, 0x3a, 0x3a, 0x39,
0x39, 0x39, 0x39, 0x38, 0x38, 0x38, 0x38, 0x37,
0x37, 0x37, 0x37, 0x36, 0x36, 0x36, 0x36, 0x35,
0x35, 0x35, 0x35, 0x34, 0x34, 0x34, 0x34, 0x34,
0x33, 0x33, 0x33, 0x33, 0x32, 0x32, 0x32, 0x32,
0x32, 0x31, 0x31, 0x31, 0x31, 0x31, 0x30, 0x30,
0x30, 0x30, 0x30, 0x2f, 0x2f, 0x2f, 0x2f, 0x2f};

/**************************************
** Constants used for conversions
**************************************/

#ifndef __8010__
#define SCALE_MASTER_CLOCK 12174
#else
#define SCALE_MASTER_CLOCK 11185
#endif

#define CENTS_1200_TO_8_BITS 150
#define CENTS_1200_TO_16_BITS 55
#define SET_BIT_4   0x0008
#define SET_BIT_16 0x8000
#define SET_BIT_17 0x10000
#define MASK_LOW_3_BITS 7
#define SHIFT_OFFSET 16
#define MAX_16_BIT 65536
#define MAX_SHIFT 31
 
#define COARSE_TUNE_ADJUST 874
 
//////////// Decay/Release constants //////////////
#ifndef __8010__
#  define MAX_DECAY_FOR_FACTOR 1800
#  define OFFSET_TO_EMU8000_DECAY 39
#  define DIVISOR_EMU8000_DECAY 75
#else
// See envdectime.XLS for details
#  define MAX_DECAY_FOR_FACTOR 1650
#  define OFFSET_TO_EMU8000_DECAY 38
#  define DIVISOR_EMU8000_DECAY 75
#endif

#define SCALE_TO_EMU8000_PITCH_MOD 6991
#define SCALED_MIN_EMU8000_PITCH_MOD -8382209L
#define SCALED_MAX_EMU8000_PITCH_MOD 8382209L
 
//////////// Low pass filter cutoff constants /////
#ifndef __8010__
#  define DIVISOR_EMU8000_FC 29
#  define OFFSET_EMU8000_FC 153
#else
// Roughly linear with TimeCents, but frequency now
// in different increments.
#  define DIVISOR_EMU8000_FC 29
#  define OFFSET_EMU8000_FC 153
#endif

//////////// Delay time constants ////////////////
#define EMU8000_DELAY_OFFSET 0x8000
#define EMU8000_MIN_DELAY -660000
#ifndef __8010__
   // This value is the negative of 725 us in Time Cents
   // scaled to 1 octave (1200 tc) = 16 bits (65536)
#  define EMU8000_DELAY_MIN_OFFSET 688380
   // This value is 1/666 us (unused)
#  define SCALE_TO_EMU8000_DELAY_TIME 1379
#else
   // This value is the negative of 666 us in Time Cents 
   // scaled to 1 octave (1200 tc) = 16 bits (65536)
   // 1200 log2(666us) * 65536 / 1200
#  define EMU8000_DELAY_MIN_OFFSET 691548
   // This value is 1/666 us (unused)
#  define SCALE_TO_EMU8000_DELAY_TIME 1502
#endif

///////////// LFO Frequency constants /////////////////
#ifndef __8010__   
   // This is 42 mHz in cents, scaled to 16 bits
   // 1200 * log2(0.042/8.176) * 65535 / 1200 
#  define EMU8000_LFO_FREQ_MIN_OFFSET 498385
#else
   // This is 45.7 mHz in cents, scaled to 16 bits
   // 1200 * log2(0.0457/8.176) * 65535 / 1200 
#  define EMU8000_LFO_FREQ_MIN_OFFSET 490402
#endif
#define EMU8000_MIN_LFO_FREQ 0 

////////////// Attack time constants ///////////////////
#ifndef __8010__
#  define MAX_ATTACK_FOR_FACTOR -600
#  define OFFSET_FOR_SPEC_EMU8000_ATTACK -500
#  define OFFSET_FOR_EMU8000_ATTACK 8
#  define DIVISOR_FOR_EMU8000_ATTACK 75
#else
// See envatttime.XLS for details
#  define MAX_ATTACK_FOR_FACTOR -700
#  define OFFSET_FOR_SPEC_EMU8000_ATTACK -600
#  define OFFSET_FOR_EMU8000_ATTACK 6
#  define DIVISOR_FOR_EMU8000_ATTACK 75
#endif

//////////// Hold time constants //////////////////
#define OFFSET_FOR_EMU8000_HOLD 127
#define MAX_EMU8000_HOLD 0
#ifndef __8010__
   // This value is the negative of 92 ms in Time Cents
   // (4130 TC) times resulotion (55)
#  define OFFSET_FOR_EMU8000_MIN_HOLD 227150
#  define EMU8000_HOLD_MIN_OFFSET -33000
#  define EMU8000_HOLD_CENTS_1200_TO_16_BITS 54
#else
   // This value is the negative of 85.33 ms in Time Cents
   // (4261 TC) times resolution (54)
//#  define OFFSET_FOR_EMU8000_MIN_HOLD 235235 
#  define EMU8000_HOLD_CENTS_1200_TO_16_BITS 54
#  define OFFSET_FOR_EMU8000_MIN_HOLD 230094
#endif
// This value is a sanity check for 46 ms in Time Cents,
// after the constant for 92 ms is added.
#define EMU8000_HOLD_MIN_OFFSET -33000
#define SCALE_TO_EMU8000_HOLD   11

#define EMU8000_SUSTAIN_OFFSET  127
#define DIVISOR_FOR_EMU8000_SUSTAIN 8

#define SCALE_TO_EMU8000_LFO_FC_MOD 2329
#define SCALED_MIN_EMU8000_LFO_FC -8384400L
#define SCALED_MAX_EMU8000_LFO_FC 8349680L

#define SCALE_TO_EMU8000_ENV_FC_MOD 1165
#define SCALED_MIN_EMU8000_ENV_FC -8388000L
#define SCALED_MAX_EMU8000_ENV_FC 8386835L

#define DIVISOR_FOR_EMU8000_FQ 12

#define UNSCALED_OFFSET_TO_EMU8000_PAN 500
#define SCALED_OFFSET_TO_EMU8000_PAN 8356000
#define SCALE_TO_EMU8000_PAN 16712
#define SCALE_TO_EMU8000_RPAN 16984

#define SCALE_TO_EMU8000_ATTEN 17
#define SCALED_EMU8000_MAX_ATTEN 0x3FC0
#define SHIFT_FOR_EMU8000_ATTEN 6 

/************************
** Translators
************************/

LONG Emu8000TransApp::
TransNoTranslation(LONG amount) {return amount;}

LONG Emu8000TransApp::
TransForceZero(LONG) {return 0;}

LONG Emu8000TransApp::
TransHzToCents(LONG sampleRateHz)
{
  //WORD pitch;
  WORD count = 31;
  //DWORD storeRate;
  //fourByteUnion remainder;
  if (sampleRateHz <= 0) return 0; // insure 'count' never goes to 0

  #ifdef SR2PITCH_FLOAT
  double value = (double) sampleRate;
  // The significance of the magic numbers:
  // 44100 should correspond to 0xE00000 or 14 * 1M or 14 * 1048576
  // This correspondance should be logarithmic, such that 
  // 0xE00000 = K1 log(2)(44100 * K2);
  // 0xD00000 = K1 log(2)(22050 * K2);
  // 0xC00000 = K1 log(2)(11025 * K2);
  // So... since:
  // log(2) (44100 / 2.69164) = 14 = 0xE;
  // 1M * log(2) (44100/2.69164) = 1M * ( log(2) (44100) - log(2) 2.69164) 
  //  = 1M * log(2) (44100) - 1M * 1.42849;
  // 1497881.4 = 1048576 * 1.42849;
  // K1 = 1048576 = 1M
  // K2 = 1/2.69164
  return (DWORD)(((1048576.0*log10(value))/LOG2_DOUBLE)-1497881.4);
  #else
  DWORD sampleRate = (DWORD)sampleRateHz;
  sampleRate *= SCALE_MASTER_CLOCK;     /* Scale 44100 or 48000 to 0x200009b8 */
  for (count = 31; count > 0; count --)
  {
    if (sampleRate & 0x80000000)     /* Detect leading "1" */
      return (DWORD) (((LONG)(count-15) << 20) +
		       logMagTable[0x7f&(sampleRate>>24)] +
		       (0x7f&(sampleRate>>17)) *
		       logSlopeTable[0x7f&(sampleRate>>24)]);
    sampleRate = sampleRate << 1;
  }
  return 0;
  #endif
}

LONG Emu8000TransApp::
TransTimeCentsToDelay(LONG amount)
{
  return (LONG)(amount) * CENTS_1200_TO_16_BITS;
}

LONG Emu8000TransApp::
TransTimeCentsToEnvDecayRelease(LONG amount)
{
  return (LONG)amount;
}

LONG Emu8000TransApp::
TransCentsMaxExcurToPitchMod(LONG amount)
{
  return (LONG)amount*SCALE_TO_EMU8000_PITCH_MOD;
}

LONG Emu8000TransApp::
TransCentsToFilterCutoff(LONG amount)
{
  return (LONG)amount;
}

LONG Emu8000TransApp::
TransCentsToLFOFreq(LONG amount)
{
  return (LONG)amount*CENTS_1200_TO_16_BITS;
}

LONG Emu8000TransApp::
TransTimeCentsToEnvAttack(LONG amount)
{
  return (LONG)amount;
}

LONG Emu8000TransApp::
TransTimeCentsToEnvHold(LONG amount)
{
	// MG this broke keynum-to-hold, add resolution offsets only at 
	// precision removal stage!

	//amount += OFFSET_FOR_EMU8000_MIN_HOLD; 
	return (LONG)amount*EMU8000_HOLD_CENTS_1200_TO_16_BITS;
}

LONG Emu8000TransApp::
TransP1PercentToEnvSustain(LONG amount)
{
  return (LONG)amount;
}

LONG Emu8000TransApp::
TransCentsMaxExcurToLFOFiltMod(LONG amount)
{
  return (LONG)amount * SCALE_TO_EMU8000_LFO_FC_MOD;
}

LONG Emu8000TransApp::
TransCbToFilterQ(LONG amount)
{
  return (LONG)amount;
}

LONG Emu8000TransApp::
TransCentsMaxExcurToEnvFiltMod(LONG amount)
{
  return ((LONG)amount * SCALE_TO_EMU8000_ENV_FC_MOD);
}

LONG Emu8000TransApp::
TransCbMaxExcurToLFOVolMod(LONG amount)
{
  return (LONG)amount;
}

LONG Emu8000TransApp::
TransP1PercentToEffectSend(LONG amount)
{
  return (LONG)amount;
}

LONG Emu8000TransApp::
TransP1PercentToPan(LONG amount)
{
  return (LONG)amount * SCALE_TO_EMU8000_PAN;
}

LONG Emu8000TransApp::
TransCbToInitialAtten(LONG amount)
{
    return (LONG)amount*SCALE_TO_EMU8000_ATTEN;
	//return (LONG)((amount*SCALE_FOR_EMU8000_ATTEN)/DIVISOR_FOR_EMU8000_ATTEN);
}

LONG Emu8000TransApp::
TransCentsToInitialPitch(LONG amount)
{
  return (LONG)amount*COARSE_TUNE_ADJUST;
}

/***********************
** Applicators
************************/
LONG Emu8000TransApp::
ApplyTransAdd(LONG amont, LONG applyTo, translator xlat)
{
  return xlat(amont) + applyTo;
}

LONG Emu8000TransApp::
ApplySimpleAdd(LONG amont, LONG applyTo, translator)
{
  return amont + applyTo;
}

/***********************
** Precision Removers
************************/

LONG Emu8000TransApp::
RemveNoTranslation(LONG amount)
{
  return (LONG)amount;
}

LONG Emu8000TransApp::
RemveForceZero(LONG)
{
  return 0L;
}

LONG Emu8000TransApp::
RemveTimeCentsToDelay(LONG amount)
{
  fourByteUnion scratch;
  DWORD value, shift;

  // Initially values are stored as bipolar (positive or negative)
  // The integer log routines work only on positive values. So check
  // for a minimum and return early. 
  if (amount <= EMU8000_MIN_DELAY) return EMU8000_DELAY_OFFSET;

  // This makes amount positive
  amount += EMU8000_DELAY_MIN_OFFSET;
  
  // Perform 2^n
  scratch.dwVal = (DWORD)amount;
  value = (SET_BIT_17 | (DWORD)scratch.wVals.w0);
  shift = SHIFT_OFFSET - (SHORT)scratch.wVals.w1;

  // Check for very large numbers and return infinite delay if too large
  if (shift > MAX_SHIFT) return 0L;
  return RangeLongMinMax(EMU8000_DELAY_OFFSET-(value>>shift), 0, 
                         EMU8000_DELAY_OFFSET);
}

LONG Emu8000TransApp::
RemveTimeCentsToEnvDecayRelease(LONG amount)
{
  // Unfortunately since there is the skewered curve in the E8K, 
  // all translation must be done at here.

  DWORD value, shift;
  if (amount > MAX_DECAY_FOR_FACTOR)
  {
    value = (amount - MAX_DECAY_FOR_FACTOR)/CENTS_1200_TO_8_BITS;
    shift = value>>3;
    if (shift > MAX_SHIFT)
      amount = 0;
    else
      amount = (SET_BIT_4|((~value)&MASK_LOW_3_BITS)) >> shift;
  }
  else
    amount = OFFSET_TO_EMU8000_DECAY - amount/DIVISOR_EMU8000_DECAY;
  return (LONG)RangeMinMax((SHORT)amount, 0, 127);
}

LONG Emu8000TransApp::
RemveCentsMaxExcurToPitchMod(LONG amount)
{
  if (amount < SCALED_MIN_EMU8000_PITCH_MOD) return -128;
  if (amount > SCALED_MAX_EMU8000_PITCH_MOD) return 127;

  // Scaling done at load/controller input time
  fourByteUnion scratch;
  scratch.dwVal = (DWORD)amount;
  return (LONG)((DWORD)scratch.byVals.by2);
}

LONG Emu8000TransApp::
RemveCentsToFilterCutoff(LONG amount)
{
  // Can this be done at load time? I fear resolution loss...
  amount = (amount/DIVISOR_EMU8000_FC) - OFFSET_EMU8000_FC;

  return (LONG)RangeMinMax((SHORT)amount, 0, 255);
}

LONG Emu8000TransApp::
RemveCentsToLFOFreq(LONG amount)
{
  fourByteUnion scratch;
  DWORD value, shift;

  amount+=EMU8000_LFO_FREQ_MIN_OFFSET;  
  if (amount<=0) return EMU8000_MIN_LFO_FREQ;

  scratch.dwVal = (DWORD)amount;
  value = (SET_BIT_17 | (DWORD)scratch.wVals.w0);  
  shift = SHIFT_OFFSET - (SHORT)(scratch.wVals.w1);
  if (shift > MAX_SHIFT)
    return EMU8000_MIN_LFO_FREQ; 
  return RangeWordMinMax((WORD)(value>>shift), 0, 0xFF);
 
}

LONG Emu8000TransApp::
RemveTimeCentsToEnvAttack(LONG amount)
{
  DWORD value, shift;
  fourByteUnion scratch;

  scratch.dwVal = amount;
  if (amount > MAX_ATTACK_FOR_FACTOR)
  {
    value = ((LONG)scratch.dwVal-(OFFSET_FOR_SPEC_EMU8000_ATTACK))
                                                /CENTS_1200_TO_8_BITS;
    shift = value>>3;
    if (shift > MAX_SHIFT)
      amount = 0;
    else
      amount = (SET_BIT_4|((~value)&MASK_LOW_3_BITS)) >> shift;
  }
  else
    amount = OFFSET_FOR_EMU8000_ATTACK - amount/DIVISOR_FOR_EMU8000_ATTACK;
  return RangeMinMax((SHORT)amount, 1, 127);
}

LONG Emu8000TransApp::
RemveTimeCentsToEnvHold(LONG amount)
{
  amount += OFFSET_FOR_EMU8000_MIN_HOLD;
  if (amount < EMU8000_HOLD_MIN_OFFSET) return OFFSET_FOR_EMU8000_HOLD;
  if (amount < 0)                return OFFSET_FOR_EMU8000_HOLD-1;

  fourByteUnion scratch;
  DWORD value, shift, shiftLess;

  scratch.dwVal = (DWORD)amount;
  value = (SET_BIT_17 | (DWORD)scratch.wVals.w0);

  if ((shiftLess = scratch.wVals.w1) > SHIFT_OFFSET)
	  return MAX_EMU8000_HOLD;

  shift = SHIFT_OFFSET - (SHORT)shiftLess;
  if (shift > MAX_SHIFT)
    return OFFSET_FOR_EMU8000_HOLD;

  return (LONG)RangeMinMax((SHORT)(OFFSET_FOR_EMU8000_HOLD - (value>>shift)), 0, 127);
}

LONG Emu8000TransApp::
RemveP1PercentToEnvSustain(LONG amount)
{
  amount = EMU8000_SUSTAIN_OFFSET - (amount/DIVISOR_FOR_EMU8000_SUSTAIN);
  return RangeMinMax((SHORT)amount, 0, 127);
}

LONG Emu8000TransApp::
RemveCentsMaxExcurToLFOFiltMod(LONG amount)
{
  if (amount < SCALED_MIN_EMU8000_LFO_FC) return -128;
  if (amount > SCALED_MAX_EMU8000_LFO_FC) return 127;

  fourByteUnion scratch;
  scratch.dwVal = (DWORD)amount;
  return (LONG)((DWORD)scratch.byVals.by2);
}

LONG Emu8000TransApp::
RemveCbToFilterQ(LONG amount)
{
  return RangeMinMax(amount/DIVISOR_FOR_EMU8000_FQ, 0, 15);
}

LONG Emu8000TransApp::
RemveCentsMaxExcurToEnvFiltMod(LONG amount)
{
  if (amount < SCALED_MIN_EMU8000_ENV_FC) return -128;
  if (amount > SCALED_MAX_EMU8000_ENV_FC) return 127;

  fourByteUnion scratch;
  scratch.dwVal = (DWORD)amount;
  return (LONG)((DWORD)scratch.byVals.by2);
}

LONG Emu8000TransApp::
RemveCbMaxExcurToLFOVolMod(LONG amount)
{
  return (LONG)RangeMinMax((SHORT)amount, -128, 127);
}

LONG Emu8000TransApp::
RemveP1PercentToEffectSend(LONG amount)
{
	if (amount<=0) return 0;
  return (LONG)RangeWordMinMax((WORD)(amount>>2), 0, 255);
}

LONG Emu8000TransApp::
RemveP1PercentToPan(LONG amount)
{
  if (amount > 0)
      amount = (amount/SCALE_TO_EMU8000_PAN)*SCALE_TO_EMU8000_RPAN;
  amount += SCALED_OFFSET_TO_EMU8000_PAN;
  if (amount < 0) return 255;

  fourByteUnion scratch;
  scratch.dwVal = (DWORD)amount;

  return (LONG)(255 - (BYTE)RangeWordMinMax(scratch.wVals.w1, 0, 255));
}

LONG Emu8000TransApp::
RemveP1PercentToNPan(LONG amount)
{
  if (amount > 0)
      amount = (amount/SCALE_TO_EMU8000_PAN)*SCALE_TO_EMU8000_RPAN;
  amount += SCALED_OFFSET_TO_EMU8000_PAN;
  if (amount < 0) return 0;

  fourByteUnion scratch;
  scratch.dwVal = (DWORD)amount;

  return (LONG)((BYTE)RangeWordMinMax(scratch.wVals.w1, 0, 255));
}


LONG Emu8000TransApp::
RemveCbToInitialAtten(LONG amount)
{
  if (amount < 0) return 0;
  if (amount >= SCALED_EMU8000_MAX_ATTEN) return 255;
  //if (amount > SCALED_EMU8000_MAX) return 255;
  return amount >> (SHIFT_FOR_EMU8000_ATTEN); //+1
  //return amount>>EMU8000_ATTEN_SHIFT;
}

LONG Emu8000TransApp::
RemveCentsToInitialPitch(LONG amount)
{
  amount >>= 8;
  return RangeLongMinMax(amount, 0, 65535);
}


/***************************
** General purpose routines
***************************/
LONG  Emu8000TransApp::
RangeLongMinMax(LONG value, LONG min, LONG max)
{
  return (value>max) ? max : (value<min) ? min : value;
}

SHORT Emu8000TransApp::
RangeMinMax(SHORT iVal, SHORT iMin, SHORT iMax)
{
  if (iVal > iMax)
    return (iMax);
  if (iVal < iMin)
    return (iMin);
  return (iVal);
}

WORD Emu8000TransApp::
RangeWordMinMax(WORD iVal, WORD iMin, WORD iMax)
{
  if (iVal > iMax)
    return (iMax);
  if (iVal < iMin)
    return (iMin);
  return (iVal);
}
