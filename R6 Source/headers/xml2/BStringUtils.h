
#ifndef _B_XML2_STRINGUTILS_H
#define _B_XML2_STRINGUTILS_H

#include <support2/Vector.h>
#include <support2/String.h>
#include <support2/SupportDefs.h>

namespace B {
namespace XML {

using namespace Support2;

// Functions
// =====================================================================
// Functions that do some fun stuff to strings
bool SplitStringOnWhitespace(const BString & str, BString & split, int32 * pos);
void MushString(BString & str);
void StripWhitespace(BString & str);

}; // namespace XML
}; // namespace B

#endif // _B_XML2_STRINGUTILS_H
