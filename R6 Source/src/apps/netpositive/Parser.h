// ===========================================================================
//	Parser.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __PARSER__
#define __PARSER__

#include "UResource.h"
#ifdef JAVASCRIPT
#include "Store.h"
#endif

#include <SupportDefs.h>
#include <String.h>

class Tag;
class DocumentBuilder;

long ParseColor(const char *valueStr);

// ===========================================================================
//	A char oriented string class,
//	BigStr uses pools (less malloc intensive than string class)

class BigStr {
public:
			BigStr();
virtual		~BigStr();
			
			void	Add(char c);
			void	AddStr(char *s);
			char*	Chars() {return mStr;}
			char	Char(const int index);
			int		Count() {return mCount;}			// Not good after finish
			int		Finish();
protected:
			char*	mStr;
			long	mCount;
			long	mSize;
};

// ============================================================================
//	Return a char for a &tidle; like string in d

int32 AmpersandCharacter(const char* d, bool* cantBe, uint32 encoding);

// ============================================================================
//	The Parser interprets HTML and sends data to a PageBuilder

class Parser : public Consumer {
public:
					Parser(DocumentBuilder *builder, uint32 encoding);
					
	void			SetEncoding(uint32 encoding);

			void	IgnoreTags();
			
	virtual	long	Write(uchar *d, long count);
	virtual	bool	Complete();
	virtual void	SetComplete(bool complete);
#ifdef JAVASCRIPT
			// This method is for implementing document.Write.  You don't want to just call
			// the normal Write in this case, because we could already be in the middle of a write,
			// and if the write dispatches a tag that invokes JavaScript, we're hosed.  For
			// these cases, store all of the bits in a temporary buffer and then return; the normal
			// Write routine will pick up the bits during its next time around.
			
			// Don't call this routine if Write currently isn't running, as the bits will get
			// dropped on the floor.
			void	ReentrantWrite(uchar *d, long count);
#endif
	
protected:
	virtual			~Parser();
			short	GetTagID();
			char*	NextAttrStrs(char *s, long *attributeID, char **valueStr, char **attrStr);
			Tag*	BuildTag(short tagID);
			void	CheckTag(short tagID);

			void 	AppendText(const char *c, size_t len);
			void 	AppendText(char c);

	virtual	bool	DispatchTag();
	virtual	void	DispatchText();

			void	ConvertTextToUTF8(const char *text, short length,
									  BString &convertedText);

			void ConvertTextFromEncoding(BString &result, const char *text, int32 length);

			enum {
				kMaxTagLength = 2500,
				kMaxTextLength = 8192
			};

			uint32	mEncoding;
			bool	mInJIS;

			DocumentBuilder*	mBuilder;	// Destination for parsed info

			bool	mInOption;
			bool	mInQuote;
			bool	mInBody;
			bool	mInTag;
			bool	mStripSpaces;
			bool	mIgnoreTags;
			bool	mInScript;
			int		mInNoScriptPos;

			//int mCommentBeginCount;
			//int mCommentEndCount;
			bool mInTagSingleQuote;
			bool mInTagDoubleQuote;
			int mLastChar;
			bool mLastCharNotSpace;

			int	mPRECount;
			bool	mInTextArea;
			bool	mInStyle;
			bool	mInSelect;

			bool	mInAmpersand;
			bool	mIgnoreSemiColon;
			short	mAmpersandCount;
			char	mAmpersand[16];

			BigStr	mTag;
			
			char	mText[kMaxTextLength];	// What is the biggest piece of text? ¥¥¥¥¥¥¥¥¥¥¥¥
			short	mTextCount;

			enum {
				kNoComment,
				kStandardComment,
				kNetscapeComment
			};			

			int		mCommentState;
			BString	mNetscapeCommentText;
#ifdef JAVASCRIPT
			BucketStore *mTempBuffer;
#endif
};

#endif
