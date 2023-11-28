#ifndef _STDLIB_H_
#define _STDLIB_H_

#include <be_setup.h>

#include <div_t.h>
#include <size_t.h>
#include <null.h>
#include <wchar_t.h>
#include <stddef.h>
#include <limits.h>
#include <sys/wait.h>
#include <alloca.h>

#if __GNUC__
#define RAND_MAX		0x7fffffff
#define	MB_CUR_MAX	(__ctype_get_mb_cur_max ())
#else
#define RAND_MAX		32767
#define MB_CUR_MAX	    1
#endif

#define EXIT_SUCCESS		0
#define EXIT_FAILURE		1

__extern_c_start

double	atof(const char *str);
int		atoi(const char *str); 
long	atol(const char *str); 

#if __GNUC__
extern long long int atoll(const char *str);
extern int __ctype_get_mb_cur_max(void);
#endif

double				strtod  (const char *str, char **end); 
long				strtol  (const char *str, char **end, int base); 
unsigned long		strtoul (const char *str, char **end, int base); 
long long			strtoll (const char *str, char **end, int base);
unsigned long long	strtoull(const char *str, char **end, int base);

void	srand(unsigned int seed); 
int		rand(void); 
int		rand_r(unsigned int *seed);
void	srandom(unsigned int seed);
int		random(void);
char 	*initstate(unsigned int seed, char *state, int n);
char 	*setstate(char *state);
char 	*realpath(const char *path, char *resolved);

#if __GNUC__

struct random_data  {
    int *fptr;		 
    int *rptr;		 
    int *state;		 
    int rand_type;		 
    int rand_deg;		 
    int rand_sep;		 
    int *end_ptr;		 
};

int 	random_r(struct random_data *buf, int *result);
int 	srandom_r(unsigned int seed, struct random_data *buf);
int 	initstate_r(unsigned int seed, void *statebuf, 
				size_t statelen, struct random_data *buf);
int 	setstate_r(void *statebuf, struct random_data *buf);

double drand48(void);
double erand48(unsigned short int xsubi[3]);

long lrand48(void);
long nrand48(unsigned short int xsubi[3]);
	 
long mrand48(void);
long jrand48(unsigned short int xsubi[3]);

void srand48(long int seedval);
unsigned short *seed48(unsigned short int seed16v[3]);
void lcong48(unsigned short int param[7]);
 
struct drand48_data  {
    unsigned short int x[3];	 
    unsigned short int a[3];	 
    unsigned short int c;	 
    unsigned short int old_x[3];  
    int init;			 
};
 
int 	drand48_r(struct drand48_data *buffer, double *result);
int 	erand48_r(unsigned short int xsubi[3],
					struct drand48_data *buffer, double *result)  ;
int 	lrand48_r(struct drand48_data *buffer, long int *result)  ;
int 	nrand48_r(unsigned short int xsubi[3],
					struct drand48_data *buffer, long int *result)  ;
int 	mrand48_r(struct drand48_data *buffer, long int *result)  ;
int 	jrand48_r(unsigned short int xsubi[3],
				  struct drand48_data *buffer, long int *result)  ;
int 	srand48_r(long int seedval, struct drand48_data *buffer)  ;
int 	seed48_r(unsigned short int seed16v[3],
				 struct drand48_data *buffer)  ;
int 	lcong48_r(unsigned short int param[7],
				  struct drand48_data *buffer)  ;

extern double __strtod_internal(const char *__nptr, char **__endptr, int __group);
extern long int __strtol_internal(const char *__nptr, char **__endptr,
								   int __base, int __group);
extern unsigned long int __strtoul_internal(const char *__nptr, char **__endptr,
											int __base, int __group);
extern long long int __strtoll_internal(const char *__nptr, char **__endptr,
										int __base, int __group);
extern unsigned long long int __strtoull_internal(const char *__nptr, char **__endptr,
												  int __base, int __group);

#if __OPTIMIZE__

extern __inline double
strtod (const char * __nptr, char ** __endptr)
{
  return __strtod_internal (__nptr, __endptr, 0);
}
extern __inline long int
strtol (const char * __nptr, char ** __endptr,
	int __base)
{
  return __strtol_internal (__nptr, __endptr, __base, 0);
}
extern __inline unsigned long int
strtoul (const char * __nptr, char ** __endptr,
	 int __base)
{
  return __strtoul_internal (__nptr, __endptr, __base, 0);
}
extern __inline long long int
strtoll (const char * __nptr, char ** __endptr,
	 int __base)
{
  return __strtoll_internal (__nptr, __endptr, __base, 0);
}
extern __inline unsigned long long int
strtoull (const char *  __nptr, char ** __endptr,
	  int __base)
{
  return __strtoull_internal (__nptr, __endptr, __base, 0);
}
extern __inline double
atof (const char *__nptr)
{
  return strtod (__nptr, (char **) NULL);
}
extern __inline int
atoi (const char *__nptr)
{
  return (int) strtol (__nptr, (char **) NULL, 10);
}
extern __inline long int
atol (const char *__nptr)
{
  return strtol (__nptr, (char **) NULL, 10);
}
extern __inline long long int
atoll (const char *__nptr)
{
  return strtoll (__nptr, (char **) NULL, 10);
}

#endif /* __OPTIMIZE__ */

#endif /* __GNUC__ */

void 	*calloc(size_t nmemb, size_t size); 
void	free(void *ptr); 
void 	*malloc(size_t size); 
void 	*realloc(void * ptr, size_t size); 

void	abort(void); 
int		atexit(void (*func)(void)); 
int		atfork(void (*func)(void));
void	exit(int status); 

extern  char **environ;
char *	getenv(const char *name); 
int 	putenv(const char *string);
int 	setenv(const char *name, const char *value, int rewrite);
void	unsetenv(const char *name);
void 	setproctitle(const char *fmt, ...);

int 	daemon(int nochdir, int noclose);
     
int		system(const char *command); 

char   *mktemp(char *name);
#if __GNUC__
int 	mkstemp(char *tmplate);
#endif /* __GNUC__ */

typedef int (*_compare_function)(const void*, const void*);
void	*bsearch(const void*, const void*, size_t, size_t, _compare_function);
void  	qsort(void*, size_t, size_t, _compare_function);
							
int		abs(int n);
long	labs(long n);


div_t	div(int numerator,  int denominator);
ldiv_t	ldiv(long numerator, long denominator);

int		mblen(const char *s, size_t n);
int		mbtowc(wchar_t *pwc, const char *s, size_t n);
int		wctomb(char *s, wchar_t wchar);
size_t	mbstowcs(wchar_t *pwcs, const char *s, size_t n);
size_t	wcstombs(char *s, const wchar_t *pwcs, size_t n);

__extern_c_end


#endif /* _STDLIB_H_ */
