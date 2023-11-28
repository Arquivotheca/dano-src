/*	$Id: DUtils.cpp,v 1.3 1998/11/19 21:50:21 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 05/11/98 14:30:57
*/

#include "bdb.h"
#include "DUtils.h"

char *p2cstr(uchar *pstr, char *cstr)
{
	memcpy(cstr, pstr + 1, *pstr);
	cstr[*pstr] = 0;
	return cstr;
} /* p2cstr */

int mcharlen(const char *src)
{
	int result, i;
	
	if ((*src & 0x80) == 0)
		return 1;
	else if ((*src & 0xE0) == 0xC0)
		i = 1;
	else if ((*src & 0xF0) == 0xE0)
		i = 2;
	else if ((*src & 0xF8) == 0xF0)
		i = 3;
	else if ((*src & 0xFC) == 0xF8)
		i = 4;
	else if ((*src & 0xFE) == 0xFC)
		i = 5;
	else
		return 1;	// clearly this is an error
	
	result = 1;
	src++;

	while (i-- && (*src++ & 0xC0) == 0x80)
		result++;
	
	return result;
} // mcharlen

int mprevcharlen(const char *s)
{
	if (*--s > 0 || (*s & 0x00C0) != 0x0080)
		return 1;

	int result = 2;
	while ((*--s & 0x00C0) == 0x0080 && result < 5)
		result++;

	ASSERT(mcharlen(s) == result);
	return result;
} /* mprevcharlen */

