#ifndef _STDIO_H_
# error "Never include gnu_stdio.h directly! Include stdio.h instead."
#endif

#ifndef _GNU_STDIO_H_ 
#define _GNU_STDIO_H_ 

#include <stddef.h>
#include <stdarg.h>
#include <sys/types.h>
#include <_G_config.h>

__extern_c_start

#ifdef __cplusplus
# define __STDIO_INLINE inline
#else
# define __STDIO_INLINE extern __inline
#endif 

#define _STDIO_USES_IOSTREAM

#define _IOFBF 0 		/* Fully buffered.  */
#define _IOLBF 1		/* Line buffered.  */
#define _IONBF 2		/* No buffering.  */

#include <libio.h>

typedef struct _IO_FILE FILE;

/* Default buffer size.  */
#ifndef BUFSIZ
# define BUFSIZ _IO_BUFSIZ
#endif

extern FILE *stdin;		 
extern FILE *stdout;		 
extern FILE *stderr;		 

extern char *tmpnam_r(char *__s);
extern int fflush_unlocked(FILE *__stream);
extern int vfprintf(FILE *__s, const char * __format, va_list  __arg);
extern int vprintf(const char *__format, va_list __arg);

__STDIO_INLINE  int
vprintf (const char * __fmt, va_list  __arg)
{
 	return vfprintf (stdout, __fmt, __arg);
}

extern int snprintf(char *   __s, size_t __maxlen, const char *   __format, ...)  
	__attribute__ ((__format__ (__printf__, 3, 4)));
extern int vsnprintf(char *   __s, size_t __maxlen, const char *   __format, va_list  __arg)
	__attribute__ ((__format__ (__printf__, 3, 0)));
	
extern char	_single_threaded;
extern int getchar(void);
extern int getc(FILE *__stream);
extern int putchar(int __c);
	
#define getc(_fp) (_single_threaded ? _IO_getc_unlocked (_fp) : _IO_getc (_fp))

__STDIO_INLINE int
getchar(void) { return getc(stdin); }

extern int getc_unlocked(FILE *__stream);
extern int getchar_unlocked(void);

__STDIO_INLINE  int
getc_unlocked (FILE *__fp) { return _IO_getc_unlocked(__fp); }

__STDIO_INLINE  int
getchar_unlocked (void) { return _IO_getc_unlocked(stdin); }

extern int fputc(int __c, FILE *__stream);
extern int putc(int __c, FILE *__stream);

#define putc(_ch, _fp) (_single_threaded ? _IO_putc_unlocked (_ch, _fp) : _IO_putc (_ch, _fp))

__STDIO_INLINE  int
putchar(int __c) { 	return putc (__c, stdout); }

extern int fputc_unlocked  (int __c, FILE *__stream)  ;
__STDIO_INLINE  int
fputc_unlocked (int __c, FILE *__stream) { return _IO_putc_unlocked (__c, __stream); }

extern int putc_unlocked  (int __c, FILE *__stream);
extern int putchar_unlocked  (int __c)  ;

__STDIO_INLINE  int
putc_unlocked (int __c, FILE *__stream) {  return _IO_putc_unlocked (__c, __stream); }

__STDIO_INLINE  int
putchar_unlocked (int __c) { return _IO_putc_unlocked (__c, stdout); }

extern size_t fread_unlocked(void *__ptr, size_t __size, 
							 size_t __n, FILE *__stream);
extern size_t fwrite_unlocked(__const void *__ptr, size_t __size, 
							  size_t __n, FILE *__stream);

extern int fsetpos  (FILE *__stream, __const fpos_t *__pos);
extern void clearerr_unlocked  (FILE *__stream);
extern int feof_unlocked  (FILE *__stream)  ;
extern int ferror_unlocked  (FILE *__stream)  ;

__STDIO_INLINE  int
feof_unlocked (FILE *__stream) {  return _IO_feof_unlocked (__stream); }

__STDIO_INLINE  int
ferror_unlocked (FILE *__stream) { return _IO_ferror_unlocked (__stream); }

extern int fileno_unlocked(FILE *__stream);
extern void flockfile(FILE *__stream);
extern int ftrylockfile(FILE *__stream);
extern void funlockfile(FILE *__stream);

__extern_c_end

#undef __STDIO_INLINE 

#endif /* _GNU_STDIO_H  */
