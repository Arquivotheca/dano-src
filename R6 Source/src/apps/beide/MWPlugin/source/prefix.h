#include <BeHeaders>

#define DEBUG 1
#include <Debug.h>


#define B_FOLLOW_LEFT_TOP ((B_FOLLOW_LEFT)+(B_FOLLOW_TOP))
#define B_FOLLOW_LEFT_TOP_RIGHT ((B_FOLLOW_LEFT)+(B_FOLLOW_TOP)+(B_FOLLOW_RIGHT))
#define B_FOLLOW_LEFT_BOTTOM ((B_FOLLOW_LEFT)+(B_FOLLOW_BOTTOM))
#define B_FOLLOW_LEFT_RIGHT_BOTTOM ((B_FOLLOW_LEFT)+(B_FOLLOW_RIGHT)+(B_FOLLOW_BOTTOM))

const ulong nil = 0;
const unsigned char EOL_CHAR = '\n';
