//========================================================================
//	MPrefsStruct.h
//	Copyright 1995 - 98 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS
//	note that bool is typedefed as unsigned char in DR6 , DR7,and DR8.  if this size 
//	changes or the padding for bools changes with real C++ bool we could be 
//	screwed here since the structs that contain bools might change in size.

#ifndef _MPREFSSTRUCT_H
#define _MPREFSSTRUCT_H

#include "IDEConstants.h"

#include <Font.h>

const ulong kMWDefaultPrefs = MW_FOUR_CHAR_CODE('MWPr');
const ulong kMWPrefs = MW_FOUR_CHAR_CODE('MWPr');
const ulong kPrefsType = MW_FOUR_CHAR_CODE('pref');

// BlockTypes in the project blockfile
const ulong kPreferencesBlockType 		= MW_FOUR_CHAR_CODE('DPrf');
const ulong kGenericPrefsBlockType 		= MW_FOUR_CHAR_CODE('GPrf');
const ulong kGenericBlockType 			= MW_FOUR_CHAR_CODE('GenB');
const ulong kEditorBlockType 			= MW_FOUR_CHAR_CODE('DEdt');
const ulong kFontBlockType 				= MW_FOUR_CHAR_CODE('DFnt');
const ulong kProjectBlockType 			= MW_FOUR_CHAR_CODE('DPrj');
const ulong kProcessorBlockType 		= MW_FOUR_CHAR_CODE('DPrc');
const ulong kLanguageBlockType 			= MW_FOUR_CHAR_CODE('DLan');
const ulong kWarningsBlockType 			= MW_FOUR_CHAR_CODE('WarN');
const ulong kLinkerBlockType 			= MW_FOUR_CHAR_CODE('DLin');
const ulong kPEFBlockType 				= MW_FOUR_CHAR_CODE('DPeF');
const ulong kAccessPathsBlockType 		= MW_FOUR_CHAR_CODE('DAcc');
const ulong kSystemPathBlockType 		= MW_FOUR_CHAR_CODE('SPth');
const ulong kProjectPathBlockType 		= MW_FOUR_CHAR_CODE('PPth');
const ulong kFileSetBlockType 			= MW_FOUR_CHAR_CODE('FSet');
const ulong kTargetsBlockType 			= MW_FOUR_CHAR_CODE('Trgg');
const ulong kTargetDataBlockType 		= MW_FOUR_CHAR_CODE('TrDt');
const ulong kDisAsmBlockType 			= MW_FOUR_CHAR_CODE('AsmD');
const ulong kSyntaxStyleBlockType 		= MW_FOUR_CHAR_CODE('SynS');
const ulong kPrivateBlockType 			= MW_FOUR_CHAR_CODE('Priv');	// added for 1.4
const ulong kBuildExtraBlockType 		= MW_FOUR_CHAR_CODE('BExt');	// added for 1.4
const ulong kRunPrefsBlockType			= MW_FOUR_CHAR_CODE('RnPr');
const ulong kMWLDx86Type 				= MW_FOUR_CHAR_CODE('mwlx');
const ulong kMWCCx86Type 				= MW_FOUR_CHAR_CODE('mwcx');

const char kRunPrefs[]			= "RunPrefs";
const char kEditorPrefs[] 		= "EditorPrefs";
const char kFontPrefs[] 		= "FontPrefs";
const char kColorPrefs[] 		= "ColorPrefs";
const char kAppEditorPrefs[] 	= "AppEditorPrefs";
const char kProjectPrefs[] 		= "ProjectPrefs";
const char kLanguagePrefs[] 	= "LanguagePrefs";
const char kLinkerPrefs[]		= "LinkerPrefs";
const char kWarningsPrefs[] 	= "WarningsPrefs";
const char kProcessorPrefs[] 	= "ProcessorPrefs";
const char kPEFPrefs[] 			= "PEFPrefs";
const char kFileSet[]			= "FileSet";		// A file set block passed to and from a project
const char kTargetPrefs[]		= "Target";
const char kTargetBlockPrefs[]	= "TargetBlock";
const char kDisAsmPrefs[]		= "DisassemblerPrefs";
const char kGenericPrefs[]		= "GenericPrefs";
const char kSyntaxStylePrefs[]	= "SyntaxPrefs";
const char kPrivatePrefs[]		= "PrivatePrefs";
const char kBuildExtrasPrefs[]	= "BuildExtrasPrefs";
const char kProjectNamePrefs[] 	= "ProjectNamePrefs";
const char kGlobalOptsPrefs[] 	= "GlobalOpts";

const char kAccessPathsPrefs[] = "AccessPathsPrefs";
const char kAccessPathsData[] = "AccessPathsData";
const char kProjectPathsPrefs[] = "ProjectPathsPrefs";
const char kSystemPathsPrefs[] = "SystemPathsPrefs";

// Name of recent document cache
const char kRecentDocumentCache[] = "RecentDocuments";
// Internal names for recent document cache BMessage
const char kVersionName[] = "version";
const char kRecentProjectName[] = "projpath";
const char kRecentWindowName[] = "winpath";

const char kOpenSelectionPrefs[] = "OpenSelection";
const char kFindPrefs[] = "Find";
const char kFindFrame[] = "FindFrame";
const char kMessageWindowPrefs[] = "MessageWindow";
const char kMessageWindowFrame[] = "MessageFrame";
const char kFileSetPrefs[] = "FileSet";
const char kPrintPrefs[] = "Print";
const char kKeyBindingPrefs[] = "KBind";

const char kProjectPrefsx86[] 	= "ProjectPrefsx86";
const char kLinkerPrefsx86[] 	= "LinkerPrefsx86";
const char kCodeGenPrefsx86[] 	= "CodeGenx86";
const char kGlobalOptsPrefsx86[] = "GlobalOptsx86";
const char kLanguagePrefsx86[] 	= "Langx86";
const char kWarningsPrefsx86[] = "Warnx86";


const long kFileNameLength = B_FILE_NAME_LENGTH_DR8;
// this is 64 in DR6, DR7, DR8
// changed to 256 in DR9

const ulong kDefaultCreator 	= '????';


struct EditorPrefs
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	int32		pVersion;
	bool		pUseDocFont;
	bool		pUseProjectFont;
	bool		pUseAppFont;
	uchar		punused1;
	// the rest of the fields are obsolete as of 1.4
	// version d2 additions
	int32		pFlashingDelay;
	bool		pBalanceWhileTyping;
	// not used yet
	bool		pReadOnlyAware;
	bool		pSavaAllBeforeUpdate;
	bool		pRememberWindowPosition;
	bool		pRememberSelection;
	uchar		punused2;
	uchar		punused3;
	uchar		punused4;
	// new for r4.1
	// If pUseExternalEditor == true, but pEditorSignature is blank, then
	// use the preferred application for the file
	bool		pUseExternalEditor;
	char		pEditorSignature[B_MIME_TYPE_LENGTH];
};

struct FontPrefs
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	int32		pVersion;
	float		pFontSize;
	int32		pTabSize;
	bool		pDoAutoIndent;
	uchar		punused1;
	uchar		punused2;
	uchar		punused3;
	font_family	pFontFamily;
	font_style	pFontStyle;
};

struct AppEditorPrefs
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	int32		pVersion;
	rgb_color	pBackgroundColor;
	rgb_color	pHiliteColor;
	int32		pFlashingDelay;
	bool		pBalanceWhileTyping;
	bool		pRelaxedPopupParsing;
	bool		pSortFunctionPopup;
	bool		pUseMultiUndo;			// not used
	bool		pRememberSelection;
	bool		pRememberWindowPosition;// not used
	bool		pRememberFontSettings;	// not used
	bool		pUnused;				// not used
};

// this is obsolete; it only existed for a short while
// but was included in some betas sent to MPTP
struct ColorPrefs
{
	int32		pVersion;
	rgb_color	pBackgroundColor;
	rgb_color	pHiliteColor;
};


enum ProjectT
{
	AppType,
	SharedLibType,
	LibraryType,
	DriverType
};

const char kDefaultAppName[] = "Application";

#ifdef __GNUC__
const char kDefaultLinkerName[] = "gcc_link";
const char kDefaultPrefixName[] = "";
#elif __INTEL__
const char kDefaultLinkerName[] = "mwldx86";
const char kDefaultPrefixName[] = "BeHeadersx86";
#else
const char kDefaultLinkerName[] = "mwldppc";
const char kDefaultPrefixName[] = "BeHeaders";
#endif

struct ProjectPrefs
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	int32		pVersion;
	char		pAppName[kFileNameLength];	// this should probably be longer ????
	ulong		pCreator;					// unused
	ulong		pConcurrentCompiles;		// unused
	// new with version d2
	ulong		pFileType;					// unused
	ProjectT	pProjectKind;
	// New with 1.1
	bool		pRunsWithDebugger;
	char		padding[3];
	// New with 1.3
	char		pAppType[kFileNameLength];	// mime type
};

struct BuildExtrasPrefs
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	int32		version;
	ulong		concurrentCompiles;

	// new in post-4.5
	ulong		compilePriority;			
	bool		stopOnError;
	bool		openErrorWindow;
	uchar		padding[2];
};

struct PrivateProjectPrefs
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	int32		version;
	bool		runsWithDebugger;	
	uchar		padding[3];
};

const uchar kDTSOff = 0;

// the following options existed prior to v 2.0 of the compiler
const uchar kDontInline = 0;
const uchar kNormalInline = 1;
const uchar kAutoInline = 2;
// these options were added and normal became smart
// auto-inlining is different
const uchar kSmartInline = 1;
const uchar kInlineDepthBase = 1;
const uchar kInlineDepth1 = 2;
const uchar kInlineDepth8 = 9;

struct LanguagePrefs
{
	void		SwapBigToHost();
	void		SwapHostToBig();
	void		SwapLittleToHost();
	void		SwapHostToLittle();

	int32		pVersion;
	char		pPrefixFile[kFileNameLength];
	bool		pActivateCpp;
	bool		pARMConform;		// unused
	bool		pEnableExceptions;
	bool		pEnableRTTI;
	bool		pDontInline;
	bool		pPoolStrings;
	bool		pDontReuseStrings;
	bool		pRequireProtos;
	bool		pANSIStrict;
	bool		pANSIKeywordsOnly;
	bool		pExpandTrigraphs;
	bool		pMPWNewLines;		// unused
	bool		pMPWPointerRules;
	bool		pEnumsAreInts;
	// new for 1.1.1
	uchar		pInlineKind;
	bool		pUseUnsignedChars;
	bool		pEnableBool;
	bool		pAutoInline;
	uchar		pUnused2;
	uchar		pUnused3;
};

struct LinkerPrefs
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	int32		pVersion;
	bool		pGenerateSYMFile;
	bool		pUseFullPath;
	bool		pGenerateLinkMap;
	// New settings
	bool		pSuppressWarnings;
	bool		pDeadStrip;
	bool		pFasterLinking;
	char		pMain[64];
	char		pTerm[64];
	char		pInit[64];
};

const char kDefaultAppMainName[] = "__start";
const char kDefaultAppInitName[] = "_init_routine_";
const char kDefaultAppTermName[] = "_term_routine_";
const char kDefaultSharedLibMainName[] = "";
const char kDefaultSharedLibInitName[] = "_init_routine_";
const char kDefaultSharedLibTermName[] = "_term_routine_";
const char kDefaultLibMainName[] = "";
const char kDefaultLibInitName[] = "";
const char kDefaultLibTermName[] = "";
const int32 kOldAccessPathsPrefs = 0x0005;
const int32 kCurrentAccessPathsVersion = 0x0006;		// next project version will be 7
// create a separate access paths prefs version so can update access paths for 1.3.2
// to match changes in PR folder layout.

struct AccessPathsPrefs
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	int32		pVersion;
	bool		pSearchInProjectTreeFirst;	// Treat <> as ""
	bool		punused1;
	bool		punused2;
	bool		punused3;
	int32		pSystemPaths;		// how many in the struct
	int32		pProjectPaths;		// how many in the struct
};

struct WarningsPrefs
{
	void		SwapBigToHost();
	void		SwapHostToBig();
	void		SwapLittleToHost();
	void		SwapHostToLittle();

	int32		pVersion;
	bool		pWarningsAreErrors;
	bool		pIllegalPragmas;
	bool		pEmptyDeclarations;
	bool		pPossibleErrors;
	bool		pUnusedVariables;
	bool		pUnusedArguments;
	bool		pExtraCommas;
	bool		pExtendedErrChecking;
	// new with 1.1.1
	bool		pHiddenVirtuals;
	// new with 1.3
	bool		pLargeArgs;
	// new with 2.2 (took two unused slots so version remains the same)
	bool		pImplicitConversion;
	bool		pNotInlined;
};

const int32 kNumWarningsOptions = 11;	// doesn't include 'warnings are errors'

enum	StructAlignT
{
	kPPC,
	k68K,
	k68K4Byte
};

enum	SchedulingT
{
	kNoScheduling,
	kSchedule601,
	kSchedule603,
	kSchedule604
};

struct ProcessorPrefs
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	int32			pVersion;
	int32			pStructAlignment;
	bool			pStringsROnly;
	bool			pStaticInTOC;
	bool			pFPContractions;
	bool			pEmitProfile;
	bool			pEmitTraceback;
	uchar			pPadding[3];
	SchedulingT		pInstructionScheduling;
	bool			pOptimizeForSpeed;
	bool			pPeepholeOn;
	uchar			pPadding2[2];
	int32			pOptimizationLevel;
};

enum ExportSymbolsT
{
	kExportNone,
	kExportAll,
	kExportUsePragma,
	kExportUseFile
};

struct PEFPrefs
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	int32			pVersion;
	ExportSymbolsT	pExportSymbols;
	// not used yet
	int32			pOldDef;
	int32			pOldImplementation;
	int32			pCurrentVersion;
	bool			pOrderByPragma;
	bool			pShareDataSection;
	bool			pExpandUninitializedData;
	char			pFragmentName[256];
};

struct DisassemblePrefs
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	int32			pVersion;
	bool			pShowCode;
	bool			pUseExtended;
	bool			pShowSource;
	bool			pOnlyOperands;
	bool			pShowData;
	bool			pExceptionTable;
	bool			pShowSYM;
	bool			pShowNameTable;
};

// Introduced in 1.1
struct FontInfoOld
{
	float			size;
	rgb_color		color;
	char64			name;		// 64 in DR8
};

struct SyntaxStylePrefsOld
{
	int32			pVersion;
	bool			useSyntaxStyling;
	uchar			padding[3];
	FontInfoOld		text;
	FontInfoOld		comments;
	FontInfoOld		keywords;
	FontInfoOld		strings;
	FontInfoOld		customkeywords;
};

// Introduced for DR9 in 1.3
struct FontInfo
{
	void		SwapBigToHost();
	void		SwapHostToBig();
	void		PrintToStream() const;

	float			size;
	rgb_color		color;
	font_family		family;
	font_style		style;
};

struct SyntaxStylePrefs
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	int32			pVersion;
	bool			useSyntaxStyling;
	uchar			unused1;
	uchar			unused2;
	uchar			unused3;
	FontInfo		text;
	FontInfo		comments;
	FontInfo		keywords;
	FontInfo		strings;
	FontInfo		customkeywords;
};

struct OpenSelectionPrefs
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	int32			version;
	char			extension[B_FILE_NAME_LENGTH_DR8];
};

struct TargetRec;

struct TargetPrefs
{
	void		SwapBigToHost();
	void		SwapHostToBig();

	int32			pVersion;
	int32			pCount;
	TargetRec*		pTargetArray;
	char			pLinkerName[B_FILE_NAME_LENGTH_DR8];
};

struct GlobalOptimizations
{
	void			SwapHostToLittle();
	void			SwapLittleToHost();
	void			SwapHostToBig();
	void			SwapBigToHost();

	int32			pVersion;
	bool			pOptimizeForSpace;
	bool			pOptimizeForSpeed;
	bool			pCommonSubExpressions;
	bool			pLoopInvariants;
	bool			pCopyPropagation;
	bool			pDeadStoreElimination;
	bool			pStrenghtReduction;
	bool			pDeadCodeElimination;
	bool			pLifetimeAnalysis;
	uchar			pUnused[3];
};

// Pref structs for the x86 pref panels
// These structs are all stored on disk in little endian format

struct ProjectPrefsx86
{
	void			SwapHostToLittle();
	void			SwapLittleToHost();

	int32			pVersion;
	uint32			pBaseAddress;
	uchar			pProjectKind;		// a ProjectT
	char			pAppType[kFileNameLength];	// mime type
	char			pAppName[kFileNameLength];	// this should probably be longer ????

};

struct LinkerPrefsx86
{
	void		SwapLittleToHost();
	void		SwapHostToLittle();

	int32		pVersion;
	bool		pGenerateSYMFile;
	bool		pUseFullPath;
	bool		pGenerateLinkMap;
	bool		pSuppressWarnings;
	bool		pGenerateCVInfo;
	char		pMain[64];
	char		pCommand[64];
};

enum {
	align1,
	align2,
	align4,
	align8,
	align16
};

struct CodeGenx86
{
	void			SwapHostToLittle();
	void			SwapLittleToHost();

	int32			pVersion;
	uchar			pAlignmentKind;
	bool			pPeepholeOn;
	bool			pUseRegisterColoring;
	bool			pInlineIntrinsics;
	bool			pShowMachineCodeListing;
	bool			pGenerateSYM;
	bool			pGenerateCodeView;
	uchar			pUnused;
};

#define RUN_ARGS_SIZE	1024

struct RunPreferences
{
	RunPreferences();

	void		SwapBigToHost();
	void		SwapHostToBig();

	uint32			Version;
	uint32			MallocDebugLevel;
	bool			RunInTerminal;
	char			Args[RUN_ARGS_SIZE];
};

#endif

