//========================================================================
//	CString.cpp
//	Copyright 1995 -1996 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	General-purpose string class

#include <string.h>
#include <ctype.h>
#include "CString.h"
#include "IDEConstants.h"
#include <Debug.h>

char *
p2cstr(
	unsigned char * str)
{
	int len = *str;
	memmove(str, str+1, len);
	str[len] = 0;
	return (char *) str;
}


unsigned char *
c2pstr(
	char * str)
{
	int len = strlen(str);
	if (len > 255)
	{
		len = 255;
	}
	memmove(str+1, str, len);
	str[0] = len;
	return (unsigned char *)str;
}


void
p2cstrncpy(
	char * cstr,
	const unsigned char * pstr,
	int size)
{
	int len = *pstr;
	if (len >= size)
	{
		len = size-1;
	}
	memmove(cstr, pstr+1, len);
	cstr[len] = 0;
}


void
c2pstrncpy(
	unsigned char * pstr,
	const char * cstr,
	int size)
{
	int len = strlen(cstr);
	if (len >= size)
	{
		len = size-1;
	}
	memmove(pstr+1, cstr, len);
	pstr[0] = len;
}


void
String::ReadFrom(
	const char * clone,
	int size)
{
	ASSERT(clone != NULL);
	ASSERT(strlen(clone) == size);
	delete[] fData;
	fData = new char[size+1];
	strcpy(fData, clone);
	fSize = size;
}


String::String()
{
	fSize = 0;
	fData = new char[1];
	*fData = 0;
}


String::String(
	const char * init)
{
	ASSERT(init != NULL);
	fData = NULL;
	fSize = 0;
	ReadFrom(init, strlen(init));
}


String::String(
	const String & clone)
{
	fData = NULL;
	ReadFrom(clone.fData, clone.fSize);
}


String::String(
	int32 	value)
{
	fData = NULL;
	char		tempString[16];

	sprintf(tempString, "%ld", value);

	ReadFrom(tempString, strlen(tempString));
}


String::String(
	char 	value)
{
	fData = new char[2];
	fData[0] = value;
	fData[1] = '\0';
	fSize = 1;
}


String::String(
	const char * 	init,
	int32			inLength)
{
	ASSERT(init != NULL);
	fData = NULL;
	fSize = 0;
	Set(init, inLength);
}


String::~String()
{
	delete[] fData;
}

#if 0
String::operator const char * ()
{
	return fData;
}
#endif

bool
String::operator == (
	const String & compare) const
{
	return !strcmp(fData, compare.fData);
}


bool
String::operator == (
	const char * compare) const
{
	return !strcmp(fData, compare);
}

bool
String::operator != (
	const String & compare) const
{
	return 0 != strcmp(fData, compare.fData);
}


bool
String::operator != (
	const char * compare) const
{
	return 0 != strcmp(fData, compare);
}


String
String::operator + (
	const String & add)
{
	String ret(*this);
	ret += add;
	return ret;
}


String
String::operator + (
	const char * add)
{
	String ret(*this);
	ret += add;
	return ret;
}

String &
String::operator += (
	const String & add)
{
	ASSERT(strlen(add.fData) == add.fSize);
	int newSize = add.fSize + fSize;
	char * newData = new char[newSize+1];
	strcpy(newData, fData);
	strcat(newData, add.fData);
	delete[] fData;
	fData = newData;
	fSize = newSize;

	return *this;
}


String &
String::operator += (
	const char * add)
{
	ASSERT(add != NULL);
	int newSize = strlen(add) + fSize;
	char * newData = new char[newSize+1];
	strcpy(newData, fData);
	strcat(newData, add);
	delete[] fData;
	fData = newData;
	fSize = newSize;

	return *this;
}


// BDS
String &
String::operator += (
	int32 add)
{
	char		tempString[16];

	sprintf(tempString, "%ld", add);

	int newSize = strlen(tempString) + fSize;
	char * 		newData = new char[newSize+1];

	strcpy(newData, fData);
	strcat(newData, tempString);
	delete[] fData;
	fData = newData;
	fSize = newSize;

	return *this;
}

String &
String::operator += (
	char add)
{
	int32		newSize = fSize+1;
	char * 		newData = new char[newSize + 1];

	strcpy(newData, fData);
	newData[fSize] = add;
	newData[newSize] = '\0';
	delete[] fData;
	fData = newData;
	fSize = newSize;

	return *this;
}

// BDS
String
String::operator + (
	int32 add)
{
	String ret(*this);
	ret += add;
	return ret;
}


String
String::operator + (
	char add)
{
	String ret(*this);
	ret += add;
	return ret;
}

//BDS
String &
String::operator = (
	int32 clone)
{
	char		tempString[16];

	sprintf(tempString, "%ld", clone);

	ReadFrom(tempString, strlen(tempString));
	return * this;
}


String &
String::operator = (
	const String & clone)
{
	ReadFrom(clone.fData, clone.fSize);
	return * this;
}


String &
String::operator = (
	const char * clone)
{
	ReadFrom(clone, strlen(clone));
	return * this;
}


//BDS
//	Insert a string at the specified offset
String &
String::Insert(
	const char * 	inString,
	int32			inOffset)
{
	if (inOffset > fSize)
		inOffset = fSize;

	if (inOffset < 0)
		inOffset = 0;

	int 		newSize = strlen(inString) + fSize;
	char * 		newData = new char[newSize+1];

	memcpy(newData, fData, inOffset);
	newData[inOffset] = 0;
	strcat(newData, inString);
	strcat(newData, fData + inOffset);

	delete[] fData;
	fData = newData;
	fSize = newSize;

	return *this;
}


//BDS
//	Replace the chars defined by inOffset and inLength by inString
String &
String::Replace(
	const char * 	inString,
	int32			inOffset,
	int32			inLength)
{
	if (inOffset > fSize)
		inOffset = fSize;
	else
	if (inOffset < 0)
		inOffset = 0;
	if (inOffset + inLength > fSize)
		inLength = (inOffset + inLength) - fSize;
	else
	if (inLength < 0)
		inLength = 0;

	int 		newSize = strlen(inString) + fSize - inLength;
	char * 		newData = new char[newSize+1];

	memcpy(newData, fData, inOffset);
	newData[inOffset] = 0;
	strcat(newData, inString);
	strcat(newData, fData + inOffset + inLength);

	delete[] fData;
	fData = newData;
	fSize = newSize;

	return *this;
}

//	Replace all instances of inCharOld with inCharNew
String &
String::Replace(
	char		inCharOld,
	char		inCharNew)
{
	for (int32 i = 0; i < fSize; i++)
		if (fData[i] == inCharOld)
			fData[i] = inCharNew;

	return *this;
}

int
String::OffsetOf(
	char		inChar) const
{
	int			result = -1;
	
	char*		ptr = strchr(fData, inChar);
	
	if (ptr)
		result = ptr - fData;
	
	return result;
}

int
String::ROffsetOf(
	char		inChar) const
{
	int			result = -1;
	
	char*		ptr = strrchr(fData, inChar);
	
	if (ptr)
		result = ptr - fData;
	
	return result;
}

// ---------------------------------------------------------------------------
//		Wordwrap
// ---------------------------------------------------------------------------
//	Wrap the text to the specified number of columns.  Doesn't remove
//	newlines but adjusts the text so that each line is less than or equal
//	to inColumns.

void
String::Wordwrap(
	int32		inColumns)
{
	uchar *			text = (uchar *) fData;
	uchar * 		end = text + fSize;
	const uchar		space = ' ';
	
	while (text < end)
	{
		uchar *		lineend = text + inColumns;
		uchar *		newline = (uchar*) strchr((const char*) text, EOL_CHAR);
		
		if (newline != nil && newline < lineend)
		{
			text = newline + 1;
			continue;
		}
		
		if (lineend >= end)
			break;

		while (*lineend != space && lineend > text)
			lineend--;
		
		if (lineend == text)
			while (*lineend != space && lineend < end)
				lineend++;
		if (lineend >= end)
			break;
		
		*lineend = EOL_CHAR;
		text = lineend + 1;
	}
}

// Set the contents of the string to be all upper case
void
String::ToUpper()
{
	for (int32 i = 0; i < fSize; i++)
		fData[i] = toupper(fData[i]);
}

// Set the contents of the string to be all lower case
void
String::ToLower()
{
	for (int32 i = 0; i < fSize; i++)
		fData[i] = tolower(fData[i]);
}

// Set the contents to the text defined by inString and inLength
void
String::Set(
	const char * 	inText,
	int32			inLength)
{
	delete [] fData;

	fData = new char[inLength + 1];
	memcpy(fData, inText, inLength);

	fData[inLength] = 0;
	fSize = inLength;
}

// ------------------------------------------------------------
// 	GlyphWidth
// ------------------------------------------------------------
// Return the number of bytes in the multibyte UTF8 character at
// the specified offset.  It doesn't matter whether the byte
// at this offset is the first, second, or third byte of
// the glyph.
// This code assumes only valid utf8 chars are in the buffer.

int
String::GlyphWidth(
	int32	inOffset)
{
	ASSERT(inOffset >= 0 && inOffset < fSize);
	int		result;
	uchar	c = fData[inOffset];

	if (c < 0x80)			// one byte char
		result = 1;
	else
	if (c >= 0xe0)			// first byte of three byte glyph
		result = 3;
	else
	if (c >= 0xc0)			// first byte of two byte glyph
		result = 2;
	else
	{
		// won't reach here if inBytes is the first byte of a glyph
		uchar	d = fData[inOffset - 1];
		
		if (d >= 0xe0)		// was second byte of three byte glyph
			result = 3;
		else
		if (d >= 0xc0)		// was second byte of two byte glyph
			result = 2;
		else
		if (d >= 0x80)
			result = 3;		// was third byte of three byte glyph
		else
			result = 1;		// invalid utf8 glyph
	}

	return result;
}
