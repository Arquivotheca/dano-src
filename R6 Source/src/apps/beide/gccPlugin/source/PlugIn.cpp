// ---------------------------------------------------------------------------
/*
	PlugIn.cpp
	
	Needed plugin interfaces to create the gcc preferences and compiler/linker
	plugins.
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			18 September 1998

*/
// ---------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>

#include "PlugInPreferences.h"
#include "PlugIn.h"
#include "CommandLineTextView.h"
#include "MProjectPrefsViewx86.h"
#include "GCCBuilder.h"
#include "GCCLinker.h"
#include "LanguageOptionsView.h"
#include "LinkerOptionsView.h"
#include "WarningOptionsView.h"
#include "CodeGenerationOptionsView.h"
#include "GCCBuilderHandler.h"


extern "C" {
_EXPORT status_t MakeAddOnView(int32 inIndex, BRect inRect, MPlugInPrefsView*& outView);
_EXPORT status_t MakeAddOnBuilder(int32 inIndex, MPlugInBuilder*& outBuilder);
_EXPORT status_t MakeAddOnLinker(MPlugInLinker*& outLinker);
}

// ---------------------------------------------------------------------------
// Messages
// ---------------------------------------------------------------------------

// These message types are written to disk, and need to be recognized from
// both PPC and x86 machines.
// Given an x86 base for these messages (and the fact that they weren't
// swapped to begin with), I swap the PPC versions.


inline ulong
CrossPlatformMessageType(ulong arg)
{
#if __POWERPC__
	return B_SWAP_INT32(arg);
#else
	return arg;
#endif
}

const ulong kCompilerMessageType = CrossPlatformMessageType('gccc');
const ulong kLinkerMessageType = CrossPlatformMessageType('gcld');

// ---------------------------------------------------------------------------
// Preference titles
// ---------------------------------------------------------------------------

const char* kLanguageOptionsTitle = "Language/C/C++ Language";
const char* kLinkerOptionsTitle = "Linker/x86 Linker";
const char* kCodeGenerationOptionsTitle = "Code Generation/x86 Code Generation";
const char* kCommonWarningOptionsTitle = "Language/Common Warnings";
const char* kWarningOptionsTitle = "Language/C/C++ Warnings";

// ---------------------------------------------------------------------------
// Additional options
// gcc has around 500 options - this provides an escape so that users can
// enter options here without resorting to makefiles
// ---------------------------------------------------------------------------

const char* kAdditionalCompilerOptionTitle = "Other/More Compiler Options";
const char* kAdditionalLinkerOptionTitle = "Other/More Linker Options";

const char* kAdditionalCompilerOptionsCaption = "Supplemental C/C++ Options";
const char* kAdditionalLinkerOptionsCaption = "Supplemental Linker Options";

const char* kAdditionalCompilerOptionsDefaults = "";
const char* kAdditionalLinkerOptionsDefaults = "";
                   
// ---------------------------------------------------------------------------
// Tool names
// ---------------------------------------------------------------------------

const char* kGCCCompilerName = "gcc";
const char* kGCCLinkerName = "gcc_link";
const char* kGCCArchiveBuilderName = "ar";

const char* kArchiveMimeType = "application/x-vnd.Be.ar-archive";

// ---------------------------------------------------------------------------
// Message names
// ---------------------------------------------------------------------------

const char* kLanguageOptionsMessageName = "gccLanguage";
const char* kLinkerOptionsMessageName = "gccLinker";
const char* kCodeGenerationOptionsMessageName = "gccCodeGeneration";
const char* kCommonWarningOptionsMessageName = "gccCommonWarning";
const char* kWarningOptionsMessageName = "gccWarning";

const char* kAdditionalCompilerOptionsMessageName = "AdditionalGCCCompilerOptions";
const char* kAdditionalLinkerOptionsMessageName = "AdditionalGCCLinkerOptions";

// ---------------------------------------------------------------------------
// Target names
// ---------------------------------------------------------------------------

const char* kTargetName = "BeOS x86 ELF C/C++";

// ---------------------------------------------------------------------------
// Static creation functions for builder/linker so we can communicate
// the builders to the CommandLineTextViews
// (and so we don't depend on order of calls between MakeAddOnBuilder/Linker/View)
// ---------------------------------------------------------------------------

GCCBuilder* gCompiler = NULL;
GCCLinker* gLinker = NULL;

static GCCBuilder* GetGCCBuilder()
{
	if (gCompiler == NULL) {
		gCompiler = new GCCBuilder;
	}

	return gCompiler;
}

// ---------------------------------------------------------------------------

static GCCLinker* GetGCCLinker()
{
	if (gLinker == NULL) {
		gLinker = new GCCLinker;
	}
	return gLinker;
}

// ---------------------------------------------------------------------------
// MakeAddOnView
// ---------------------------------------------------------------------------

status_t
MakeAddOnView(int32 inIndex, BRect inRect, MPlugInPrefsView*& outView)
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "MakeAddOnView\n");
#endif

	long result = B_NO_ERROR;

	switch (inIndex)
	{
		case 0:
			outView = new MProjectPrefsViewx86(inRect);
			break;
		case 1:
			outView = new LanguageOptionsView(inRect,
											  kLanguageOptionsTitle,
											  kLanguageOptionsMessageName,
											  kCompilerMessageType,
											  kCompileUpdate);
			break;
		case 2:
			outView = new LinkerOptionsView(inRect,
											kLinkerOptionsTitle,
											kLinkerOptionsMessageName,
											kLinkerMessageType,
											kLinkUpdate);
			break;
		case 3:
			outView = new CommonWarningOptionsView(inRect,
												   kCommonWarningOptionsTitle,
												   kCommonWarningOptionsMessageName,
												   kCompilerMessageType,
												   kCompileUpdate);
			break;
		case 4:
			outView = new WarningOptionsView(inRect,
											 kWarningOptionsTitle,
											 kWarningOptionsMessageName,
											 kCompilerMessageType,
											 kCompileUpdate);
			break;
		case 5:
			outView = new CodeGenerationOptionsView(inRect,
													kCodeGenerationOptionsTitle,
													kCodeGenerationOptionsMessageName,
													kCompilerMessageType,
													kCompileUpdate);
			break;
		case 6:
			outView = new CommandLineTextView(inRect,
											  kAdditionalCompilerOptionTitle,
											  kAdditionalCompilerOptionsCaption,
											  kAdditionalCompilerOptionsMessageName,
											  kCompilerMessageType, 
											  kCompileUpdate,
											  kAdditionalCompilerOptionsDefaults,
											  new GCCBuilderHandler<GCCBuilder>(GetGCCBuilder()));
			break;
		case 7:
			outView = new CommandLineTextView(inRect,
											  kAdditionalLinkerOptionTitle,
											  kAdditionalLinkerOptionsCaption,
											  kAdditionalLinkerOptionsMessageName,
											  kLinkerMessageType, 
											  kLinkUpdate,
											  kAdditionalLinkerOptionsDefaults,
											  new GCCBuilderHandler<GCCLinker>(GetGCCLinker()));
			break;
		default:
			result = B_ERROR;
	}

	return result;	
}

// ---------------------------------------------------------------------------
// MakeAddOnBuilder
// ---------------------------------------------------------------------------

status_t
MakeAddOnBuilder(int32 inIndex, MPlugInBuilder*& outBuilder)
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "MakeAddOnBuilder\n");
#endif

	long result = B_OK;

	switch (inIndex)
	{
		case 0:
			outBuilder = GetGCCBuilder();
			break;

		default:
			result = B_ERROR;
	}

#ifdef _GCCDEBUG_
	fprintf(stderr, "MakeAddOnBuilder - returning %d\n", result);
#endif

	return result;	
}

// ---------------------------------------------------------------------------
// MakeAddOnLinker
// ---------------------------------------------------------------------------

status_t
MakeAddOnLinker(MPlugInLinker*& outLinker)
{
#ifdef _GCCDEBUG_
	fprintf(stderr, "MakeAddOnLinker\n");
#endif

	outLinker = GetGCCLinker();
	return B_OK;	
}
