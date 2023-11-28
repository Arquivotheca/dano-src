
#ifndef _INTERFACE2_UTILS_H_
#define _INTERFACE2_UTILS_H_

#include <interface2/InterfaceDefs.h>

namespace B {
namespace Interface2 {

dimth parse_dimth(const char *str, dimth::unit defUnits = dimth::pixels);
int32 count_dimths(const char *str);
int32 parse_dimths(const char *str, dimth *values, int32 numValues, dimth::unit defUnits = dimth::pixels);

} } // namespace B::Interface2

#endif
