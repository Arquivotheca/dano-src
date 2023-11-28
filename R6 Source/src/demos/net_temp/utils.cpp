// ===========================================================================
//	Utils.cpp
// 	©1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#include <Be.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "butil.h"

//	Convert a string to upper case

void upstr(char *str)
{
	while (*str) {
		char c = toupper(*str);
		*str++ = c;
	}
}

//	Convert a string to lower case

void lostr(char *str)
{
	while (*str) {
		char c = tolower(*str);
		*str++ = c;
	}
}

//	Simple util to allocate a new string

char *SetStr(char *oldStr, char *newStr)
{
	if (oldStr) free(oldStr);
	if (!newStr) return 0;
	if (oldStr = (char *)malloc(strlen(newStr) + 1))
		strcpy(oldStr,newStr);
	return oldStr;
}


//	Handy string utils

char*  SkipChars(const char* str, const char* skip)
{
	while (str[0] && strchr(skip, str[0]))
		str++;
	return str;
}

char*  SkipStr(const char* str, const char* skip)
{
	
	int i = strlen(skip);
	if (strncmp(skip,str,i) == 0)
		return str + i;
	return NULL;
}

// ===========================================================================
// ===========================================================================

CString::CString() : mStr(NULL), mLength(0)
{
}

CString::CString(const char* str)
{
	if (str) {
		mLength = strlen(str);
		mStr = (char*)malloc(mLength + 1);
		strcpy(mStr,str);
	} else {
		mStr = NULL;
		mLength = 0;
	}
}

CString::CString(const CString& s)
{
	if (s.mStr) {
		mLength = strlen(s.mStr);
		mStr = (char*)malloc(mLength + 1);
		strcpy(mStr,s.mStr);
	} else {
		mLength = 0;
		mStr = NULL;
	}
}

CString& CString::operator=(const CString& s)
{
	if (mStr) {
		free(mStr);
		mStr = NULL;
	}
	mLength = 0;
	if (s.mStr != NULL) {
		mLength = strlen(s.mStr);
		mStr = (char *)malloc(mLength + 1);
		strcpy(mStr,s.mStr);
	}
	return *this;
}

int CString::Length()
{
	return mLength;
}

void CString::Lowercase()
{
	if (mStr == NULL)
		return;
	char *s = mStr;
	while (s[0]) {
		s[0] = tolower(s[0]);
		s++;
	}
}

void CString::Uppercase()
{
	if (mStr == NULL)
		return;
	char *s = mStr;
	while (s[0]) {
		s[0] = toupper(s[0]);
		s++;
	}
}

CString& CString::operator+=(const CString& str)
{
	if (str.mLength == 0)
		return;
	mLength += str.mLength;
	char *s = (char *)malloc(mLength + 1);
	if (mStr) {
		strcpy(s,mStr);
		strcat(s,str.mStr);
		free(mStr);
	} else
		strcpy(s,str.mStr);
	mStr = s;
	return *this;
}

CString& CString::operator=(const char* str)
{
	if (mStr) {
		free(mStr);
		mStr = NULL;
	}
	mLength = 0;
	if (str == NULL || str[0] == 0)
		return *this;
		
	mLength = strlen(str);
	mStr = (char *)malloc(mLength + 1);
	strcpy(mStr,str);
	return *this;
}

void CString::Set(const char* str, int count)
{
	mLength = count;
	if (mStr) {
		free(mStr);
		mStr = NULL;
	}
	if (count) {
		mStr = (char *)malloc(count + 1);
		memcpy(mStr,str,count);
		mStr[count] = 0;
	}
}

void CString::CropTrailingSpace()
{
	if (mStr == NULL)
		return;
		
	int j;
	for (j = strlen(mStr); j; --j)
		if (isspace(mStr[j]))
			mStr[j] = '\0';
		else
			break;
	mLength = strlen(mStr);
	if (mLength == 0) {
		free(mStr);
		mStr = NULL;
	}
}
	

CString::~CString()
{
	if (mStr)
		free(mStr);
}


// ===========================================================================
