#include <List.h>
#include <string.h>
#include <stdio.h>

#include "TranslationConsts.h"
#include "FilterUtils.h"

/* Character-class masks: */
#define A_ CHARTYPE_LETTER_MASK
#define D_ CHARTYPE_NUMBER_MASK
#define P_ CHARTYPE_PUNCT_MASK
#define S_ CHARTYPE_SPACE_MASK
#define L_ CHARTYPE_LC_MASK
#define U_ CHARTYPE_UC_MASK
#define V_ CHARTYPE_VOWEL_MASK

uchar Latin1_CharClass[256] =
{
	/* 00 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, S_, S_, 0, 0, S_, 0, 0,
	/* 10 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 20 */ S_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_,
	/* 30 */ D_, D_, D_, D_, D_, D_, D_, D_, D_, D_, P_, P_, P_, P_, P_, P_,
	/* 40 */ P_, A_|U_|V_, A_|U_, A_|U_, A_|U_, A_|U_|V_, A_|U_, A_|U_,
	/* 48 */ A_|U_, A_|U_|V_, A_|U_, A_|U_, A_|U_, A_|U_, A_|U_, A_|U_|V_,
	/* 50 */ A_|U_, A_|U_, A_|U_, A_|U_, A_|U_, A_|U_|V_, A_|U_, A_|U_,
	/* 58 */ A_|U_, A_|U_, A_|U_, P_, P_, P_, P_, P_,
	/* 60 */ P_, A_|L_|V_, A_|L_, A_|L_, A_|L_, A_|L_|V_, A_|L_, A_|L_,
	/* 68 */ A_|L_, A_|L_|V_, A_|L_, A_|L_, A_|L_, A_|L_, A_|L_, A_|L_|V_,
	/* 70 */ A_|L_, A_|L_, A_|L_, A_|L_, A_|L_, A_|L_|V_, A_|L_, A_|L_,
	/* 78 */ A_|L_, A_|L_, A_|L_, P_, P_, P_, P_, P_,
	/* 80 */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 90 */ 0, 0, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, 0, 0, 0,
	/* A0 */ 0, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_,
	/* B0 */ P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_, P_,
	/* C0 */ A_|U_|V_, A_|U_|V_, A_|U_|V_, A_|U_|V_, A_|U_|V_, A_|U_|V_, A_|U_|V_, A_|U_,
	/* C8 */ A_|U_|V_, A_|U_|V_, A_|U_|V_, A_|U_|V_, A_|U_|V_, A_|U_|V_, A_|U_|V_, A_|U_|V_,
	/* D0 */ A_|U_, A_|U_, A_|U_|V_, A_|U_|V_, A_|U_|V_, A_|U_|V_, A_|U_|V_, P_,
	/* D8 */ A_|U_|V_, A_|U_|V_, A_|U_|V_, A_|U_|V_, A_|U_|V_, A_|U_, A_|U_, A_|U_,
	/* E0 */ A_|L_|V_, A_|L_|V_, A_|L_|V_, A_|L_|V_, A_|L_|V_, A_|L_|V_, A_|L_|V_, A_|L_,
	/* E8 */ A_|L_|V_, A_|L_|V_, A_|L_|V_, A_|L_|V_, A_|L_|V_, A_|L_|V_, A_|L_|V_, A_|L_|V_, 
	/* F0 */ A_|L_|V_, A_|L_, A_|L_|V_, A_|L_|V_, A_|L_|V_, A_|L_|V_, A_|L_|V_, P_,
	/* F8 */ A_|L_|V_, A_|L_|V_, A_|L_|V_, A_|L_|V_, A_|L_|V_, A_|L_, A_|L_, A_|L_
};

//---------------------------------------------------------
//
// Font Stuff
//
//---------------------------------------------------------


//-------------------------------------------------------------------
// TFontBE
//-------------------------------------------------------------------

TFontBE::TFontBE(BFont *font, font_family *beFontFamily, font_style *beFontStyle)
{	
	mFontName = new char[strlen((char *)beFontFamily) + strlen((char *)beFontStyle) + 2];
	if (mFontName)
	{
		strcpy(mFontName, (char *)beFontFamily);
		strcat(mFontName, " ");
		strcat(mFontName, (char *)beFontStyle);
	}
	
	mFontID = font->FamilyAndStyle();
	mCharSet = 0;

	if (font->Blocks().Includes(B_KATAKANA_BLOCK))
		mCharSet = 128;
	
	setFontFamilyCode(beFontFamily);
}


TFontBE::~TFontBE(void)
{
	if (mFontName)
		delete [] mFontName;
}


void TFontBE::setFontFamilyCode(font_family *beFontFamily)
{
	mFontFamilyCode = FFC_NIL;
	mFamilyDefault = false;
	
	char *fontFamily = (char*)(&beFontFamily[0]);
	if (!fontFamily)
		return;
		
	// These are hard-coded Be font names that correspond to font family names.
	// Be doesn't give us a way a general way to get a font family code, 
	// which is why font names have to be entered.
	
	// ROMAN - proportionally spaced serif		
	if (!strcmp("Baskerville", fontFamily) ||
		!strcmp("ClassGarmnd BT", fontFamily) ||
		!strcmp("Dutch801 Rm BT", fontFamily))
	{
		mFontFamilyCode = FFC_ROMAN;
		if (!strcmp("Dutch801 Rm BT", fontFamily))
			mFamilyDefault = true;
		return;
	}
	
	// SWISS - proportionally spaced sans-serif
	if (!strcmp("Swis721 BT", fontFamily) ||
		!strcmp("Humnst777 BT", fontFamily) ||
		!strcmp("Monospac821 BT", fontFamily) ||
		!strcmp("ProFont", fontFamily) ||
		!strcmp("ProFontISOLatin 1", fontFamily) ||
		!strcmp("Swis911 XCm BT", fontFamily) ||
		!strcmp("VAGRounded BT", fontFamily))
	{
		mFontFamilyCode = FFC_SWISS;
		if (!strcmp("Swis721 BT", fontFamily))
			mFamilyDefault = true;
		return;
	}
	
	// MODERN - fixed pitch serif and sans-serif
	if (!strcmp("Courier 10 BT", fontFamily))
	{
		mFontFamilyCode = FFC_MODERN;
		// no default needed here as it will always be be_fixed_font
		return;
	}
	
	// SCRIPT
	if (!strcmp("CommercialScript", fontFamily) ||
		!strcmp("Embassy", fontFamily))
	{
		mFontFamilyCode = FFC_SCRIPT;
		mFamilyDefault = true;
		return;
	}
	
	// DECOR - 'Old English' & 'ITC Zapf Chancery'
	if (!strcmp("CopprplGoth Bd BT", fontFamily))
	{
		mFontFamilyCode = FFC_DECOR;
		mFamilyDefault = true;
		return;
	}
	
	// TECH - technical, symbol, and mathematical
	if (!strcmp("SymbolProp BT", fontFamily))
	{
		mFontFamilyCode = FFC_TECH;
		mFamilyDefault = true;
		return;
	}
		
	// BIDI - arabic, hebrew, or bidirectional
}


//-------------------------------------------------------------------
// TFontTable
//-------------------------------------------------------------------

TFontTable::TFontTable(void)
{
	mDefaultBeFontIndex = 0;
			
	mFontTableBE = new BList(50);
		
	// Load all of the BE fonts
	loadAllBeFonts();
	
	// init the table of Be font ids that are family defaults
	initDefaultFamilies();
}


TFontTable::~TFontTable(void)
{
	if (mFontTableBE)
	{	
		for (long i = mFontTableBE->CountItems() - 1; i >= 0; i--)
			delete (TFontBE *)(mFontTableBE->ItemAt(i));
		
		delete mFontTableBE;
	}
}


// Fill a table once with all of the loaded Be fonts, so that we don't have to repeatedly load them.
void TFontTable::loadAllBeFonts(void)
{
	if (!mFontTableBE)
		return;
			
	font_family	family; 
	font_style	style; 
	uint32		flags;
	int32		numStyles; 
	int32		numFamilies = count_font_families();
	BFont		font;
	TFontBE	*	fontBE;
	 
	for (int32 i = 0; i < numFamilies; i++)
	{ 
		if (get_font_family(i, &family, &flags) == B_OK)
		{ 
			numStyles = count_font_styles(family);
			
			for (int32 j = 0; j < numStyles; j++ ) 
			{ 
				if (get_font_style(family, j, &style, &flags) == B_OK) 
				{
					font.SetFamilyAndStyle(family, style);
					fontBE = new TFontBE(&font, &family, &style);
					mFontTableBE->AddItem(fontBE); 
		
					if (fontBE->GetFontID() == be_plain_font->FamilyAndStyle())
						mDefaultBeFontIndex = mFontTableBE->CountItems() - 1;
				} 
			} 
		} 
	}
}


// For each font family code, pick a Be font be the default
void TFontTable::initDefaultFamilies(void)
{
	// default all to the Be plain font
	for (long i = 0; i < FFC_FAMILY_COUNT; i++)
		mDefaultFamilyBeFontID[i] = be_plain_font->FamilyAndStyle();
	
	// loop thru all of the Be fonts, and find the ones that are family defaults
	TFontBE	*	fontBE;
	for (int32 i = mFontTableBE->CountItems() - 1; i >= 0; i--)
	{
		fontBE = (TFontBE *)(mFontTableBE->ItemAt(i));
		
		if (fontBE->IsFamilyDefault())
			mDefaultFamilyBeFontID[fontBE->GetFontFamilyCode()] = fontBE->GetFontID();
	}
	
	// MODERN doesn't have default, but is always be_fixed_font	
	mDefaultFamilyBeFontID[FFC_MODERN] = be_fixed_font->FamilyAndStyle();
}


// Find the Be font that closest matches this utf font name and RTF style family code and charSet
uint32 TFontTable::FindBestMatch(char *fontName, int32 fontFamilyCode, int32 charSet)
{
	// make sure that the fontFamilyCode is valid since it may be used in array lookup below
	if (fontFamilyCode < 0 || fontFamilyCode >= FFC_FAMILY_COUNT)
	{
		IFDEBUG(fprintf(stderr, "WORD FindBestMatch: ERROR bad family: %d\n", fontFamilyCode));
		fontFamilyCode = FFC_NIL;
	}

	TFontBE	*	fontBE;
	TFontBE *	fontMatchCharSet = 0;

	// first see if there is a straight match on the name
	for (int32 i = mFontTableBE->CountItems() - 1; i >= 0; i--)
	{
		fontBE = (TFontBE *)(mFontTableBE->ItemAt(i));
		if (fontName && !strcmp(fontName, fontBE->GetFontName()))
			return fontBE->GetFontID();
						
		if (charSet && !fontMatchCharSet && fontBE->GetCharSet() == charSet)
			fontMatchCharSet = fontBE;
	}		
	
	// No name match at this point
	
	// If we matched a char set, then return that font					
	if (fontMatchCharSet)
		return fontMatchCharSet->GetFontID();
	
	// We could possibly do a strstr partial string match on part of the names
	// to see if they match, but I don't think that's worth it.
	
	// Use the font family code to get the default for that family
	return mDefaultFamilyBeFontID[fontFamilyCode];
}


// Look thru the table of be fonts to find the one that matches this beFontID
int32 TFontTable::GetBeFontIndex(uint32 beFontID)
{
	TFontBE *	fontBE;
	
	for (int32 i = 0; i < mFontTableBE->CountItems(); i++)
	{
		fontBE = (TFontBE *)(mFontTableBE->ItemAt(i));
		if (beFontID == fontBE->GetFontID())
			return i;
	}
	
	// Didn't find this font id in the table, so give default font index.	
	return mDefaultBeFontIndex;
}


// Look thru the table of be fonts to find the one that matches this beFontID, and return char set
int32 TFontTable::GetBeFontCharSet(uint32 beFontID)
{
	int32		index = GetBeFontIndex(beFontID);
	TFontBE *	fontBE = (TFontBE *)(mFontTableBE->ItemAt(index));
	
	return fontBE->GetCharSet();
}


//---------------------------------------------------------
//
// UTF8 Stuff
//
//---------------------------------------------------------


// From Be Newsletter#82.  Given a null-terminated string of characters,
// will return the number of characters (not bytes) in the string.
uint32 GetStringCharCount(const char *cString)
{
	uint32 count = 0;
	while (*cString) 
	{
		if (IS_CHAR_START_UTF8(*cString++))
			count++;
	}
	
	return count;
}


uint32 GetStringCharCount(const char *cString, uint32 length)
{
	uint32 count = 0;
	while (length)
	{
		if (IS_CHAR_START_UTF8(*cString++))
			count++;
		length--;	
	}
	return count;
}


// Bogus -- This code is also in the application.  Should use a common file or something
bool UTF8IsLegal(const uchar* textPtr, long byteCount)
{
	const uchar* start = textPtr;
	while (byteCount > 0)
	{
		if (!IS_CHAR_START_UTF8(*textPtr))
			return false;

		long charBytes = CHAR_BYTES_UTF8(*textPtr);
		textPtr++;
		byteCount--;

		for (int x = 1; x < charBytes; x++)
		{
			if (byteCount <= 0)
				return false;

			if ((*textPtr & 0xC0) != 0x80)
				return false;
			textPtr++;
			byteCount--;
		}
	}
	return true;
}

ushort UTF8ToUnicode(const uchar* textPtr)
{
	long charBytes = CHAR_BYTES_UTF8(*textPtr);
	
	switch (charBytes)
	{
		case 1:
			return *textPtr;
			
		case 2:
			return ((ushort)(textPtr[0] & 0x1F) << 6) | (textPtr[1] & 0x3F);
			
		case 3:
			return ((ushort)(textPtr[0] & 0x0F) << 12) | ((ushort)(textPtr[1] & 0x3F) << 6) | (textPtr[2] & 0x3F);
			
		default:
			DEBUGSTR("Bad utf8 character");
	
	}
	return 0;
}

bool IsCharLetter(const uchar *byte1Ptr, bool latin1Only)
{
	if (IS_SINGLE_BYTE_CHAR(*byte1Ptr))
		return (Latin1_CharClass[*byte1Ptr] & CHARTYPE_LETTER_MASK);
		
	// It's multi-byte
	// These come in order, so utf8 0xC280-C2BF map to latin1 0x80-BF, and
	// 0xC380-C3BF map to latin1 0xC0-FF
	if (*byte1Ptr == 0xC2)
		return (Latin1_CharClass[*(++byte1Ptr)] & CHARTYPE_LETTER_MASK);
		
	if (*byte1Ptr == 0xC3)
		return (Latin1_CharClass[((*(++byte1Ptr)) | 0x60)] & CHARTYPE_LETTER_MASK);
		
	if (latin1Only)
		return false;
		
	ushort c = UTF8ToUnicode(byte1Ptr);
	
	// Exclude these unicode ranges, and assume everything else is a letter.
	// It's not 100%, but unless we can query each unicode character to find out
	// it's type, then we can't be perfect.
	if ((c >= 0x02B0 && c <= 0x02FF) ||		// Spacing Modifier Letters
		(c >= 0x0300 && c <= 0x036F) ||		// Combining Diacritical Marks
		(c >= 0x2000 && c <= 0x2FFF))		// General Punctuation - CJK
		return false;
	
	return true;
}
