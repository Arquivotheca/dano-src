/*******************************************************************************
/
/	File:			UnicodeBlockObjects.h
/
/   Description:    Predefined unicode_block objects
/
/	Copyright 1998, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef _UNICODEBLOCKOBJECTS_H
#define _UNICODEBLOCKOBJECTS_H

#include <Font.h>


/* Unicode block list with their unicode encoding range	*/       
const unicode_block B_BASIC_LATIN_BLOCK(					/* 0000 - 007F			*/				0x0000000000000000LL, 0x0000000000000001LL);			
const unicode_block B_LATIN1_SUPPLEMENT_BLOCK(				/* 0080 - 00FF			*/				0x0000000000000000LL, 0x0000000000000002LL);			
const unicode_block B_LATIN_EXTENDED_A_BLOCK(				/* 0100 - 017F			*/				0x0000000000000000LL, 0x0000000000000004LL);			
const unicode_block B_LATIN_EXTENDED_B_BLOCK(				/* 0180 - 024F			*/				0x0000000000000000LL, 0x0000000000000008LL);			
const unicode_block B_IPA_EXTENSIONS_BLOCK(					/* 0250 - 02AF			*/				0x0000000000000000LL, 0x0000000000000010LL);			
const unicode_block B_SPACING_MODIFIER_LETTERS_BLOCK(		/* 02B0 - 02FF			*/				0x0000000000000000LL, 0x0000000000000020LL);			
const unicode_block B_COMBINING_DIACRITICAL_MARKS_BLOCK(	/* 0300 - 036F			*/				0x0000000000000000LL, 0x0000000000000040LL);			
const unicode_block B_BASIC_GREEK_BLOCK(					/* 0370 - 03CF			*/				0x0000000000000000LL, 0x0000000000000080LL);			
const unicode_block B_GREEK_SYMBOLS_AND_COPTIC_BLOCK(		/* 03D0 - 03FF			*/				0x0000000000000000LL, 0x0000000000000100LL);			
const unicode_block B_CYRILLIC_BLOCK(						/* 0400 - 04FF			*/				0x0000000000000000LL, 0x0000000000000200LL);			
const unicode_block B_ARMENIAN_BLOCK(						/* 0530 - 058F			*/				0x0000000000000000LL, 0x0000000000000400LL);			
const unicode_block B_BASIC_HEBREW_BLOCK(					/* 0590 - 05CF			*/				0x0000000000000000LL, 0x0000000000000800LL);			
const unicode_block B_HEBREW_EXTENDED_BLOCK(				/* 05D0 - 05FF			*/				0x0000000000000000LL, 0x0000000000001000LL);			
const unicode_block B_BASIC_ARABIC_BLOCK(					/* 0600 - 0670			*/				0x0000000000000000LL, 0x0000000000002000LL);			
const unicode_block B_ARABIC_EXTENDED_BLOCK(				/* 0671 - 06FF			*/				0x0000000000000000LL, 0x0000000000004000LL);			
//const unicode_block B_SYRIAC_BLOCK(							/* 0700 - 074F			*/				0x0000000000000000LL, 0x0000000000002000LL);			
//const unicode_block B_THAANA_BLOCK(							/* 0780 - 07BF			*/				0x0000000000000000LL, 0x0000000000002000LL);			
const unicode_block B_DEVANAGARI_BLOCK(						/* 0900 - 097F			*/				0x0000000000000000LL, 0x0000000000008000LL);			
const unicode_block B_BENGALI_BLOCK(						/* 0980 - 09FF			*/				0x0000000000000000LL, 0x0000000000010000LL);			
const unicode_block B_GURMUKHI_BLOCK(						/* 0A00 - 0A7F			*/				0x0000000000000000LL, 0x0000000000020000LL);			
const unicode_block B_GUJARATI_BLOCK(						/* 0A80 - 0AFF			*/				0x0000000000000000LL, 0x0000000000040000LL);			
const unicode_block B_ORIYA_BLOCK(							/* 0B00 - 0B7F			*/				0x0000000000000000LL, 0x0000000000080000LL);			
const unicode_block B_TAMIL_BLOCK(							/* 0B80 - 0BFF			*/				0x0000000000000000LL, 0x0000000000100000LL);			
const unicode_block B_TELUGU_BLOCK(							/* 0C00 - 0C7F			*/				0x0000000000000000LL, 0x0000000000200000LL);			
const unicode_block B_KANNADA_BLOCK(						/* 0C80 - 0CFF			*/				0x0000000000000000LL, 0x0000000000400000LL);			
const unicode_block B_MALAYALAM_BLOCK(						/* 0D00 - 0D7F			*/				0x0000000000000000LL, 0x0000000000800000LL);			
//const unicode_block B_SINHALA_BLOCK(							/* 0D80 - 0DFF			*/				0x0000000000000000LL, 0x0000000001000000LL);			
const unicode_block B_THAI_BLOCK(							/* 0E00 - 0E7F			*/				0x0000000000000000LL, 0x0000000001000000LL);			
const unicode_block B_LAO_BLOCK(							/* 0E80 - 0EFF			*/				0x0000000000000000LL, 0x0000000002000000LL);			
const unicode_block B_BASIC_GEORGIAN_BLOCK(					/* 10A0 - 10CF			*/				0x0000000000000000LL, 0x0000000004000000LL);			
const unicode_block B_GEORGIAN_EXTENDED_BLOCK(				/* 10D0 - 10FF			*/				0x0000000000000000LL, 0x0000000008000000LL);			
const unicode_block B_HANGUL_JAMO_BLOCK(					/* 1100 - 11FF			*/				0x0000000000000000LL, 0x0000000010000000LL);			
const unicode_block B_LATIN_EXTENDED_ADDITIONAL_BLOCK(		/* 1E00 - 1EFF			*/				0x0000000000000000LL, 0x0000000020000000LL);			
const unicode_block B_GREEK_EXTENDED_BLOCK(					/* 1F00 - 1FFF			*/				0x0000000000000000LL, 0x0000000040000000LL);			
const unicode_block B_GENERAL_PUNCTUATION_BLOCK(			/* 2000 - 206F			*/				0x0000000000000000LL, 0x0000000080000000LL);			
const unicode_block B_SUPERSCRIPTS_AND_SUBSCRIPTS_BLOCK(	/* 2070 - 209F			*/				0x0000000000000000LL, 0x0000000100000000LL);			
const unicode_block B_CURRENCY_SYMBOLS_BLOCK(				/* 20A0 - 20CF			*/				0x0000000000000000LL, 0x0000000200000000LL);			
const unicode_block B_COMBINING_MARKS_FOR_SYMBOLS_BLOCK(	/* 20D0 - 20FF			*/				0x0000000000000000LL, 0x0000000400000000LL);			
const unicode_block B_LETTERLIKE_SYMBOLS_BLOCK(				/* 2100 - 214F			*/				0x0000000000000000LL, 0x0000000800000000LL);			
const unicode_block B_NUMBER_FORMS_BLOCK(					/* 2150 - 218F			*/				0x0000000000000000LL, 0x0000001000000000LL);			
const unicode_block B_ARROWS_BLOCK(							/* 2190 - 21FF			*/				0x0000000000000000LL, 0x0000002000000000LL);			
const unicode_block B_MATHEMATICAL_OPERATORS_BLOCK(			/* 2200 - 22FF			*/				0x0000000000000000LL, 0x0000004000000000LL);			
const unicode_block B_MISCELLANEOUS_TECHNICAL_BLOCK(		/* 2300 - 23FF			*/				0x0000000000000000LL, 0x0000008000000000LL);			
const unicode_block B_CONTROL_PICTURES_BLOCK(				/* 2400 - 243F			*/				0x0000000000000000LL, 0x0000010000000000LL);			
const unicode_block B_OPTICAL_CHARACTER_RECOGNITION_BLOCK(	/* 2440 - 245F			*/				0x0000000000000000LL, 0x0000020000000000LL);			
const unicode_block B_ENCLOSED_ALPHANUMERICS_BLOCK(			/* 2460 - 24FF			*/				0x0000000000000000LL, 0x0000040000000000LL);			
const unicode_block B_BOX_DRAWING_BLOCK(					/* 2500 - 257F			*/				0x0000000000000000LL, 0x0000080000000000LL);			
const unicode_block B_BLOCK_ELEMENTS_BLOCK(					/* 2580 - 259F			*/				0x0000000000000000LL, 0x0000100000000000LL);			
const unicode_block B_GEOMETRIC_SHAPES_BLOCK(				/* 25A0 - 25FF			*/				0x0000000000000000LL, 0x0000200000000000LL);			
const unicode_block B_MISCELLANEOUS_SYMBOLS_BLOCK(			/* 2600 - 26FF			*/				0x0000000000000000LL, 0x0000400000000000LL);			
const unicode_block B_DINGBATS_BLOCK(						/* 2700 - 27BF			*/				0x0000000000000000LL, 0x0000800000000000LL);			
const unicode_block B_CJK_SYMBOLS_AND_PUNCTUATION_BLOCK(	/* 3000 - 303F			*/				0x0000000000000000LL, 0x0001000000000000LL);			
const unicode_block B_HIRAGANA_BLOCK(						/* 3040 - 309F			*/				0x0000000000000000LL, 0x0002000000000000LL);			
const unicode_block B_KATAKANA_BLOCK(						/* 30A0 - 30FF			*/				0x0000000000000000LL, 0x0004000000000000LL);			
const unicode_block B_BOPOMOFO_BLOCK(						/* 3100 - 312F			*/				0x0000000000000000LL, 0x0008000000000000LL);			
const unicode_block B_HANGUL_COMPATIBILITY_JAMO_BLOCK(		/* 3130 - 318F			*/				0x0000000000000000LL, 0x0010000000000000LL);			
const unicode_block B_CJK_MISCELLANEOUS_BLOCK(				/* 3190 - 319F			*/				0x0000000000000000LL, 0x0020000000000000LL);			
const unicode_block B_ENCLOSED_CJK_LETTERS_AND_MONTHS_BLOCK(/* 3200 - 32FF			*/				0x0000000000000000LL, 0x0040000000000000LL);			
const unicode_block B_CJK_COMPATIBILITY_BLOCK(				/* 3300 - 33FF			*/				0x0000000000000000LL, 0x0080000000000000LL);			
const unicode_block B_HANGUL_BLOCK(							/* AC00 - D7AF			*/				0x0000000000000000LL, 0x0100000000000000LL);			
const unicode_block B_HIGH_SURROGATES_BLOCK(				/* D800 - DBFF			*/				0x0000000000000000LL, 0x0200000000000000LL);			
const unicode_block B_LOW_SURROGATES_BLOCK(					/* DC00 - DFFF			*/				0x0000000000000000LL, 0x0400000000000000LL);			
const unicode_block B_CJK_UNIFIED_IDEOGRAPHS_BLOCK(			/* 4E00 - 9FFF			*/				0x0000000000000000LL, 0x0800000000000000LL);			
const unicode_block B_PRIVATE_USE_AREA_BLOCK(				/* E000 - F8FF			*/				0x0000000000000000LL, 0x1000000000000000LL);			
const unicode_block B_CJK_COMPATIBILITY_IDEOGRAPHS_BLOCK(	/* F900 - FAFF			*/				0x0000000000000000LL, 0x2000000000000000LL);			
const unicode_block B_ALPHABETIC_PRESENTATION_FORMS_BLOCK(	/* FB00 - FB4F			*/				0x0000000000000000LL, 0x4000000000000000LL);			
const unicode_block B_ARABIC_PRESENTATION_FORMS_A_BLOCK(	/* FB50 - FDFF			*/				0x0000000000000000LL, 0x8000000000000000LL);			
const unicode_block B_COMBINING_HALF_MARKS_BLOCK(			/* FE20 - FE2F			*/				0x0000000000000001LL, 0x0000000000000000LL);			
const unicode_block B_CJK_COMPATIBILITY_FORMS_BLOCK(		/* FE30 - FE4F			*/				0x0000000000000002LL, 0x0000000000000000LL);			
const unicode_block B_SMALL_FORM_VARIANTS_BLOCK(			/* FE50 - FE6F			*/				0x0000000000000004LL, 0x0000000000000000LL);			
const unicode_block B_ARABIC_PRESENTATION_FORMS_B_BLOCK(	/* FE70 - FEFE			*/				0x0000000000000008LL, 0x0000000000000000LL);			
const unicode_block B_HALFWIDTH_AND_FULLWIDTH_FORMS_BLOCK(	/* FF00 - FFEF			*/				0x0000000000000010LL, 0x0000000000000000LL);			
const unicode_block B_SPECIALS_BLOCK(						/* FEFF and FFF0 - FFFF	*/				0x0000000000000020LL, 0x0000000000000000LL);			
const unicode_block B_TIBETAN_BLOCK(						/* 0F00 - 0FFF			*/				0x0000000000000040LL, 0x0000000000000000LL);			
//const unicode_block B_MYANMAR_BLOCK(						/* 1000 - 109F			*/				0x0000000000000040LL, 0x0000000000000000LL);			


#endif /* _UNICODEBLOCKOBJECTS_H */
