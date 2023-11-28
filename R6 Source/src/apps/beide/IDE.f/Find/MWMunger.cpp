/************************************************************************/
/*	Project...: C++ and ANSI-C Compiler Environment 					*/
/*	Name......: MWMunger.c												*/
/*	Purpose...: Munger functions										*/
/*	Copyright.: ©Copyright 1993 by metrowerks inc. All rights reserved. */
/************************************************************************/

//#include "MWSystem.h"
#include "MWMunger.h"
//#include "MWStrings.h"

#include <ctype.h>
#include <string.h>

#define NUL 	 0
#define SQUARE 255
typedef unsigned char String255[256];
typedef unsigned char CharArray[256];

unsigned char __CUppr[] = {
/*	 0	 1	 2	 3	 4	 5	 6	 7	 8	 9	 A	 B	 C	 D	 E	 F	*/
	000,001,002,003,004,005,006,007,010,011,012,013,014,015,016,017,
	020,021,022,023,024,025,026,027,030,031,032,033,034,035,036,037,
	' ','!','"','#','$','%','&',047,'(',')','*','+',',','-','.','/',
	'0','1','2','3','4','5','6','7','8','9',':',';','<','=','>','?',
	'@','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',
	'P','Q','R','S','T','U','V','W','X','Y','Z','[','\\',']','^','_',
	'`','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',
	'P','Q','R','S','T','U','V','W','X','Y','Z','{','|','}','~',0x7F,
	'Ä','Å','Ç','É','Ñ','Ö','Ü','Á','À','Â','Ä','Ã','Å','Ç','É','È',
	'Ê','Ë','Í','Ì','Î','Ï','Ñ','Ó','Ò','Ô','Ö','õ','Ú','Ù','Û','Ü',
	'†','°','¢','£','§','•','¶','ß','®','©','™','´','¨','≠','Æ','Ø',
	'∞','±','≤','≥','¥','µ','∂','∑','∏','π','∫','ª','º','Ω','Æ','Ø',
	'¿','¡','¬','√','ƒ','≈','∆','«','»','…',0xCA,'À','Ã','Õ','Œ','Œ',
	'–','—','“','”','‘','’','÷','◊','Ÿ','Ÿ','⁄','€','‹','›','ﬁ','ﬂ',
	'‡','·','‚','„','‰','Â','Ê','Á','Ë','È','Í','Î','Ï','Ì','Ó','Ô',
	'','Ò','Ú','Û','Ù','ı','ˆ','˜','¯','˘','˙','˚','¸','˝','˛','ˇ',
};

/****************************************************************/
/* Purpose..: Check if the character is a delimitators. 		*/
/* Input....: a character										*/
/* Returns..: TRUE/FALSE										*/
/****************************************************************/
static bool IsDelimitators(char ch)
{
//	if (isalphanumeric(ch) || ch == '_')
	if (isalnum(ch) || ch == '_')			// Are these the same??
		return(FALSE);
	return(TRUE);
}

/****************************************************************/
/* Purpose..: Reverse a string. 								*/
/* Input....: in string 										*/
/* Input....: out string										*/
/* Input....: string length 									*/
/* Returns..: ---												*/
/****************************************************************/
static void ReverseString(unsigned char in[], unsigned char out[], int32 length)
{
	int32 i;

	for (i = 0; i < length; i++) {
		out[length - i - 1] = in[i];
	}
	out[length] = '\0';
}

/****************************************************************/
/* Purpose..: Uppercase a string.								*/
/* Input....: string pointer									*/
/* Input....: string length 									*/
/* Returns..: ---												*/
/****************************************************************/
static void UppercaseString(unsigned char string[], int32 length)
{
	int32 i;
	unsigned char *p = __CUppr;

	for (i = 0; i < length; i++) {
		string[i] = p[string[i]];
	}
	string[length] = '\0';
}

/****************************************************************/
/* Purpose..: Initialize the skip table.						*/
/* Input....: pattern											*/
/* Input....: skiptable 										*/
/* Returns..: ---												*/
/****************************************************************/
static void ComputeSkip(unsigned char pattern[], CharArray skipTable)
{
	int32 j;
	int32 patternLength;
	int ch;

	patternLength = strlen((char *)pattern);

	for (ch = NUL; ch <= SQUARE; ch++) {
		skipTable[ch] = patternLength;
	}
	for (j = 0; j < patternLength; j++) {
		skipTable[pattern[j]] = patternLength - j - 1;
	}
}

/****************************************************************/
/* Purpose..: Search forward for 'pattern' in 'text'.			*/
/* Input....: pattern											*/
/* Input....: patternLength 									*/
/* Input....: text												*/
/* Input....: textLength										*/
/* Input....: offset											*/
/* Input....: caseSensitive 									*/
/* Returns..: '-1' if not found else position					*/
/****************************************************************/
static int32 BoyerMoore(unsigned char pattern[], int32 patternLength, unsigned char text[],
					   int32 textLength, int32 offset, bool caseSensitive)
{
	CharArray skipTable;
	int32 textIndex;
	int32 patternIndex;
	unsigned char *p = __CUppr;

	if (!caseSensitive) {
		UppercaseString(pattern, patternLength);
	}
	ComputeSkip(pattern, skipTable);

	textIndex = offset + patternLength - 1L;
	if (textIndex > textLength) {
		return(-1);
	}
	patternIndex = patternLength - 1L;
	if (caseSensitive) {
		do {
			if (text[textIndex] == pattern[patternIndex]) {
				textIndex--;
				patternIndex--;
			} else {
				if (patternLength - patternIndex >
					skipTable[text[textIndex]]) {
					textIndex = textIndex + patternLength - patternIndex;
				} else {
					textIndex = textIndex + skipTable[text[textIndex]];
				}
				patternIndex = patternLength - 1L;
			}
		} while (!((patternIndex < 0L) || (textIndex >= textLength)));
	} else {
		do {
			if (p[text[textIndex]] == pattern[patternIndex]) {
				textIndex--;
				patternIndex--;
			} else {
				if (patternLength - patternIndex >
					skipTable[p[text[textIndex]]]) {
					textIndex = textIndex + patternLength - patternIndex;
				} else {
					textIndex = textIndex +
								skipTable[p[text[textIndex]]];
				}
				patternIndex = patternLength - 1L;
			}
		} while (!((patternIndex < 0L) || (textIndex >= textLength)));
	}
	if (patternIndex < 0L) {
		return(textIndex + 1L);
	} else {
		return(-1L);
	}
}

/****************************************************************/
/* Purpose..: Search backward for 'pattern' in 'text'.			*/
/* Input....: pattern											*/
/* Input....: patternLength 									*/
/* Input....: text												*/
/* Input....: textLength										*/
/* Input....: offset											*/
/* Input....: caseSensitive 									*/
/* Returns..: '-1' if not found else position					*/
/****************************************************************/
static int32 ReverseBoyerMoore(unsigned char pattern[], int32 patternLength, unsigned char text[],
							  int32 	/*textLength*/,	/* not used */
		int32 offset, bool caseSensitive)
{
	CharArray skipTable;
	int32 textIndex;
	int32 patternIndex;
	String255 reversePattern;
	unsigned char *p = __CUppr;

	if (!caseSensitive) {
		UppercaseString(pattern, patternLength);
	}
	ReverseString(pattern, reversePattern, patternLength);

	ComputeSkip(reversePattern, skipTable);

	textIndex = offset - patternLength;
	if (textIndex < 0L) {
		return(-1L);
	}
	patternIndex = 0L;

	if (caseSensitive) {
		do {
			if (text[textIndex] == pattern[patternIndex]) {
				textIndex++;
				patternIndex++;

			} else {
				if (patternIndex + 1L > skipTable[text[textIndex]]) {
					textIndex = textIndex - (patternIndex + 1L);
				} else {
					textIndex = textIndex - skipTable[text[textIndex]];
				}
				patternIndex = 0L;

			}
		} while (!((patternIndex >= patternLength) || (textIndex < 0L)));
	} else {
		do {
			if (p[text[textIndex]] == pattern[patternIndex]) {
				textIndex++;
				patternIndex++;

			} else {
				if (patternIndex + 1 > skipTable[p[text[textIndex]]]) {
					textIndex = textIndex - (patternIndex + 1L);
				} else {
					textIndex = textIndex -
								skipTable[p[text[textIndex]]];
				}
				patternIndex = 0L;

			}
		} while (!((patternIndex >= patternLength) || (textIndex < 0L)));
	}
	if (patternIndex >= patternLength) {
		return(textIndex - patternLength);
	} else {
		return(-1L);
	}
}

/****************************************************************/
/* Purpose..: Supersedes Toolbox's Munger.                      */
/*				   Forward/Backward, Whole word/Substring		*/
/*				   and Case sensitive/Case insensitive added.	*/
/* Input....: pattern											*/
/* Input....: patternLength 									*/
/* Input....: text												*/
/* Input....: textLength										*/
/* Input....: offset											*/
/* Input....: forward 									*/
/* Input....: wholeWord 										*/
/* Input....: caseSensitive 									*/
/* Returns..: '-1' if not found else position					*/
/****************************************************************/
int32 MWMunger(const char pattern[], int32 patternLength, const char text[],
			  int32 textLength, int32 offset, bool forward,
			  bool wholeWord, bool caseSensitive)
{
	int32 result;
	int32 where;

	if (wholeWord) {
		bool isAWholeWord = IsDelimitators(pattern[0]) &&
						IsDelimitators(pattern[patternLength - 1]);
		if (forward) {
			where = offset;
			while (1L) {

				result = BoyerMoore((unsigned char *)pattern, patternLength, (unsigned char *)text, textLength,
									where, caseSensitive);

				if (result == -1L || isAWholeWord) {
					return(result);
				}
				where = result + patternLength;

				if ((result == 0L) &&
					(IsDelimitators(text[result + patternLength]))) {
					return(result);
				}
				if (IsDelimitators(text[result - 1L]) &&
					IsDelimitators(text[result + patternLength])) {
					return(result);
				}
			}
		} else {
			where = offset;

			while (1L) {
				result = ReverseBoyerMoore((unsigned char *)pattern, patternLength, (unsigned char *)text,
										   textLength, where, caseSensitive);

				if (result == -1L || isAWholeWord) {
					return(result);
				}
				where = result;

				if ((result == 0L) &&
					(IsDelimitators(text[result + patternLength]))) {
					return(result);
				}
				if (IsDelimitators(text[result - 1L]) &&
					IsDelimitators(text[result + patternLength])) {
					return(result);
				}
			}
		}
	} else {
		if (forward) {
			result = BoyerMoore((unsigned char *)pattern, patternLength, (unsigned char *)text, textLength,
								offset, caseSensitive);
		} else {
			result = ReverseBoyerMoore((unsigned char *)pattern, patternLength, (unsigned char *)text,
									   textLength, offset, caseSensitive);
		}
	}
	return result;
}

