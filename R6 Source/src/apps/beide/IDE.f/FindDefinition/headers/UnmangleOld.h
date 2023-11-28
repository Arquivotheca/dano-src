/************************************************************************/
/*	Project...:	C++ and ANSI-C Compiler Environment						*/
/*	Name......:	Unmangle.h												*/
/*	Purpose...: C++ name unmangler										*/
/*  Copyright.: ©Copyright 1995 by metrowerks inc. All rights reserved. */
/************************************************************************/

#include <stddef.h>

extern void MWUnmangle(const char *mangled_name,char *unmangled_name,size_t buffersize);
extern void MWUnmangleClassName(const char *mangled_name,char *unmangled_name,size_t buffersize);
