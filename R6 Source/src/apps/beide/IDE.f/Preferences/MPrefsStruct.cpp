//========================================================================
//	MPrefsStruct.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Byte-swapping functions for the prefs structs

#include <stdio.h>

#include "MPrefsStruct.h"
#include "MTargetTypes.h"
#include "MFileSet.h"

#include <byteorder.h>
#include <Debug.h>

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
EditorPrefs::SwapBigToHost()
{
	pVersion = B_BENDIAN_TO_HOST_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
EditorPrefs::SwapHostToBig()
{
	pVersion = B_HOST_TO_BENDIAN_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
FontPrefs::SwapBigToHost()
{
	pVersion = B_BENDIAN_TO_HOST_INT32(pVersion);
	pFontSize = B_BENDIAN_TO_HOST_FLOAT(pFontSize);
	pTabSize = B_BENDIAN_TO_HOST_INT32(pTabSize);
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
FontPrefs::SwapHostToBig()
{
	pVersion = B_HOST_TO_BENDIAN_INT32(pVersion);
	pFontSize = B_HOST_TO_BENDIAN_FLOAT(pFontSize);
	pTabSize = B_HOST_TO_BENDIAN_INT32(pTabSize);
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
AppEditorPrefs::SwapBigToHost()
{
	pVersion = B_BENDIAN_TO_HOST_INT32(pVersion);
	pFlashingDelay = B_BENDIAN_TO_HOST_INT32(pFlashingDelay);
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
AppEditorPrefs::SwapHostToBig()
{
	pVersion = B_HOST_TO_BENDIAN_INT32(pVersion);
	pFlashingDelay = B_HOST_TO_BENDIAN_INT32(pFlashingDelay);
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
ProjectPrefs::SwapBigToHost()
{
	pVersion = B_BENDIAN_TO_HOST_INT32(pVersion);
	pProjectKind = (ProjectT) B_BENDIAN_TO_HOST_INT32(pProjectKind);
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
ProjectPrefs::SwapHostToBig()
{
	pVersion = B_HOST_TO_BENDIAN_INT32(pVersion);
	pProjectKind = (ProjectT) B_BENDIAN_TO_HOST_INT32(pProjectKind);
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
BuildExtrasPrefs::SwapBigToHost()
{
	version = B_BENDIAN_TO_HOST_INT32(version);
	concurrentCompiles = B_BENDIAN_TO_HOST_INT32(concurrentCompiles);
	compilePriority = B_BENDIAN_TO_HOST_INT32(compilePriority);
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
BuildExtrasPrefs::SwapHostToBig()
{
	version = B_HOST_TO_BENDIAN_INT32(version);
	concurrentCompiles = B_HOST_TO_BENDIAN_INT32(concurrentCompiles);
	compilePriority = B_HOST_TO_BENDIAN_INT32(compilePriority);
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
PrivateProjectPrefs::SwapBigToHost()
{
	version = B_BENDIAN_TO_HOST_INT32(version);
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
PrivateProjectPrefs::SwapHostToBig()
{
	version = B_HOST_TO_BENDIAN_INT32(version);
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
LanguagePrefs::SwapBigToHost()
{
	pVersion = B_BENDIAN_TO_HOST_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
LanguagePrefs::SwapHostToBig()
{
	pVersion = B_HOST_TO_BENDIAN_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
LanguagePrefs::SwapLittleToHost()
{
	pVersion = B_LENDIAN_TO_HOST_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
LanguagePrefs::SwapHostToLittle()
{
	pVersion = B_HOST_TO_LENDIAN_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
LinkerPrefs::SwapBigToHost()
{
	pVersion = B_BENDIAN_TO_HOST_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
LinkerPrefs::SwapHostToBig()
{
	pVersion = B_HOST_TO_BENDIAN_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
AccessPathsPrefs::SwapBigToHost()
{
	pVersion = B_BENDIAN_TO_HOST_INT32(pVersion);
	pSystemPaths = B_BENDIAN_TO_HOST_INT32(pSystemPaths);
	pProjectPaths = B_BENDIAN_TO_HOST_INT32(pProjectPaths);
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
AccessPathsPrefs::SwapHostToBig()
{
	pVersion = B_HOST_TO_BENDIAN_INT32(pVersion);
	pSystemPaths = B_HOST_TO_BENDIAN_INT32(pSystemPaths);
	pProjectPaths = B_HOST_TO_BENDIAN_INT32(pProjectPaths);
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
WarningsPrefs::SwapBigToHost()
{
	pVersion = B_BENDIAN_TO_HOST_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
WarningsPrefs::SwapHostToBig()
{
	pVersion = B_HOST_TO_BENDIAN_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
WarningsPrefs::SwapLittleToHost()
{
	pVersion = B_LENDIAN_TO_HOST_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
WarningsPrefs::SwapHostToLittle()
{
	pVersion = B_HOST_TO_LENDIAN_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
ProcessorPrefs::SwapBigToHost()
{
	pVersion = B_BENDIAN_TO_HOST_INT32(pVersion);
	pStructAlignment = B_BENDIAN_TO_HOST_INT32(pStructAlignment);
	pOptimizationLevel = B_BENDIAN_TO_HOST_INT32(pOptimizationLevel);
	pInstructionScheduling = (SchedulingT) B_BENDIAN_TO_HOST_INT32(pInstructionScheduling);
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
ProcessorPrefs::SwapHostToBig()
{
	pVersion = B_HOST_TO_BENDIAN_INT32(pVersion);
	pStructAlignment = B_HOST_TO_BENDIAN_INT32(pStructAlignment);
	pOptimizationLevel = B_HOST_TO_BENDIAN_INT32(pOptimizationLevel);
	pInstructionScheduling = (SchedulingT) B_HOST_TO_BENDIAN_INT32(pInstructionScheduling);
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
PEFPrefs::SwapBigToHost()
{
	pVersion = B_BENDIAN_TO_HOST_INT32(pVersion);
	pExportSymbols = (ExportSymbolsT) B_BENDIAN_TO_HOST_INT32(pExportSymbols);
//	...following currently unused...
//	pOldDef = B_BENDIAN_TO_HOST_INT32(pOldDef);
//	pOldImplementation = B_BENDIAN_TO_HOST_INT32(pOldImplementation);
//	pCurrentVersion = B_BENDIAN_TO_HOST_INT32(pCurrentVersion);
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
PEFPrefs::SwapHostToBig()
{
	pVersion = B_HOST_TO_BENDIAN_INT32(pVersion);
	pExportSymbols = (ExportSymbolsT) B_HOST_TO_BENDIAN_INT32(pExportSymbols);
//	... following currently unused ...
//	pOldDef = B_HOST_TO_BENDIAN_INT32(pOldDef);
//	pOldImplementation = B_HOST_TO_BENDIAN_INT32(pOldImplementation);
//	pCurrentVersion = B_HOST_TO_BENDIAN_INT32(pCurrentVersion);
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
DisassemblePrefs::SwapBigToHost()
{
	pVersion = B_BENDIAN_TO_HOST_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
DisassemblePrefs::SwapHostToBig()
{
	pVersion = B_HOST_TO_BENDIAN_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
FontInfo::SwapBigToHost()
{
	size = B_BENDIAN_TO_HOST_FLOAT(size);
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
FontInfo::SwapHostToBig()
{
	size = B_HOST_TO_BENDIAN_FLOAT(size);
}

// ---------------------------------------------------------------------------
//		PrintToStream
// ---------------------------------------------------------------------------

void
FontInfo::PrintToStream() const
{
	printf("FontInfo: size = %f, family = %s, style = %s\n", size, family, style);
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
SyntaxStylePrefs::SwapBigToHost()
{
	pVersion = B_BENDIAN_TO_HOST_INT32(pVersion);
	text.SwapBigToHost();
	comments.SwapBigToHost();
	keywords.SwapBigToHost();
	strings.SwapBigToHost();
	customkeywords.SwapBigToHost();
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
SyntaxStylePrefs::SwapHostToBig()
{
	pVersion = B_HOST_TO_BENDIAN_INT32(pVersion);
	text.SwapHostToBig();
	comments.SwapHostToBig();
	keywords.SwapHostToBig();
	strings.SwapHostToBig();
	customkeywords.SwapHostToBig();
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
OpenSelectionPrefs::SwapBigToHost()
{
	version = B_BENDIAN_TO_HOST_INT32(version);
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
OpenSelectionPrefs::SwapHostToBig()
{
	version = B_HOST_TO_BENDIAN_INT32(version);
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
TargetPrefs::SwapBigToHost()
{
	ASSERT(sizeof(TargetRec) == 140);
	if (B_HOST_IS_LENDIAN)
	{
		// swap count first so we know how many recs there are
		pVersion = B_BENDIAN_TO_HOST_INT32(pVersion);
		pCount = B_BENDIAN_TO_HOST_INT32(pCount);

		// Swap all the target recs
		int32		count = pCount;
		TargetRec*	recs = pTargetArray;
		for (int32 i = 0; i < count; i++)
		{
			recs[i].Flags = B_BENDIAN_TO_HOST_INT16(recs[i].Flags);
		}
	}
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
TargetPrefs::SwapHostToBig()
{
	if (B_HOST_IS_LENDIAN)
	{
		// Swap all the target recs
		int32		count = pCount;
		TargetRec*	recs = pTargetArray;
		for (int32 i = 0; i < count; i++)
		{
			recs[i].Flags = B_HOST_TO_BENDIAN_INT16(recs[i].Flags);
		}

		// swap count last
		pVersion = B_HOST_TO_BENDIAN_INT32(pVersion);
		pCount = B_HOST_TO_BENDIAN_INT32(pCount);
	}
}

// ---------------------------------------------------------------------------
//		SwapLittleToHost
// ---------------------------------------------------------------------------

void
ProjectPrefsx86::SwapLittleToHost()
{
	pVersion = B_LENDIAN_TO_HOST_INT32(pVersion);
	pBaseAddress = B_LENDIAN_TO_HOST_INT32(pBaseAddress);
}

// ---------------------------------------------------------------------------
//		SwapHostToLittle
// ---------------------------------------------------------------------------

void
ProjectPrefsx86::SwapHostToLittle()
{
	pVersion = B_HOST_TO_LENDIAN_INT32(pVersion);
	pBaseAddress = B_HOST_TO_LENDIAN_INT32(pBaseAddress);
}

// ---------------------------------------------------------------------------
//		SwapLittleToHost
// ---------------------------------------------------------------------------

void
LinkerPrefsx86::SwapLittleToHost()
{
	pVersion = B_LENDIAN_TO_HOST_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapHostToLittle
// ---------------------------------------------------------------------------

void
LinkerPrefsx86::SwapHostToLittle()
{
	pVersion = B_HOST_TO_LENDIAN_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapLittleToHost
// ---------------------------------------------------------------------------

void
CodeGenx86::SwapLittleToHost()
{
	pVersion = B_LENDIAN_TO_HOST_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapHostToLittle
// ---------------------------------------------------------------------------

void
CodeGenx86::SwapHostToLittle()
{
	pVersion = B_HOST_TO_LENDIAN_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapLittleToHost
// ---------------------------------------------------------------------------

void
GlobalOptimizations::SwapLittleToHost()
{
	pVersion = B_LENDIAN_TO_HOST_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapHostToLittle
// ---------------------------------------------------------------------------

void
GlobalOptimizations::SwapHostToLittle()
{
	pVersion = B_HOST_TO_LENDIAN_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapHostToLittle
// ---------------------------------------------------------------------------

void
GlobalOptimizations::SwapHostToBig()
{
	pVersion = B_HOST_TO_BENDIAN_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapLittleToHost
// ---------------------------------------------------------------------------

void
GlobalOptimizations::SwapBigToHost()
{
	pVersion = B_BENDIAN_TO_HOST_INT32(pVersion);
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
TargetHeader::SwapBigToHost()
{
	version = B_BENDIAN_TO_HOST_INT32(version);
	count = B_BENDIAN_TO_HOST_INT32(count);
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
TargetHeader::SwapHostToBig()
{
	version = B_HOST_TO_BENDIAN_INT32(version);
	count = B_HOST_TO_BENDIAN_INT32(count);
}

// ---------------------------------------------------------------------------
//		SwapBigToHost
// ---------------------------------------------------------------------------

void
FileSetHeader::SwapBigToHost()
{
	Version = B_BENDIAN_TO_HOST_INT32(Version);
	RecordCount = B_BENDIAN_TO_HOST_INT32(RecordCount);
	Size = B_BENDIAN_TO_HOST_INT32(Size);
}

// ---------------------------------------------------------------------------
//		SwapHostToBig
// ---------------------------------------------------------------------------

void
FileSetHeader::SwapHostToBig()
{
	Version = B_HOST_TO_BENDIAN_INT32(Version);
	RecordCount = B_HOST_TO_BENDIAN_INT32(RecordCount);
	Size = B_HOST_TO_BENDIAN_INT32(Size);
}

// ---------------------------------------------------------------------------
//	RunPreferences
// ---------------------------------------------------------------------------

RunPreferences::RunPreferences()
{
	memset(this, 0, sizeof(RunPreferences));
	Version = 0;
}


void RunPreferences::SwapBigToHost()
{
	Version = B_BENDIAN_TO_HOST_INT32(Version);
	MallocDebugLevel = B_BENDIAN_TO_HOST_INT32(MallocDebugLevel);
}
// ---------------------------------------------------------------------------

void RunPreferences::SwapHostToBig()
{
	Version = B_HOST_TO_BENDIAN_INT32(Version);
	MallocDebugLevel = B_HOST_TO_BENDIAN_INT32(MallocDebugLevel);
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------

