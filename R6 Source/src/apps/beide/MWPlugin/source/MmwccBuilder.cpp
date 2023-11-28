//========================================================================
//	MmwccBuilder.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <Debug.h>
#include <string.h>

#include "MmwccBuilder.h"
#include "PlugInPreferences.h"
#include "CString.h"
#include "IDEMessages.h"
#include "MProject.h"
#include "MPlugin.h"

#include <ByteOrder.h>
#include <Message.h>
#include <String.h>
#include <Path.h>

const char *	mwccToolName = "mwccppc";
const char *	mwdisToolName = "mwdisppc";
const char *	mwldToolName = "mwldppc";

const unsigned char PATH_DELIM = '/';
// ---------------------------------------------------------------------------
// mwccSettings
// ---------------------------------------------------------------------------

mwccSettings::mwccSettings()
{
	MDefaultPrefs::SetProcessorDefaults(fProcessorPrefs);
	MDefaultPrefs::SetLanguageDefaults(fLanguagePrefs);
	MDefaultPrefs::SetWarningsDefaults(fWarningsPrefs);
	MDefaultPrefs::SetDisassemblerDefaults(fDisassemblePrefs);
	MDefaultPrefs::SetGlobalOptsDefaults(fGlobalOptsPrefs);
}

mwccSettings::~mwccSettings()
{
}

// ---------------------------------------------------------------------------
//		MmwccBuilder
// ---------------------------------------------------------------------------

MmwccBuilder::MmwccBuilder()
{
}

// ---------------------------------------------------------------------------
//		MmwccBuilder
// ---------------------------------------------------------------------------

MmwccBuilder::~MmwccBuilder()
{
}

// ---------------------------------------------------------------------------
//		GetToolName
// ---------------------------------------------------------------------------

status_t
MmwccBuilder::GetToolName(
	MProject*	/*inProject*/,
	char* 		outName,
	long		/*inBufferLength*/,
	MakeStageT	/*inStage*/,
	MakeActionT	inAction)
{
	switch (inAction)
	{
		case kDisassemble:
			strcpy(outName, mwdisToolName);
			break;

		default:
			strcpy(outName, mwccToolName);
			break;
	}
	
	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------
//		LinkerName
// ---------------------------------------------------------------------------

const char *
MmwccBuilder::LinkerName()
{
	return mwldToolName;
}

// ---------------------------------------------------------------------------
//		Actions
// ---------------------------------------------------------------------------

MakeActionT
MmwccBuilder::Actions()
{
	return (kPrecompile | kCompile | kPreprocess | kCheckSyntax | kDisassemble);
}

// ---------------------------------------------------------------------------
//		MessageDataType
// ---------------------------------------------------------------------------

ulong
MmwccBuilder::MessageDataType()
{
	return kMWCCPlugType;
}

// ---------------------------------------------------------------------------
//		Flags
// ---------------------------------------------------------------------------

ulong
MmwccBuilder::Flags()
{
	return kIDEAware;
}

// ---------------------------------------------------------------------------
//		MakeStages
// ---------------------------------------------------------------------------

MakeStageT
MmwccBuilder::MakeStages()
{
	return (kPrecompileStage | kCompileStage);
}

// ---------------------------------------------------------------------------
//		ValidateSettings
// ---------------------------------------------------------------------------

bool
MmwccBuilder::ValidateSettings(
	BMessage&	inOutMessage)
{
	bool		changed = false;
	long		len;
	int32		version;

	// Processor Prefs
	ProcessorPrefs*			processorPrefs;
	if (B_NO_ERROR == inOutMessage.FindData(kProcessorPrefs, kMWCCPlugType, (const void**) &processorPrefs, &len))
	{
		if (B_BENDIAN_TO_HOST_INT32(processorPrefs->pVersion) != kCurrentVersion || len != sizeof(ProcessorPrefs))
		{
			ProcessorPrefs	processor = *processorPrefs;
			
			processor.SwapBigToHost();
			processor.pVersion = kCurrentVersion;
			inOutMessage.RemoveName(kProcessorPrefs);
			processor.SwapHostToBig();
			inOutMessage.AddData(kProcessorPrefs, kMWCCPlugType, &processor, sizeof(processor));
			changed = true;
		}
	}
	else
	{
		ProcessorPrefs defaultProcessorPrefs;
		MDefaultPrefs::SetProcessorDefaults(defaultProcessorPrefs);
		defaultProcessorPrefs.SwapHostToBig();
		inOutMessage.AddData(kProcessorPrefs, kMWCCPlugType, &defaultProcessorPrefs, sizeof(ProcessorPrefs));
		changed = true;
	}

	// Language Prefs
	LanguagePrefs*		languagePrefs;
	if (B_NO_ERROR == inOutMessage.FindData(kLanguagePrefs, kMWCCPlugType, (const void**) &languagePrefs, &len))
	{
		version = B_BENDIAN_TO_HOST_INT32(languagePrefs->pVersion);
		if (version < kCurrentVersion || len != sizeof(LanguagePrefs))
		{
			// Update it here
			LanguagePrefs		langPrefs = *languagePrefs;
			
			if (version < kDR9Version)
			{
				langPrefs.SwapBigToHost();
				if (langPrefs.pDontInline)
					langPrefs.pInlineKind = kDontInline;
				else
					langPrefs.pInlineKind = kSmartInline;
				
				langPrefs.pAutoInline = false;
				langPrefs.pUseUnsignedChars = false;
				langPrefs.pEnableBool = true;
				langPrefs.pEnableRTTI = true;
				langPrefs.pEnableExceptions = true;
			}
			else	// version is DR9 version
			{
				if (langPrefs.pInlineKind == kAutoInline)
				{
					langPrefs.pInlineKind = kSmartInline;
					langPrefs.pAutoInline = true;
				}
			}
			
			langPrefs.pVersion = kCurrentVersion;

			inOutMessage.RemoveName(kLanguagePrefs);
			langPrefs.SwapHostToBig();
			inOutMessage.AddData(kLanguagePrefs, kMWCCPlugType, &langPrefs, sizeof(langPrefs));
			changed = true;
		}
	}
	else
	{
		LanguagePrefs defaultLanguagePrefs;
		MDefaultPrefs::SetLanguageDefaults(defaultLanguagePrefs);
		defaultLanguagePrefs.SwapHostToBig();
		inOutMessage.AddData(kLanguagePrefs, kMWCCPlugType, &defaultLanguagePrefs, sizeof(LanguagePrefs));
		changed = true;
	}
	
	// Warnings Prefs
	WarningsPrefs*		warningsPrefs;
	if (B_NO_ERROR == inOutMessage.FindData(kWarningsPrefs, kMWCCPlugType, (const void**) &warningsPrefs, &len))
	{
		version = B_BENDIAN_TO_HOST_INT32(warningsPrefs->pVersion);
		if (version < kCurrentVersion || len != sizeof(WarningsPrefs))
		{
			WarningsPrefs		warnings = *warningsPrefs;

			warnings.SwapBigToHost();

			if (warnings.pVersion < kDR8lateVersion)
				warnings.pHiddenVirtuals = false;

			if (warnings.pVersion < kDR9Version)
			{
				warnings.pLargeArgs = false;
				warnings.pImplicitConversion = false;
				warnings.pNotInlined = false;
			}

			warnings.pVersion = kCurrentVersion;
			warnings.SwapHostToBig();
			inOutMessage.RemoveName(kWarningsPrefs);
			inOutMessage.AddData(kWarningsPrefs, kMWCCPlugType, &warnings, sizeof(warnings));
			changed = true;
		}
	}
	else
	{
		WarningsPrefs defaultWarningsPrefs;
		MDefaultPrefs::SetWarningsDefaults(defaultWarningsPrefs);
		defaultWarningsPrefs.SwapHostToBig();
		inOutMessage.AddData(kWarningsPrefs, kMWCCPlugType, &defaultWarningsPrefs, sizeof(WarningsPrefs));
		changed = true;
	}

	// Disassembler Prefs
	DisassemblePrefs*		disassemblePrefs;
	if (B_NO_ERROR == inOutMessage.FindData(kDisAsmPrefs, kMWCCPlugType, (const void**) &disassemblePrefs, &len))
	{
		if (B_BENDIAN_TO_HOST_INT32(disassemblePrefs->pVersion) != kCurrentVersion || len != sizeof(DisassemblePrefs))
		{
			DisassemblePrefs	disassemble = *disassemblePrefs;
			
			disassemble.SwapBigToHost();
			disassemble.pVersion = kCurrentVersion;
			disassemble.SwapHostToBig();
			inOutMessage.RemoveName(kDisAsmPrefs);
			inOutMessage.AddData(kDisAsmPrefs, kMWCCPlugType, &disassemble, sizeof(disassemble));
			changed = true;
		}
	}
	else
	{
		DisassemblePrefs defaultDisassemblePrefs;
		MDefaultPrefs::SetDisassemblerDefaults(defaultDisassemblePrefs);
		defaultDisassemblePrefs.SwapHostToBig();
		inOutMessage.AddData(kDisAsmPrefs, kMWCCPlugType, &defaultDisassemblePrefs, sizeof(DisassemblePrefs));
		changed = true;
	}
	
	// Global Optimizations
	GlobalOptimizations*			globaloptsPrefs;
	if (B_NO_ERROR == inOutMessage.FindData(kGlobalOptsType, kMWCCPlugType, (const void**) &globaloptsPrefs, &len))
	{
		if (B_BENDIAN_TO_HOST_INT32(globaloptsPrefs->pVersion) != kCurrentVersion || len != sizeof(GlobalOptimizations))
		{
			// Update it here
			GlobalOptimizations		globalprefs = *globaloptsPrefs;
			
			globalprefs.SwapBigToHost();

			globalprefs.pVersion = kCurrentVersion;

			globalprefs.SwapHostToBig();
			inOutMessage.RemoveName(kGlobalOptsType);
			inOutMessage.AddData(kGlobalOptsType, kMWCCPlugType, &globalprefs, sizeof(globalprefs));
			changed = true;
		}
	}
	else
	{
		GlobalOptimizations defaultGlobalOptsPrefs;
		MDefaultPrefs::SetGlobalOptsDefaults(defaultGlobalOptsPrefs);
		defaultGlobalOptsPrefs.SwapHostToBig();			
		inOutMessage.AddData(kGlobalOptsType, kMWCCPlugType, &defaultGlobalOptsPrefs, sizeof(GlobalOptimizations));
		changed = true;
	}
	
	return changed;
}

// ---------------------------------------------------------------------------
//		BuildPrecompileArgv
// ---------------------------------------------------------------------------

long
MmwccBuilder::BuildPrecompileArgv(
	MProject&	inProject,
	BList& 		inArgv,
	MFileRec&	inFileRec)
{
	BuildPrecompileActionArgv(inProject, inArgv, inFileRec);
	
	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------
//		BuildCompileArgv
// ---------------------------------------------------------------------------

long
MmwccBuilder::BuildCompileArgv(
	MProject&	inProject,
	BList& 		inArgv,
	MakeActionT inAction,
	MFileRec&	inFileRec)
{
	ASSERT((inAction & Actions()) != 0);
	long		result = B_NO_ERROR;

	switch (inAction)
	{
		case kPrecompile:
			BuildPrecompileActionArgv(inProject, inArgv, inFileRec);
			break;

		case kCompile:
			BuildCompilerArgv(inProject, inArgv, inFileRec);
			break;

		case kPreprocess:
			BuildPreprocessArgv(inProject, inArgv, inFileRec);
			break;

		case kCheckSyntax:
			BuildCheckSyntaxArgv(inProject, inArgv, inFileRec);
			break;

		case kDisassemble:
			BuildDisassembleArgv(inProject, inArgv, inFileRec);
			break;
	}

	return result;
}

// ---------------------------------------------------------------------------
//		MessageToPrefs
// ---------------------------------------------------------------------------
//	Copy the prefs from the BMessage to the prefs structs.

void
MmwccBuilder::MessageToPrefs(MProject& inProject)
{
	BMessage	msg;
	
	inProject.GetPrefs(kMWCCPlugType, msg);
	mwccSettings* cache = fSettingsMap.GetSettings(inProject);
	
	long	len;

	LanguagePrefs*	langPrefs;
	if (B_NO_ERROR == msg.FindData(kLanguagePrefs, kMWCCPlugType, (const void**) &langPrefs, &len))
	{
		cache->fLanguagePrefs = *langPrefs;
		cache->fLanguagePrefs.SwapBigToHost();
	}
	ProcessorPrefs*	processorPrefs;
	if (B_NO_ERROR == msg.FindData(kProcessorPrefs, kMWCCPlugType, (const void**) &processorPrefs, &len))
	{
		cache->fProcessorPrefs = *processorPrefs;
		cache->fProcessorPrefs.SwapBigToHost();
	}
	WarningsPrefs*	warnPrefs;
	if (B_NO_ERROR == msg.FindData(kWarningsPrefs, kMWCCPlugType, (const void**) &warnPrefs, &len))
	{
		cache->fWarningsPrefs = *warnPrefs;
		cache->fWarningsPrefs.SwapBigToHost();
	}
	DisassemblePrefs*	disassemblePrefs;
	if (B_NO_ERROR == msg.FindData(kDisAsmPrefs, kMWCCPlugType, (const void**) &disassemblePrefs, &len))
	{
		cache->fDisassemblePrefs = *disassemblePrefs;
		cache->fDisassemblePrefs.SwapBigToHost();
	}
	GlobalOptimizations*	globaloptsPrefs;
	if (B_NO_ERROR == msg.FindData(kGlobalOptsType, kMWCCPlugType, (const void**) &globaloptsPrefs, &len))
	{
		cache->fGlobalOptsPrefs = *globaloptsPrefs;
		cache->fGlobalOptsPrefs.SwapBigToHost();
	}
}

// ---------------------------------------------------------------------------
//		BuildPostLinkArgv
// ---------------------------------------------------------------------------
//	mwcc doesn't work at postlink time.

long
MmwccBuilder::BuildPostLinkArgv(
	MProject&	/*inProject*/,
	BList& 		/*inArgv*/,
	MFileRec&	/*inFileRec*/)
{
	return B_ERROR;
}

// ---------------------------------------------------------------------------
//		BuildTargetFileName
// ---------------------------------------------------------------------------
//	Generate a unique target file name.  Since there can be multiple files
//	in the project with the same name the .o file name needs to be unique.
//	Even though there may be multiple files with the same name in the 
//	project they will all have different 'unique names' where the unique name
//	includes a relative path to the file.  The target file name is the 
//	uniqe name with path delimiters changed to periods.  So
//	folder1/folder2/file1.cpp
//    becomes
//	folder1.folder2.file1.o

void
MmwccBuilder::BuildTargetFileName(
	const char*		inName,
	char*			outName) const
{
	char *			periodPtr = nil;

	do {
		switch (*inName)
		{
			case PATH_DELIM:
				*outName++ = '.';		// translate '/' -> '.'
				inName++;
				break;

			case '.':
				periodPtr = outName;	// remember where period is
				// fall through

			default:
				*outName++ = *inName++;	// copy byte
				break;
		}
	}	while (*inName != '\0');

	if (periodPtr == nil)
	{
		*outName = '.';
		periodPtr = outName;
	}

	*++periodPtr = 'o';
	*++periodPtr = '\0';
}

// ---------------------------------------------------------------------------
//		AppendDashOh
// ---------------------------------------------------------------------------
//	append the -o option to the end of the argv array.

void
MmwccBuilder::AppendDashOh(
	String			objectPath,
	BList&			inArgv,
	const char *	inFileName)
{
	char		name[1000] = { 0 };
	
	strcpy(name, objectPath);
	
	BuildTargetFileName(inFileName, &name[strlen(name)]);

	inArgv.AddItem(strdup("-o"));
	inArgv.AddItem(strdup(name));
}

// ---------------------------------------------------------------------------
//		AppendDashCee
// ---------------------------------------------------------------------------
//	append -c option to the end of the argv array.

inline void
MmwccBuilder::AppendDashCee(
	BList&			inArgv,
	const char*		inFilePath)
{
	inArgv.AddItem(strdup("-c"));
	inArgv.AddItem(strdup(inFilePath));
}

// ---------------------------------------------------------------------------
//		BuildCompilerArgv
// ---------------------------------------------------------------------------
//	Build the argv to be used when compiling a sourcefile.

void
MmwccBuilder::BuildCompilerArgv(
	MProject&		inProject,
	BList&			inArgv,
	MFileRec& 		inFileRec)
{
	// Build the Argv
	inArgv.AddItem(strdup("-g"));		// sym info on

	mwccSettings* cache = fSettingsMap.GetSettings(inProject);

	LanguageArgv(cache->fLanguagePrefs, inArgv);
	WarningsArgv(cache->fWarningsPrefs, inArgv);
	ProcessorArgv(cache->fProcessorPrefs, cache->fGlobalOptsPrefs, inArgv);
	AppendDashOh(cache->fObjectPath, inArgv, inFileRec.name);
	AppendDashCee(inArgv, inFileRec.path);
}

// ---------------------------------------------------------------------------
//		BuildPreprocessArgv
// ---------------------------------------------------------------------------
//	Build the argv to be used when preprocessing a source file.

void
MmwccBuilder::BuildPreprocessArgv(
	MProject&		inProject,
	BList&			inArgv,
	MFileRec& 		inFileRec)
{
	inArgv.AddItem(strdup("-E"));	// -E is preprocess

	mwccSettings* cache = fSettingsMap.GetSettings(inProject);

	LanguageArgv(cache->fLanguagePrefs, inArgv);
	WarningsArgv(cache->fWarningsPrefs, inArgv);
	ProcessorArgv(cache->fProcessorPrefs, cache->fGlobalOptsPrefs, inArgv);
	AppendDashCee(inArgv, inFileRec.path);
}

// ---------------------------------------------------------------------------
//		BuildPreCompileArgv
// ---------------------------------------------------------------------------

void
MmwccBuilder::BuildPrecompileActionArgv(
	MProject&		inProject,
	BList&			inArgv,
	MFileRec& 		inFileRec)
{
	mwccSettings* cache = fSettingsMap.GetSettings(inProject);

	LanguageArgv(cache->fLanguagePrefs, inArgv, false);
	WarningsArgv(cache->fWarningsPrefs, inArgv);
	ProcessorArgv(cache->fProcessorPrefs, cache->fGlobalOptsPrefs, inArgv);

	inArgv.AddItem(strdup(inFileRec.path));
	inArgv.AddItem(strdup("-precompile"));
	
	// Munge the output file name 'Headers.pch' => 'Headers'
	char*		namePtr = strdup(inFileRec.path);
	char*		sptr = strrchr(namePtr, '.');	// Find the last '.' and shorten the file name

	if (sptr)
		*sptr = 0;

	inArgv.AddItem(namePtr);
}

// ---------------------------------------------------------------------------
//		BuildCheckSyntaxArgv
// ---------------------------------------------------------------------------
//	Build the argv to be used when preprocessing a source file.

void
MmwccBuilder::BuildCheckSyntaxArgv(
	MProject&		inProject,
	BList&			inArgv,
	MFileRec& 		inFileRec)
{
	inArgv.AddItem(strdup("-nocodegen"));

	mwccSettings* cache = fSettingsMap.GetSettings(inProject);

	LanguageArgv(cache->fLanguagePrefs, inArgv);
	WarningsArgv(cache->fWarningsPrefs, inArgv);
	ProcessorArgv(cache->fProcessorPrefs, cache->fGlobalOptsPrefs, inArgv);
	AppendDashCee(inArgv, inFileRec.path);
}

// ---------------------------------------------------------------------------
//		BuildDisassembleArgv
// ---------------------------------------------------------------------------
//	Build the argv to be used when disassembling a source file.

void
MmwccBuilder::BuildDisassembleArgv(
	MProject&		inProject,
	BList&			inArgv,
	MFileRec& 		inFileRec)
{
	// Build the command line options
	mwccSettings* cache = fSettingsMap.GetSettings(inProject);
	DisassemblePrefs& disassemblePrefs = cache->fDisassemblePrefs;
	if (! disassemblePrefs.pShowCode)			// default is show code
		inArgv.AddItem(strdup("-d"));
	else
	{
		if (! disassemblePrefs.pUseExtended)	// default is use extended
		{
			inArgv.AddItem(strdup("-fmt"));
			inArgv.AddItem(strdup("nox"));
		}
		if (disassemblePrefs.pShowSource)		// default is don't mix
			inArgv.AddItem(strdup("-mix"));
		if (disassemblePrefs.pOnlyOperands)	// default is off
			inArgv.AddItem(strdup("-h"));
	}
	if (! disassemblePrefs.pShowData)			// default is show data
	{
		inArgv.AddItem(strdup("-nodata"));
	}
	else
	if (! disassemblePrefs.pExceptionTable)	// default is on
	{
		inArgv.AddItem(strdup("-xtables"));
		inArgv.AddItem(strdup("off"));
	}
	if (disassemblePrefs.pShowSYM)				// default is off
	{
		inArgv.AddItem(strdup("-sym"));
		inArgv.AddItem(strdup("on"));
	}
	if (! disassemblePrefs.pShowNameTable)		// default is on
	{
		inArgv.AddItem(strdup("-nonames"));
	}

	// Build the path to the object file
	char			path[1024];
	
	// Build the path to a .o file
	strcpy(path, cache->fObjectPath);

	char*		ptr = strrchr(path, '/');
	ptr++;

	BuildTargetFileName(inFileRec.name, ptr);

	inArgv.AddItem(strdup(path));
}

// ---------------------------------------------------------------------------
//		LanguageArgv
// ---------------------------------------------------------------------------
//	Add the language settings to the argv to be used when compiling a sourcefile.
//	Don't use a prefix file when precompiling.

void
MmwccBuilder::LanguageArgv(
	const LanguagePrefs& languagePrefs,
	BList&			inArgv,
	bool			inUsePrefixFile)
{
	// Prefix file
	if (inUsePrefixFile && languagePrefs.pPrefixFile[0] != 0)
	{
		inArgv.AddItem(strdup("-prefix"));
		inArgv.AddItem(strdup(languagePrefs.pPrefixFile));		// not a full path
	}
	// Language settings
	if (languagePrefs.pActivateCpp)
	{
		inArgv.AddItem(strdup("-dialect"));		
		inArgv.AddItem(strdup("cplus"));		
	}
	if (! languagePrefs.pEnableExceptions)
	{
		inArgv.AddItem(strdup("-Cpp_exceptions"));		
		inArgv.AddItem(strdup("off"));		
	}
	if (! languagePrefs.pEnableRTTI)
	{
		inArgv.AddItem(strdup("-rtti"));		
		inArgv.AddItem(strdup("off"));		
	}
	if (languagePrefs.pPoolStrings)
	{
		inArgv.AddItem(strdup("-str"));		
		inArgv.AddItem(strdup("pool"));		
	}
	if (languagePrefs.pDontReuseStrings)
	{
		inArgv.AddItem(strdup("-str"));		
		inArgv.AddItem(strdup("dontreuse"));		
	}
	if (languagePrefs.pRequireProtos)
	{
		inArgv.AddItem(strdup("-r"));		
	}
	if (languagePrefs.pExpandTrigraphs)
	{
		inArgv.AddItem(strdup("-trigraphs"));		
		inArgv.AddItem(strdup("on"));		
	}
	if (languagePrefs.pANSIStrict)				// default is strict off
	{
		inArgv.AddItem(strdup("-strict"));		
		inArgv.AddItem(strdup("on"));		
	}
	if (languagePrefs.pANSIKeywordsOnly)		// default is 'ansi keywords only' off
	{
		inArgv.AddItem(strdup("-appleext"));		
		inArgv.AddItem(strdup("off"));		
	}
	if (! languagePrefs.pEnumsAreInts)			// default is enums int
	{
		inArgv.AddItem(strdup("-enum"));		
		inArgv.AddItem(strdup("min"));		
	}
	if (languagePrefs.pInlineKind != kSmartInline)	// default is normal
	{
		inArgv.AddItem(strdup("-inline"));		
		if (languagePrefs.pInlineKind == kDontInline)
			inArgv.AddItem(strdup("off"));		
		else
		{
			char		depth[2] = { 0, 0 };
			depth[0] = '0' + languagePrefs.pInlineKind - kInlineDepthBase;
			inArgv.AddItem(strdup(depth));		// inline depth
		}
	}
	if (languagePrefs.pAutoInline)				// default is auto off
	{
		inArgv.AddItem(strdup("-inline"));		
		inArgv.AddItem(strdup("auto"));		
	}
	if (! languagePrefs.pEnableBool)			// default is bool on
	{
		inArgv.AddItem(strdup("-bool"));		
		inArgv.AddItem(strdup("off"));		
	}
	if (languagePrefs.pUseUnsignedChars)		// default is char signed
	{
		inArgv.AddItem(strdup("-char"));		
		inArgv.AddItem(strdup("unsigned"));		
	}
	if (languagePrefs.pMPWPointerRules)		// default is not relaxed pointers
	{
		inArgv.AddItem(strdup("-relax_pointers"));		
	}
}

// ---------------------------------------------------------------------------
//		ProcessorArgv
// ---------------------------------------------------------------------------
//	Add the processor settings to the argv to be used when compiling a sourcefile.

void
MmwccBuilder::ProcessorArgv(
	const ProcessorPrefs&		processorPrefs,
	const GlobalOptimizations&	globalOptPrefs,
	BList&						inArgv)
{
	// Processor Settings
	if (processorPrefs.pStringsROnly)
		inArgv.AddItem(strdup("-rostr"));
	if (! processorPrefs.pStaticInTOC)
	{
		inArgv.AddItem(strdup("-toc_data"));
		inArgv.AddItem(strdup("off"));		
	}
	if (! processorPrefs.pFPContractions)
	{
		inArgv.AddItem(strdup("-fp_contract"));
		inArgv.AddItem(strdup("off"));		
	}
	if (processorPrefs.pEmitProfile)
	{
		inArgv.AddItem(strdup("-profile"));
		inArgv.AddItem(strdup("on"));		
	}
	if (processorPrefs.pEmitTraceback)
	{
		inArgv.AddItem(strdup("-tb"));	// -traceback -> -tb on for 2.2
		inArgv.AddItem(strdup("on"));		
	}

	// Optimizations
	inArgv.AddItem(strdup("-opt"));		

	if (processorPrefs.pInstructionScheduling != kNoScheduling ||
		! processorPrefs.pOptimizeForSpeed ||
		processorPrefs.pPeepholeOn ||
		processorPrefs.pOptimizationLevel != 0 ||
		globalOptPrefs.pOptimizeForSpace ||
		globalOptPrefs.pOptimizeForSpeed ||
		globalOptPrefs.pCommonSubExpressions ||
		globalOptPrefs.pLoopInvariants ||
		globalOptPrefs.pCopyPropagation ||
		globalOptPrefs.pDeadStoreElimination ||
		globalOptPrefs.pStrenghtReduction ||
		globalOptPrefs.pDeadCodeElimination ||
		globalOptPrefs.pLifetimeAnalysis)
	{
		String		optimize;
		if (processorPrefs.pInstructionScheduling != kNoScheduling)
		{
			optimize += "schedule";
		}
		if (! processorPrefs.pOptimizeForSpeed)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "space";		// size -> space for 2.2
		}
		if (processorPrefs.pPeepholeOn)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "peep";
		}
		if (processorPrefs.pOptimizationLevel != 0)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			switch (processorPrefs.pOptimizationLevel)
			{
				case 1:
					optimize += "level=1";
					break;
				case 2:
					optimize += "level=2";
					break;
				case 3:
					optimize += "level=3";
					break;
				case 4:
					optimize += "level=4";
					break;
			}
		}

		// global IR optimizations
		if (globalOptPrefs.pOptimizeForSpace)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "space";
		}
		if (globalOptPrefs.pOptimizeForSpeed)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "speed";
		}
		if (globalOptPrefs.pCommonSubExpressions)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "cse";
		}
		if (globalOptPrefs.pLoopInvariants)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "loop";
		}
		if (globalOptPrefs.pCopyPropagation)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "propagation";
		}
		if (globalOptPrefs.pDeadStoreElimination)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "deadstore";
		}
		if (globalOptPrefs.pStrenghtReduction)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "strength";
		}
		if (globalOptPrefs.pDeadCodeElimination)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "deadcode";
		}
		if (globalOptPrefs.pLifetimeAnalysis)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "lifetimes";
		}

		inArgv.AddItem(strdup(optimize));		
	}
	else {
		inArgv.AddItem(strdup("off"));		// default is -opt on
	}
	
	// -opt schedule601 -> -opt schedule -proc 601 for 2.2
	// we need to go through the scheduling preferences again and
	// set up the -processor option
	if (processorPrefs.pInstructionScheduling != kNoScheduling) {
		inArgv.AddItem(strdup("-proc"));
		switch (processorPrefs.pInstructionScheduling)
			{
				case kSchedule601:
					inArgv.AddItem(strdup("601"));
					break;
				case kSchedule603:
					inArgv.AddItem(strdup("603"));
					break;
				case kSchedule604:
					inArgv.AddItem(strdup("604"));
					break;
			}
	}
}

// ---------------------------------------------------------------------------
//		WarningsArgv
// ---------------------------------------------------------------------------
//	Add the warnings settings to the argv to be used when compiling a sourcefile.

void
MmwccBuilder::WarningsArgv(
	const WarningsPrefs&	warningsPrefs,
	BList&					inArgv)
{
	long	warningsCount = warningsPrefs.pIllegalPragmas + warningsPrefs.pEmptyDeclarations +
		warningsPrefs.pPossibleErrors + warningsPrefs.pUnusedVariables + warningsPrefs.pUnusedArguments + 
		warningsPrefs.pExtraCommas + warningsPrefs.pExtendedErrChecking + warningsPrefs.pHiddenVirtuals +
		warningsPrefs.pLargeArgs + warningsPrefs.pImplicitConversion + warningsPrefs.pNotInlined;

	// Warnings
	if (warningsPrefs.pWarningsAreErrors)
	{
		inArgv.AddItem(strdup("-w"));
		inArgv.AddItem(strdup("iserr"));
	}

	if (warningsCount != 0)
	{
		if (warningsCount == kNumWarningsOptions)
		{
			inArgv.AddItem(strdup("-w2"));	// All Warnings on	
		}
		else
		{
			inArgv.AddItem(strdup("-w"));

			String		warnings;
			if (warningsPrefs.pIllegalPragmas)
			{
				warnings += "pragmas";
			}
			if (warningsPrefs.pEmptyDeclarations)
			{
				if (warnings.GetLength() != 0)
					warnings += ",";
				warnings += "emptydecl";
			}
			if (warningsPrefs.pPossibleErrors)
			{
				if (warnings.GetLength() != 0)
					warnings += ",";
				warnings += "possible";
			}
			if (warningsPrefs.pUnusedVariables)
			{
				if (warnings.GetLength() != 0)
					warnings += ",";
				warnings += "unusedvar";
			}
			if (warningsPrefs.pUnusedArguments)
			{
				if (warnings.GetLength() != 0)
					warnings += ",";
				warnings += "unusedarg";
			}
			if (warningsPrefs.pExtraCommas)
			{
				if (warnings.GetLength() != 0)
					warnings += ",";
				warnings += "extracomma";
			}
			if (warningsPrefs.pExtendedErrChecking)
			{
				if (warnings.GetLength() != 0)
					warnings += ",";
				warnings += "extended";
			}
			if (warningsPrefs.pHiddenVirtuals)
			{
				if (warnings.GetLength() != 0)
					warnings += ",";
				warnings += "hidevirtual";
			}
			if (warningsPrefs.pLargeArgs)
			{
				if (warnings.GetLength() != 0)
					warnings += ",";
				warnings += "largeargs";
			}
			if (warningsPrefs.pImplicitConversion)
			{
				if (warnings.GetLength() != 0)
					warnings += ",";
				warnings += "implicit";
			}
			if (warningsPrefs.pNotInlined)
			{
				if (warnings.GetLength() != 0)
					warnings += ",";
				warnings += "notinlined";
			}
			inArgv.AddItem(strdup(warnings));	// Add the warnings string	
		}
	}
}

// ---------------------------------------------------------------------------
//		FileIsDirty
// ---------------------------------------------------------------------------

bool
MmwccBuilder::FileIsDirty(
	MProject&		inProject,
	MFileRec&		inFileRec,	
	MakeStageT		inStage,
	MakeActionT		/*inAction*/,
	time_t			inModDate)
{
	bool		dirty = false;

	switch (inStage)
	{
		case kPrecompileStage:
			dirty = PCHFileIsDirty(inModDate, inFileRec.path);
			break;

		case kCompileStage:
			dirty = SourceFileIsDirty(inProject, inModDate, inFileRec.name);
			break;
	}
	
	return dirty;
}

// ---------------------------------------------------------------------------
//		SourceFileIsDirty
// ---------------------------------------------------------------------------

bool
MmwccBuilder::SourceFileIsDirty(
	MProject&		inProject,
	time_t			inModDate,
	const char *	inFileName)
{
	bool			dirty = true;
	BEntry			objectFile;
	char			name[B_FILE_NAME_LENGTH + 1];

	BuildTargetFileName(inFileName, &name[0]);
	mwccSettings* cache = fSettingsMap.GetSettings(inProject);
	BDirectory directory(&cache->fObjectsDirectory);
	if (directory.InitCheck() != B_OK) {
		this->UpdateObjectsDirectory(inProject);
		directory.SetTo(&cache->fObjectsDirectory);
	}
	
	if (B_NO_ERROR == directory.FindEntry(name, &objectFile) && objectFile.IsFile())
	{
		time_t		modDate;

		if (B_NO_ERROR == objectFile.GetModificationTime(&modDate) &&
			inModDate <= modDate)
		{
			dirty = false;
		}
	}
	
	return dirty;
}

// ---------------------------------------------------------------------------
//		PCHFileIsDirty
// ---------------------------------------------------------------------------

bool
MmwccBuilder::PCHFileIsDirty(
	time_t			inModDate,
	const char *	inFilePath)
{
	bool			dirty = true;
	entry_ref		ref;
	char			name[1000];
	
	strcpy(name, inFilePath);
	
	char *		p = strrchr(name, '.');
	if (p != nil)
		*p = 0;
	
	if (B_NO_ERROR == get_ref_for_path(name, &ref))
	{
		BEntry			pchFile(&ref);
		time_t			modDate;

		if (B_NO_ERROR == pchFile.GetModificationTime(&modDate) &&
			inModDate < modDate)
			dirty = false;
	}

	return dirty;
}

// ---------------------------------------------------------------------------
//		UpdateObjectsDirectory
// ---------------------------------------------------------------------------
//	We cache the objects directory object.

void
MmwccBuilder::UpdateObjectsDirectory(MProject& inProject)
{
	mwccSettings* cache = fSettingsMap.GetSettings(inProject);

	if (B_NO_ERROR == inProject.GetProjectRef(cache->fProjectRef))
	{
		BEntry			project(&cache->fProjectRef);
		BEntry			projectDirEntry;
		BDirectory		projectDir;
		BEntry			objectsEntry;
		BDirectory		objectsDirectory;
				
		// create the full name of the objects directory
		// something like "(Objects.myProject)"
	
		BString dirName(kObjectDirectoryName);
		entry_ref ref;
		char projectName[B_PATH_NAME_LENGTH];
		inProject.GetProjectRef(ref);
		strcpy(projectName, ref.name);
		char* extension = strrchr(projectName, '.');
		if (extension) {
			*extension = NIL;
		}	
		dirName.Insert(".", dirName.Length()-1);
		dirName.Insert(projectName, dirName.Length()-1);

		if (B_NO_ERROR == project.GetParent(&projectDir) &&
			B_NO_ERROR == projectDir.GetEntry(&projectDirEntry))
		{
			// It should always exist at this point but just in case, build it
			// If it doesn't exist, see if the old style exists and rename it
			status_t	err;
			if (B_NO_ERROR == projectDir.FindEntry(dirName.String(), &objectsEntry))
			{
				objectsEntry.GetRef(&cache->fObjectsDirectory);
			}
			else if (projectDir.FindEntry(kObjectDirectoryName, &objectsEntry) == B_OK)
			{
				objectsEntry.Rename(dirName.String());
				objectsEntry.GetRef(&cache->fObjectsDirectory);
			}
			else
			{
				status_t err = projectDir.CreateDirectory(dirName.String(), &objectsDirectory);
				ASSERT(B_NO_ERROR == err);
				objectsDirectory.GetEntry(&objectsEntry);
				objectsEntry.GetRef(&cache->fObjectsDirectory);
			}
	
			BPath		path;

			// Cache the path to the objects directory
			if (B_NO_ERROR == objectsEntry.GetPath(&path))
			{
				cache->fObjectPath = path.Path();
				cache->fObjectPath += '/';			// append a slash
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		ParseMessageText
// ---------------------------------------------------------------------------

long
MmwccBuilder::ParseMessageText(
	MProject&	/*inProject*/,
	const char*	/*text*/,
	BList&		/*outList*/)
{
	return B_ERROR;
}

// ---------------------------------------------------------------------------
//		CodeDataSize
// ---------------------------------------------------------------------------

void
MmwccBuilder::CodeDataSize(
	MProject&	/*inProject*/,
	const char* /*inFilePath*/,
	long&	outCodeSize,
	long&	outDataSize)
{
	outCodeSize = -1;
	outDataSize = -1;
}

// ---------------------------------------------------------------------------
//		GenerateDependencies
// ---------------------------------------------------------------------------

long
MmwccBuilder::GenerateDependencies(
	MProject&	/*inProject*/,
	const char*	/*inFilePath*/,
	BList&		/*outList*/)
{
	return B_ERROR;
}

// ---------------------------------------------------------------------------
//		GetTargetFilePaths
// ---------------------------------------------------------------------------
//	Add the full paths of the target files for this source file to the
//	targetlist.

void
MmwccBuilder::GetTargetFilePaths(
	MProject&		inProject,
	MFileRec&		inFileRec,
	BList&			inOutTargetFileList)
{
	char*		ptr = strrchr(inFileRec.name, '.');

	// ignore pch files and .h files
	if (inFileRec.makeStage != kIgnoreStage && 
		(ptr == nil || (0 != strcmp(".pch", ptr) && 0 != strcmp(".pch++", ptr))))
	{
		char		oFilePath[1024];
		
		mwccSettings* cache = fSettingsMap.GetSettings(inProject);
		strcpy(oFilePath, cache->fObjectPath);

		BuildTargetFileName(inFileRec.name, &oFilePath[strlen(oFilePath)]);

		inOutTargetFileList.AddItem(strdup(oFilePath));
	}
}

// ---------------------------------------------------------------------------
//		ProjectChanged
// ---------------------------------------------------------------------------

void
MmwccBuilder::ProjectChanged(
	MProject&	inProject,
	ChangeT		inChange)
{
	switch (inChange)
	{			
		case kProjectOpened:
			fSettingsMap.AddSettings(inProject);
			MessageToPrefs(inProject);
			UpdateObjectsDirectory(inProject);
			break;

		case kProjectClosed:
			fSettingsMap.RemoveSettings(inProject);
			break;

		case kPrefsChanged:
			MessageToPrefs(inProject);
			break;

		case kBuildStarted:
		case kFilesAdded:
		case kFilesRemoved:
		case kFilesRearranged:
		case kRunMenuItemChanged:
		case kLinkDone:
			// Don't do anything for now
			break;
	}
}


