//========================================================================
//	MKeywordList.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "IDEConstants.h"
#include "MKeywordList.h"
#include "MWEditUtils.h"
#include <ctype.h>
#include <string.h>
#include <SupportKit.h>
// #include <algobase.h>

static char*	sCCppKeywords = {
"and and_eq asm auto bitand bitor bool break case catch char class "
"compl const const_cast continue default delete do double dynamic_cast "
"else enum explicit extern false float for friend goto if "
"inline int long mutable namespace new not not_eq operator or or_eq "
"pascal private protected public register reinterpret_cast return short "
"signed sizeof static static_cast struct switch template this throw true "
"try typedef typeid typename union unsigned using virtual void volatile "
"wchar_t while xor xor_eq"
};
static char*	sObjectCKeywords = {
"@class @defs @encode @end @implementation @interface @protocol @private "
"@protected @public @selector bycopy byref in inout oneway out"
};
static char*	sCCppDirectives = {
"define elif else endif error if ifdef ifndef line undef"
};
static char*	sCCppPragmas = {
"far_code mark near_code nosyminline optimization_level options overload "
"parameter pointers_in_A0 pointers_in_D0 pool_strings pop precompile_target "
"push segment smart_code unused"
};
static char*	sCCppPragmasOnOffReset = {
"a6frames align_array_members always_import auto_inline cfm68k_32bitpcrel "
"cfm68k_codegen cfm68k_farthreshold "
"cfm68k_force_indirect code68020 code68349 code68881 cplusplus cpp_extensions "
"d0_pointers direct_destruction disable_registers dont_inline dont_reuse_strings "
"enumsalwaysint exceptions extended_errorcheck far_data far_strings far_vtables "
"force_active fourbyteints global_optimizer ignore_oldsytle macsbug marathon "
"mpwc mpwc_relax mpwc_newline multibyteaware objective_c oldstyle_symbols once "
"only_std_keywords optimize_for_size pcrelstrings peephole profile "
"readonly_strings require_prototypes safe_prep scheduling side_effects "
"static_inlines sym traceback trigraphs warn_emptydecl warn_extracomment "
"warn_illpragma warn_implicitconv warn_possunwant warn_unusedarg warn_unusedvar ANSI_strict "
"ARM_conform IEEEdoubles RTTI"
};
static char*	sCCppPragmasList = {
"export import internal lib_export"
};

static char*	sJavaKeywords = {
"abstract boolean break byte byvalue case cast catch char class const continue "
"default do double else extends false final finally float for future generic "
"goto if implements import inner instanceof int interface long native new null "
"operator outer package private protected public rest return short static super "
"switch synchronized this throw throws threadsafe transient true try var void "
"volatile while"
};

// Update these counters when modifying the above lists
const int32 kCCppKeywordCount = 74;
const int32 kCCppDirectivesCount = 10;
const int32 kCCppPragmasOnOffResetCount = 60;
const int32 kCCppPragmasCount = 17;
const int32 kCCppPragmasListCount = 4;
const int32 kJavaKeywordsCount = 60;

const int32 kKeywordClumpSize = 10;

MKeywordList*	MKeywordList::sKeywordObjects[kKeywordListCount];

// ---------------------------------------------------------------------------
//		InitLists
// ---------------------------------------------------------------------------

void
MKeywordList::InitLists()
{
	ASSERT(sizeof(KeywordEntry) % 4 == 0);

	MKeywordList*		list = new MKeywordList(kCCppKeywordCount);
	list->AddKeywords(sCCppKeywords, 0);
	sKeywordObjects[kCCppKeywordIndex] = list;

	list = new MKeywordList(kCCppDirectivesCount + 2);
	list->AddKeywords(sCCppDirectives, 0);
	list->AddKeywords("include", kIsIncludeFlag);
	list->AddKeywords("pragma", kIsPragmaFlag);
	sKeywordObjects[kCCppDirectivesIndex] = list;

	list = new MKeywordList(kCCppPragmasOnOffResetCount + kCCppPragmasCount + kCCppPragmasListCount);
	list->AddKeywords(sCCppPragmasOnOffReset, kOnOffResetFlag);
	list->AddKeywords(sCCppPragmas, 0);
	list->AddKeywords(sCCppPragmasList, kAllowsListFlag);
	sKeywordObjects[kCCppPragmasOnOffResetIndex] = list;

	list = new MKeywordList(kJavaKeywordsCount);
	list->AddKeywords(sJavaKeywords, 0);
	sKeywordObjects[kJavaKeywordsIndex] = list;
}

// ---------------------------------------------------------------------------
//		DeleteLists
// ---------------------------------------------------------------------------

void
MKeywordList::DeleteLists()
{
	for (int32 i = 0; i < kKeywordListCount; i++)
		delete sKeywordObjects[i];
}

// ---------------------------------------------------------------------------
//		MKeywordList
// ---------------------------------------------------------------------------
//	Constructor

MKeywordList::MKeywordList(
	int32	inEntryCount)
{
	if (inEntryCount > 0)
	{
		fMaxKeywords = inEntryCount;
	}
	else
	{
		fMaxKeywords = kKeywordClumpSize;
	}

	fKeywords = new KeywordEntry[fMaxKeywords];
	fKeywordCount = 0;

	fKeywords->name[0] = '\0';
}

// ---------------------------------------------------------------------------
//		~MKeywordList
// ---------------------------------------------------------------------------
//	Constructor

MKeywordList::~MKeywordList()
{
	delete[] fKeywords;
}

// ---------------------------------------------------------------------------
//		IncreaseListSize
// ---------------------------------------------------------------------------

void 
MKeywordList::IncreaseListSize()
{
	int32		newKeywordCount = fMaxKeywords + kKeywordClumpSize;
//	fMaxKeywords += kKeywordClumpSize;
	KeywordEntry* 	temp = new KeywordEntry[newKeywordCount];

	if (fKeywords != nil)
	{
		memcpy(temp, fKeywords, fKeywordCount * sizeof(KeywordEntry));
		delete[] fKeywords;
	}

	fKeywords = temp;
	fMaxKeywords = newKeywordCount;
}

// ---------------------------------------------------------------------------
//		AddKeywords
// ---------------------------------------------------------------------------
//	Add a null terminated list of keywords to the list.
//	Keywords are simply separated by whitespace.
//	This is optimized for the case where the list is already
//	sorted, but this isn't a requirement.

void
MKeywordList::AddKeywords(
	const char *	inKeyword,
	char			inFlags)
{
	int32	index = 0;
	char	c;
	char	keyword[128];
	int32	keyIndex;

	while (inKeyword[index] != '\0')
	{
		// Eat whitespace
		while ((c = inKeyword[index]) != '\0' && ! (isalnum(c) || c == '_'))
			index++;
		
		if (c != '\0')
		{
			// Copy keyword to buffer
			keyIndex = 0;
			while ((c = inKeyword[index]) != '\0' && (isalnum(c) || c == '_'))
			{
				keyword[keyIndex++] = c;
				index++;
			}
	
			keyword[keyIndex] = '\0';		// null terminate the keyword

			// Add it
			AddAKeyword(keyword, inFlags);
		}
	}
}

// ---------------------------------------------------------------------------
//		AddKeyword
// ---------------------------------------------------------------------------
//	Add a keyword to the list.

void
MKeywordList::AddAKeyword(
	const char *	inKeyword,
	char			inFlags)
{
	int32				index = fKeywordCount;
	KeywordEntry*		entry = &fKeywords[max(fKeywordCount - 1L, 0L)];
	
	// Check if the keyword goes after the last entry
	// this is an optimization for adding keywords in sort order
	if (strcmp(inKeyword, entry->name) > 0 ||
		! FindKeyword(inKeyword, index, entry))
	{
		if (fKeywordCount + 1 > fMaxKeywords)
			IncreaseListSize();

		entry = &fKeywords[index];
		int32	numtomove = fKeywordCount - index;
		if (numtomove > 0)
			memmove(&fKeywords[index + 1], entry, numtomove * sizeof(KeywordEntry));
		
		entry->flags = inFlags;
		entry->length = strlen(inKeyword);
		ASSERT(entry->length <= sizeof(entry->name));
		strncpy(entry->name, inKeyword, sizeof(entry->name));
		fKeywordCount++;
	}
}

// ---------------------------------------------------------------------------
//		IsKeyword
// ---------------------------------------------------------------------------
//	Is this string in the list.

bool
MKeywordList::IsKeyword(
	const char *	inKeyword,
	char&			outFlags,
	int32&			outLength)
{
	int32			index;
	KeywordEntry*	entry;

	if (FindKeyword(inKeyword, index, entry))
	{
		outFlags = entry->flags;
		outLength = entry->length;

		return true;
	}
	else
		return false;
}

// ---------------------------------------------------------------------------
//		FindKeyword
// ---------------------------------------------------------------------------
//	Is this string in the list?  If so return its index and its EntryPtr,
//	 if not return  the position where it should go in the list.

bool
MKeywordList::FindKeyword(
	const char *	inKeyword,
	int32&			outIndex,
	KeywordEntry*&	outEntry)
{
	int32			top = fKeywordCount - 1;
	int32			bottom = 0;
	int32			middle;
	int32			comparison;
	
	while (top >= bottom)
	{
		middle = (bottom + top) / 2;
		
		KeywordEntry*	entry = &fKeywords[middle];
		
		comparison = CompareStrings(inKeyword, entry->name);

		if (comparison < 0 )
		{
			top = middle - 1;
		}
		else
		if (comparison > 0)
		{
			bottom = middle + 1;
		}
		else
		{
			outIndex = middle;
			outEntry = entry;
			return true;			// found; early exit
		}
	}
	
	// Not found
	outIndex = bottom;
	
	return false;
}

