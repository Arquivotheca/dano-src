/* winceutl.h  
 */

/* (c) COPYRIGHT 1993-98           NOMBAS, INC.
 *                                 64 SALEM ST.
 *                                 MEDFORD, MA 02155  USA
 * 
 * ALL RIGHTS RESERVED
 * 
 * This software is the property of Nombas, Inc. and is furnished under
 * license by Nombas, Inc.; this software may be used only in accordance
 * with the terms of said license.  This copyright notice may not be removed,
 * modified or obliterated without the prior written permission of Nombas, Inc.
 * 
 * This software is a Trade Secret of Nombas, Inc.
 * 
 * This software may not be copied, transmitted, provided to or otherwise made
 * available to any other person, company, corporation or other entity except
 * as specified in the terms of said license.
 * 
 * No right, title, ownership or other interest in the software is hereby
 * granted or transferred.
 * 
 * The information contained herein is subject to change without notice and
 * should not be construed as a commitment by Nombas, Inc.
 */

#if defined(__JSE_WINCE__) && !defined(_WINCEUTL_H)
#define _WINCEUTL_H

#include "jseopt.h"
#include <tchar.h>
#include <ctype.h>
#include <wtypes.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <commdlg.h>

#ifdef __cplusplus
extern "C" {
#endif

extern jsechar **_tenviron;
 
/* <stdio.h> */ 

#define _TEOF       WEOF

/* Seek method constants */
#define SEEK_CUR    1
#define SEEK_END    2
#define SEEK_SET    0

#ifndef _FILE_DEFINED
typedef struct {
   int WillDefineLater;
} FILE;
#define _FILE_DEFINED
#endif

FILE *stdin;
FILE *stdout;
FILE *stderr;

FILE * _tfopen(const jsechar * filespec,const jsechar *mode);
FILE *_tfreopen( const jsechar *path, const jsechar *mode, FILE *stream );
size_t fread( void *buffer, size_t size, size_t count, FILE *stream );
size_t fwrite( const void *buffer, size_t size, size_t count, FILE *stream );
int fseek( FILE *stream, long offset, int origin );
int fclose( FILE *stream );
int fflush( FILE *stream );
long ftell( FILE *stream );
int vfwprintf( FILE *stream, const jsechar *format, va_list argptr );
int fwprintf( FILE *stream, const jsechar *format,...);
jsechar *fgetws( jsechar *string, int n, FILE *stream );
FILE *tmpfile( void );
wint_t _ungettc( wint_t c, FILE *stream );
int _tremove( const jsechar *path );
int _trename( const jsechar *oldname, const jsechar *newname );
jsechar *_ttmpnam( jsechar *string );
void _tperror( const jsechar *string );

typedef long int fpos_t;
extern int fscanf(FILE *fp,const jsechar *format,...);
extern int sscanf(const jsechar *buffer,const jsechar *format,...);

int fgetpos( FILE *stream, fpos_t *pos );
int fsetpos( FILE *stream, const fpos_t *pos );
void rewind( FILE *stream );
void clearerr( FILE *stream );
int ferror( FILE *stream );
int feof( FILE *stream );



int _fileno( FILE *stream );
#define fileno _fileno


/* <conio.h> */
int getch( void );

/* <stdlib.h> ??? */
void exit( int status );
int _tsystem( const jsechar *command );
jsechar *_tfullpath( jsechar *absPath, const jsechar *relPath, size_t maxLength );
jsechar *_ltot( long value, jsechar *string, int radix );

/* <stddef.h> */
typedef unsigned int size_t;

/* <sys\stat.h> */
int _tstat( const jsechar *path, struct _stat *buffer );

/* <direct.h> */
int _tchdir( const jsechar *dirname );
jsechar *_tgetcwd( jsechar *buffer, int maxlen );
int _tmkdir( const jsechar *dirname );
int _trmdir( const jsechar *dirname );



/* <math.h> */
double atof( const char *string );

/* <search.h> */
void *bsearch( const void *key, const void *base, size_t num, size_t width, int ( __cdecl *compare ) ( const void *elem1, const void *elem2 ) );

/* <time.h> */

typedef long clock_t;

struct tm {
        int tm_sec;     /* seconds after the minute - [0,59] */
        int tm_min;     /* minutes after the hour - [0,59] */
        int tm_hour;    /* hours since midnight - [0,23] */
        int tm_mday;    /* day of the month - [1,31] */
        int tm_mon;     /* months since January - [0,11] */
        int tm_year;    /* years since 1900 */
        int tm_wday;    /* days since Sunday - [0,6] */
        int tm_yday;    /* days since January 1 - [0,365] */
        int tm_isdst;   /* daylight savings time flag */
        };

#define CLOCKS_PER_SEC  1000

time_t time( time_t *timer );
clock_t clock( void );
jsechar *_tctime( const time_t *timer );
time_t mktime( struct tm *timeptr );
jsechar *_tasctime( const struct tm *timeptr );
struct tm *gmtime( const time_t *timer );
struct tm *localtime( const time_t *timer );
size_t _tcsftime( jsechar *string, size_t maxsize, const jsechar *format, const struct tm *timeptr );

/* <timeb.h> */
struct _timeb {
	time_t time;
	unsigned short millitm;
	short timezone;
	short dstflag;
	};

void _ftime( struct _timeb *timeptr );

#define timeb     _timeb
#define ftime     _ftime

/* <io.h> */
int _taccess( const jsechar *path, int mode );
int _chsize( int handle, long size );
int _locking( int handle, int mode, long nbytes );

#define chsize    _chsize
#define locking   _locking

/* File attribute constants for _findfirst() */

#define _A_NORMAL	0x00	/* Normal file - No read/write restrictions */
#define _A_RDONLY	0x01	/* Read only file */
#define _A_HIDDEN	0x02	/* Hidden file */
#define _A_SYSTEM	0x04	/* System file */
#define _A_SUBDIR	0x10	/* Subdirectory */
#define _A_ARCH 	0x20	/* Archive file */


/* <sys\stat.h> && <sys\types> */
/* define structure for returning status information */
typedef unsigned short _ino_t;
typedef unsigned int _dev_t;
typedef long _off_t;

#define ino_t _ino_t
#define dev_t _dev_t
#define off_t _off_t

struct _stat {
	_dev_t st_dev;
	_ino_t st_ino;
	unsigned short st_mode;
	short st_nlink;
	short st_uid;
	short st_gid;
	_dev_t st_rdev;
	_off_t st_size;
	time_t st_atime;
	time_t st_mtime;
	time_t st_ctime;
	};

#define stat      _stat

#define _S_IFMT 	0170000 	/* file type mask */
#define _S_IFDIR	0040000 	/* directory */
#define _S_IFCHR	0020000 	/* character special */
#define _S_IFIFO	0010000 	/* pipe */
#define _S_IFREG	0100000 	/* regular */
#define _S_IREAD	0000400 	/* read permission, owner */
#define _S_IWRITE	0000200 	/* write permission, owner */
#define _S_IEXEC	0000100 	/* execute/search permission, owner */

#define S_IFMT   _S_IFMT
#define S_IFDIR  _S_IFDIR
#define S_IFCHR  _S_IFCHR
#define S_IFREG  _S_IFREG
#define S_IREAD  _S_IREAD
#define S_IWRITE _S_IWRITE
#define S_IEXEC  _S_IEXEC

/* <sys\locking> */
#define _LK_UNLCK	0	/* unlock the file region */
#define _LK_LOCK	1	/* lock the file region */
#define _LK_NBLCK	2	/* non-blocking lock */
#define _LK_RLCK	3	/* lock for writing */
#define _LK_NBRLCK	4	/* non-blocking lock for writing */

#define LK_UNLCK    _LK_UNLCK
#define LK_LOCK     _LK_LOCK
#define LK_NBLCK    _LK_NBLCK
#define LK_RLCK     _LK_RLCK
#define LK_NBRLCK   _LK_NBRLCK

#if 0
#define fdopen_jsechar      fdopen
#define fsopen_jsechar      _fsopen
#define rmtmp_jsechar       _rmtmp
#define setbuf_jsechar      setbuf
#define setvbuf_jsechar     setvbuf
#endif /* 0 */

/* <assert.h> */

#ifdef NDEBUG
#  define assert(exp)   ((void)0)
#else
   _CRTIMP void __cdecl _assert(void *, void *, unsigned);
#  define assert(exp) (void)( (exp) || (_assert(#exp, __FILE__, __LINE__), 0) )
#endif /* NDEBUG */


///////////////////////////////////////////////////////////////////////////

/* following issues are related to API calls which unsupported by Windows CE*/

#define  HDROP                    HANDLE

#define  SW_RESTORE               SW_SHOWNORMAL
#define  SM_CXFRAME               SM_CXFIXEDFRAME
#define  SM_CYFRAME               SM_CYFIXEDFRAME

#define  SIZENORMAL               SIZE_RESTORED
#define  SIZEFULLSCREEN           SIZE_MAXIMIZED
#define  SIZEZOOMSHOW             SIZE_MAXSHOW

#define  SYSTEM_FIXED_FONT        SYSTEM_FONT

#define  MB_TASKMODAL             MB_APPLMODAL
#define  MB_SYSTEMMODAL           MB_APPLMODAL

#define  GCL_HBRBACKGROUND        -10

typedef struct tagMINMAXINFO {
    POINT ptReserved;
    POINT ptMaxSize;
    POINT ptMaxPosition;
    POINT ptMinTrackSize;
    POINT ptMaxTrackSize;
} MINMAXINFO, *PMINMAXINFO, *LPMINMAXINFO;

#define  WM_GETMINMAXINFO         0x0024
#define  WM_NCCREATE              0x0081
#if defined(_WIN32_WCE_EMULATION)
#  define  WM_NCDESTROY           WM_APP-1
#else
#  define  WM_NCDESTROY           0x0082
#endif
#define  WM_DROPFILES             0x0233

#ifdef __cplusplus
}
#endif
#endif
