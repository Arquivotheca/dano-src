// DNub.cpp - implementation for the non-pure-virtual parts of the DNub interface

#include "DNub.h"
#include <strstream>

DNub::DNub()
	: BLocker("DNub")
{
}

DNub::~DNub()
{
}

void 
DNub::ReadString(ptr_t address, char *s, size_t max)
{
	ASSERT(max >= 1);
	char c;

	if (--max > 0)
	{
		do
		{
			ReadData(address++, &c, 1);
			*s++ = c;
		}
		while (--max && c);
	}
	
	*s = 0;
}

void 
DNub::ReadString(ptr_t address, string &s)
{
	char c;
	std::strstream st;
	st << '"';

	while (true)
	{
		ReadData(address++, &c, 1);
		if (c == 0)
			break;
		else if (iscntrl(c))
		{
			switch (c)
			{
				case '\t':	st << "\\t"; break;
				case '\n':	st << "\\n"; break;
				case '\r':	st << "\\r"; break;
				default:	st << std::oct << c; break;
			}
		}
		else
			st << c;
	}

	st << '"' << std::ends;
	s = st.str();
	delete[] st.str();
}
