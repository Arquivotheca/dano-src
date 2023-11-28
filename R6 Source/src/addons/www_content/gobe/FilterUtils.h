#ifndef __FILTERUTILS_H__
#define __FILTERUTILS_H__

#include <Font.h>
#include <UnicodeBlockObjects.h>

//
// FilterUtils.h
//
//	DON'T PUT ANY SQUIRREL OBJECTS IN HERE
//	It should all be generic Be stuff.
//  Include Messages.h if you need message ids, and Consts.h for constants from Squirrel
//

#define TWIPS_PER_POINT				20


//---------------------------------------------------------
//
// Font Stuff
//
//---------------------------------------------------------

// Our Font Family Codes
// These correspond to an RTF enum, so don't reorder them
enum {FFC_NIL = 0, FFC_ROMAN, FFC_SWISS, FFC_MODERN, FFC_SCRIPT, FFC_DECOR, FFC_TECH, FFC_BIDI, FFC_FAMILY_COUNT};


//-------------------------------------------------------------------
// TFontBE
//-------------------------------------------------------------------

class TFontBE
{
public:	
					TFontBE(BFont *font, font_family *beFontFamily, font_style *beFontStyle);
	virtual 		~TFontBE();
	
			char *	GetFontName(void)		{return mFontName;}
			uint32	GetFontID(void)			{return mFontID;}
			int32	GetFontFamilyCode(void)	{return mFontFamilyCode;}
			int32	GetCharSet(void)		{return mCharSet;}
			bool	IsFamilyDefault(void)	{return mFamilyDefault;}
	
private:
			void	setFontFamilyCode(font_family *beFontFamily);
				
	char *			mFontName;
	uint32			mFontID;			// this is the Be internal family and style id
	int32			mFontFamilyCode;	// this is a best guess as to what the family category is
	bool			mFamilyDefault;		// whether this is the default font for the font family code
	int32			mCharSet;
};


//-------------------------------------------------------------------
// TFontTable
//-------------------------------------------------------------------

class TFontTable
{
public:	
					TFontTable(void);
	virtual 		~TFontTable(void);

			int32	GetBeFontIndex(uint32 beFontID);
			int32	GetBeFontCharSet(uint32 beFontID);
			uint32	FindBestMatch(char *fontName, int32 fontFamilyCode, int32 charSet);
		
protected:	
			void	loadAllBeFonts();
			void	initDefaultFamilies();
			
	BList *			mFontTableBE;
	int32			mDefaultBeFontIndex;	
	uint32			mDefaultFamilyBeFontID[FFC_FAMILY_COUNT];
};

//---------------------------------------------------------
//
// UTF8 Stuff
//
//---------------------------------------------------------
#define CHARTYPE_LETTER_MASK 0x01		/* letter character class */
#define CHARTYPE_NUMBER_MASK 0x02		/* number character class */
#define CHARTYPE_PUNCT_MASK 0x04		/* punctuation character class */
#define CHARTYPE_SPACE_MASK 0x08		/* space character class */
#define CHARTYPE_LC_MASK 0x10			/* lower-case alpha char. class */
#define CHARTYPE_UC_MASK 0x20			/* upper-case alpha char. class */
#define CHARTYPE_VOWEL_MASK 0x40		/* vowel class */

// ISO Latin1 lookup table
extern uchar Latin1_CharClass[]; 

// Tells whether this byte is the first byte in a utf8 character. (Be Newsletter #75)
inline bool IS_CHAR_START_UTF8(uchar byte) 			{return (byte & 0xc0) != 0x80;}

// Tells whether this utf8 character is single or multi-byte
inline bool IS_SINGLE_BYTE_CHAR(uchar byte1)		{return !(byte1 & 0x80);}
inline bool IS_MULTI_BYTE_CHAR(uchar byte1)			{return (byte1 & 0x80);}

inline bool IS_CHAR_NUMBER(const uchar *byte1Ptr)			{return (IS_SINGLE_BYTE_CHAR(*byte1Ptr) && Latin1_CharClass[*byte1Ptr] & CHARTYPE_NUMBER_MASK);}

// Pierre's inline that returns the number of bytes that the utf8 character
// contains when passed the first byte (1-4).  (Be Newsletter#82)
inline uint32 CHAR_BYTES_UTF8(const uchar byte1) 	{return (((0xE5000000 >> ((byte1 >> 3) & 0x1E)) & 3) + 1);}


// Returns the number of characters in the string (not bytes)
uint32 GetStringCharCount(const char *cString);
uint32 GetStringCharCount(const char *cString, uint32 length);

// Converts a UTF8 character into a unicode value
ushort 	UTF8ToUnicode(const uchar* textPtr);

bool UTF8IsLegal(const uchar* textPtr, long byteCount);
bool IsCharLetter(const uchar *byte1Ptr, bool latin1Only = false);

#endif // __FILTERUTILS_H__
