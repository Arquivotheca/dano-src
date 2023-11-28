//*****************************************************************************
//
//                             Copyright (c) 1996
//                E-mu Systems Proprietary All rights Reserved
//
//*****************************************************************************

//*****************************************************************************
//
// Filename: emuattr.h
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

#ifndef __EMUATTR_H
#define __EMUATTR_H

#include "datatype.h"

typedef struct atRevisionTag
{
  CHAR phase;  // A=alpha, B=beta, V=version, etc
  BYTE major;  // Major release number (incompatible with previous)
  BYTE minor;  // Minor release number (compatible with same 'major', 
               //                       with extensions and/or revisions)
  BYTE revnum; // Revision number of Major.Minor
} atRevision;

#define MAX_AUTHORS 5

typedef struct attributesTag
{
  atRevision revision;
  CHAR *propString;
  CHAR *authors[MAX_AUTHORS];
} attributes;

typedef struct storeAttributesTag
{
  CHAR *moduleName;
  attributes attrib;
}storeAttributes;

#endif
