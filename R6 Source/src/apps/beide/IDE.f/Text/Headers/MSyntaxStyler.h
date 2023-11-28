//==================================================================
//	MSyntaxStyle.h
//	Copyright 1996  Metrowerks Corporation, All Rights Reserved.
//==================================================================

#ifndef _MSYNTAXSTYLE_H
#define _MSYNTAXSTYLE_H

#include "STE.h"
#include "Utils.h"

enum TextStateT
{
	NoneStyle,
	TextStyle,
	CCommentStyle,
	CppCommentStyle,
	StringStyle,
	KeywordStyle,
	CustomKeywordStyle,
	PragmaExpressionStyle,
	PreprocessorKeywordStyle,
	IncludeQuoteStyle
};

enum StyleT
{
	sTextStyle,
	sCommentStyle,
	sStringStyle,
	sKeywordStyle,
	sCustomKeywordStyle
};


class MIDETextView;
class STELineBuffer;
class STEStyleBuffer;
class STEStyleRange;
class STETextBuffer;


class MSyntaxStyler
{
public:
								MSyntaxStyler(
									SuffixType		inSuffix,
									MIDETextView&	inTextView,
									STETextBuffer&	inText,
									STELineBuffer&	inLines,
									STEStyleBuffer&	inStyles);
	void						SetStyle(
									StyleT				inStyleKind,
									const STEStyle&		inStyle);
	bool						SetSuffixType(
									SuffixType		inSuffix);
	
	void						ParseAllText();
	void						TextChanged(
									int32		inOffset,
									int32		inLength,
									bool		inAdd);

	bool						SetTextStyle(
									int32		inStartOffset,
									int32		inEndOffset,
									TextStateT	inState);

	bool						IsCustomKeyword(
									const char*	inText);
	STEStyle					GetTextStyle()
								{
									return fTextStyle;
								}

	TextStateT					GetStyleAtPosition(int32 position);
	
	static void 				Init();

private:
	
	MIDETextView&				fTextView;
	STELineBuffer&				mLines;
	STEStyleBuffer&				mStyles;
	STETextBuffer&				mText;
	STEStyleRange*				fRange;
	int32						fBeginOffset;
	int32						fEndOffset;
	int32						fLastChangedOffset;
	STEStyle 					fTextStyle;
	STEStyle 					fCommentStyle;
	STEStyle 					fStringStyle;
	STEStyle 					fKeywordStyle;
	STEStyle 					fCustomKeywordStyle;
	SuffixType					fSuffix;

typedef	bool (MSyntaxStyler::*parse_func)(int32, int32, TextStateT&);

	parse_func					fParseText;

	void						ReParse(
									int32			inStartOffset,
									int32			inLength,
									TextStateT		inCurrentState);

	void						ParseByLines(
									int32			inStartOffset,
									int32			inendOffset,
									TextStateT		inCurrentState = TextStyle,
									bool			inforce = false);

	bool						ParseText(
									int32			inOffset,
									int32			inLength,
									TextStateT&		inoutCurrentState);
	bool						ParseTextCCpp(
									int32			inOffset,
									int32			inLength,
									TextStateT&		inoutCurrentState);
	bool						ParseTextJava(
									int32			inOffset,
									int32			inLength,
									TextStateT&		inoutCurrentState);

	void						InitRange();
	void						KillRange();

	bool						IsKeyword(
									const char*	inText);
	bool						IsPreprocessorKeyword(
									const char*	cptr,
									int32		inOffset,
									int32&		outLen,
									bool&		outIncludeFound);

	bool						IsJavaKeyword(
									const char*	inText);

	bool						OffsetIsStartOfStyle(
									int32	inOffset);
};

#endif
