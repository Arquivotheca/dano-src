// ---------------------------------------------------------------------------
/*
	PlugIn.h
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			18 September 1998

*/
// ---------------------------------------------------------------------------

#ifndef _PLUGIN_H
#define _PLUGIN_H

#include <SupportDefs.h>
#include <ByteOrder.h>

// ---------------------------------------------------------------------------
// defines for turning on/off debugging
// ---------------------------------------------------------------------------

// #define DEBUG 1
// #define _GCCDEBUG_ 1

// ---------------------------------------------------------------------------
// static names used throughout plugin
// ---------------------------------------------------------------------------

extern const char* kGCCCompilerName;
extern const char* kGCCLinkerName;
extern const char* kGCCArchiveBuilderName;
extern const char* kTargetName;

extern const char* kArchiveMimeType;

// ---------------------------------------------------------------------------
// message types for all gcc panels/plugins
// ---------------------------------------------------------------------------

extern const ulong kCompilerMessageType;
extern const ulong kLinkerMessageType;

// ---------------------------------------------------------------------------
// message names used throughout plugin
// ---------------------------------------------------------------------------

extern const char* kLanguageOptionsMessageName;
extern const char* kLinkerOptionsMessageName;
extern const char* kCodeGenerationOptionsMessageName;
extern const char* kCommonWarningOptionsMessageName;
extern const char* kWarningOptionsMessageName;

extern const char* kAdditionalCompilerOptionsMessageName;
extern const char* kAdditionalLinkerOptionsMessageName;

extern const char* kAdditionalCompilerOptionsDefaults;
extern const char* kAdditionalLinkerOptionsDefaults;


// NULL only works for pointers in gcc
// but I just can't bring myself to do a char = 0...
#define NIL (0)

#endif
