// MediaIndex.h

#ifndef __MEDIAINDEX_H
#define __MEDIAINDEX_H

#include <SupportDefs.h>

struct MediaIndex {
		bigtime_t		start_time;
		off_t			entrypos;
		int32			entrysize;
		int32			flags;        /* keyframe, etc */
};

#endif
