/*	$Id: DUtils.h,v 1.4 1999/01/21 15:18:46 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 05/11/98 14:30:44
*/

#ifndef DUTILS_H
#define DUTILS_H

#include <ListItem.h>

struct p2cstr {
	p2cstr(const uchar *pstr)
		{ memcpy(buf, pstr + 1, *pstr); buf[*pstr] = 0; };
	operator char *()
		{ return buf; };
	char buf[256];
};

template<class T>
class DItem : public BStringItem {
public:
	DItem(const char *str, T t)
		: BStringItem(str)
		, fData(t)
	{};
	
	bool operator == (const DItem<T>& i)
		{ return fData == i.fData; }

	bool operator< (const DItem<T>& i)
		{ return fData < i.fData; }
	
	T fData;
};

class StSetFlag
{
  public:
	StSetFlag(bool& flag) : fFlag(flag)	{ fReset = fFlag; fFlag = true;	}
	StSetFlag(bool *flag) : fFlag(*flag)	{ fReset = fFlag; fFlag = true; }
	~StSetFlag()							{ fFlag = fReset; }
  private:
	bool& fFlag, fReset;
};

int mprevcharlen(const char *s);
int mcharlen(const char *s);

#endif
