#ifndef _DV_DEBUG_H
#define _DV_DEBUG_H

extern int32 debug_level;

#define PRINTF(a, b) \
		do { \
			if (a < debug_level) { \
				printf(DEBUG_PREFIX "%s: ", __FUNCTION__); \
				printf b; \
			} \
		} while (0)


#endif
