#ifndef C_MPEG2_BUFFER_H

#define C_MPEG2_BUFFER_H

#include <be_prim.h>

typedef struct buffer_
{
	void *data;
	size_t size;
	
	bool has_start_time;
	bigtime_t start_time;
	
	bool repeat_first_field;
	bool tagged;
	
	void *cookie;	// app defined
	
	void (*acquire_ref)(struct buffer_ *);
	void (*release_ref)(struct buffer_ *);
	
	struct buffer_ *next;
	int32 code;

	vint32 internal_ref_count;
		
} buffer_t;

#endif
