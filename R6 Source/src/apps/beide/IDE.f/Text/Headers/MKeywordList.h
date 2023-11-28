//========================================================================
//	MKeywordList.h
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MKEYWORDLIST_H
#define _MKEYWORDLIST_H

#include <SupportDefs.h>
#include <Debug.h>

struct KeywordEntry {
	char			flags;
	char			length;
	char			name[26];
};

const int32 kKeywordListCount = 4;

const int32 kCCppKeywordIndex = 0;
const int32 kCCppDirectivesIndex = 1;
const int32 kCCppPragmasOnOffResetIndex = 2;
const int32 kJavaKeywordsIndex = 3;

const char kOnOffResetFlag = 1;
const char kAllowsListFlag = 2;
const char kIsIncludeFlag = 4;
const char kIsPragmaFlag = 8;

class MKeywordList
{
public:

								MKeywordList(
									int32	inEntryCount = 0);
								~MKeywordList();
								
	void						AddKeywords(
									const char *	inKeyword,
									char			inFlags);
	void						AddAKeyword(
									const char *	inKeyword,
									char			inFlags);
	bool						IsKeyword(
									const char *	inKeyword,
									char&			outFlags,
									int32&			outLength);
	bool						FindKeyword(
									const char *	inKeyword,
									int32&			outIndex,
									KeywordEntry*&	outEntry);

	static void					InitLists();
	static void					DeleteLists();
	static MKeywordList*		Keywordlist(
									int32	inIndex);
private:

	int32					fKeywordCount;
	int32					fMaxKeywords;
	KeywordEntry*			fKeywords;
	
	static MKeywordList*	sKeywordObjects[kKeywordListCount];

	void					IncreaseListSize();
};

inline MKeywordList* 
MKeywordList::Keywordlist(
	int32	inIndex)
{
	ASSERT(inIndex >= 0 && inIndex <= kKeywordListCount);
	return sKeywordObjects[inIndex];
}

#endif
