
/******************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1996. All rights Reserved.
*
*******************************************************************************
*/

/******************************************************************************
*
* Filename: Xlat8000.h
*
* Authors: Mike Guzewicz
*
* Description:  SoundFont to EMU8000 hardware translation and application
*               NOT to be used with Xlat8KRT.h!
*
* History:
*
* Version    Person        Date         Reason
* -------    ---------   -----------  ---------------------------------------
*  0.001       MG        Mar 13, 96   File Created
*
******************************************************************************
*/

#ifndef __XLAT8000_H
#define __XLAT8000_H

#include "datatype.h"
#include "xlatgen.h"
#include "win_mem.h"

class Emu8000TransApp: public EmuAbstractTranslatorApplicatorClass
{
  public:

    Emu8000TransApp(void);
    ~Emu8000TransApp(void);

    void *operator new(size_t size) {return NewAlloc(size);}
    void operator delete(void *ptr) {DeleteAlloc(ptr);}

	static EmuAbstractTranslatorApplicatorClass *Alloc()
     {return new Emu8000TransApp;}

    void   BuildTables(void);
    void   BlastTables(void);

  private:

    void BuildDestIndexTable(void);

    static LONG TransNoTranslation(LONG amount);
    static LONG TransForceZero(LONG);
    static LONG TransHzToCents(LONG);
    static LONG TransTimeCentsToDelay(LONG amount);
    static LONG TransTimeCentsToEnvDecayRelease(LONG amount);
    static LONG TransCentsMaxExcurToPitchMod(LONG amount);
    static LONG TransCentsToFilterCutoff(LONG amount);
    static LONG TransCentsToLFOFreq(LONG amount);
    static LONG TransTimeCentsToEnvAttack(LONG amount);
    static LONG TransTimeCentsToEnvHold(LONG amount);
    static LONG TransP1PercentToEnvSustain(LONG amount);
    static LONG TransCentsMaxExcurToLFOFiltMod(LONG amount);
    static LONG TransCbToFilterQ(LONG amount);
    static LONG TransCentsMaxExcurToEnvFiltMod(LONG amount);
    static LONG TransCbMaxExcurToLFOVolMod(LONG amount);
    static LONG TransP1PercentToEffectSend(LONG amount);
    static LONG TransP1PercentToPan(LONG amount);
    static LONG TransCbToInitialAtten(LONG amount);
    static LONG TransCbToLFOVolMod(LONG amount);
    static LONG TransCentsToInitialPitch(LONG amount);

    static LONG ApplyTransAdd(LONG amount, LONG applyTo, translator xlat);
    static LONG ApplySimpleAdd(LONG, LONG, translator);

    static LONG RemveNoTranslation(LONG amount);
    static LONG RemveForceZero(LONG);
    static LONG RemveTimeCentsToDelay(LONG amount);
    static LONG RemveTimeCentsToEnvDecayRelease(LONG amount);
    static LONG RemveCentsMaxExcurToPitchMod(LONG amount);
    static LONG RemveCentsToFilterCutoff(LONG amount);
    static LONG RemveCentsToLFOFreq(LONG amount);
    static LONG RemveTimeCentsToEnvAttack(LONG amount);
    static LONG RemveTimeCentsToEnvHold(LONG amount);
    static LONG RemveP1PercentToEnvSustain(LONG amount);
    static LONG RemveCentsMaxExcurToLFOFiltMod(LONG amount);
    static LONG RemveCbToFilterQ(LONG amount);
    static LONG RemveCentsMaxExcurToEnvFiltMod(LONG amount);
    static LONG RemveCbMaxExcurToLFOVolMod(LONG amount);
    static LONG RemveP1PercentToEffectSend(LONG amount);
    static LONG RemveP1PercentToPan(LONG amount);
    static LONG RemveP1PercentToNPan(LONG amount);
    static LONG RemveCbToInitialAtten(LONG amount);
    static LONG RemveCentsToInitialPitch(LONG amount);

    static SHORT RangeMinMax(SHORT value, SHORT min, SHORT max);
	static WORD  RangeWordMinMax(WORD value, WORD min, WORD max);
    static LONG  RangeLongMinMax(LONG value, LONG min, LONG max);
};
#endif
