#ifndef _STRING_H_
#define _STRING_H_

#include <be_setup.h>

#include <size_t.h>
#include <null.h>

__extern_c_start

void   *memchr(const void *src, int val, size_t len);
int	    memcmp(const void *src1, const void *src2, size_t len);
void   *memcpy (void *dst, const void *src, size_t len);
void   *memmove(void *dst, const void *src, size_t len);
void   *memset(void *dst, int val, size_t len);

#if __POWERPC__    /* prototypes for some special versions of memcpy() */
void *memcpy_c2nc(void *dst, const void *src, size_t len);
void *memcpy_nc2c(void *dst, const void *src, size_t len);
void *memcpy_nc2nc(void *dst, const void *src, size_t len);
void *memmove_nc(void *dst, const void *src, size_t len);
void *memset_nc(void *dst, int val, size_t len);
#endif

size_t  strlen(const char *str);
char   *strcpy(char *dst, const char *src);
	
char   *strncpy(char *dst, const char *src, size_t len);

char   *strcat(char *dst, const char *src);
char   *strncat(char *dst, const char *src, size_t len);

int     strcmp(const char *str1, const char *str2);
int     strncmp(const char *str1, const char *str2, size_t len);

int     strcoll(const char *str1, const char *str2);
size_t  strxfrm(char *str1, const char *str2, size_t len);

char   *strchr(const char *str, int chr);
char   *strrchr(const char *str, int chr);
char   *strpbrk(const char *str, const char *set);
size_t  strspn(const char *str, const char *set);
size_t  strcspn(const char *str, const char *set);
char   *strtok(char *str, const char *set);
char   *strtok_r(char *str, const char *set, char **save_ptr);

char   *strstr(const char *str, const char *pat);

char   *strerror(int errnum);
const char *strsignal(int sig);


/* prototypes for some useful but non-standard string routines */
int		strcasecmp(const char *str1, const char *str2);
int		strncasecmp(const char *str1, const char *str2, size_t nchars);

char   *strdup(const char *str);
char   *stpcpy(char *dest, const char *src);
const char *strtcopy(char *dest, const char *src, int size);	/* terminated strncpy() */

size_t	 strlcat(char *dst, const char *src, size_t len);
size_t	 strlcpy(char *dst, const char *src, size_t len);

char   *strlwr(char *str); 
char   *strupr(char *str);

char 	 *strsep(char **stringp, const char *delim);
__extern_c_end

#define __max_errstr	64   /* for internal use (ppc) */

#if 0
#if __GNUC__ 
# ifdef __OPTIMIZE__
#  include <stringinline.h>
# endif
#endif
#endif


#endif  /* _STRING_H_ */
