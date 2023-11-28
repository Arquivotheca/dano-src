
#ifndef _BE_PRIMITIVES_H
#define _BE_PRIMITIVES_H

#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>

/*-------------------------------------------------------------*/
/*----- Shorthand type formats --------------------------------*/

typedef	signed char				int8;
typedef unsigned char			uint8;
typedef volatile signed char   	vint8;
typedef volatile unsigned char	vuint8;

typedef	short					int16;
typedef unsigned short			uint16;
typedef volatile short			vint16;
typedef volatile unsigned short	vuint16;

typedef	long					int32;
typedef unsigned long			uint32;
typedef volatile long			vint32;
typedef volatile unsigned long	vuint32;

typedef	long long					int64;
typedef unsigned long long			uint64;
typedef volatile long long			vint64;
typedef volatile unsigned long long	vuint64;

typedef volatile long			vlong;
typedef volatile int			vint;
typedef volatile short			vshort;
typedef volatile char			vchar;

typedef volatile unsigned long	vulong;
typedef volatile unsigned int	vuint;
typedef volatile unsigned short	vushort;
typedef volatile unsigned char	vuchar;

typedef unsigned char			uchar;
typedef unsigned short          unichar;

/*-------------------------------------------------------------*/
/*----- Descriptive formats -----------------------------------*/

typedef int32					status_t;
typedef int64					bigtime_t;
typedef uint32					type_code;
typedef uint32					perform_code;

/*-----------------------------------------------------*/
/*----- min and max comparisons -----------------------*/ 
/*----- min() and max() won't work in C++ -------------*/

#define min_c(a,b) ((a)>(b)?(b):(a))
#define max_c(a,b) ((a)>(b)?(a):(b))

#ifndef __cplusplus
#ifndef min
#define min(a,b) ((a)>(b)?(b):(a))
#endif
#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif
#endif 

/*-----------------------------------------------------*/
/*----- Grandfathering --------------------------------*/

#ifndef __cplusplus
typedef unsigned char			bool;
#define false	0
#define true	1
#endif 

#ifndef NULL
#define NULL 	(0)
#endif

/*----LIMITS--------------------------------*/
#define B_FILE_NAME_LENGTH		NAME_MAX
#define B_PATH_NAME_LENGTH 		MAXPATHLEN
#define B_ATTR_NAME_LENGTH		(B_FILE_NAME_LENGTH-1)
#define B_MIME_TYPE_LENGTH		(B_ATTR_NAME_LENGTH - 15)
#define B_MAX_SYMLINKS			SYMLINK_MAX


/*----FILE OPEN MODES--------------------------------*/

#define B_READ_ONLY 	O_RDONLY  /* read only */
#define B_WRITE_ONLY 	O_WRONLY /* write only */
#define B_READ_WRITE 	O_RDWR   /* read and write */

#define	B_FAIL_IF_EXISTS	O_EXCL		/* exclusive create */
#define B_CREATE_FILE		O_CREAT		/* create the file */
#define B_ERASE_FILE		O_TRUNC		/* erase the file's data */
#define B_OPEN_AT_END	   	O_APPEND	/* point to the end of the data */

/*---------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

/*----- Atomic functions; old value is returned -----------------*/
extern int32	atomic_add(vint32 *value, int32 addvalue);
extern int32	atomic_and(vint32 *value, int32 andvalue);
extern int32	atomic_or(vint32 *value, int32 orvalue);	

/*----- Other stuff ---------------------------------------------*/
extern void *	get_stack_frame(void);

#ifdef __cplusplus
}
#endif

#include <errno.h>

#endif	// _BE_PRIMITIVES_H
