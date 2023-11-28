// ---------------------------------------------------------------------------
/*
	GCCOptions.cpp
	
	Copyright (c) 1998 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			27 October 1998

	See GCCOptions.h for explanation and details.

*/
// ---------------------------------------------------------------------------

#include "GCCOptions.h"
#include "PlugInUtil.h"

#include <string.h>

#include <List.h>
#include <Mime.h>

#include <stdio.h>

// ---------------------------------------------------------------------------
//	Global option arrays
//	WARNING!!! If you change the order or add/delete from these arrays
//	you must also change the accompanying enumeration.
//	WARNING!!! ORDER IS IMPORTANT!!!
// ---------------------------------------------------------------------------

OptionInfo LanguageSettings::fgLanguageOptionInfo[kMaxLanguageOptions] =
{
	{"-x c", "C", false, OptionInfo::kMultiWord},
	{"-x c++", "C++", false, OptionInfo::kMultiWord},
	{"-ansi", "Compile for ANSI C", false, OptionInfo::OptionInfo::kSingleWord},
	{"-trigraphs", "Support ANSI C trigraphs", false, OptionInfo::kSingleWord},
	{"-funsigned-char", "Make 'char' be signed by default", false, OptionInfo::kSingleWord},
	{"-funsigned-bitfields", "Make bitfields be unsigned by default", false, OptionInfo::kSingleWord},
};		

// ---------------------------------------------------------------------------

OptionInfo LinkerSettings::fgLinkerOptionInfo[kMaxLinkerOptions] =
{
	{"-Xlinker -s", "Strip all symbols", false, OptionInfo::kMultiWord},
	{"-Xlinker -x", "Strip all local symbols", false, OptionInfo::kMultiWord},
};

// ---------------------------------------------------------------------------

OptionInfo CodeGenerationSettings::fgCodeGenerationOptionInfo[kMaxCodeGenerationOptions] =
{
	{"-O0", "None", false, OptionInfo::kSingleWord},
	{"-O1", "Some", false, OptionInfo::kSingleWord},
	{"-O2", "More", false, OptionInfo::kSingleWord},
	{"-O3", "Full", true, OptionInfo::kSingleWord},
	{"-Os", "Optimize for space rather than for speed", false, OptionInfo::kSingleWord},
	{"-no-fpic", "Do not generate position independent code (use for drivers only)", false, OptionInfo::OptionInfo::kSingleWord},
	{"-fno-implicit-templates", "Only emit code for explicit template instantiations", false, OptionInfo::kSingleWord},
	{"-fkeep-inline-functions", "Generate code for functions even if they are fully inlined", false, OptionInfo::kSingleWord},
	{"-g", "Generate debugging information", false, OptionInfo::kSingleWord},
	{"-p", "Generate profiling code", false, OptionInfo::kSingleWord},
};

// ---------------------------------------------------------------------------

OptionInfo CommonWarningSettings::fgCommonWarningOptionInfo[kMaxCommonWarningOptions] =
{
// with -Wall don't report multi-character literals or private constructor/destructors
	{"-Wall -Wno-multichar -Wno-ctor-dtor-privacy", "Warn about common potential errors (warns about all of the following...)", true, OptionInfo::kMultiWord},
// warnings covered by -Wall
		{"-Wparentheses", "Warn about possible missing parentheses", false, OptionInfo::kSingleWord},
		{"-Wreturn-type", "Warn about inconsistent return types", false, OptionInfo::kSingleWord},
		{"-Wswitch", "Warn about enumerated switches missing a specific case", false, OptionInfo::kSingleWord},
		{"-Wunused", "Warn when a variable is unused", false, OptionInfo::kSingleWord},
		{"-Wuninitialized", "Warn about unitialized automatic variables", false, OptionInfo::kSingleWord},
		{"-Wreorder", "Warn when the compiler reorders class member initializations", false, OptionInfo::kSingleWord},
		{"-Wnon-virtual-dtor", "Warn about nonvirtual destructors", false, OptionInfo::kSingleWord},
		{"-Wunknown-pragmas", "Warn about unrecognized pragmas", false, OptionInfo::kSingleWord},
		{"-Wsign-compare", "Warn about signed/unsigned comparisons", false, OptionInfo::kSingleWord},
		{"-Wchar-subscripts", "Warn about subscripts with type 'char'", false, OptionInfo::kSingleWord},
		{"-Wformat", "Warn about printf() format anomalies", false, OptionInfo::kSingleWord},
		{"-Wtrigraphs", "Warn if trigraphs are encountered", false, OptionInfo::kSingleWord}
};

// ---------------------------------------------------------------------------

OptionInfo WarningSettings::fgWarningOptionInfo[kMaxWarningOptions] =
{
	{"-w", "disabled", false, OptionInfo::kSingleWord},
	{"-Werror", "treated as errors", false, OptionInfo::kSingleWord},
	{"-pedantic -Wno-long-long", "Issue all warnings demanded by strict ANSI C/C++", false, OptionInfo::kMultiWord},
	{"-Wshadow", "Warn when one local variable shadows another", false, OptionInfo::kSingleWord},
	{"-Wbad-function-cast", "Warn about casting functions to incompatible types", false, OptionInfo::kSingleWord},
	{"-Wcast-qual", "Warn about casts which discard qualifiers", false, OptionInfo::kSingleWord},
	{"-Wconversion ", "Warn about possibly confusing type conversions", false, OptionInfo::kSingleWord},
	{"-Winline", "Warn when an inlined function cannot be inlined", false, OptionInfo::kSingleWord},
	{"-Wextern-inline ", "Warn when a function is declared extern, then inline", false, OptionInfo::kSingleWord},
	{"-Wwrite-strings", "Mark literal strings as 'const char *'", false, OptionInfo::kSingleWord},
	{"-Woverloaded-virtual", "Warn about overloaded virtual function names", true, OptionInfo::kSingleWord},
	{"-Wold-style-cast", "Warn if a C style cast is used in a program", false, OptionInfo::kSingleWord},
	{"-Weffc++", "Warn about violations of \"Effective C++\" style rules", false, OptionInfo::kSingleWord},
};

// ---------------------------------------------------------------------------
//	Settings - static members
// ---------------------------------------------------------------------------

const int32	LanguageSettings::kCurrentVersion = 1;
const int32	LinkerSettings::kCurrentVersion = 1;
const int32	CodeGenerationSettings::kCurrentVersion = 2;
const int32	CommonWarningSettings::kCurrentVersion = 1;
const int32	WarningSettings::kCurrentVersion = 1;

// ---------------------------------------------------------------------------
//	OptionInfo member functions
// ---------------------------------------------------------------------------

void
OptionInfo::AddOption(BList& outList)
{
	// Add the option string to outList
	// Metrowerks says the string must be created by malloc
	
	switch (fOptionStringKind) {
		case kSingleWord:
			outList.AddItem(strdup(fOptionString));
			break;
		case kMultiWord:
			PlugInUtil::AddMultipleOptions(outList, fOptionString);
			break;
	}
}


// ---------------------------------------------------------------------------
//	GCCOptionUtil static helper functions
// ---------------------------------------------------------------------------

void
GCCOptionUtil::SetDefaults(bool settings[], OptionInfo info[], long optionCount)
{
	for (int i = 0; i < optionCount; i++) {
		settings[i] = info[i].fDefaultSetting;
	}
}

// ---------------------------------------------------------------------------

void
GCCOptionUtil::AddAllOptions(BList& outList, bool settings[], OptionInfo info[], long optionCount)
{
	// Iterate through the settings list
	// For each setting that is on, grab the object text and add it to the outList
	
	for (int i = 0; i < optionCount; i++) {
		if (settings[i] == true) {
			info[i].AddOption(outList);
		}
	}
}

// ---------------------------------------------------------------------------
// Helper function for ProjectPrefsx86
// ---------------------------------------------------------------------------

const int32	kCurrentVersion = 1;

void
SetProjectPrefsx86Defaults(ProjectPrefsx86& outPrefs)
{
	// taken from MDefaultPrefs::SetProjectDefaultsx86
	
	memset(&outPrefs, 0, sizeof(ProjectPrefsx86));
	
	outPrefs.pVersion = kCurrentVersion;
	outPrefs.pProjectKind = AppType;
	outPrefs.pBaseAddress = 0x80000000;		// default base address for apps (not used for gcc)
	strcpy(outPrefs.pAppName, kDefaultAppName);
	strcpy(outPrefs.pAppType, B_ELF_APP_MIME_TYPE);
}

// ---------------------------------------------------------------------------
// Helper function for CodeGenerationSettings
// Explicit template function for updating CodeGenerationSettings
// ---------------------------------------------------------------------------

void
UpdateSetting(CodeGenerationSettings& newSettings, 
			  const CodeGenerationSettings& defaultSettings,
			  int32 newVersion,
			  const CodeGenerationSettings& oldSettings,
			  int32 oldVersion)
{
	if (newVersion == 2 && oldVersion == 1) {
		newSettings = oldSettings;
		newSettings.SetOption(kGenerateProfileCode, defaultSettings.GetOption(kGenerateProfileCode));
		newSettings.fVersion = CodeGenerationSettings::kCurrentVersion;
	}
	else {
		newSettings = defaultSettings;
	}
}

