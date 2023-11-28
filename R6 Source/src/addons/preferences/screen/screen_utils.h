#if ! defined SCREEN_UTILS_INCLUDED
#define SCREEN_UTILS_INCLUDED 1

#include <Screen.h>

const double PREDICATE_EPSILON = 0.01;

double rate_from_display_mode(const display_mode *dm);
void dump_mode(const display_mode *dm);
const char *spaceToString(uint32 cs);
bool modes_match(const display_mode *mode1, const display_mode *mode2);

#endif
