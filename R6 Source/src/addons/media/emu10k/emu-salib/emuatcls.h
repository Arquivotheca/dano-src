//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: emuatcls.h
//
// Author: Mike Guzewicz
//
// Description:
//    Attribute class to be used in modules. 
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  -----------------------------------------
// MikeGuz             Jul 25, 1996  Initial creation.
//
//*****************************************************************************

#ifndef __EMUATCLS_H
#define __EMUATCLS_H

#include "emuattr.h"
#include "omega.h"

class EMUEXPORT EmuModuleAttributes : public OmegaClass
{

  public:

  EmuModuleAttributes(storeAttributes *tAttrib)
                       : OmegaClass(tAttrib->moduleName)
  { attrib = &tAttrib->attrib; }
    
  ~EmuModuleAttributes(void) {;}

  CHAR       * GetModuleAttributes(attributes *a)
	       {a->revision = *GetModuleRevision();
		a->propString = GetModulePropString();
		GetModuleAuthors(a->authors);
		return GetModuleName();}
  atRevision * GetModuleRevision(void) {return &attrib->revision;}
  CHAR       * GetModulePropString(void) {return attrib->propString;}
  void         GetModuleAuthors(CHAR *a[])
	       {a[0] = attrib->authors[0];
		a[1] = attrib->authors[1];
		a[2] = attrib->authors[2];
		a[3] = attrib->authors[3];
		a[4] = attrib->authors[4];}

  private:

  attributes *attrib;
};

#endif
