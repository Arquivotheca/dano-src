//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: transgen.h
//
// Author: Michael Preston
//
// Description: Base class for synthesizer translators
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  -----------------------------------------
// Michael Preston     Oct 15, 1996  Initial development.
//
//*****************************************************************************

#ifndef __TRANSGEN_H
#define __TRANSGEN_H

// Include files
#include "datatype.h"
#include "win_mem.h"
#include "stringcl.h"

// Forward declarations
class EmuAbstractTranslatorApplicatorClass;
class QFArtData;

class TranslatorBase
{
   public:

   void *operator new(size_t size) {return NewAlloc(size);}
   void operator delete(void *ptr) {DeleteAlloc(ptr);}

   TranslatorBase() {xlatapp=NULL;}
   TranslatorBase(EmuAbstractTranslatorApplicatorClass *xlatapparg)
      {xlatapp = xlatapparg;}

   virtual ~TranslatorBase() {}

   virtual void SetFileType(Str strFileExtension)=0;
   virtual void SetDefaultParameters()=0;
   virtual void SetSoundParameters(void *sndparams, BOOL bStereo)=0;
   virtual void SetParameter(WORD wIndex, LONG lValue, BOOL bAdd)=0;
   virtual void SetParameterAndTranslate(WORD wIndex, LONG lValue, BOOL bAdd)=0;
   virtual LONG GetSourceParameter(WORD wIndex)=0;
   virtual LONG GetDestParameter(WORD wIndex)=0;
   virtual WORD GetDestination(WORD wIndex)=0;
   virtual void Translate()=0;
   virtual void TranslateAndStore(QFArtData& qfad)=0;

   protected:

   EmuAbstractTranslatorApplicatorClass *xlatapp;
};

typedef TranslatorBase
        *(*TranslatorAlloc)(EmuAbstractTranslatorApplicatorClass *);

#endif
