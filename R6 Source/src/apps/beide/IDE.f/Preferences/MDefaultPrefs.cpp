//========================================================================
//	MDefaultPrefs.cpp
//	Copyright 1995 - 97 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	Utility class for setting default preferences.
//	BDS

#include <string.h>

#include "MDefaultPrefs.h"
#include "MDefaultTargets.h"
#include "IDEMessages.h"
#include "Utils.h"

#include <OS.h>
#include <Mime.h>

#include <stdio.h>

// ---------------------------------------------------------------------------
//		SetEditorDefaults
// ---------------------------------------------------------------------------

void
MDefaultPrefs::SetEditorDefaults(
	EditorPrefs& 	outPrefs)
{
	outPrefs.pVersion = kCurrentVersion;
	outPrefs.pUseDocFont = true;
	outPrefs.pUseProjectFont = false;
	outPrefs.pUseAppFont = false;
	
	// version 2 additions
	outPrefs.pBalanceWhileTyping = true;
	outPrefs.pFlashingDelay = 10;

	outPrefs.pReadOnlyAware = false;
	outPrefs.pSavaAllBeforeUpdate = true;
	outPrefs.pRememberWindowPosition = true;
	outPrefs.pRememberSelection = true;
	
	// version r4.1 additions
	outPrefs.pUseExternalEditor = false;
	outPrefs.pEditorSignature[0] = 0;
}

// ---------------------------------------------------------------------------
//		SetAppEditorDefaults
// ---------------------------------------------------------------------------

void
MDefaultPrefs::SetAppEditorDefaults(
	AppEditorPrefs& 	outPrefs)
{
	memset(&outPrefs, 0, sizeof(outPrefs));
	outPrefs.pVersion = kCurrentVersion;
	outPrefs.pBackgroundColor = white;
	outPrefs.pHiliteColor = kGrey216;
	outPrefs.pFlashingDelay = 10;
	outPrefs.pBalanceWhileTyping = true;
	outPrefs.pRelaxedPopupParsing = true;
	outPrefs.pSortFunctionPopup = false;
	outPrefs.pUseMultiUndo = true;
	outPrefs.pRememberSelection = true;
	outPrefs.pRememberWindowPosition = true;
	outPrefs.pRememberFontSettings = true;
}

// ---------------------------------------------------------------------------
//		SetFontDefaults
// ---------------------------------------------------------------------------

void
MDefaultPrefs::SetFontDefaults(
	FontPrefs& 	outPrefs)
{
	memset(&outPrefs, 0, sizeof(outPrefs));
	outPrefs.pVersion = kCurrentVersion;
	GetBFontFamilyAndStyle(*be_fixed_font, outPrefs.pFontFamily, outPrefs.pFontStyle);
	outPrefs.pFontSize = be_fixed_font->Size();
	outPrefs.pTabSize = 4;
	outPrefs.pDoAutoIndent = true;
}

// ---------------------------------------------------------------------------
//		SetProjectDefaults
// ---------------------------------------------------------------------------

void
MDefaultPrefs::SetProjectDefaults(
	ProjectPrefs& 	outPrefs)
{
	memset(&outPrefs, 0, sizeof(outPrefs));
	outPrefs.pVersion = kCurrentVersion;
	strcpy(outPrefs.pAppName, kDefaultAppName);
	outPrefs.pCreator = kDefaultCreator;
	outPrefs.pConcurrentCompiles = 1;
	outPrefs.pFileType = kAppType;
	outPrefs.pProjectKind = AppType;
	outPrefs.pRunsWithDebugger = false;
	// ProjectPrefs are only used for MWPlugin/ppc
	// If we have get a new MWPlugin/x86, then this will have
	// to change based on something like a target type
	strcpy(outPrefs.pAppType, B_PEF_APP_MIME_TYPE);
}

// ---------------------------------------------------------------------------
//		SetLanguageDefaults
// ---------------------------------------------------------------------------

void
MDefaultPrefs::SetLanguageDefaults(
	LanguagePrefs& 	outPrefs)
{
	memset(&outPrefs, 0, sizeof(outPrefs));
	outPrefs.pVersion = kCurrentVersion;

	strcpy(outPrefs.pPrefixFile, kDefaultPrefixName);
	outPrefs.pActivateCpp = true;
	outPrefs.pARMConform = false;
	outPrefs.pEnableExceptions = true;
	outPrefs.pEnableRTTI = true;
	outPrefs.pDontInline = false;		// obsolete as of 1.1.1
	outPrefs.pPoolStrings = false;
	outPrefs.pDontReuseStrings = false;
	outPrefs.pRequireProtos = true;
	outPrefs.pANSIStrict = true;
	outPrefs.pANSIKeywordsOnly = false;
	outPrefs.pExpandTrigraphs = false;
	outPrefs.pMPWNewLines = false;
	outPrefs.pMPWPointerRules = false;
	outPrefs.pEnumsAreInts = true;
	// new as of 1.1.1
	outPrefs.pInlineKind = kSmartInline;
	outPrefs.pUseUnsignedChars = false;
	outPrefs.pEnableBool = true;
	// new as of R3, 1.5
	outPrefs.pAutoInline = false;	
}

// ---------------------------------------------------------------------------
//		SetAccessPathsDefaults
// ---------------------------------------------------------------------------

void
MDefaultPrefs::SetAccessPathsDefaults(
	AccessPathsPrefs& 	outPrefs)
{
	memset(&outPrefs, 0, sizeof(outPrefs));
	outPrefs.pVersion = kCurrentAccessPathsVersion;
	outPrefs.pSearchInProjectTreeFirst = false;
	// new fields
	outPrefs.pSystemPaths = 0;
	outPrefs.pProjectPaths = 0;
}

// ---------------------------------------------------------------------------
//		SetWarningsDefaults
// ---------------------------------------------------------------------------

void
MDefaultPrefs::SetWarningsDefaults(
	WarningsPrefs& 	outPrefs)
{
	outPrefs.pVersion = kCurrentVersion;
	outPrefs.pWarningsAreErrors = false;
	outPrefs.pIllegalPragmas = false;
	outPrefs.pEmptyDeclarations = false;
	outPrefs.pPossibleErrors = false;
	outPrefs.pUnusedVariables = false;
	outPrefs.pUnusedArguments = false;
	outPrefs.pExtraCommas = false;
	outPrefs.pExtendedErrChecking = false;
	outPrefs.pHiddenVirtuals = false;
	outPrefs.pLargeArgs = false;
	outPrefs.pImplicitConversion = false;
	outPrefs.pNotInlined = false;
}

// ---------------------------------------------------------------------------
//		SetProcessorDefaults
// ---------------------------------------------------------------------------

void
MDefaultPrefs::SetProcessorDefaults(
	ProcessorPrefs& 	outPrefs)
{
	outPrefs.pVersion = kCurrentVersion;
	outPrefs.pStructAlignment = kPPC;
	outPrefs.pStringsROnly = false;
	outPrefs.pStaticInTOC = true;
	outPrefs.pFPContractions = true;
	outPrefs.pEmitProfile = false;
	outPrefs.pEmitTraceback = false;
	outPrefs.pInstructionScheduling = kNoScheduling;
	outPrefs.pOptimizeForSpeed = true;
	outPrefs.pPeepholeOn = false;
	outPrefs.pOptimizationLevel = 0;
}

// ---------------------------------------------------------------------------
//		SetLinkerDefaults
// ---------------------------------------------------------------------------

void
MDefaultPrefs::SetLinkerDefaults(
	LinkerPrefs& 	outPrefs)
{
	memset(&outPrefs, 0, sizeof(outPrefs));
	outPrefs.pVersion = kCurrentVersion;
	outPrefs.pGenerateSYMFile = false;
	outPrefs.pUseFullPath = true;
	outPrefs.pGenerateLinkMap = true;
	outPrefs.pSuppressWarnings = false;
	outPrefs.pDeadStrip = true;
	outPrefs.pFasterLinking = true;
	strcpy(outPrefs.pMain, kDefaultAppMainName);
	strcpy(outPrefs.pInit, kDefaultAppInitName);
	strcpy(outPrefs.pTerm, kDefaultAppTermName);
}

// ---------------------------------------------------------------------------
//		SetPEFDefaults
// ---------------------------------------------------------------------------

void
MDefaultPrefs::SetPEFDefaults(
	PEFPrefs& 	outPrefs)
{
	memset(&outPrefs, 0, sizeof(outPrefs));
	outPrefs.pVersion = kCurrentVersion;
	outPrefs.pExportSymbols = kExportNone;
	// not used yet
	outPrefs.pOldDef = 0;
	outPrefs.pOldImplementation = 0;
	outPrefs.pCurrentVersion = 0;
	outPrefs.pOrderByPragma = false;
	outPrefs.pShareDataSection = false;
	outPrefs.pExpandUninitializedData = false;
	// outPrefs.pFragmentName is set to blank by memset
}

// ---------------------------------------------------------------------------
//		SetTargetsDefaults
// ---------------------------------------------------------------------------

void
MDefaultPrefs::SetTargetsDefaults(
	TargetPrefs& 	outPrefs)
{
	outPrefs.pVersion = kCurrentTargetVersion;
	outPrefs.pCount = kDefaultTargetCount;
	outPrefs.pTargetArray = new TargetRec[kDefaultTargetCount];
	memcpy(outPrefs.pTargetArray, sDefaultTargets, kDefaultTargetCount * sizeof(TargetRec));
	memset(outPrefs.pLinkerName, 0, sizeof(outPrefs.pLinkerName));
	strcpy(outPrefs.pLinkerName, kDefaultLinkerName);
}

// ---------------------------------------------------------------------------
//		UpdateTargets
// ---------------------------------------------------------------------------
//	Change all the mwcc entries to mwccppc.

void
MDefaultPrefs::UpdateTargets(
	TargetPrefs& 	outPrefs)
{
	int32		count = outPrefs.pCount;
	TargetRec*	recs = outPrefs.pTargetArray;
	
	for (int i = 0; i < count; i++)
	{
		if (0 == strcmp(recs[i].ToolName, "mwcc"))
			strcpy(recs[i].ToolName, "mwccppc");
		else
		if (0 == strcmp(recs[i].ToolName, "mwld"))
			strcpy(recs[i].ToolName, "mwldppc");
	}

	if (0 == strcmp(outPrefs.pLinkerName, "mwld"))
		strcpy(outPrefs.pLinkerName, "mwldppc");

	outPrefs.pVersion = kCurrentTargetVersion;
}

// ---------------------------------------------------------------------------
//		SetDisassemblerDefaults
// ---------------------------------------------------------------------------

void
MDefaultPrefs::SetDisassemblerDefaults(
	DisassemblePrefs& 	outPrefs)
{
	outPrefs.pVersion = kCurrentVersion;
	outPrefs.pShowCode = true;
	outPrefs.pUseExtended = true;
	outPrefs.pShowSource = false;
	outPrefs.pOnlyOperands = false;
	outPrefs.pShowData = true;
	outPrefs.pExceptionTable = true;
	outPrefs.pShowSYM = false;
	outPrefs.pShowNameTable = true;
}

// ---------------------------------------------------------------------------
//		SetfontInfoDefaults
// ---------------------------------------------------------------------------

void
MDefaultPrefs::SetFontInfoDefaults(
	FontInfo& 	outPrefs)
{
	memset(&outPrefs, 0, sizeof(outPrefs));
	GetBFontFamilyAndStyle(*be_fixed_font, outPrefs.family, outPrefs.style);
	outPrefs.size = be_fixed_font->Size();
}

// ---------------------------------------------------------------------------
//		SetSyntaxStylingDefaults
// ---------------------------------------------------------------------------

void
MDefaultPrefs::SetSyntaxStylingDefaults(
	SyntaxStylePrefs& 	outPrefs)
{
	memset(&outPrefs, 0, sizeof(outPrefs));
	outPrefs.pVersion = kCurrentVersion;
	outPrefs.useSyntaxStyling = true;

	SetFontInfoDefaults(outPrefs.text);
	outPrefs.text.color = black;

	SetFontInfoDefaults(outPrefs.comments);
	outPrefs.comments.color = red;

	SetFontInfoDefaults(outPrefs.keywords);
	outPrefs.keywords.color = blue;

	SetFontInfoDefaults(outPrefs.strings);
	outPrefs.strings.color = kGrey144;
}

// ---------------------------------------------------------------------------
//		SetOpenSelectionDefaults
// ---------------------------------------------------------------------------
//	The extension shown in the open selection window.

void
MDefaultPrefs::SetOpenSelectionDefaults(
	OpenSelectionPrefs& 	outPrefs)
{
	outPrefs.version = kCurrentVersion;
	memset(outPrefs.extension, 0, sizeof(outPrefs.extension));
	strcpy(outPrefs.extension, ".h");
}

// ---------------------------------------------------------------------------
//		SetPrivateDefaults
// ---------------------------------------------------------------------------

void
MDefaultPrefs::SetPrivateDefaults(
	PrivateProjectPrefs& 	outPrefs)
{
	memset(&outPrefs, 0, sizeof(outPrefs));
	outPrefs.version = kCurrentVersion;
	outPrefs.runsWithDebugger = false;
}

// ---------------------------------------------------------------------------
//		SetBuildExtraDefaults
// ---------------------------------------------------------------------------

void
MDefaultPrefs::SetBuildExtraDefaults(
	BuildExtrasPrefs& 	outPrefs)
{
	memset(&outPrefs, 0, sizeof(outPrefs));
	outPrefs.version = kCurrentVersion;
	// 0 means "Same as CPUs"
	outPrefs.concurrentCompiles = 0;
	outPrefs.compilePriority = B_NORMAL_PRIORITY;
	outPrefs.stopOnError = false;
	outPrefs.openErrorWindow = true;
}

// ---------------------------------------------------------------------------
//		SetProjectDefaultsx86
// ---------------------------------------------------------------------------

void
MDefaultPrefs::SetProjectDefaultsx86(
	ProjectPrefsx86& 	outPrefs)
{
	memset(&outPrefs, 0, sizeof(outPrefs));
	outPrefs.pVersion = kCurrentVersion;
	outPrefs.pProjectKind = AppType;
	outPrefs.pBaseAddress = 0x80000000;		// default base address for apps
	strcpy(outPrefs.pAppName, kDefaultAppName);
	strcpy(outPrefs.pAppType, "application/x-vnd.Be-peexecutable");
}

// ---------------------------------------------------------------------------
//		SetLinkerDefaultsx86
// ---------------------------------------------------------------------------

void
MDefaultPrefs::SetLinkerDefaultsx86(
	LinkerPrefsx86& 	outPrefs)
{
	memset(&outPrefs, 0, sizeof(outPrefs));
	outPrefs.pVersion = kCurrentVersion;
	outPrefs.pGenerateSYMFile = false;
	outPrefs.pUseFullPath = true;
	outPrefs.pGenerateLinkMap = true;
	outPrefs.pSuppressWarnings = false;
	outPrefs.pGenerateCVInfo = false;
	strcpy(outPrefs.pMain, kDefaultAppMainName);	// same as ppc
	// command file name is blank
}

// ---------------------------------------------------------------------------
//		SetCodeGenx86Defaults
// ---------------------------------------------------------------------------

void
MDefaultPrefs::SetCodeGenx86Defaults(
	CodeGenx86& 	outPrefs)
{
	memset(&outPrefs, 0, sizeof(outPrefs));
	outPrefs.pVersion = kCurrentVersion;
	outPrefs.pAlignmentKind = align4;
	outPrefs.pPeepholeOn = true;
	outPrefs.pShowMachineCodeListing = false;
	outPrefs.pUseRegisterColoring = false;
	outPrefs.pInlineIntrinsics = false;
	outPrefs.pGenerateSYM = true;
	outPrefs.pGenerateCodeView = false;
}

// ---------------------------------------------------------------------------
//		SetGlobalOptsDefaults
// ---------------------------------------------------------------------------

void
MDefaultPrefs::SetGlobalOptsDefaults(
	GlobalOptimizations& 	outPrefs)
{
	memset(&outPrefs, 0, sizeof(outPrefs));
	outPrefs.pVersion = kCurrentVersion;
	outPrefs.pOptimizeForSpace = false;
	outPrefs.pOptimizeForSpeed = true;
	outPrefs.pCommonSubExpressions = false;
	outPrefs.pLoopInvariants = false;
	outPrefs.pCopyPropagation = false;
	outPrefs.pDeadStoreElimination = false;
	outPrefs.pStrenghtReduction = false;
	outPrefs.pDeadStoreElimination = false;
	outPrefs.pDeadCodeElimination = false;
	outPrefs.pLifetimeAnalysis = false;
}



