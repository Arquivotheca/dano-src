#ifndef _MEDIA_PLAYER_DEBUG_H
#define _MEDIA_PLAYER_DEBUG_H

#include <stdarg.h>
#include <stdio.h>

extern bool debug_enabled;

inline void writelog(const char *fmt, ...)
{
	if (debug_enabled) {
		va_list ap; 
		va_start(ap, fmt); 
		vfprintf(stderr, fmt, ap);
		va_end(ap); 		
	}
}

#endif

