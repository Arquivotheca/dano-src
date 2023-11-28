
/******************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1996. All rights Reserved.
*
*******************************************************************************
*/

/******************************************************************************
*
* Filename: GenXlat.h
*
* Authors: Mike Guzewicz
*
* Description:  Generic SoundFont to hardware translation and application
*
* History:
*
* Version    Person        Date         Reason
* -------    ---------   -----------  ---------------------------------------
*  0.001       MG        Mar 13, 96   File Created
*
******************************************************************************
*/

#ifndef __GENXLAT_H
#define __GENXLAT_H

#include "datatype.h"
#include "omega.h"

class EmuAbstractTranslatorApplicatorClass : public OmegaClass
{
  public:

    typedef LONG (*translator)(LONG sfValue);
    typedef LONG (*applicator)(LONG sfValue, LONG translatedValue, translator xlat);
    typedef LONG (*precremove)(LONG  translatedValue);

    translator *transDestIndex;
    applicator *applyDestIndex;
    precremove *remveDestIndex;

	EmuAbstractTranslatorApplicatorClass(CHAR *moduleName) : OmegaClass(moduleName) {;}

    virtual ~EmuAbstractTranslatorApplicatorClass() {}
    virtual void BuildTables(void) = 0;
    virtual void BlastTables(void) = 0;
};

typedef EmuAbstractTranslatorApplicatorClass
        *(*EmuAbstractTranslatorApplicatorAlloc)();

#endif
