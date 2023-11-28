// ===========================================================================
//	Utils.h
// 	©1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __UTILS__
#define __UTILS__

#include <ctype.h>
#include <string.h>

#define huge

#define BEBOX

//	Types used

#ifdef BEBOX
#define Boolean bool
#define nil 0
#define false 0
#define true 1
#define OSErr short
#define noErr 0
#define OSType long

//#include "MessageWindow.h"	// pprint

typedef unsigned char Byte;
#endif


//===========================================================================
//	Common utils


//typedef unsigned long ulong;
//typedef unsigned short ushort;
//typedef unsigned char uchar;

// ===========================================================================


void upstr(char *str);						//	Convert a string to upper case
void lostr(char *str);						//	Convert a string to lower case
char *SetStr(char *oldStr, char *newStr);	//	Simple util to allocate a new string
char*  SkipChars(const char* str, const char* skip);	// Skip a list of defined chars
char*  SkipStr(const char* str, const char* skip);		// Skip a string if found

// ===========================================================================

class CString {
public:
			CString();
			CString(const char* str);
			CString(const CString& s);
			
			~CString();
			
int			Length();
void		Lowercase();
void		Uppercase();

CString&	operator+=(const CString& str);

CString&	operator=(const CString& s);
CString&	operator=(const char* str);

			operator const char*() const { return mStr; };
			
void		Set(const char* str, int count);
void		CropTrailingSpace();


protected:
	char*	mStr;
	int		mLength;
};

//===========================================================================

#endif
