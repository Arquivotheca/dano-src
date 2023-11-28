//==================================================================
//	MWEditUtils.cpp
//	Copyright 1996  Metrowerks Corporation, All Rights Reserved.
//==================================================================

#include <ctype.h>
#include "MWEditUtils.h"
#include "IDEConstants.h"

// ---------------------------------------------------------------------------
//		MWEdit_StrCmp
// ---------------------------------------------------------------------------
//	Check to see if the text starts with name.

bool
MWEdit_StrCmp(
	register const char*	name,
	register const char*	text)
{
	while (* (uchar*) name != '\0')
		if (*(uchar*) name++ != *(uchar*) text++)
			return false;

	return ! (isalpha(*(uchar*)text) || *(uchar*)text == '_' || (*(uchar*)text >= '0' && *(uchar*)text <= '9'));
}

// ---------------------------------------------------------------------------
//		CompareStrings
// ---------------------------------------------------------------------------

int32
CompareStrings(
	const char * inOne,
	const char * inTwo)
{
	while (*(unsigned char *)inOne == *(unsigned char *)inTwo && 
		*(unsigned char *)inOne != 0)
	{
		inOne++;
		inTwo++;
	}

	return *(unsigned char *)inOne - *(unsigned char *)inTwo;
}

// ---------------------------------------------------------------------------
//		CompareStringsNoCase
// ---------------------------------------------------------------------------

int32
CompareStringsNoCase(
	const char * inOne,
	const char * inTwo)
{
	while (toupper(*(unsigned char *)inOne) == toupper(*(unsigned char *)inTwo) && 
		*(unsigned char *)inOne != 0)
	{
		inOne++;
		inTwo++;
	}

	return toupper(*(unsigned char *)inOne) - toupper(*(unsigned char *)inTwo);
}

// ---------------------------------------------------------------------------
//		FindEnd
// ---------------------------------------------------------------------------
//	Search for the end of the specified string.   The end of the string is 
//	specified by hitting a newline a null, or the end of inTextLen.
//	The value returned points to either the newline or null or one past
//	the end of the string if the end is reached without finding a newline or null.
//	Need to look for Mac returns also
//	because this is reading a file that has been opened without 
//	converting to newline format.

char *
FindEnd(const char * inText, int32 inTextLen)
{
	// unsigned chars to prevent sign extension
	unsigned char*			cp = (unsigned char*) inText;
	const unsigned char*	term = (unsigned char*) inText + inTextLen;
	
	while (cp < term && *cp != EOL_CHAR && *cp != MAC_RETURN && *cp != 0)
	{
		cp++;
	}
	
	return (char*) cp;
}

// ---------------------------------------------------------------------------
//		MWEdit_LineStart
// ---------------------------------------------------------------------------
//	Return the first character of the line which contains text_pos or start 
//	of text ...

const char *
MWEdit_LineStart(
	const char *text_pos, 
	const char *buffer_start)
{
	while (text_pos > buffer_start && * (text_pos -1) != '\n')
		text_pos--;
	return(text_pos);
}

// ---------------------------------------------------------------------------
//		MWEdit_NextLineMono
// ---------------------------------------------------------------------------
//	Return the first character of the next line ignoring any coloring info ...

const char *
MWEdit_NextLineMono(const char *text_pos, const char *limit)
{
	while (*text_pos != 0 && *text_pos != EOL_CHAR && text_pos < limit)
		text_pos++;
	if (*text_pos != 0 && text_pos < limit)
		text_pos++;
	return(text_pos);
}

static char cchars[256] = { 
//      0    1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
/*0*/ '\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0',
/*1*/ '\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0',
/*2*/  ' ', '!', '"', '#', '$', '%', '&','\'', '(', ')', '*', '+', ',', '-', '.', '/',
/*3*/  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>','\?',
/*4*/  '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
/*5*/  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[','\\', ']', '^', '_',
/*6*/  '`', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
/*7*/  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '{', '|', '}', '~','\0',
/*8*/  'A', 'A', 'C', 'E', 'N', 'O', 'U', 'A', 'A', 'A', 'A', 'A', 'A', 'C', 'E', 'E',
/*9*/  'E', 'E', 'I', 'I', 'I', 'I', 'N', 'O', 'O', 'O', 'O', 'O', 'U', 'U', 'U', 'U',
/*A*/ '\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0',
/*B*/ '\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0',
/*C*/ '\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0',
/*D*/ '\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0',
/*E*/ '\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0',
/*F*/ '\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0'};

/****************************************************************/
/* Purpose..: Case insensitive comparison of two C strings 		*/
/* Input....: Pointer to the two strings ...					*/
/* Returns..: -ve s1 < s2 | 0 s1 = s2 | +ve s1 > s2				*/
/****************************************************************/
int stricmp (const char *s1, const char *s2)
{
register char *tab = cchars;

	while (*s1 && tab[*s1] == tab[*s2]) {
		s1++;
		s2++;
		}
	return tab[*s1] - tab[*s2];
}

int strincmp(const char *s1, const char *s2, size_t n)
{
	for (; 0 < n; ++s1, ++s2, --n)
	{
	    int ch1 = tolower(*(unsigned char *)s1);
	    int ch2 = tolower(*(unsigned char *)s2);
		if (ch1 != ch2)
			return (ch1 < ch2 ? -1 : +1);
		else if (ch1 == '\0')
			return (0);
	}
	return (0);
}

bool MWEdit_IsLetter(int c)
{
	return (isalnum(c) || (c == '_') || (c == '#') || (c == '$') || (c == '~'));//isalphanumeric
}

/*
 *	MWEdit_PrevSubWord
 *
 *	Finds the position of the start of the previous 'sub-word' in the text.
 */
const char* MWEdit_PrevSubWord(const char *text_pos, const char *buffer_start)
{
	// Find an identifier
	while ((text_pos > buffer_start) && !MWEdit_IsLetter(*(text_pos - 1)))
		text_pos--;

	if(text_pos > buffer_start)
	{
		text_pos -= 1;

		bool found = false;
		while (text_pos > buffer_start && !found)
		{
			char 	c1 = text_pos[-1];
			char 	c2 = text_pos[0];
			bool found1;
			
			bool c1_isletter = isalpha(c1);
			bool c2_isletter = isalpha(c2);

			// stop searching if we find a letter / non-letter boundary, or
			// if we find a lowercase letter followed by an upper case
			found1 = (c1_isletter && !c2_isletter) || (!c1_isletter && c2_isletter) ||
					 (islower(c1) && isupper(c2));

			// stop searching if we find an uppercase letter followed by a lower case
			found = found1 || (isupper(c1) && islower(c2));

			if(!found1)
				text_pos--;
		}
	}

	return text_pos;
}

/*
 *	MWEdit_NextSubWord
 *
 *	Finds the position of the start of the next 'sub-word' in the text.
 */
const char* MWEdit_NextSubWord(const char *text_pos, const char *limit)
{
	while (text_pos < limit && !MWEdit_IsLetter(*text_pos))
		text_pos++;

	limit -= 1;
	if(text_pos < limit)
	{
		bool found = false;
		bool	acronym = false;

		while (text_pos < limit && *text_pos && !found)
		{
			char 	c1 = text_pos[0];
			char 	c2 = text_pos[1];
			
			bool found1;

			bool c1_isletter = isalpha(c1);
			bool c2_isletter = isalpha(c2);
			bool c1_isupper = isupper(c1);
			bool c2_isupper = isupper(c2);
			
			// Check if there's a sequence of capital letters
			acronym = acronym || (c1_isupper && c2_isupper);

			// Stop searching if we find a sequence of capital letters followed
			// by a lowercase letter.
			found1 = (acronym && c1_isupper && islower(c2));

			// Stop searching if there's a non-letter next to a letter or if there's
			// a lowercase letter next to a capital letter
			found = found1 ||
					(c1_isletter && !c2_isletter) || (!c1_isletter && c2_isletter) ||
					(islower(c1) && c2_isupper) ||
					(!isalnum(c2) && c2 != '_') ;

			if(!found1)
				text_pos++;
		}
	}
	return text_pos;
}

