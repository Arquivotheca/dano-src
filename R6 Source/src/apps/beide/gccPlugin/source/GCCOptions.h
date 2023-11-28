// ---------------------------------------------------------------------------
/*
	GCCOptions.h
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			27 October 1998

	C/C++ Language options:
		-x c			Treat source as C files (regardless of file extension)
		-x c++			Treat source as C++ files (regardless of file extension)
	
		-ansi					Compile for ANSI C
		-trigraphs				Support ANSI C trigraphs
		-funsigned-char			Make 'char' be signed by default
		-funsigned-bitfields	Make bitfields be unsigned by default
	
	
	
	Linker Options:
		-Xlinker -s			Strip all symbols
		-Xlinker -x			Strip all local symbols
		
	Code Generation:
		-O0			No optimization
		-O1			Some optimization
		-O2			More optimization
		-O3			Full optimization
		-Os			Optimize for space rather than for speed

		-no-fpic	Don't generate position independent code (driver's only)
		-fno-implicit-templates	Only emit code for explicit template instantiations
		-fkeep-inline-functions	Generate code for functions even if they are fully inlined

		-g			Generate debugging information (on BeOS defaults to -gdwarf-2)
	
	Warnings:
		-w			Inhibit all warning messages
		-Werror		Treat warnings as errors
  		-pedantic   Issue all warnings demanded by strict ANSI C/C++
					(-Wno-long-long included with -pedantic)
	
		-Wshadow			Warn when one local variable shadows another
		-Wbad-function-cast	Warn about casting functions to incompatible types
		-Wcast-qual			Warn about casts which discard qualifiers
		-Wconversion 		Warn about possibly confusing type conversions
		-Winline			Warn when an inlined function cannot be inlined
		-Wextern-inline		Warn when a function is declared extern, then inline
		-Woverloaded-virtual	Warn about overloaded virtual function names
		-Wold-style-cast	Warn if a C style cast is used in a program
		-Weffc++			Warn about violations of "Effective C++" style rules
	
	-Wall 			Issue warnings
							(-Wno-multichar -Wno-ctor-dtor-privacy included with -Wall)
		-Wparentheses		Warn about possible missing parentheses
		-Wreturn-type		Warn about inconsistent return types
		-Wswitch			Warn about enumerated switches missing a specific case
		-Wunused			Warn when a variable is unused
		-Wuninitialized		Warn about unitialized automatic variables
		-Wreorder			Warn when the compiler reorders class member initializations
		-Wnon-virtual-dtor	Warn about nonvirtual destructors
		-Wunknown-pragmas	Warn about unrecognized pragmas
		-Wsign-compare		Warn about signed/unsigned comparisons
		-Wchar-subscripts	Warn about subscripts with type 'char'
		-Wformat			Warn about printf() format anomalies
		-Wtrigraphs			Warn if trigraphs are encountered

	There are other warnings that are covered by the -Wall, but they are less exciting to
	turn on individually (and they don't fit in one preference panel).
	Some that I don't control individually...
			-Wcomment
			-Wmain

	The options are driven through a table of OptionInfo.
	For each category of options, I can index into the table to get both the
	user or compiler text.  This allows me to iterate through the options in an
	(almost) generic way.

*/
// ---------------------------------------------------------------------------

#ifndef _GCCOPTIONS_H
#define _GCCOPTIONS_H

#include "MPrefsStruct.h"

#include <SupportDefs.h>
#include <ByteOrder.h>

class BList;

// ---------------------------------------------------------------------------
//	Class OptionInfo
//	Information needed for generating user and compiler text
//	fOptionString 	- the string passed to the compiler
//	fUserText		- the text displayed to user in preference panel
//	fDefaultSetting	- true if the option should be enabled by default
//	fStringKind		- dictates to AddOption how to interprete fOptionString
// ---------------------------------------------------------------------------

class OptionInfo
{
public:
	enum EOptionStringType { kSingleWord, kMultiWord };

	const char* 		fOptionString;
	const char* 		fUserText;
	bool				fDefaultSetting;
	EOptionStringType	fOptionStringKind;

	void AddOption(BList& outList);
};

// ---------------------------------------------------------------------------
//	Option enums
//	WARNING!!! If the enumerations are modified, 
//	the arrays accompaning option array must also be modified
//	WARNING!!! ORDER IS IMPORTANT!!!
// ---------------------------------------------------------------------------

enum ELanguageOption {
	kFilesAreC = 0,
	kFilesAreCPlusPlus,
	kANSI,
		kFirstLanguageCheckBox = kANSI,
	kTrigraphs,
	kUnsignedChar,
	kUnsignedBitfields,
	kMaxLanguageOptions
};

// ---------------------------------------------------------------------------

enum ELinkerOption {
	kStripSymbols = 0,
	kStripLocals,
	kMaxLinkerOptions
};

// ---------------------------------------------------------------------------

enum ECodeGenerationOption {
	kOptLevel0 = 0,
		kFirstOptLevel = kOptLevel0,
	kOptLevel1,
	kOptLevel2,
	kOptLevel3,
		kLastOptLevel = kOptLevel3,
	kOptSpace,										// WARNING! Currently, it is important to put -Os AFTER -On
		kFirstCheckBoxOption = kOptSpace,			// mark the first check box option
	kPIC,
	kExplicitTemplates,
	kKeepInlines,
	kGenerateDebugSymbols,
	kGenerateProfileCode,
		kLastCheckBoxOption = kGenerateProfileCode,	// mark the last check box option
	kMaxCodeGenerationOptions
};

// ---------------------------------------------------------------------------

enum ECommonWarningOption {
	kWarnAll = 0,
	kWarnParentheses,
		kFirstSubWarning = kWarnParentheses,	// mark the beginning of the -Wall dependents
	kWarnReturnType,
	kWarnSwitch,
	kWarnUnused,
	kWarnUninitialized,
	kWarnReorder,
	kWarnNonVirtualDtor,
	kWarnUnknownPragma,
	kWarnSignCompare,
	kWarnCharSubscript,
	kWarnFormat,
	kWarnTrigraphs,
	kMaxCommonWarningOptions
};	

// ---------------------------------------------------------------------------

enum EWarningOption {
	kInhibitWarnings = 0,
	kWarningsAreErrors,
	kWarnPedantic,
		kFirstWarningCheckBox = kWarnPedantic,
	kWarnShadow,
	kWarnFunctionCast,
	kWarnCastQual,
	kWarnConverion,
	kWarnInline,
	kWarnExternInline,
	kWarnWriteStrings,
	kWarnOverload,
	kWarnOldStyleCast,
	kWarnEffectiveCPlus,
	kMaxWarningOptions
};	

// ---------------------------------------------------------------------------
//	Utility class used by all the preferece setting classes
// ---------------------------------------------------------------------------

class GCCOptionUtil 
{
public:
	static void SetDefaults(bool settings[], OptionInfo info[], long optionCount);
	static void AddAllOptions(BList& outList, bool settings[], OptionInfo info[], long optionCount);
};

// ---------------------------------------------------------------------------
//	Preference Settings Classes
//		LanguageSettings
//		LinkerSettings
//		OptimizationSettings
//		WarningSettings
//	(Templates don't work here because of the size of the private array) 
// ---------------------------------------------------------------------------


class LanguageSettings {
public:
	bool GetOption(ELanguageOption which) const				{ return fSettings[which]; 	}
	void SetOption(ELanguageOption which, bool setTo)		{ fSettings[which] = setTo;	}
	const char* GetUserText(ELanguageOption which) const	{ return fgLanguageOptionInfo[which].fUserText; }
	void AddAllOptions(BList& outList)					{ GCCOptionUtil::AddAllOptions(outList,
																					   fSettings,
																					   fgLanguageOptionInfo, 
																					   kMaxLanguageOptions); }
	void SetDefaults()									{ GCCOptionUtil::SetDefaults(fSettings, 
																				    fgLanguageOptionInfo, 
																				    kMaxLanguageOptions);
														  fVersion = kCurrentVersion; }

	void SwapLittleToHost()								{ fVersion = B_LENDIAN_TO_HOST_INT32(fVersion); }
	void SwapHostToLittle()								{ fVersion = B_HOST_TO_LENDIAN_INT32(fVersion); }
	
public:
	int32				fVersion;
	static const int32	kCurrentVersion;

private:
	bool 				fSettings[kMaxLanguageOptions];
	static OptionInfo	fgLanguageOptionInfo[kMaxLanguageOptions];
};

// ---------------------------------------------------------------------------

class LinkerSettings {
public:
	bool GetOption(ELinkerOption which) const			{ return fSettings[which]; 	}
	void SetOption(ELinkerOption which, bool setTo)		{ fSettings[which] = setTo;	}
	const char* GetUserText(ELinkerOption which) const	{ return fgLinkerOptionInfo[which].fUserText; }
	void AddAllOptions(BList& outList)				{ GCCOptionUtil::AddAllOptions(outList,
																				   fSettings,
																				   fgLinkerOptionInfo, 
																				   kMaxLinkerOptions); }
	void SetDefaults()								{ GCCOptionUtil::SetDefaults(fSettings, 
																			     fgLinkerOptionInfo, 
																			     kMaxLinkerOptions);
													  fVersion = kCurrentVersion; }

	void SwapLittleToHost()							{ fVersion = B_LENDIAN_TO_HOST_INT32(fVersion); }
	void SwapHostToLittle()							{ fVersion = B_HOST_TO_LENDIAN_INT32(fVersion); }

public:
	int32				fVersion;
	static const int32	kCurrentVersion;

private:
	bool 				fSettings[kMaxLinkerOptions];
	static OptionInfo	fgLinkerOptionInfo[kMaxLinkerOptions];
};

// ---------------------------------------------------------------------------

class CodeGenerationSettings {
public:
	bool GetOption(ECodeGenerationOption which) const				{ return fSettings[which]; 	}
	void SetOption(ECodeGenerationOption which, bool setTo)			{ fSettings[which] = setTo;	}
	const char* GetUserText(ECodeGenerationOption which) const		{ return fgCodeGenerationOptionInfo[which].fUserText; }
	void AddOption(ECodeGenerationOption which, BList& outList)	{ fgCodeGenerationOptionInfo[which].AddOption(outList); }
	void AddAllOptions(BList& outList)							{ GCCOptionUtil::AddAllOptions(outList,
																							   fSettings,
																							   fgCodeGenerationOptionInfo, 
																							   kMaxCodeGenerationOptions); }
	void SetDefaults()											{ GCCOptionUtil::SetDefaults(fSettings, 
																						    fgCodeGenerationOptionInfo, 
																						    kMaxCodeGenerationOptions);
																  fVersion = kCurrentVersion; }

	void SwapLittleToHost()										{ fVersion = B_LENDIAN_TO_HOST_INT32(fVersion); }
	void SwapHostToLittle()										{ fVersion = B_HOST_TO_LENDIAN_INT32(fVersion); }

public:
	int32					fVersion;
	static const int32		kCurrentVersion;

private:
	bool 					fSettings[kMaxCodeGenerationOptions];
	static OptionInfo		fgCodeGenerationOptionInfo[kMaxCodeGenerationOptions];
};

// ---------------------------------------------------------------------------
// Conversion function for version 1 -> version 2
// ---------------------------------------------------------------------------

extern void UpdateSetting(CodeGenerationSettings& newSettings, 
			  			  const CodeGenerationSettings& defaultSettings,
			 			  int32 newVersion,
			 			  const CodeGenerationSettings& oldSettings,
			 			  int32 oldVersion);

// ---------------------------------------------------------------------------

class CommonWarningSettings {
public:
	bool GetOption(ECommonWarningOption which) const			{ return fSettings[which]; 	}
	void SetOption(ECommonWarningOption which, bool setTo)		{ fSettings[which] = setTo;	}
	const char* GetUserText(ECommonWarningOption which)	const	{ return fgCommonWarningOptionInfo[which].fUserText; }
	void AddAllOptions(BList& outList)						{ GCCOptionUtil::AddAllOptions(outList,
																						   fSettings,
																						   fgCommonWarningOptionInfo, 
																						   kMaxCommonWarningOptions); }
	void SetDefaults()										{ GCCOptionUtil::SetDefaults(fSettings, 
																			 	 	     fgCommonWarningOptionInfo, 
																				 	     kMaxCommonWarningOptions);
													 		  fVersion = kCurrentVersion; }	

	void SwapLittleToHost()									{ fVersion = B_LENDIAN_TO_HOST_INT32(fVersion); }
	void SwapHostToLittle()									{ fVersion = B_HOST_TO_LENDIAN_INT32(fVersion); }

public:
	int32				fVersion;
	static const int32	kCurrentVersion;

private:
	bool 				fSettings[kMaxCommonWarningOptions];
	static OptionInfo 	fgCommonWarningOptionInfo[kMaxCommonWarningOptions];
};

// ---------------------------------------------------------------------------

class WarningSettings {
public:
	bool GetOption(EWarningOption which) const				{ return fSettings[which]; 	}
	void SetOption(EWarningOption which, bool setTo)		{ fSettings[which] = setTo;	}
	const char* GetUserText(EWarningOption which) const		{ return fgWarningOptionInfo[which].fUserText; }
	void AddAllOptions(BList& outList)					{ GCCOptionUtil::AddAllOptions(outList,
																					   fSettings,
																					   fgWarningOptionInfo, 
																					   kMaxWarningOptions); }
	void SetDefaults()									{ GCCOptionUtil::SetDefaults(fSettings, 
																			  	     fgWarningOptionInfo, 
																			 	     kMaxWarningOptions);
														  fVersion = kCurrentVersion; }	

	void SwapLittleToHost()								{ fVersion = B_LENDIAN_TO_HOST_INT32(fVersion); }
	void SwapHostToLittle()								{ fVersion = B_HOST_TO_LENDIAN_INT32(fVersion); }

public:
	int32				fVersion;
	static const int32	kCurrentVersion;

private:
	bool 				fSettings[kMaxWarningOptions];
	static OptionInfo 	fgWarningOptionInfo[kMaxWarningOptions];
};

// ---------------------------------------------------------------------------
// Member functions for Projectprefsx86
// I stole ProjectPrefsx86 for the gccPlugin project preferences from the BeIDE 
// itself for the project preferences (which was probably a bad idea, ...
// but only uninformed at the time).  Since we don't link with the source
// for ProjectPrefsx86, I provide the swapping functions here
// 
// ---------------------------------------------------------------------------

inline void
ProjectPrefsx86::SwapLittleToHost()
{
	pVersion = B_LENDIAN_TO_HOST_INT32(pVersion);
}

// ---------------------------------------------------------------------------

inline void
ProjectPrefsx86::SwapHostToLittle()
{
	pVersion = B_HOST_TO_LENDIAN_INT32(pVersion);
}

extern void SetProjectPrefsx86Defaults(ProjectPrefsx86& outPrefs);

#endif
