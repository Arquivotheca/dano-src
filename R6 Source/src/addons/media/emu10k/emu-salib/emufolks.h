//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: emufolks.h
//
// Author: Mike Guzewicz
//
// Description:
//    List of people who worked on the code
//
// History:
//
// Person              Date          Reason
// ------------------  ------------  -----------------------------------------
// MikeGuz             Jul 25, 1996  Initial creation.
//
//*****************************************************************************

#ifndef __EMUFOLKS_H
#define __EMUFOLKS_H

#include "datatype.h"

enum enEngineers
{
  eeRobC,
  eeMikeG,
  eeDaveO,
  eeMikeP,
  eeSteveV,
  eeVinceV,
  eeMattW,
  eeJohnK,
  eeEricL,

  eeEnd
};

BEGINEMUCTYPE

extern CHAR *engineerName[];
extern CHAR copyright[];
extern CHAR sfTrademark[];

ENDEMUCTYPE

#endif
