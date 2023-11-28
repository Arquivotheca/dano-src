//========================================================================
//	MmwccBuilderx86.cpp
//	Copyright 1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <Debug.h>
#include <string.h>

#include "MmwccBuilderx86.h"
#include "PlugInPreferences.h"
#include "CString.h"
#include "IDEMessages.h"
#include "MProject.h"

const char *	mwccToolName = "mwccx86";
const char *	mwdisToolName = "mwdisx86";
const char *	mwldToolName = "mwldx86";

const unsigned char PATH_DELIM = '/';

// ---------------------------------------------------------------------------
//		¥ MmwccBuilderx86
// ---------------------------------------------------------------------------

MmwccBuilderx86::MmwccBuilderx86()
{
}

// ---------------------------------------------------------------------------
//		¥ MmwccBuilderx86
// ---------------------------------------------------------------------------

MmwccBuilderx86::~MmwccBuilderx86()
{
}

// ---------------------------------------------------------------------------
//		¥ GetToolName
// ---------------------------------------------------------------------------

status_t
MmwccBuilderx86::GetToolName(
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
//		¥ LinkerName
// ---------------------------------------------------------------------------

const char *
MmwccBuilderx86::LinkerName()
{
	return mwldToolName;
}

// ---------------------------------------------------------------------------
//		¥ Actions
// ---------------------------------------------------------------------------

MakeActionT
MmwccBuilderx86::Actions()
{
	return (kPrecompile | kCompile | kPreprocess | kCheckSyntax | kDisassemble);
}

// ---------------------------------------------------------------------------
//		¥ MessageDataType
// ---------------------------------------------------------------------------

ulong
MmwccBuilderx86::MessageDataType()
{
	return kMWCCx86Type;
//	return 'mwcx';
}

// ---------------------------------------------------------------------------
//		¥ Flags
// ---------------------------------------------------------------------------

ulong
MmwccBuilderx86::Flags()
{
	return kIDEAware;
}

// ---------------------------------------------------------------------------
//		¥ MakeStages
// ---------------------------------------------------------------------------

MakeStageT
MmwccBuilderx86::MakeStages()
{
	return (kPrecompileStage | kCompileStage);
}

// ---------------------------------------------------------------------------
//		¥ ValidateSettings
// ---------------------------------------------------------------------------

bool
MmwccBuilderx86::ValidateSettings(
	BMessage&	inOutMessage)
{
	bool		changed = false;
	long		len;

	// Project Prefs
	ProjectPrefsx86*			projectPrefs;
	if (B_NO_ERROR == inOutMessage.FindData(kProjectPrefsx86, kMWCCx86Type, &projectPrefs, &len))
	{
		if (B_LENDIAN_TO_HOST_INT32(projectPrefs->pVersion) != kCurrentVersion || len != sizeof(ProjectPrefsx86))
		{
			// Update it here
			ProjectPrefsx86		projPrefs = *projectPrefs;
			
			projPrefs.SwapLittleToHost();

			projPrefs.pVersion = kCurrentVersion;

			projPrefs.SwapHostToLittle();
			inOutMessage.RemoveName(kProjectPrefsx86);
			inOutMessage.AddData(kProjectPrefsx86, kMWCCx86Type, &projPrefs, sizeof(projPrefs));
			changed = true;
		}
	}
	else
	{
		MDefaultPrefs::SetProjectDefaultsx86(fProjectPrefs);
		fProjectPrefs.SwapHostToLittle();			
		inOutMessage.AddData(kProjectPrefsx86, kMWCCx86Type, &fProjectPrefs, sizeof(fProjectPrefs));
		fProjectPrefs.SwapLittleToHost();			
		changed = true;
	}

	// Code Gen
	CodeGenx86*			codegenPrefs;
	if (B_NO_ERROR == inOutMessage.FindData(kCodeGenPrefsx86, kMWCCx86Type, &codegenPrefs, &len))
	{
		if (B_LENDIAN_TO_HOST_INT32(codegenPrefs->pVersion) != kCurrentVersion || len != sizeof(ProjectPrefsx86))
		{
			// Update it here
			CodeGenx86		codePrefs = *codegenPrefs;
			
			codePrefs.SwapLittleToHost();

			codePrefs.pVersion = kCurrentVersion;

			codePrefs.SwapHostToLittle();
			inOutMessage.RemoveName(kCodeGenPrefsx86);
			inOutMessage.AddData(kCodeGenPrefsx86, kMWCCx86Type, &codePrefs, sizeof(codePrefs));
			changed = true;
		}
	}
	else
	{
		MDefaultPrefs::SetCodeGenx86Defaults(fCodeGenPrefs);
		fCodeGenPrefs.SwapHostToLittle();			
		inOutMessage.AddData(kCodeGenPrefsx86, kMWCCx86Type, &fCodeGenPrefs, sizeof(fCodeGenPrefs));
		fCodeGenPrefs.SwapLittleToHost();			
		changed = true;
	}

	// Global Optimizations
	GlobalOptimizations*			globaloptsPrefs;
	if (B_NO_ERROR == inOutMessage.FindData(kGlobalOptsPrefsx86, kMWCCx86Type, &globaloptsPrefs, &len))
	{
		if (B_LENDIAN_TO_HOST_INT32(globaloptsPrefs->pVersion) != kCurrentVersion || len != sizeof(GlobalOptimizations))
		{
			// Update it here
			GlobalOptimizations		globalprefs = *globaloptsPrefs;
			
			globalprefs.SwapLittleToHost();

			globalprefs.pVersion = kCurrentVersion;

			globalprefs.SwapHostToLittle();
			inOutMessage.RemoveName(kGlobalOptsPrefsx86);
			inOutMessage.AddData(kGlobalOptsPrefsx86, kMWCCx86Type, &globalprefs, sizeof(globalprefs));
			changed = true;
		}
	}
	else
	{
		MDefaultPrefs::SetGlobalOptsDefaults(fGlobalOptsPrefs);
		fGlobalOptsPrefs.SwapHostToLittle();			
		inOutMessage.AddData(kGlobalOptsPrefsx86, kMWCCx86Type, &fGlobalOptsPrefs, sizeof(fGlobalOptsPrefs));
		fGlobalOptsPrefs.SwapLittleToHost();			
		changed = true;
	}

	// Language Prefs
	LanguagePrefs*		languagePrefs;
	if (B_NO_ERROR == inOutMessage.FindData(kLangPrefsType, kCompilerType, &languagePrefs, &len))
	{
		if (B_LENDIAN_TO_HOST_INT32(languagePrefs->pVersion) < kCurrentVersion || len != sizeof(LanguagePrefs))
		{
			// Update it here
			LanguagePrefs		langPrefs = *languagePrefs;
			
			langPrefs.SwapLittleToHost();

			langPrefs.pVersion = kCurrentVersion;

			langPrefs.SwapHostToLittle();
			inOutMessage.RemoveName(kLangPrefsType);
			inOutMessage.AddData(kLangPrefsType, kCompilerType, &langPrefs, sizeof(langPrefs));
			changed = true;
		}
	}
	else
	{
		MDefaultPrefs::SetLanguageDefaults(fLanguagePrefs);
		fLanguagePrefs.SwapHostToLittle();
		inOutMessage.AddData(kLangPrefsType, kCompilerType, &fLanguagePrefs, sizeof(fLanguagePrefs));
		fLanguagePrefs.SwapLittleToHost();
		changed = true;
	}
	
	// Warnings Prefs
	WarningsPrefs*		warningsPrefs;
	if (B_NO_ERROR == inOutMessage.FindData(kWarningPrefsType, kCompilerType, &warningsPrefs, &len))
	{
		if (B_LENDIAN_TO_HOST_INT32(warningsPrefs->pVersion) < kCurrentVersion || len != sizeof(WarningsPrefs))
		{
			WarningsPrefs		warnings = *warningsPrefs;
			warnings.SwapLittleToHost();
			warnings.pVersion = kCurrentVersion;
			warnings.SwapHostToLittle();
			inOutMessage.RemoveName(kWarningPrefsType);
			inOutMessage.AddData(kWarningPrefsType, kCompilerType, &warnings, sizeof(warnings));
			changed = true;
		}
	}
	else
	{
		MDefaultPrefs::SetWarningsDefaults(fWarningsPrefs);
		fWarningsPrefs.SwapHostToLittle();
		inOutMessage.AddData(kWarningPrefsType, kCompilerType, &fWarningsPrefs, sizeof(fWarningsPrefs));
		fWarningsPrefs.SwapLittleToHost();
		changed = true;
	}

	return changed;
}

// ---------------------------------------------------------------------------
//		¥ BuildPrecompileArgv
// ---------------------------------------------------------------------------

long
MmwccBuilderx86::BuildPrecompileArgv(
	BList& 		inArgv,
	MFileRec&	inFileRec)
{
	BuildPrecompileActionArgv(inArgv, inFileRec);
	
	return B_NO_ERROR;
}

// ---------------------------------------------------------------------------
//		¥ BuildCompileArgv
// ---------------------------------------------------------------------------

status_t
MmwccBuilderx86::BuildCompileArgv(
	BList& 		inArgv,
	MakeActionT inAction,
	MFileRec&	inFileRec)
{
	ASSERT((inAction & Actions()) != 0);
	status_t		result = B_NO_ERROR;

	switch (inAction)
	{
		case kPrecompile:
			BuildPrecompileActionArgv(inArgv, inFileRec);
			break;

		case kCompile:
			BuildCompilerArgv(inArgv, inFileRec);
			break;

		case kPreprocess:
			BuildPreprocessArgv(inArgv, inFileRec);
			break;

		case kCheckSyntax:
			BuildCheckSyntaxArgv(inArgv, inFileRec);
			break;

		case kDisassemble:
			BuildDisassembleArgv(inArgv, inFileRec);
			break;
	}

	return result;
}

// ---------------------------------------------------------------------------
//		¥ MessageToPrefs
// ---------------------------------------------------------------------------
//	Copy the prefs from the BMessage to the prefs structs.

void
MmwccBuilderx86::MessageToPrefs()
{
	BMessage	msg;
	
	fProject->GetPrefs(kMWCCx86Type, msg);
	
	long	len;

	ProjectPrefsx86*	projPrefs;
	if (B_NO_ERROR == msg.FindData(kProjectPrefsx86, kMWCCx86Type, &projPrefs, &len))
	{
		fProjectPrefs = *projPrefs;
		fProjectPrefs.SwapLittleToHost();
	}
	CodeGenx86*	codegenPrefs;
	if (B_NO_ERROR == msg.FindData(kCodeGenPrefsx86, kMWCCx86Type, &codegenPrefs, &len))
	{
		fCodeGenPrefs = *codegenPrefs;
		fCodeGenPrefs.SwapLittleToHost();
	}
	GlobalOptimizations*	globaloptsPrefs;
	if (B_NO_ERROR == msg.FindData(kGlobalOptsPrefsx86, kMWCCx86Type, &globaloptsPrefs, &len))
	{
		fGlobalOptsPrefs = *globaloptsPrefs;
		fGlobalOptsPrefs.SwapLittleToHost();
	}
	LanguagePrefs*	langPrefs;
	if (B_NO_ERROR == msg.FindData(kLangPrefsType, kCompilerType, &langPrefs, &len))
	{
		fLanguagePrefs = *langPrefs;
		fLanguagePrefs.SwapLittleToHost();
	}
	WarningsPrefs*	warnPrefs;
	if (B_NO_ERROR == msg.FindData(kWarningPrefsType, kCompilerType, &warnPrefs, &len))
	{
		fWarningsPrefs = *warnPrefs;
		fWarningsPrefs.SwapLittleToHost();
	}
}

// ---------------------------------------------------------------------------
//		¥ BuildPostLinkArgv
// ---------------------------------------------------------------------------
//	mwcc doesn't work at postlink time.

status_t
MmwccBuilderx86::BuildPostLinkArgv(
	BList& 		/*inArgv*/,
	MFileRec&	/*inFileRec*/)
{
	return B_ERROR;
}

// ---------------------------------------------------------------------------
//		¥ BuildTargetFileName
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
MmwccBuilderx86::BuildTargetFileName(
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
//		¥ AppendDashOh
// ---------------------------------------------------------------------------
//	append the -o option to the end of the argv array.

void
MmwccBuilderx86::AppendDashOh(
	BList&			inArgv,
	const char *	inFileName)
{
	char		name[1024] = { 0 };
	
	strcpy(name, fObjectPath);
	
	BuildTargetFileName(inFileName, &name[strlen(name)]);

	inArgv.AddItem(strdup("-o"));
	inArgv.AddItem(strdup(name));
}

// ---------------------------------------------------------------------------
//		¥ AppendDashCee
// ---------------------------------------------------------------------------
//	append -c option to the end of the argv array.

inline void
MmwccBuilderx86::AppendDashCee(
	BList&			inArgv,
	const char*		inFilePath)
{
	inArgv.AddItem(strdup("-c"));
	inArgv.AddItem(strdup(inFilePath));
}

// ---------------------------------------------------------------------------
//		¥ BuildCompilerArgv
// ---------------------------------------------------------------------------
//	Build the argv to be used when compiling a sourcefile.

void
MmwccBuilderx86::BuildCompilerArgv(
	BList&			inArgv,
	MFileRec& 		inFileRec)
{
	// Build the Argv
	LanguageArgv(inArgv);
	MachinecodeArgv(inArgv);
	WarningsArgv(inArgv);
	ProcessorArgv(inArgv);
	AppendDashOh(inArgv, inFileRec.name);
	AppendDashCee(inArgv, inFileRec.path);
}

// ---------------------------------------------------------------------------
//		¥ BuildPreprocessArgv
// ---------------------------------------------------------------------------
//	Build the argv to be used when preprocessing a source file.

void
MmwccBuilderx86::BuildPreprocessArgv(
	BList&			inArgv,
	MFileRec& 		inFileRec)
{
	inArgv.AddItem(strdup("-E"));	// -E is preprocess

	LanguageArgv(inArgv);
	WarningsArgv(inArgv);
	ProcessorArgv(inArgv);
	AppendDashCee(inArgv, inFileRec.path);
}

// ---------------------------------------------------------------------------
//		¥ BuildPreCompileArgv
// ---------------------------------------------------------------------------

void
MmwccBuilderx86::BuildPrecompileActionArgv(
	BList&			inArgv,
	MFileRec& 		inFileRec)
{
	LanguageArgv(inArgv, false);
	WarningsArgv(inArgv);
	ProcessorArgv(inArgv);

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
//		¥ BuildCheckSyntaxArgv
// ---------------------------------------------------------------------------
//	Build the argv to be used when checking syntx for a source file.

void
MmwccBuilderx86::BuildCheckSyntaxArgv(
	BList&			inArgv,
	MFileRec& 		inFileRec)
{
	inArgv.AddItem(strdup("-nocodegen"));

	LanguageArgv(inArgv);
	WarningsArgv(inArgv);
	ProcessorArgv(inArgv);
	AppendDashCee(inArgv, inFileRec.path);
}

// ---------------------------------------------------------------------------
//		¥ BuildDisassembleArgv
// ---------------------------------------------------------------------------
//	Build the argv to be used when disassembling a source file.

void
MmwccBuilderx86::BuildDisassembleArgv(
	BList&			inArgv,
	MFileRec& 		inFileRec)
{
#if 0
	inArgv.AddItem(strdup("-machinecodelist"));
	inArgv.AddItem(strdup("-g"));		// sym info on makes the compiler include source
	LanguageArgv(inArgv);
	WarningsArgv(inArgv);
	ProcessorArgv(inArgv);
	AppendDashCee(inArgv, inFileRec.path);
	AppendDashOh(inArgv, inFileRec.name);

#else
	// Build the command line options
	// Build the path to the object file
	char			path[1024];
	
	// Build the path to a .o file
	strcpy(path, fObjectPath);

	char*		ptr = strrchr(path, '/');
	ptr++;

	BuildTargetFileName(inFileRec.name, ptr);

	inArgv.AddItem(strdup(path));
#endif
}

// ---------------------------------------------------------------------------
//		¥ LanguageArgv
// ---------------------------------------------------------------------------
//	Add the language settings to the argv to be used when compiling a sourcefile.
//	Don't use a prefix file when precompiling.

void
MmwccBuilderx86::LanguageArgv(
	BList&			inArgv,
	bool			inUsePrefixFile)
{
	// Prefix file
	if (inUsePrefixFile && fLanguagePrefs.pPrefixFile[0] != 0)
	{
		inArgv.AddItem(strdup("-prefix"));
		inArgv.AddItem(strdup(fLanguagePrefs.pPrefixFile));		// not a full path
	}
	// Language settings
	if (fLanguagePrefs.pActivateCpp)
	{
		inArgv.AddItem(strdup("-dialect"));		
		inArgv.AddItem(strdup("cplus"));		
	}
	if (! fLanguagePrefs.pEnableExceptions)
	{
		inArgv.AddItem(strdup("-Cpp_exceptions"));		
		inArgv.AddItem(strdup("off"));		
	}
	if (! fLanguagePrefs.pEnableRTTI)
	{
		inArgv.AddItem(strdup("-rtti"));		
		inArgv.AddItem(strdup("off"));		
	}
	if (fLanguagePrefs.pPoolStrings)
	{
		inArgv.AddItem(strdup("-str"));		
		inArgv.AddItem(strdup("pool"));		
	}
	if (fLanguagePrefs.pDontReuseStrings)
	{
		inArgv.AddItem(strdup("-str"));		
		inArgv.AddItem(strdup("dontreuse"));		
	}
	if (fLanguagePrefs.pRequireProtos)
	{
		inArgv.AddItem(strdup("-r"));		
	}
	if (fLanguagePrefs.pExpandTrigraphs)
	{
		inArgv.AddItem(strdup("-trigraphs"));		
		inArgv.AddItem(strdup("on"));		
	}
	if (fLanguagePrefs.pANSIStrict)				// default is strict off
	{
		inArgv.AddItem(strdup("-strict"));		
		inArgv.AddItem(strdup("on"));		
	}
	if (fLanguagePrefs.pANSIKeywordsOnly)		// default is 'ansi keywords only' off
	{
		inArgv.AddItem(strdup("-appleext"));		
		inArgv.AddItem(strdup("off"));		
	}
	if (! fLanguagePrefs.pEnumsAreInts)			// default is enums int
	{
		inArgv.AddItem(strdup("-enum"));		
		inArgv.AddItem(strdup("min"));		
	}
	if (fLanguagePrefs.pInlineKind != kNormalInline)	// default is normal
	{
		inArgv.AddItem(strdup("-inline"));		
		if (fLanguagePrefs.pInlineKind == kDontInline)
			inArgv.AddItem(strdup("off"));		
		else
			inArgv.AddItem(strdup("auto"));		// auto inline
	}
	if (! fLanguagePrefs.pEnableBool)			// default is bool on
	{
		inArgv.AddItem(strdup("-bool"));		
		inArgv.AddItem(strdup("off"));		
	}
	if (fLanguagePrefs.pUseUnsignedChars)		// default is char signed
	{
		inArgv.AddItem(strdup("-char"));		
		inArgv.AddItem(strdup("unsigned"));		
	}
	if (fLanguagePrefs.pMPWPointerRules)		// default is not relaxed pointers
	{
		inArgv.AddItem(strdup("-relax_pointers"));		
	}
}

// ---------------------------------------------------------------------------
//		¥ ProcessorArgv
// ---------------------------------------------------------------------------
//	Add the codegen and optimizer settings to the argv to be used when 
//	compiling a sourcefile.

void
MmwccBuilderx86::ProcessorArgv(
	BList&		inArgv)
{
	// Optimizations
	if (fCodeGenPrefs.pPeepholeOn ||
		fCodeGenPrefs.pUseRegisterColoring ||
		fCodeGenPrefs.pInlineIntrinsics ||
		fGlobalOptsPrefs.pOptimizeForSpace ||
		fGlobalOptsPrefs.pOptimizeForSpeed ||
		fGlobalOptsPrefs.pCommonSubExpressions ||
		fGlobalOptsPrefs.pLoopInvariants ||
		fGlobalOptsPrefs.pCopyPropagation ||
		fGlobalOptsPrefs.pDeadStoreElimination ||
		fGlobalOptsPrefs.pStrenghtReduction ||
		fGlobalOptsPrefs.pDeadCodeElimination ||
		fGlobalOptsPrefs.pLifetimeAnalysis)
	{
		String		optimize;
		if (fCodeGenPrefs.pPeepholeOn)
		{
			optimize = "peep";
		}
		if (fCodeGenPrefs.pUseRegisterColoring)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "color";
		}
		if (fCodeGenPrefs.pInlineIntrinsics)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "intrinsics";
		}
		if (fGlobalOptsPrefs.pOptimizeForSpace)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "size";
		}
		if (fGlobalOptsPrefs.pOptimizeForSpeed)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "speed";
		}
		if (fGlobalOptsPrefs.pCommonSubExpressions)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "cse";
		}
		if (fGlobalOptsPrefs.pLoopInvariants)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "loop";
		}
		if (fGlobalOptsPrefs.pCopyPropagation)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "propagation";
		}
		if (fGlobalOptsPrefs.pDeadStoreElimination)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "deadstore";
		}
		if (fGlobalOptsPrefs.pStrenghtReduction)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "strength";
		}
		if (fGlobalOptsPrefs.pDeadCodeElimination)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "deadcode";
		}
		if (fGlobalOptsPrefs.pLifetimeAnalysis)
		{
			if (optimize.GetLength() != 0)
				optimize += ",";
			optimize += "lifetimes";
		}

		inArgv.AddItem(strdup("-opt"));		
		inArgv.AddItem(strdup(optimize));		
	}
}

// ---------------------------------------------------------------------------
//		¥ WarningsArgv
// ---------------------------------------------------------------------------
//	Add the warnings settings to the argv to be used when compiling a sourcefile.

void
MmwccBuilderx86::WarningsArgv(
	BList&		inArgv)
{
	int32	warningsCount = fWarningsPrefs.pIllegalPragmas + fWarningsPrefs.pEmptyDeclarations +
		fWarningsPrefs.pPossibleErrors + fWarningsPrefs.pUnusedVariables + fWarningsPrefs.pUnusedArguments + 
		fWarningsPrefs.pExtraCommas + fWarningsPrefs.pExtendedErrChecking + fWarningsPrefs.pHiddenVirtuals +
		fWarningsPrefs.pLargeArgs;

	// Warnings
	if (fWarningsPrefs.pWarningsAreErrors)
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
			if (fWarningsPrefs.pIllegalPragmas)
			{
				warnings += "pragmas";
			}
			if (fWarningsPrefs.pEmptyDeclarations)
			{
				if (warnings.GetLength() != 0)
					warnings += ",";
				warnings += "emptydecl";
			}
			if (fWarningsPrefs.pPossibleErrors)
			{
				if (warnings.GetLength() != 0)
					warnings += ",";
				warnings += "possible";
			}
			if (fWarningsPrefs.pUnusedVariables)
			{
				if (warnings.GetLength() != 0)
					warnings += ",";
				warnings += "unusedvar";
			}
			if (fWarningsPrefs.pUnusedArguments)
			{
				if (warnings.GetLength() != 0)
					warnings += ",";
				warnings += "unusedarg";
			}
			if (fWarningsPrefs.pExtraCommas)
			{
				if (warnings.GetLength() != 0)
					warnings += ",";
				warnings += "extracomma";
			}
			if (fWarningsPrefs.pExtendedErrChecking)
			{
				if (warnings.GetLength() != 0)
					warnings += ",";
				warnings += "extended";
			}
			if (fWarningsPrefs.pHiddenVirtuals)
			{
				if (warnings.GetLength() != 0)
					warnings += ",";
				warnings += "hidevirtual";
			}
			if (fWarningsPrefs.pLargeArgs)
			{
				if (warnings.GetLength() != 0)
					warnings += ",";
				warnings += "largeargs";
			}
			
			inArgv.AddItem(strdup(warnings));	// Add the warnings string	
		}
	}
}

// ---------------------------------------------------------------------------
//		¥ MachinecodeArgv
// ---------------------------------------------------------------------------
//	Add the machine code listing if the box is checked.

void
MmwccBuilderx86::MachinecodeArgv(
	BList&			inArgv)
{
	if (fCodeGenPrefs.pShowMachineCodeListing)
	{
		inArgv.AddItem(strdup("-machinecodelist"));
		inArgv.AddItem(strdup("-g"));		// sym info on makes the compiler include source
	}
}

// ---------------------------------------------------------------------------
//		¥ FileIsDirty
// ---------------------------------------------------------------------------

bool
MmwccBuilderx86::FileIsDirty(
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
			dirty = SourceFileIsDirty(inModDate, inFileRec.name);
			break;
	}
	
	return dirty;
}

// ---------------------------------------------------------------------------
//		¥ SourceFileIsDirty
// ---------------------------------------------------------------------------

bool
MmwccBuilderx86::SourceFileIsDirty(
	time_t			inModDate,
	const char *	inFileName)
{
	bool			dirty = true;
	BEntry			objectFile;
	char			name[B_FILE_NAME_LENGTH + 1];

	BuildTargetFileName(inFileName, &name[0]);

	if (B_NO_ERROR == fObjectsDirectory.FindEntry(name, &objectFile) && objectFile.IsFile())
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
//		¥ PCHFileIsDirty
// ---------------------------------------------------------------------------

bool
MmwccBuilderx86::PCHFileIsDirty(
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
//		¥ UpdateObjectsDirectory
// ---------------------------------------------------------------------------
//	We cache the objects directory object.

void
MmwccBuilderx86::UpdateObjectsDirectory()
{
	if (B_NO_ERROR == fProject->GetProjectRef(fProjectRef))
	{
		BEntry			project(&fProjectRef);
		BEntry			projectDirEntry;
		BDirectory		projectDir;
		BEntry			objectsEntry;
		
		if (B_NO_ERROR == project.GetParent(&projectDir) &&
			B_NO_ERROR == projectDir.GetEntry(&projectDirEntry))
		{
			// It should always exist at this point but just in case, build it
			status_t	err;
			if (B_NO_ERROR == projectDir.FindEntry(kObjectDirectoryName, &objectsEntry))
			{
				err = fObjectsDirectory.SetTo(&objectsEntry);
			}
			else
			{
				status_t		err = projectDir.CreateDirectory(kObjectDirectoryName, &fObjectsDirectory);
				ASSERT(B_NO_ERROR == err);
				fObjectsDirectory.GetEntry(&objectsEntry);
			}
	
			BPath		path;

			// Cache the path to the objects directory
			if (B_NO_ERROR == objectsEntry.GetPath(&path))
			{
				fObjectPath = path.Path();
				fObjectPath += '/';			// append a slash
			}
		}
	}
}

// ---------------------------------------------------------------------------
//		¥ ParseMessageText
// ---------------------------------------------------------------------------

status_t
MmwccBuilderx86::ParseMessageText(
	const char*	/*text*/,
	BList&		/*outList*/)
{
	return B_ERROR;
}

// ---------------------------------------------------------------------------
//		¥ CodeDataSize
// ---------------------------------------------------------------------------

void
MmwccBuilderx86::CodeDataSize(
	const char* /*inFilePath*/,
	long&	outCodeSize,
	long&	outDataSize)
{
	outCodeSize = -1;
	outDataSize = -1;
}

// ---------------------------------------------------------------------------
//		¥ GenerateDependencies
// ---------------------------------------------------------------------------

status_t
MmwccBuilderx86::GenerateDependencies(
	const char*	/*inFilePath*/,
	BList&		/*outList*/)
{
	return B_ERROR;
}

// ---------------------------------------------------------------------------
//		¥ GetTargetFilePaths
// ---------------------------------------------------------------------------
//	Add the full paths of the target files for this source file to the
//	targetlist.

void
MmwccBuilderx86::GetTargetFilePaths(
	MFileRec&		inFileRec,
	BList&			inOutTargetFileList)
{
	char*		ptr = strrchr(inFileRec.name, '.');

	// ignore pch files and .h files
	if (inFileRec.makeStage != kIgnoreStage && 
		(ptr == nil || (0 != strcmp(".pch", ptr) && 0 != strcmp(".pch++", ptr))))
	{
		char		oFilePath[1024];
		
		strcpy(oFilePath, fObjectPath);

		BuildTargetFileName(inFileRec.name, &oFilePath[strlen(oFilePath)]);

		inOutTargetFileList.AddItem(strdup(oFilePath));
	}
}

// ---------------------------------------------------------------------------
//		¥ ProjectChanged
// ---------------------------------------------------------------------------

void
MmwccBuilderx86::ProjectChanged(
	ChangeT		inChange,
	MProject*	inProject)
{
	switch (inChange)
	{
		case kProjectOpened:
			fProject = inProject;
			UpdateObjectsDirectory();
			MessageToPrefs();
			break;

		case kProjectClosed:
			fProject = nil;
			break;

		case kPrefsChanged:
			MessageToPrefs();
			break;

		case kFilesAdded:
		case kFilesRemoved:
		case kFilesRearranged:
		case kBuildStarted:
			// Don't do anything for now
			break;
	}
}


