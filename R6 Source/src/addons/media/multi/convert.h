#include <stdio.h>

/* uchar */
static inline void 
copy_uchar_into_uchar(char *from, char *to, int32 from_stride, int32 to_stride, int32 frames_to_copy)
{
	for (int i=0; i<frames_to_copy; i++)
	{
		*(uchar *)to = *(uchar *)from;
		to += to_stride;
		from += from_stride;
	}	
}

static inline void 
copy_short_into_uchar(char *from, char *to, int32 from_stride, int32 to_stride, int32 frames_to_copy)
{
	double convert = 127.0/32767.0;
	for (int i=0; i<frames_to_copy; i++)
	{
		*(uchar *)to = (uchar)(((double)*(int16 *)from * convert) + 128);
		to += to_stride;
		from += from_stride;
	}	
}

static inline void 
copy_int_into_uchar(char *from, char *to, int32 from_stride, int32 to_stride, int32 frames_to_copy)
{
	double convert = 127.0/2147483647.0;
	for (int i=0; i<frames_to_copy; i++)
	{
		*(uchar *)to = (uchar)(((double)*(int32 *)from * convert) + 128);
		to += to_stride;
		from += from_stride;
	}	
}

static inline void 
copy_float_into_uchar(char *from, char *to, int32 from_stride, int32 to_stride, int32 frames_to_copy)
{
	double convert = 127.0;
	for (int i=0; i<frames_to_copy; i++)
	{
		*(uchar *)to = (uchar)(((double)*(float *)from * convert) + 128);
		to += to_stride;
		from += from_stride;
	}	
}

/* short */
static inline void 
copy_uchar_into_short(char *from, char *to, int32 from_stride, int32 to_stride, int32 frames_to_copy)
{
	double convert = 32767.0/127.0;
	for (int i=0; i<frames_to_copy; i++)
	{
		*(int16 *)to = (int16)(((double)*(uchar *)from - 128) * convert);
		to += to_stride;
		from += from_stride;
	}	
}

static inline void 
copy_short_into_short(char *from, char *to, int32 from_stride, int32 to_stride, int32 frames_to_copy)
{
	for (int i=0; i<frames_to_copy; i++)
	{
		*(int16 *)to = *(int16 *)from;
		to += to_stride;
		from += from_stride;
	}	
}

static inline void 
copy_int_into_short(char *from, char *to, int32 from_stride, int32 to_stride, int32 frames_to_copy)
{
	double convert = 32767.0/2147483647.0;
	for (int i=0; i<frames_to_copy; i++)
	{
		*(int16 *)to = (int16)((double)*(int32 *)from * convert);
		to += to_stride;
		from += from_stride;
	}	
}

static inline void 
copy_float_into_short(char *from, char *to, int32 from_stride, int32 to_stride, int32 frames_to_copy)
{
	double convert = 32767.0;
	for (int i=0; i<frames_to_copy; i++)
	{
		*(int16 *)to = (int16)((double)*(float *)from * convert);
		to += to_stride;
		from += from_stride;
	}	
}

/* int */
static inline void 
copy_uchar_into_int(char *from, char *to, int32 from_stride, int32 to_stride, int32 frames_to_copy)
{
	double convert = 2147483647.0/127.0;
	for (int i=0; i<frames_to_copy; i++)
	{
		*(int32 *)to = (int32)(((double)*(uchar *)from - 128) * convert);
		to += to_stride;
		from += from_stride;
	}	
}

static inline void 
copy_short_into_int(char *from, char *to, int32 from_stride, int32 to_stride, int32 frames_to_copy)
{
	double convert = 2147483647.0/32767.0;
	for (int i=0; i<frames_to_copy; i++)
	{
		*(int32 *)to = (int32)((double)*(int16 *)from * convert);
		to += to_stride;
		from += from_stride;
	}	
}

static inline void 
copy_int_into_int(char *from, char *to, int32 from_stride, int32 to_stride, int32 frames_to_copy)
{
	for (int i=0; i<frames_to_copy; i++)
	{
		*(int32 *)to = *(int32 *)from;
		to += to_stride;
		from += from_stride;
	}	
}

static inline void 
copy_float_into_int(char *from, char *to, int32 from_stride, int32 to_stride, int32 frames_to_copy)
{
	double convert = 2147483647.0;
	for (int i=0; i<frames_to_copy; i++)
	{
		*(int32 *)to = (int32)((double)*(float *)from * convert);
		to += to_stride;
		from += from_stride;
	}	
}

/* float */
static inline void 
copy_uchar_into_float(char *from, char *to, int32 from_stride, int32 to_stride, int32 frames_to_copy)
{
	double convert = 1.0/127.0;
	for (int i=0; i<frames_to_copy; i++)
	{
		*(float *)to = (float)(((double)*(uchar *)from - 128) * convert);
		to += to_stride;
		from += from_stride;
	}	

}

static inline void 
copy_short_into_float(char *from, char *to, int32 from_stride, int32 to_stride, int32 frames_to_copy)
{
	double convert = 1.0/32767.0;
	for (int i=0; i<frames_to_copy; i++)
	{
		*(float *)to = (float)((double)*(int16 *)from * convert);
		to += to_stride;
		from += from_stride;
	}	
}

static inline void 
copy_int_into_float(char *from, char *to, int32 from_stride, int32 to_stride, int32 frames_to_copy)
{
	double convert = 1.0/2147483647.0;
	for (int i=0; i<frames_to_copy; i++)
	{
		*(float *)to = (float)((double)*(int32 *)from * convert);
		to += to_stride;
		from += from_stride;
	}	
}

static inline void 
copy_float_into_float(char *from, char *to, int32 from_stride, int32 to_stride, int32 frames_to_copy)
{
	for (int i=0; i<frames_to_copy; i++)
	{
		*(float *)to = *(float *)from;
		to += to_stride;
		from += from_stride;
	}	
}
