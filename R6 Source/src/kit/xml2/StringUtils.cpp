
#include <xml2/BStringUtils.h>
#include <ctype.h>
#include <stdio.h>

namespace B {
namespace XML {



// =====================================================================
void
MushString(BString & str)
{
	if (str.Length() == 0)
		return;
	char* buf = str.LockBuffer(str.Length());
	char* begin = buf;
	char* pos = buf;
	bool lastSpace = false;
	while (*buf) {
		if (pos == begin && isspace(*buf)) {
			buf++;
			continue;
		}
		if (pos < buf) *pos = *buf;
		if (*pos == '\n' || *pos == '\r' || *pos == '\t')
			*pos = ' ';
		if (isspace(*buf)) {
			if (!lastSpace) {
				pos++;
				lastSpace = true;
			}
		} else {
			lastSpace = false;
			pos++;
		}
		buf++;
	}
	if (pos > begin && isspace(*(pos-1)))
		*(pos-1) = '\0';
	else
		*pos = '\0';
	str.UnlockBuffer(pos-begin);
}


// =====================================================================
void
StripWhitespace(BString & str)
{
	char* buf = str.LockBuffer(str.Length());
	char* begin = buf;
	char* pos = buf;
	while (*buf) {
		if (isspace(*buf)) {
			buf++;
			continue;
		}
		*pos++ = *buf++;
	}
	*pos = '\0';
	str.UnlockBuffer(pos-begin);
}


// Return the next string in str, starting after pos, in split.
// split will start in the next character after any whitespace happening
// at pos, and continue up until, but not including any whitespace.
// =====================================================================
bool
SplitStringOnWhitespace(const BString & str, BString & split, int32 * pos)
{
	if (*pos >= str.Length())
		return false;
	
	int count = 0;
	const char * p = str.String() + *pos;
	
	// Use up any preceeding whitespace
	while (*p && isspace(*p))
		p++;
	if (*p == '\0')
		return false;
		
	const char * s = p;
	
	// Go until the end
	while (*p && !isspace(*p))
	{
		count++;
		p++;
	}
	
	split.SetTo(s, count);
	*pos = p - str.String();
	return true;
}



}; // namespace XML
}; // namespace B


