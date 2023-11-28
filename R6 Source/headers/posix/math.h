#ifdef __cplusplus
#define exception __math_exception
#endif
#ifndef _MATH_H_
#define _MATH_H_

#include <be_setup.h>

__extern_c_start

double acos(double);
double asin(double);
double atan(double);
double atan2(double, double);
double ceil(double);
double cos(double);
double cosh(double);
double exp(double);
double fabs(double);
double floor(double);
double fmod(double, double);
double frexp(double, int *);
double ldexp(double, int);
double log(double);
double log10(double);
double modf(double, double *);
double pow(double, double);
double sin(double);
double sinh(double);
double sqrt(double);
double tan(double);
double tanh(double);

/* the Berkeley standard math functions */
extern double erf(double);
extern double erfc(double);
extern double gamma(double);
extern double hypot(double, double);
extern double j0(double);
extern double j1(double);
extern double jn(int, double);
extern double lgamma(double);
extern double y0(double);
extern double y1(double);
extern double yn(int, double);


#if __GNUC__

extern double gamma_r(double , int *);
extern double lgamma_r(double , int *);

#endif /* __GNUC__ */

extern double acosh(double);
extern double asinh(double);
extern double atanh(double);
extern double cbrt(double);
extern double expm1(double);
extern double logb(double);
extern int    ilogb(double);
extern double log1p(double);
extern double nextafter(double, double);
extern double remainder(double, double);
extern double rint(double);
extern double scalb (double, double);

extern int signgam;

#ifdef __cplusplus
struct __exception;
extern int matherr(struct __exception *);
struct __exception  {
#else
struct exception;
extern int matherr(struct exception *);
struct exception {
#endif
    int type;
    char *name;
    double arg1;
    double arg2;
    double retval;
};

#define DOMAIN		1
#define SING		2
#define OVERFLOW	3
#define UNDERFLOW	4
#define TLOSS		5
#define PLOSS		6

/*
 * other stuff...
 */
extern double significand(double);
extern double copysign(double, double);
extern double scalbn(double, int);
extern double drem(double, double);
extern int isnan(double);
extern int isfinite(double);
extern int finite(double);
extern float modff(float, float*);

#if __GNUC__	/* Seems to be floating point format specific (IEEE?) */

extern float acosf(float x);
extern float asinf(float x);
extern float atanf(float x);
extern float atan2f(float y, float x);
extern float cosf(float x);
extern float sinf(float x);
extern float tanf(float x);
extern float coshf(float x);
extern float sinhf(float x);
extern float tanhf(float x);
extern float acoshf(float x);
extern float asinhf(float x);
extern float atanhf(float x);
extern float expf(float x);
extern float frexpf(float x, int *exponent);
extern float ldexpf(float x, int exponent);
extern float logf(float x);
extern float log10f(float x);
extern float expm1f(float x);
extern float log1pf(float x);
extern float logbf(float x);
extern float powf(float x, float y);
extern float sqrtf(float x);
extern float hypotf(float x, float y);
extern float cbrtf(float x);
extern float ceilf(float x);
extern float fabsf(float x);
extern float floorf(float x);
extern float fmodf(float x, float y);
extern int isinff(float value);
extern int finitef(float value);
extern float infnanf(int error);
extern float dremf(float x, float y);
extern float significandf(float x);
extern float copysignf(float x, float y);
extern int isnanf(float value);
extern float j0f(float);
extern float j1f(float);
extern float jnf(int, float);
extern float y0f(float);
extern float y1f(float);
extern float ynf(int, float);
extern float erff(float);
extern float erfcf(float);
extern float gammaf(float);
extern float lgammaf(float);
extern float gammaf_r(float, int *);
extern float lgammaf_r(float, int *);
extern float rintf(float x);
extern float nextafterf(float x, float y);
extern float remainderf(float x, float y);
extern float scalbf(float x, float n);
extern float scalbnf(float x, int n);
extern int ilogbf(float x);

#define	__HUGE_VAL_bytes	{ 0, 0, 0, 0, 0, 0, 0xf0, 0x7f }
#define __huge_val_t	union { unsigned char __c[8]; double __d; }
#ifndef HUGE_VAL
#define HUGE_VAL	(((__huge_val_t) { __c: __HUGE_VAL_bytes }).__d)
#endif

#define __HUGE_VALF_bytes	{ 0, 0, 0x80, 0x7f }
#define __huge_valf_t	union { unsigned char __c[4]; float __f; }
#define HUGE_VALF	(((__huge_valf_t) { __c: __HUGE_VALF_bytes }).__f)

#define __HUGE_VALL_bytes	{ 0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0x7f, 0, 0 }
#define __huge_vall_t	union { unsigned char __c[12]; long double __ld; }
#define HUGE_VALL	(((__huge_vall_t) { __c: __HUGE_VALL_bytes }).__ld)

#define NAN \
  (((union { unsigned __l __attribute__((__mode__(__SI__))); float __d; })  \
    { __l: 0x7fc00000UL }).__d)

#define INFINITY	HUGE_VALF

/* All floating-point numbers can be put in one of these categories.  */
enum
  {
    FP_NAN,
# define FP_NAN FP_NAN
    FP_INFINITE,
# define FP_INFINITE FP_INFINITE
    FP_ZERO,
# define FP_ZERO FP_ZERO
    FP_SUBNORMAL,
# define FP_SUBNORMAL FP_SUBNORMAL
    FP_NORMAL
# define FP_NORMAL FP_NORMAL
  };

/*
 * prototypes for funcs used in macros
 */
extern int __fpclassifyf(float);
extern int __signbitf(float);
extern int __finitef(float);
extern int __isnanf(float);
extern int __isinff(float);

extern int __fpclassifyl(double);
extern int __signbitl(double);
extern int __finitel(double);
extern int __isnanl(double);
extern int __isinfl(double);

extern int __fpclassify(double);
extern int __signbit(double);
extern int __finite(double);
extern int __isnan(double);
extern int __isinf(double);

/* Return number of classification appropriate for X.  */
# define fpclassify(x) \
     (sizeof (x) == sizeof (float) ?                                          \
        __fpclassifyf (x)                                                     \
      : sizeof (x) == sizeof (double) ?                                       \
        __fpclassify (x) : __fpclassifyl (x))

/* Return nonzero value if sign of X is negative.  */
# define signbit(x) \
     (sizeof (x) == sizeof (float) ?                                          \
        __signbitf (x)                                                        \
      : sizeof (x) == sizeof (double) ?                                       \
        __signbit (x) : __signbitl (x))

/* Return nonzero value if X is not +-Inf or NaN.  */
# define isfinite(x) \
     (sizeof (x) == sizeof (float) ?                                          \
        __finitef (x)                                                         \
      : sizeof (x) == sizeof (double) ?                                       \
        __finite (x) : __finitel (x))

/* Return nonzero value if X is neither zero, subnormal, Inf, nor NaN.  */
# define isnormal(x) (fpclassify (x) == FP_NORMAL)

/* Return nonzero value if X is a NaN.  We could use `fpclassify' but
   we already have this functions `__isnan' and it is faster.  */
# define isnan(x) \
     (sizeof (x) == sizeof (float) ?                                          \
        __isnanf (x)                                                          \
      : sizeof (x) == sizeof (double) ?                                       \
        __isnan (x) : __isnanl (x))

/* Return nonzero value is X is positive or negative infinity.  */
# define isinf(x) \
     (sizeof (x) == sizeof (float) ?                                          \
        __isinff (x)                                                          \
      : sizeof (x) == sizeof (double) ?                                       \
        __isinf (x) : __isinfl (x))


#elif __POWERPC__
/* float versions of the standard functions */
double exp2(double x);
double log2(double x);
double scalbn(double x, int n); 

double nearbyint(double x);
double round(double x);
double trunc(double x);
double remquo(double x, double y, int *quo);
double fdim(double x, double y);
double fmax(double x, double y);
double fmin(double x, double y);
double scalbln(double x, long n);
long int lrint(double x);
long long llrint(double x);
long int lround(double x);
long long llround(double x);

extern const double *__huge_val;
extern const float 	*__huge_val_float;
extern const double	*__huge_val_extended;
extern const double	*__nan_val;
extern const float  *__nan_val_float;


#ifndef HUGE_VAL
#define HUGE_VAL 	(*__huge_val)
#endif
#define HUGE_VALF	(*__huge_val_float)
#define HUGE_VALL	(*__huge_val_extended)
#define INFINITY	(*(double *) __huge_val)
#define NAN			(*(double *) __nan_val)

#define FP_NAN       1  /*   quiet NaN (signaling Nan supported on MAC but nowhere else. */
#define FP_INFINITE  2  /*   + or - infinity      */
#define FP_ZERO      3  /*   + or - zero          */
#define FP_NORMAL    4  /*   all normal numbers   */
#define FP_SUBNORMAL 5  /*   denormal numbers     */

#ifdef __HI
# undef __HI
#endif

# define __LO(x) (*(1+(int*)&x) )
# define __HI(x) (*(int*)&x)

/* start out w/ definitions of inlines/macros which are neither extern "C" nor in namespace std */
#pragma cplusplus on
/* fpclassify for floats */

inline int __fpclassifyf(float x)
{
 switch( (*(int*)&x)&0x7f800000 )
  {
  case 0x7f800000:
   {
    if((*(int*)&x)&0x007fffff) return FP_NAN;
    else return FP_INFINITE;
    break;
   }
  case 0:
   {
    if((*(int*)&x)&0x007fffff) return FP_SUBNORMAL;
    else return FP_ZERO; 
    break; 
   }
  }
  return FP_NORMAL;
}  

/* fpclassify for doubles or integral types */

inline int __fpclassifyd(double x) 
{
 switch(__HI(x)&0x7ff00000 )
  {
   case 0x7ff00000:
   {
    if((__HI(x)&0x000fffff) || (__LO(x)&0xffffffff)) return FP_NAN;
    else return FP_INFINITE;
    break;
   }
  case 0:
  {
    if((__HI(x)&0x000fffff) || (__LO(x)&0xffffffff)) return FP_SUBNORMAL;
    else return FP_ZERO; 
    break; 
  }
  
  }
  return FP_NORMAL;
} 
 
inline int __signbitf(float x) {return (int)((int)(*(int*)&x)&0x80000000);}
inline int __signbitd(double x){return  (int)(     __HI(x)&0x80000000);}

/*  7.7.3  Classification macros */

#define fpclassify(x)  \
	 ((sizeof(x) == sizeof(float))  ? __fpclassifyf((float)(x)) \
     :  __fpclassifyd((double)(x)) )
	

#define isnormal(x) (fpclassify(x) == FP_NORMAL)
#define isnan(x)    (fpclassify(x) == FP_NAN)
#define isinf(x)    (fpclassify(x) == FP_INFINITE)
#define isfinite(x) ((fpclassify(x) > FP_INFINITE))
  
#define signbit(x)  \
		 ((sizeof(x) == 4)  ? __signbitf((float)(x)) \
		:  __signbitd((double)(x)) )

__extern_c_end

/* inlines work for both C and C++, there is no
ambiguity since we have exactly one overload */
                         
                         
/* C9X foof's-- only the 22 K&R math functions have actual foof
   implementations(prototypes), keep the
   others as inlines until they are actually written.
*/   

    inline float acosf(float x)
		{return (float)acos((double)x);}
	inline float asinf(float x)
		{return (float)asin((double)x);}
	inline float atanf(float x)
		{return (float)atan((double)x);}
	inline float atan2f(float y, float x)
		{return (float)atan2((double)y, (double)x);}
	inline float cosf(float x)
		{return (float)cos((double)x);}
	inline float sinf(float x)
		{return (float)sin((double)x);}
	inline float tanf(float x)
		{return (float)tan((double)x);}
	inline float coshf(float x)
		{return (float)cosh((double)x);}
	inline float sinhf(float x)
		{return (float)sinh((double)x);}
	inline float tanhf(float x)
		{return (float)tanh((double)x);}
	inline float expf(float x)
		{return (float)exp((double)x);}
	inline float frexpf(float x, int* exp)
		{return (float)frexp((double)x, exp);}
	inline float ldexpf(float x, int exp)
		{return (float)ldexp((double)x, exp);}
	inline float logf(float x)
		{return (float)log((double)x);}
	inline float log10f(float x)
		{return (float)log10((double)x);}
    inline float fabsf(float x)
		{return (float)fabs((double)x);}
		
	inline float powf(float x, float y)
		{return (float)pow((double)x, (double)y);}
	inline float sqrtf(float x)
		{return (float)sqrt((double)x);}
	inline float ceilf(float x)
		{return (float)ceil((double)x);}
	inline float floorf(float x)
		{return (float)floor((double)x);}
	inline float fmodf(float x, float y)
		{return (float)fmod((double)x, (double)y);}

    inline float acoshf(float x)
		{return (float)acosh((double)x);}
	inline float asinhf(float x)
		{return (float)asinh((double)x);}
	inline float atanhf(float x)
		{return (float)atanh((double)x);}
	inline float exp2f(float x)
		{return (float)exp2((double)x);}
	inline float expm1f(float x)
		{return (float)expm1((double)x);}
	inline float log1pf(float x)
		{return (float)log1p((double)x);}
	inline float log2f(float x)
		{return (float)log2((double)x);}
	inline float logbf(float x)
		{return (float)logb((double)x);}
	inline float scalbnf(float x, int n)
		{return (float)scalbn((double)x, n);}
	inline float scalblnf(float x, long int n)
		{return (float)scalbn((double)x, n);}
	inline float hypotf(float x, float y)
		{return (float)hypot((double)x, (double)y);}

    inline float erff(float x)
		{return (float)erf((double)x);}
	inline float erfcf(float x)
		{return (float)erfc((double)x);}
	inline float gammaf(float x)
		{return (float)gamma((double)x);}
	inline float lgammaf(float x)
		{return (float)lgamma((double)x);}
	inline float nextafterf(float x, float y)
		{return (float)nextafter((double)x, (double)y);}
	inline float nearbyintf(float x)
		{return (float)nearbyint((double)x);}
	inline float rintf(float x)
		{return (float)rint((double)x);}
	inline long int lrintf(float x)
		{return lrint((double)x);}
	inline long long llrintf(float x)
		{return llrint((double)x);}
	inline float roundf(float x)
		{return (float)round((double)x);}
	inline long int lroundf(float x)
		{return lround((double)x);}
	inline long long llroundf(float x)
		{return llround((double)x);}
	inline float truncf(float x)
		{return (float)trunc((double)x);}
	inline float remainderf(float x, float y)
		{return (float)remainder((double)x, (double)y);}
	inline float copysignf(float x, float y)
		{return (float)copysign((double)x, (double)y);}
	inline float remquof(float x, float y, int *quo)
		{return (float)remquo((double)x, (double)y, quo);}
	inline float fdimf(float x, float y)
		{return (float)fdim((double)x, (double)y);}
	inline float fmaxf(float x, float y)
		{return (float)fmax((double)x, (double)y);}
	inline float fminf(float x, float y)
		{return (float)fmin((double)x, (double)y);}
	
    inline double scalbln(double x, long n)
		{return (double)scalbn(x, (long)n);}
	inline long long llrint(double x)
		{return (long long)rint(x);}
	inline long long llround(double x)
		{return (long long)round(x);}
			
/* long double math functions(fool's), we do not support true long double outside of macos 68K */
			
	inline long double acosl(long double x)
		{return acos((double)x);}
	inline long double asinl(long double x)
		{return asin((double)x);}
	inline long double atanl(long double x)
		{return atan((double)x);}
	inline long double atan2l(long double y, long double x)
		{return atan2((double)y, (double)x);}
	inline long double cosl(long double x)
		{return cos((double)x);}
	inline long double sinl(long double x)
		{return sin((double)x);}
	inline long double tanl(long double x)
		{return tan((double)x);}
	inline long double coshl(long double x)
		{return cosh((double)x);}
	inline long double sinhl(long double x)
		{return sinh((double)x);}
	inline long double tanhl(long double x)
		{return tanh((double)x);}
	inline long double acoshl(long double x)
		{return acosh((double)x);}
	inline long double asinhl(long double x)
		{return asinh((double)x);}
	inline long double atanhl(long double x)
		{return atanh((double)x);}
	inline long double expl(long double x)
		{return exp((double)x);}
	inline long double frexpl(long double x, int* exp)
		{return frexp((double)x, exp);}
	inline long double ldexpl(long double x, int exp)
		{return ldexp((double)x, exp);}
	inline long double logl(long double x)
		{return log((double)x);}
	inline long double log10l(long double x)
		{return log10((double)x);}
	inline long double modfl(long double x, long double* iptr)
		{
			double iptrd;
			long double result = modf((double)x, &iptrd);
			*iptr = iptrd;
			return result;
		}
	
	inline long double exp2l(long double x)
		{return exp2((double)x);}
	inline long double expm1l(long double x)
		{return expm1((double)x);}
	inline long double log1pl(long double x)
		{return log1p((double)x);}
	inline long double log2l(long double x)
		{return log2((double)x);}
	inline long double logbl(long double x)
		{return logb((double)x);}
	inline long double scalbnl(long double x, int n)
		{return scalbn((double)x, n);}
	inline long double scalblnl(long double x, long int n)
		{return scalbn((double)x, n);}
	inline long double fabsl(long double x)
		{return fabs((double)x);}
	inline long double powl(long double x, long double y)
		{return pow((double)x, (double)y);}
	inline long double sqrtl(long double x)
		{return sqrt((double)x);}
	inline long double hypotl(long double x, long double y)
		{return hypot((double)x, (double)y);}
		
	inline long double erfl(long double x)
		{return erf((double)x);}
	inline long double erfcl(long double x)
		{return erfc((double)x);}
	inline long double gammal(long double x)
		{return gamma((double)x);}
	inline long double lgammal(long double x)
		{return lgamma((double)x);}
	inline long double nextafterl(long double x, long double y)
		{return nextafter((double)x, (double)y);}
	
	inline long double ceill(long double x)
		{return ceil((double)x);}
	inline long double floorl(long double x)
		{return floor((double)x);}
	inline long double nearbyintl(long double x)
		{return nearbyint((double)x);}
	inline long double rintl(long double x)
		{return rint((double)x);}
	inline long int lrintl(long double x)
		{return lrint((double)x);}
	inline long long llrintl(long double x)
		{return llrint((double)x);}
	inline long double roundl(long double x)
		{return round((double)x);}
	inline long int lroundl(long double x)
		{return lround((double)x);}
	inline long long llroundl(long double x)
		{return llround((double)x);}
	inline long double truncl(long double x)
		{return trunc((double)x);}
	inline long double fmodl(long double x, long double y)
		{return fmod((double)x, (double)y);}
	inline long double remainderl(long double x, long double y)
		{return remainder((double)x, (double)y);}
	inline long double copysignl(long double x, long double y)
		{return copysign((double)x, (double)y);}
	inline long double remquol(long double x, long double y, int *quo)
		{return remquo((double)x, (double)y, quo);}
	
	inline long double fdiml(long double x, long double y)
		{return fdim((double)x, (double)y);}
	inline long double fmaxl(long double x, long double y)
		{return fmax((double)x, (double)y);}
	inline long double fminl(long double x, long double y)
		{return fmin((double)x, (double)y);}

#pragma cplusplus reset

__extern_c_start

#endif /* __POWERPC__ */

__extern_c_end

#ifndef M_E
#define M_E         2.7182818284590452354   /* e */
#endif
#ifndef M_LOG2E
#define M_LOG2E     1.4426950408889634074   /* log 2e */
#endif
#ifndef M_LOG10E
#define M_LOG10E    0.43429448190325182765  /* log 10e */
#endif
#ifndef M_LN2
#define M_LN2       0.69314718055994530942  /* log e2 */
#endif
#ifndef M_LN10
#define M_LN10      2.30258509299404568402  /* log e10 */
#endif
#ifndef M_PI
#define M_PI        3.14159265358979323846  /* pi */
#endif
#ifndef M_PI_2
#define M_PI_2      1.57079632679489661923  /* pi/2 */
#endif
#ifndef M_1_PI
#define M_1_PI      0.31830988618379067154  /* 1/pi */
#endif
#ifndef M_PI_4
#define M_PI_4      0.78539816339744830962  /* pi/4 */
#endif
#ifndef M_2_PI
#define M_2_PI      0.63661977236758134308  /* 2/pi */
#endif
#ifndef M_2_SQRTPI
#define M_2_SQRTPI  1.12837916709551257390  /* 2/sqrt(pi) */
#endif
#ifndef M_SQRT2
#define M_SQRT2     1.41421356237309504880  /* sqrt(2) */
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2   0.70710678118654752440  /* 1/sqrt(2) */
#endif

#ifndef PI                      /* as in stroustrup */
#define PI  M_PI
#endif
#ifndef PI2
#define PI2  M_PI_2
#endif


#endif /* _MATH_H_ */
#ifdef __cplusplus
#undef exception
#endif
