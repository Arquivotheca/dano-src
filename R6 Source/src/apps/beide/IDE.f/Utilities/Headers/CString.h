//========================================================================
//	CString.h
//	Copyright 1995 -1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	General-purpose string class

#ifndef _CSTRING_H
#define _CSTRING_H

#include <SupportDefs.h>

class String
{
		char *					fData;
		int						fSize;

		void					ReadFrom(
									const char * clone,
									int size);
public:
								String();
								String(
									const char * init);
								String(
									const String & clone);
								String(
									int32 value);
								String(
									char value);
								String(
									const char * 	init,
									int32			inLength);
								~String();

								operator const char * () const
								{
									return fData;
								}

		bool					operator == (
									const String & compare) const;
		bool					operator == (
									const char * compare) const;

		bool					operator != (
									const String & compare) const;
		bool					operator != (
									const char * compare) const;

		String					operator + (
									const String & add);
		String					operator + (
									const char * add);
		String					operator + (
									int32 add);
		String					operator + (
									char add);

		String &				operator += (
									const String & add);
		String &				operator += (
									const char * add);
		String &				operator += (
									int32 add);
		String &				operator += (
									char add);
		String &				operator = (
									const String & clone);
		String &				operator = (
									const char * clone);
		String &				operator = (
									int32 clone);

		String &				Insert(
									const char * 	inString,
									int32			inOffset);
		String &				Replace(
									const char * 	inString,
									int32			inOffset,
									int32			inLength);
		String &				Replace(
									char			inCharOld,
									char			inCharNew);

		int						OffsetOf(
									char			inChar) const;
		int						ROffsetOf(
									char			inChar) const;

		void					ToUpper();
		void					ToLower();

		void					Wordwrap(
									int32		inColumns);

		void					Set(
									const char * 	inText, 
									int32 			inLength);
		int						GlyphWidth(
									int32	inOffset);

		int						GetLength() const
								{
									return fSize;
								}
};

char *				p2cstr(
	unsigned char * str);
unsigned char *		c2pstr(
	char * str);
void p2cstrncpy(
	char * cstr,
	const unsigned char * pstr,
	int maxLen);
void c2pstrncpy(
	unsigned char * cstr,
	const char * pstr,
	int maxLen);

#endif
