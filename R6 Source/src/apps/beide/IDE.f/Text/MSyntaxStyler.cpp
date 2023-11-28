//==================================================================
//	MSyntaxStyler.cpp
//	Copyright 1996  Metrowerks Corporation, All Rights Reserved.
//==================================================================

#include <ctype.h>
#include <string.h>

#include "MSyntaxStyler.h"
#include "STEngine.h"
#include "MIDETextView.h"
#include "MWEditUtils.h"
#include "MKeywordList.h"

const rgb_color	kTextColor = { 0, 0, 0, 255 };			// black
const rgb_color	kCommentColor = { 255, 0, 0, 255 };		// red
const rgb_color	kStringColor = { 144, 144, 144, 255 };	// grey	
const rgb_color	kKeywordColor = { 0, 0, 255, 255 };		//blue

const STEStyle* kTextStyle;			// default font
const STEStyle* kCommentStyle;		// default font
const STEStyle* kStringStyle;		// default font
const STEStyle* kKeywordStyle;		// default font

const int32 kMaxRuns = 50;

// A style range object that holds 50 style runs and
// is interchangeable with an STEStyleRange
struct MStyleRange : public STEStyleRange
{
	STEStyleRun		runs1[kMaxRuns];	// array of kMaxRuns number of runs
};

// ---------------------------------------------------------------------------
//		MSyntaxStyler
// ---------------------------------------------------------------------------
//	Constructor

MSyntaxStyler::MSyntaxStyler(
	SuffixType		inSuffix,
	MIDETextView&	inTextView,
	STETextBuffer&	inText,
	STELineBuffer&	inLines,
	STEStyleBuffer&	inStyles)
	: fTextView(inTextView),
	 mText(inText), mLines(inLines), mStyles(inStyles)
{
	fTextStyle = *kTextStyle;
	fCommentStyle = *kCommentStyle;
	fStringStyle = *kStringStyle;
	fKeywordStyle = *kKeywordStyle;

	// Set the kind of syntax styling
	fSuffix = kInvalidSuffix;
	if (inSuffix == kInvalidSuffix)
		fSuffix = kJavaSuffix;		// must be different from inSuffix
	SetSuffixType(inSuffix);
}

// ---------------------------------------------------------------------------
//		Init
// ---------------------------------------------------------------------------
//	The be fonts don't exist until after the BApplication constructor has
//	run so we have to construct them here rather than make them static objects.

void
MSyntaxStyler::Init()
{
	kTextStyle = new STEStyle(*be_fixed_font, kTextColor, 0);
	kCommentStyle = new STEStyle(*be_fixed_font, kCommentColor, 0);
	kStringStyle = new STEStyle(*be_fixed_font, kStringColor, 0);
	kKeywordStyle = new STEStyle(*be_fixed_font, kKeywordColor, 0);

	((STEStyle*)kTextStyle)->SetSpacing(B_BITMAP_SPACING);
	((STEStyle*)kCommentStyle)->SetSpacing(B_BITMAP_SPACING);
	((STEStyle*)kStringStyle)->SetSpacing(B_BITMAP_SPACING);
	((STEStyle*)kKeywordStyle)->SetSpacing(B_BITMAP_SPACING);
}

// ---------------------------------------------------------------------------
//		InitRange
// ---------------------------------------------------------------------------

void
MSyntaxStyler::InitRange()
{
	// Initialize the Style range object
	fRange = new MStyleRange;
	fRange->count = 0;
	fBeginOffset = 0;
	fEndOffset = -1;
	fLastChangedOffset = 0;
}

// ---------------------------------------------------------------------------
//		KillRange
// ---------------------------------------------------------------------------

inline void
MSyntaxStyler::KillRange()
{
	delete fRange;
}

// ---------------------------------------------------------------------------
//		ParseText
// ---------------------------------------------------------------------------
//	Dispatch the pointer to member function in a way that hides
//	its extreme ugliness.
//	I use a pointer to member here rather than virtual functions
//	in order to make the syntaxstyler class easier to use.
//	The same effect could be obtained by having separate
//	java and ccpp syntax classes but this would be more akward
//	to use.
//	It could also use an if statement for the dispatch but that is more
//	error prone, awkward to use, and less efficient.

inline bool
MSyntaxStyler::ParseText(
	int32			inOffset,
	int32			inLength,
	TextStateT&		inoutCurrentState)
{
	return (this->*fParseText)(inOffset, inLength, inoutCurrentState);
}

// ---------------------------------------------------------------------------
//		SetSuffixType
// ---------------------------------------------------------------------------
//	Adjusts the kind of syntax styling to be done by this object.
//	Doesn't update the view.  Call ParseAllText for that.
//	Returns true if the suffix type changed.

bool
MSyntaxStyler::SetSuffixType(
	SuffixType		inSuffix)
{
	bool		result = false;

	if (inSuffix != fSuffix)
	{
		switch (inSuffix)
		{
			case kJavaSuffix:
				fParseText = &MSyntaxStyler::ParseTextJava;
				break;
			
			default:
				fParseText = &MSyntaxStyler::ParseTextCCpp;
				break;				
		}
		
		fSuffix = inSuffix;
		result = true;
	}
	
	return result;
}

// ---------------------------------------------------------------------------
//		ParseAllText
// ---------------------------------------------------------------------------

void
MSyntaxStyler::ParseAllText()
{
	InitRange();

	TextStateT		style = NoneStyle;
	ParseText(0, fTextView.TextLength(), style);

	if (fRange->count > 0)
		fTextView.SetStyleRange(fBeginOffset, fEndOffset, fRange, false);

	fTextView.Refresh(0, fTextView.TextLength(), true, false);

	KillRange();
}

// ---------------------------------------------------------------------------
//		SetStyle
// ---------------------------------------------------------------------------
//	Set the style info for the specified style type.  This function
//	doesn't update the view or reparse the text.

void
MSyntaxStyler::SetStyle(
	StyleT				inStyleKind,
	const STEStyle&		inStyle)
{
	STEStyle*		style = nil;
	
	switch (inStyleKind)
	{
		case sTextStyle:
			style = &fTextStyle;
			break;
		case sCommentStyle:
			style = &fCommentStyle;
			break;
		case sStringStyle:
			style = &fStringStyle;
			break;
		case sKeywordStyle:
			style = &fKeywordStyle;
			break;
		default:
			ASSERT(false);
			break;
	}

	*style = inStyle;
}

// ---------------------------------------------------------------------------
//		SetTextStyle
// ---------------------------------------------------------------------------
//	Offsets represent the position of the insertion point before or after
//	characters.  So a one character style will have inEndOffset = inStartOffset + 1

bool
MSyntaxStyler::SetTextStyle(
	int32		inStartOffset,
	int32		inEndOffset,
	TextStateT	inState)
{
	if (inStartOffset >= inEndOffset)
		return false;

	ASSERT(inStartOffset >= fEndOffset);

	int32 			runIndex = mStyles.OffsetToRun(inStartOffset);
	STEStyleRun		run1 = mStyles[runIndex];
	runIndex = min(++runIndex, mStyles.NumRuns() - 1);
	STEStyleRun		run2 = mStyles[runIndex];
	bool			styleChanged = true;

	if ((run1.style.extra == inState && run1.offset <= inStartOffset) && 
			(run2.offset >= inEndOffset))
		styleChanged = false;

	STEStyleRunPtr run = &fRange->runs[fRange->count];
	
	switch(inState)
	{
		case NoneStyle:
		case TextStyle:
		case IncludeQuoteStyle:
			run->style = fTextStyle;
			break;

		case CCommentStyle:
		case CppCommentStyle:
			run->style = fCommentStyle;
			break;

		case StringStyle:
			run->style = fStringStyle;
			break;

		case KeywordStyle:
		case PragmaExpressionStyle:
		case PreprocessorKeywordStyle:
		case CustomKeywordStyle:
			run->style = fKeywordStyle;
			break;

		default:
			ASSERT(false);
			break;
	}

	run->offset = inStartOffset - fBeginOffset;
	run->style.extra = inState;
	
	ASSERT(run->offset >= fEndOffset - fBeginOffset);

	fRange->count++;
	fEndOffset = inEndOffset;
	
	if (fRange->count >= kMaxRuns)
	{
		fTextView.SetStyleRange(fBeginOffset, inEndOffset, fRange, false);
		fRange->count = 0;
		fBeginOffset = inEndOffset + 1;
	}
	
	if (styleChanged)
		fLastChangedOffset = inEndOffset;

	return styleChanged;
}

// ---------------------------------------------------------------------------
//		ParseByLines
// ---------------------------------------------------------------------------

void
MSyntaxStyler::ParseByLines(
	int32			inStartOffset,
	int32			inEndOffset,
	TextStateT		inCurrentState,
	bool			inForce)		// force use of inCurrentState
{
	int32				lineNumber = fTextView.OffsetToLine(inStartOffset);
	int32				startLineNumber = lineNumber;
	const STELine* 		line;
	const STELine*		nextLine;
	int32				nextLineOffset = 0;
	int32				lineIncrement = 1;
	int32				lineLength;
	int32 				runIndex;
	int32				lastLine = mLines.NumLines();
	bool				more = true;
	STEStyleRun			run;

	InitRange();

	if (! inForce)
	{
		// Get currentState from style at the start of the line
		line = mLines[lineNumber];
		runIndex = mStyles.OffsetToRun(line->offset);
		run = mStyles[runIndex];
		inCurrentState =  (TextStateT) run.style.extra;
	}

	while (lineNumber <= lastLine && (more || nextLineOffset < inEndOffset))
	{
		line = mLines[lineNumber];
		lineNumber += lineIncrement;
		lineIncrement *= 2;

		if (lineNumber <= lastLine)
		{
			nextLine = mLines[lineNumber];
			nextLineOffset = nextLine->offset;
		}
		else
			nextLineOffset = mText.Length();

		lineLength = nextLineOffset - line->offset;

		more = ParseText(line->offset, lineLength, inCurrentState);
	}

	if (fRange->count > 0)
		fTextView.SetStyleRange(fBeginOffset, fEndOffset, fRange, false);

	if (fLastChangedOffset < inStartOffset)
		fLastChangedOffset = inStartOffset + 1;
	fTextView.Refresh(inStartOffset, fLastChangedOffset, true, false);

	KillRange();
}

// ---------------------------------------------------------------------------
//		ReParse
// ---------------------------------------------------------------------------

void
MSyntaxStyler::ReParse(
	int32			inStartOffset,
	int32			inLength,
	TextStateT		inCurrentState)
{
	InitRange();

	ParseText(inStartOffset, inLength, inCurrentState);

	if (fRange->count > 0)
		fTextView.SetStyleRange(fBeginOffset, fEndOffset, fRange, false);

	if (fLastChangedOffset < inStartOffset)
		fLastChangedOffset = inStartOffset + 1;
	fTextView.Refresh(inStartOffset, fLastChangedOffset, true, false);

	KillRange();
}

// ---------------------------------------------------------------------------
//		TextChanged
// ---------------------------------------------------------------------------
//	Reparse the text after a keydown, cut, paste, or clear.

void
MSyntaxStyler::TextChanged(
	int32		inOffset,
	int32		inLength,
	bool		inAdd)
{
	bool			needsRefresh = false;
	int32 			runIndex = mStyles.OffsetToRun(inOffset);
	STEStyleRun		run = mStyles[runIndex++];
	STEStyleRun		nextRun;
	
	if (runIndex < mStyles.NumRuns())
		nextRun = mStyles[runIndex];
	else
		nextRun.offset = mText.Length();

	int32			styleOffset = run.offset;
	int32			styleLength = nextRun.offset - run.offset;
	bool			remainsIntactAtStart;
	bool			remainsIntactAtEnd;
	TextStateT		currentState = (TextStateT) run.style.extra;
	uchar 			c;

	styleLength += styleOffset - 1;
	while (((c = mText[styleLength]) == TAB || c == SPACE || c == EOL_CHAR) && 
		styleLength >= styleOffset)
		styleLength--;
	styleLength -= styleOffset -1;

	switch (run.style.extra)
	{
		case NoneStyle:
		case TextStyle:
			ParseByLines(inOffset, nextRun.offset);
			break;

		case CCommentStyle:
			remainsIntactAtStart = (styleLength >= 4 && (inOffset > styleOffset + 1 ||
				(mText[styleOffset] == '/' && mText[styleOffset + 1] == '*')));
			
			if (! remainsIntactAtStart)
			{
				ParseByLines(inOffset, nextRun.offset, NoneStyle, true);
			}
			else
			{
				remainsIntactAtEnd = (inOffset < styleOffset + styleLength &&
					((mText[styleOffset + styleLength - 2] == '*' && 
					mText[styleOffset + styleLength - 1] == '/') ||
					styleOffset + styleLength == mText.Length()));

				if (! remainsIntactAtEnd)
				{
					if (inOffset > styleOffset + styleLength - 2)
						ParseByLines(styleOffset + styleLength - 2, nextRun.offset + 1);
					else
						ParseByLines(inOffset, nextRun.offset + 1);
				}
				else	// Remains intact
				if (inLength > 1)
				{
					ParseByLines(inOffset, nextRun.offset);
				}
				else 	// Check if single char terminated comment /**/
				if ((inOffset - styleOffset >= 2 && mText[inOffset] == '*' && mText[inOffset + 1] == '/') ||
					(inOffset - styleOffset >= 3 && mText[inOffset - 1] == '*' && mText[inOffset] == '/'))
				{
					ParseByLines(inOffset - 2, nextRun.offset);
				}
				else	// Special case for insert '*' into sequence of "/*/" -> "/**/" 
				if ((mText[inOffset] == '*' && mText[inOffset + 1] == '*') &&
					mText[inOffset + 2] == '/' && mText[inOffset - 1] == '/')
				{
					ParseByLines(inOffset, nextRun.offset, NoneStyle, true);
				}
				else
					needsRefresh = true;	// no change to style
			}
			break;

		case CppCommentStyle:
			remainsIntactAtStart = (inOffset > styleOffset + 1) ||
				(mText[styleOffset] == '/' && mText[styleOffset + 1] == '/');
			
			if (remainsIntactAtStart)
			{
				if (inAdd)
				{
					if (inLength > 1)
						ParseByLines(inOffset, nextRun.offset, CppCommentStyle);
					else
					if (mText[inOffset] == EOL_CHAR)
						ParseByLines(inOffset, nextRun.offset, NoneStyle, true);
					else
						needsRefresh = true;
					// if length == 1 do nothing unless it's a newline
				}
				else
				{
					// Backspace from next line or cut that removes the eol?
					remainsIntactAtEnd = mText[styleOffset + styleLength] == EOL_CHAR ||
						mText[styleOffset + styleLength] == '\0';

					if (! remainsIntactAtEnd)
						ParseByLines(inOffset, nextRun.offset);
					else
						needsRefresh = true;			
				}
			}
			else
			{
				int32			linenumber = fTextView.OffsetToLine(inOffset);
				int32			len;

				if (linenumber < fTextView.CountLines())
				{
					const STELine*	line = mLines[linenumber + 1];
					len = line->offset - styleOffset;
				}
				else
				{
					len = fTextView.TextLength() - styleOffset;
				}

				ParseByLines(styleOffset, styleOffset + len, TextStyle, true);
			}
			break;

		case StringStyle:
			// This string code is essentially the same as the CComment code
			remainsIntactAtStart = (inOffset > styleOffset || mText[styleOffset] == '"');
			
			if (! remainsIntactAtStart)
			{
				ParseByLines(styleOffset, nextRun.offset, NoneStyle, true);
			}
			else
			{
				remainsIntactAtEnd = (styleLength >= 2 && inOffset < styleOffset + styleLength &&
					mText[styleOffset + styleLength - 1] == '"' &&
						mText[styleOffset + styleLength - 2] != '\\');

				if (! remainsIntactAtEnd && inOffset > styleOffset + styleLength - 1)
				{
					ParseByLines(styleOffset + styleLength - 1, nextRun.offset);
				}
				else	// Remains intact
						// Check if single char terminated string
						// or if char is after the closing " of the string
				if (inLength > 1 || inOffset > styleOffset + styleLength ||
					! remainsIntactAtEnd ||
					(mText[inOffset] == '"' && mText[inOffset - 1] != '\\') || 
					(mText[inOffset + 1] == '"') ||
					(mText[inOffset] == '\'' && mText[inOffset - 1] == '"') &&
					mText[inOffset - 2] == '\'')
				{
					ParseByLines(inOffset - 1, nextRun.offset);
				}
				else
					needsRefresh = true;
			}
			break;

		case KeywordStyle:
			ParseByLines(styleOffset, nextRun.offset);
			break;

		case CustomKeywordStyle:
			break;

		case PreprocessorKeywordStyle:
			ParseByLines(styleOffset, nextRun.offset);
			break;

		case IncludeQuoteStyle:
		{
			int32			lineNumber = fTextView.OffsetToLine(inOffset);
			const STELine*	line = mLines[++lineNumber];

			ParseByLines(inOffset, line->offset - 1);
		}
			break;
	}

	if (needsRefresh)
		fTextView.Refresh(inOffset, inOffset + inLength, true, false);
}

#pragma mark -
#pragma mark ## C/C++ ##

// ---------------------------------------------------------------------------
//		IsKeyword
// ---------------------------------------------------------------------------

inline bool
MSyntaxStyler::IsKeyword(
	const char*	cptr)
{
	char	flags;
	int32	length;
	
	return MKeywordList::Keywordlist(kCCppKeywordIndex)->MKeywordList::IsKeyword(cptr, flags, length);
}

// ---------------------------------------------------------------------------
//		IsCustomKeyword
// ---------------------------------------------------------------------------

inline bool
MSyntaxStyler::IsCustomKeyword(
	const char*	/*cptr*/)
{
	return false;
}

// ---------------------------------------------------------------------------

TextStateT
MSyntaxStyler::GetStyleAtPosition(int32 position)
{
	int32 runIndex = mStyles.OffsetToRun(position);
	STEStyleRun run = mStyles[runIndex];
	return (TextStateT) run.style.extra;
}

// ---------------------------------------------------------------------------
//		OffsetIsStartOfStyle
// ---------------------------------------------------------------------------
//	Find if the spescified offset is the start of a style.  This is needed
//	to tell if a double quote is at the start or end of a style when
//	reparsing text at an arbitrary offset.


bool
MSyntaxStyler::OffsetIsStartOfStyle(
	int32	inOffset)
{
	// Flush any styles 
	if (fRange->count > 0)
	{
		fTextView.SetStyleRange(fBeginOffset, fEndOffset, fRange, false);
		fRange->count = 0;
		fBeginOffset = fEndOffset + 1;
	}

	// get the style at the specified offset
	int32				runIndex = mStyles.OffsetToRun(inOffset);
	STEStyleRun			run = mStyles[runIndex];

	return inOffset == run.offset;
}

// ---------------------------------------------------------------------------
//		ParseTextCCpp
// ---------------------------------------------------------------------------

const int32 kMaxTokenLen = 30;

bool
MSyntaxStyler::ParseTextCCpp(
	int32			inOffset,
	int32			inLength,
	TextStateT&		inoutCurrentState)
{
	TextStateT		currentState = inoutCurrentState;
	int32			text = inOffset;
	int32			term = text + inLength;
	int32			styleStart = text;
	bool			styleChanged = false;
	char			token[kMaxTokenLen+1];
	int32			tokenLen;
	int32			temp;
	uchar			c;
#if DEBUG
	const char*		textP = fTextView.Text();
#endif

	// Preamble for special cases
	switch (inoutCurrentState)
	{
		case PreprocessorKeywordStyle:
			if (text < term && mText[text] == '#')
				text++;
			break;

		// need to increment if at the start of the string
		// but not if at the end of the string
		case StringStyle:
			if (text < term && mText[text] == '"' && OffsetIsStartOfStyle(text))
				text++;
			break;
	}

	// Parse the text
	while (text < term && mText[text] != '\0')
	{
		switch (currentState)
		{
			case CCommentStyle:
			{
				// Loop until we get to the end of the comment
				// (make sure we stop if we hit the end of our text)
				// Special handling for "/*/" by checking that the style just didn't start one
				// character previous.  We just can't check for "/*/" because it could be something
				// like:  "/* this is a comment with single slash at end /*/"
				bool startOfStyleIsHere = this->OffsetIsStartOfStyle(styleStart);
				while (text < term && (c = mText[text]) != '\0' && 
					! (c == '*' && mText[text+1] == '/' && (text > styleStart + 1 || startOfStyleIsHere == false)))
					text++;

				if (text < term || (mText[text-2] == '*' && mText[text-1] == '/'))
				{
					currentState = TextStyle;
					text += 2;
				}
				styleChanged |= SetTextStyle(styleStart, text, CCommentStyle);
				styleStart = text;
				break;
			}
			case CppCommentStyle:
				while (text < term && ((c = mText[text]) != EOL_CHAR && c != '\0'))
					text++;

				styleChanged |= SetTextStyle(styleStart, text, CppCommentStyle);
				styleStart = text;
				currentState = TextStyle;
				break;

			case StringStyle:
				// look for '"' or "\\" and not '\"'
				while (text < term && ((c = mText[text]) != '\0' && c != '"'))
				{
					if (c == '\\')	// skip over any escaped chars (especially \")
						text += 2;
					else
						text++;
				}

				if (text < term)
					text++;
				styleChanged |= SetTextStyle(styleStart, text, StringStyle);
				styleStart = text;
				if (text < term)
					currentState = TextStyle;
				break;

			case KeywordStyle:
				tokenLen = 0;
				
				while (tokenLen < kMaxTokenLen && text < term && ((c = mText[text]) == '_' || isalnum(c)))		// skip to next token
				{
					token[tokenLen++] = c;
					text++;
				}
				token[tokenLen] = '\0';
	
				if (tokenLen < kMaxTokenLen && IsKeyword(token))
				{
					styleChanged |= SetTextStyle(styleStart, text, KeywordStyle);
					styleStart = text;			
				}

				currentState = TextStyle;
				break;

			case PreprocessorKeywordStyle:
				bool	include;
				int32	keywordLen;
				
				tokenLen = 0;

				while (text < term && ((c = mText[text]) == SPACE || c == TAB))
					text++;
				temp = text;
				
				// Copy token to buffer
				while (text < term && tokenLen < kMaxTokenLen && ((c = mText[text]) == '_' || isalnum(c)))		// skip to next token
				{
					token[tokenLen++] = c;
					text++;
				}
				token[tokenLen] = '\0';

				if (tokenLen < kMaxTokenLen && IsPreprocessorKeyword(token, temp, keywordLen, include))
				{
					text += keywordLen - tokenLen;
					styleChanged |= SetTextStyle(styleStart, text, PreprocessorKeywordStyle);	// set keyword
					styleStart = text;
					
					if (include)
					{
						temp = text;
						while (temp < term && ((c = mText[temp]) == SPACE || c == TAB))
							temp++;

						uchar	m = 0;	// matching character: "" or <>
						if (c == '"')
							m = '"';
						else
						if (c == '<')
							m = '>';

						if (m != 0)
						{
							temp++;
							while (temp < term && ((c = mText[temp]) != m && c != EOL_CHAR))
								temp++;
							if (c == m)
								temp++;
							SetTextStyle(text, temp, IncludeQuoteStyle);	// set include quote
							text = temp;
							styleStart = text;
						}
					}
				}

				currentState = TextStyle;
				break;

			case NoneStyle:
			case TextStyle:
			case IncludeQuoteStyle:
				while (text < term && ((c = mText[text]) == SPACE || c == TAB))
					text++;

				switch (c)
				{
					case '#':
						if (styleStart == inOffset)	// beginning of text?
						{
							currentState = PreprocessorKeywordStyle;
							styleStart = text;
							text++;
						}
						else
							text++;
						break;

					case EOL_CHAR:
						if (styleStart != text)
						{
							styleChanged |= SetTextStyle(styleStart, text, TextStyle);	// end text
							styleStart = ++text;
						}
						else
							text++;

						while (text < term && ((c = mText[text]) == SPACE || c == TAB))
							text++;
						
						if (c == '#' && text < term)
						{
							currentState = PreprocessorKeywordStyle;
							text++;
						}
						break;

					case '/':
						c = mText[text+1];
						if (c == '/')
						{
							styleChanged |= SetTextStyle(styleStart, text, TextStyle);
							currentState = CppCommentStyle;
							styleStart = text;
							text += 2;
						}
						else
						if (c == '*')
						{
							styleChanged |= SetTextStyle(styleStart, text, TextStyle);
							currentState = CCommentStyle;
							styleStart = text;
							if (mText[text+2] == '/')	// ignore '/*/'
								text += 3;
							else
								text += 2;
						}
						else
							text++;
						break;

					case '"':
						// Check for '"' or '\"'
						if (! (mText[text+1] == '\'' && mText[text-1] == '\'') &&	// '"'
							! (mText[text+1] == '\'' && mText[text-1] == '\\' && mText[text-2] == '\'') &&	// '\"'
							! (mText[text-1] == '\\'))		// \"`
						{
							styleChanged |= SetTextStyle(styleStart, text, TextStyle);
							currentState = StringStyle;
							styleStart = text;
						}
						
						text++;
						break;
					
					default:
						if (isalnum(c) || c == '_')
						{						
							temp = text;
							tokenLen = 0;
							
							// Copy token to buffer
							// We're only interested in tokens that can be keywords
							while (text < term && tokenLen < kMaxTokenLen && ((c = mText[text]) == '_' || isalnum(c)))		// skip to next token
							{
								token[tokenLen++] = c;
								text++;
							}
							token[tokenLen] = '\0';

							if (tokenLen < kMaxTokenLen)
							{
								if (IsKeyword(token))
								{
									SetTextStyle(styleStart, temp, TextStyle);				// end text
									styleChanged |= SetTextStyle(temp, text, KeywordStyle);	// set keyword
									currentState = TextStyle;
									styleStart = text;
								}
								else
								if (IsCustomKeyword(token))
								{
									SetTextStyle(styleStart, temp, TextStyle);				// end text
									styleChanged |= SetTextStyle(styleStart, text, CustomKeywordStyle);
									currentState = TextStyle;
									styleStart = text;
								}
							}
							else
							if (tokenLen == kMaxTokenLen)
							{
								while (text < term && ((c = mText[text]) == '_' || isalnum(c)))// skip to next token
									text++;
							}
						}
						else
							text++;
						break;
				}
				break;
			
			default:
				ASSERT(false);
				break;
		}
	}
	ASSERT(text <= term);

	if (styleStart != text)
	{
		text = min(text, term);
		styleChanged |= SetTextStyle(styleStart, text, currentState);
	}
	
	inoutCurrentState = currentState;

	return styleChanged;
}

// ---------------------------------------------------------------------------
//		IsPreprocessorKeyword
// ---------------------------------------------------------------------------

bool
MSyntaxStyler::IsPreprocessorKeyword(
	const char*		cptr,				// pointer to token
	int32			inOffset,			// offset of token in mText
	int32&			outLen,				// if the token is a preprocessor keyword, its length (including on/off/reset)
	bool&			outIncludeFound)	// is the preprocessor keyword '#include'?
{
	int32			offset;
	int32			n = 0;
	int32			whiteOne = 0;
	int32			whiteTwo = 0;
	int32			tokenLen;
	char			token[kMaxTokenLen+1];
	char			flags;
	int32			length;
	uchar			c = cptr[0];

	outIncludeFound = false;

	if (MKeywordList::Keywordlist(kCCppDirectivesIndex)->MKeywordList::IsKeyword(cptr, flags, length))
	{
		// Is it pragma?
		if ((flags & kIsPragmaFlag) != 0)
		{
			n = 6;
			offset = inOffset + n;
	
			while ((c = mText[offset]) == SPACE || c == TAB)
			{
				offset++;
				whiteOne++;
			}
	
			tokenLen = 0;
			while (tokenLen < kMaxTokenLen && ((c = mText[offset]) == '_' || isalnum(c)))		// skip to next token
			{
				token[tokenLen++] = c;
				offset++;
			}
			token[tokenLen] = '\0';
			
			cptr = token;
			
			// Is it one of the pragmas that takes on/off/reset/list?
			if (tokenLen < kMaxTokenLen &&
				MKeywordList::Keywordlist(kCCppPragmasOnOffResetIndex)->MKeywordList::IsKeyword(cptr, flags, length))
			{
				n += length;

				bool		allowlist = (flags & kAllowsListFlag) != 0;
				bool		onoffreset = (flags & kOnOffResetFlag) != 0;
				if (onoffreset || allowlist)
				{
					offset = inOffset + whiteOne + n;
					while ((c = mText[offset]) == SPACE || c == TAB)
					{
						offset++;
						whiteTwo++;
					}
	
					tokenLen = 0;
					while (tokenLen < kMaxTokenLen && ((c = mText[offset]) == '_' || isalnum(c)))		// skip to next token
					{
						token[tokenLen++] = c;
						offset++;
					}
					token[tokenLen] = '\0';
					
					switch (*cptr)
					{
						case 'l':
							if (allowlist && MWEdit_StrCmp("list", cptr))	{ n += 4; break; }
							break;
	
						case 'o':
							if (MWEdit_StrCmp("off", cptr))					{ n += 3; break; }
							if (MWEdit_StrCmp("on", cptr))					{ n += 2; break; }
							break;
	
						case 'r':
							if (MWEdit_StrCmp("reset", cptr))				{ n += 5; break; }
							break;
					}
				}
			}
		}
		else	// It's include or some other precompiler directive
		{
			if ((flags & kIsIncludeFlag) != 0)		// Is it include?
			{
				outIncludeFound = true;
			}
			
			n = length;
		}
	}

	if (n != 0)
	{
		outLen = n + whiteOne + whiteTwo;
		return true;
	}
	else
		return false;
	
}

#pragma mark -
#pragma mark ## Java ##

// ---------------------------------------------------------------------------
//		IsJavaKeyword
// ---------------------------------------------------------------------------

inline bool
MSyntaxStyler::IsJavaKeyword(
	const char*	cptr)
{
	char	flags;
	int32	length;
	
	return MKeywordList::Keywordlist(kJavaKeywordsIndex)->MKeywordList::IsKeyword(cptr, flags, length);
}

// ---------------------------------------------------------------------------
//		ParseTextJava
// ---------------------------------------------------------------------------

bool
MSyntaxStyler::ParseTextJava(
	int32			inOffset,
	int32			inLength,
	TextStateT&		inoutCurrentState)
{
	TextStateT		currentState = inoutCurrentState;
	int32			text = inOffset;
	int32			term = text + inLength;
	int32			styleStart = text;
	bool			styleChanged = false;
	char			token[kMaxTokenLen+1];
	int32			tokenLen;
	int32			temp;
	uchar			c;

	// Preamble for special cases
	switch (inoutCurrentState)
	{
		// need to increment if at the start of the string
		// but not if at the end of the string
		case StringStyle:
			if (mText[text] == '"' && OffsetIsStartOfStyle(text))
				text++;
			break;
	}

	// Parse the text
	while (text < term && mText[text] != '\0')
	{
		switch (currentState)
		{
			case CCommentStyle:
			{
				bool startOfStyleIsHere = this->OffsetIsStartOfStyle(styleStart);
				while (text < term && (c = mText[text]) != '\0' && 
					! (c == '*' && mText[text+1] == '/' && (text > styleStart + 1 || startOfStyleIsHere == false)))
					text++;

				if (text < term || (mText[text-2] == '*' && mText[text-1] == '/'))
				{
					currentState = TextStyle;
					text += 2;
				}
				styleChanged |= SetTextStyle(styleStart, text, CCommentStyle);
				styleStart = text;
				break;
			}
			
			case CppCommentStyle:
				while (text < term && (c = mText[text]) != EOL_CHAR && c != '\0')
					text++;

				styleChanged |= SetTextStyle(styleStart, text, CppCommentStyle);
				styleStart = text;
				currentState = TextStyle;
				break;

			case StringStyle:
				// look for '"' or "\\" and not '\"'
				while (text < term && ((c = mText[text]) != '\0' && c != '"'))
				{
					if (c == '\\')	// skip over any escaped chars (especially \")
						text += 2;
					else
						text++;
				}

				if (text < term)
					text++;
				styleChanged |= SetTextStyle(styleStart, text, StringStyle);
				styleStart = text;
				if (text < term)
					currentState = TextStyle;
				break;

			case KeywordStyle:
				tokenLen = 0;
				
				while (tokenLen < kMaxTokenLen && text < term && ((c = mText[text]) == '_' || isalnum(c)))		// skip to next token
				{
					token[tokenLen++] = c;
					text++;
				}
				token[tokenLen] = '\0';
	
				if (tokenLen < kMaxTokenLen && IsJavaKeyword(token))
				{
					styleChanged |= SetTextStyle(styleStart, text, KeywordStyle);
					styleStart = text;			
				}

				currentState = TextStyle;
				break;

			case NoneStyle:
			case TextStyle:
				while (text < term && ((c = mText[text]) == SPACE || c == TAB))
					text++;

				switch (c)
				{
					case EOL_CHAR:
						if (styleStart != text)
						{
							styleChanged |= SetTextStyle(styleStart, text, TextStyle);	// end text
							styleStart = ++text;
						}
						else
							text++;

						while (text < term && ((c = mText[text]) == SPACE || c == TAB))
							text++;
						
						if (c == '#' && text < term)
						{
							currentState = PreprocessorKeywordStyle;
							text++;
						}
						break;

					case '/':
						c = mText[text+1];
						if (c == '/')
						{
							styleChanged |= SetTextStyle(styleStart, text, TextStyle);
							currentState = CppCommentStyle;
							styleStart = text;
							text += 2;
						}
						else
						if (c == '*')
						{
							styleChanged |= SetTextStyle(styleStart, text, TextStyle);
							currentState = CCommentStyle;
							styleStart = text;
							if (mText[text+2] == '/')	// ignore '/*/'
								text += 3;
							else
								text += 2;
						}
						else
							text++;
						break;

					case '"':
						// Check for '"' or '\"'
						if (! (mText[text+1] == '\'' && mText[text-1] == '\'') &&	// '"'
							! (mText[text+1] == '\'' && mText[text-1] == '\\' && mText[text-2] == '\'') &&	// '\"'
							! (mText[text-1] == '\\'))		// \"`
						{
							styleChanged |= SetTextStyle(styleStart, text, TextStyle);
							currentState = StringStyle;
							styleStart = text;
						}
						
						text++;
						break;
					
					default:
						if (isalnum(c) || c == '_')
						{						
							temp = text;
							int32	tokenLen = 0;
							
							// Copy token to buffer
							// We're only interested in tokens that can be keywords
							while (text < term && tokenLen < kMaxTokenLen && ((c = mText[text]) == '_' || isalnum(c)))		// skip to next token
							{
								token[tokenLen++] = c;
								text++;
							}
							token[tokenLen] = '\0';

							if (tokenLen < kMaxTokenLen)
							{
								if (IsJavaKeyword(token))
								{
									SetTextStyle(styleStart, temp, TextStyle);				// end text
									styleChanged |= SetTextStyle(temp, text, KeywordStyle);	// set keyword
									currentState = TextStyle;
									styleStart = text;
								}
								else
								if (IsCustomKeyword(token))
								{
									SetTextStyle(styleStart, temp, TextStyle);				// end text
									styleChanged |= SetTextStyle(styleStart, text, CustomKeywordStyle);
									currentState = TextStyle;
									styleStart = text;
								}
							}
							else
							if (tokenLen == kMaxTokenLen)
							{
								while (text < term && (c = mText[text]) == '_' || isalnum(c))// skip to next token
									text++;
							}
						}
						else
							text++;
						break;
				}
				break;
			
			default:
				ASSERT(false);
				break;
		}
	}
	
	if (styleStart != text)
	{
		text = min(text, term);
		styleChanged |= SetTextStyle(styleStart, text, currentState);
	}
	
	inoutCurrentState = currentState;

	return styleChanged;
}
