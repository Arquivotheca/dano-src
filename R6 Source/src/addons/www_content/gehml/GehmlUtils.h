
#ifndef _GEHMLUTIL_H_
#define _GEHMLUTIL_H_

#include "GehmlDefs.h"

#include <www/util.h>

dimth parse_dimth(const char *str, dimth::unit defUnits = dimth::pixels);
int32 count_dimths(const char *str);
int32 parse_dimths(const char *str, dimth *values, int32 numValues, dimth::unit defUnits = dimth::pixels);

#endif
