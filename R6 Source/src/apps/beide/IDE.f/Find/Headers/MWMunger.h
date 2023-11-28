/************************************************************************/
/*	Project...: C++ and ANSI-C Compiler Environment 					*/
/*	Name......: MWMunger.h												*/
/*	Purpose...: Munger functions										*/
/*	Copyright.: ©Copyright 1993 by metrowerks inc. All rights reserved. */
/************************************************************************/

#ifndef __MWMUNGER_H__
#define __MWMUNGER_H__

#include <SupportDefs.h>

extern unsigned char __CUppr[];

extern int32 MWMunger(const char pattern[], int32 patternLength, const char text[],
					 int32 textLength, int32 offset, bool forward,
					 bool wholeWord, bool caseSensitive);

#endif
