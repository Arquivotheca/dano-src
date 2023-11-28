// screen_private.h
// 20 Jun 1997
//
// Banished from InterfaceDefs.h because they don't fit in with
// future API directions.

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif

status_t	set_screen_refresh_rate(int32 index, float rate, bool stick = TRUE);
status_t	adjust_crt(int32 index, uchar x_position, uchar y_position,
					   uchar x_size, uchar y_size, bool stick = TRUE);




