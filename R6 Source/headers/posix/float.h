#if __GNUC__
# include_next <float.h>
#else

#ifndef __FLOAT_H__
#define __FLOAT_H__

#include <be_setup.h>

__extern_c_start

#define FLT_ROUNDS					1 
#define FLT_RADIX					2
#define FLT_MANT_DIG				24
#define FLT_DIG						6
#define FLT_MIN_EXP					(-125)
#define FLT_MIN_10_EXP				(-37)
#define FLT_MAX_EXP					128
#define FLT_MAX_10_EXP				38

typedef union {
	unsigned char c[4];
	float f;
} __fcu_t;

typedef union {
	unsigned char c[8];
	double d;
} __dcu_t;

typedef union {
	unsigned char c[8];
	double e;
} __ecu_t;

# define FLT_MAX					(__float_max.f)
# define FLT_EPSILON				(__float_epsilon.f)
# define FLT_MIN					(__float_min.f)

extern const __fcu_t __float_min, __float_max, __float_epsilon;

#define DBL_MANT_DIG				53
#define DBL_DIG						15
#define DBL_MIN_EXP					(-1021)
#define DBL_MIN_10_EXP				(-308)
#define DBL_MAX_EXP					1024
#define DBL_MAX_10_EXP				308

#define LDBL_MANT_DIG				DBL_MANT_DIG
#define LDBL_DIG					DBL_DIG	
#define LDBL_MIN_EXP				DBL_MIN_EXP	
#define LDBL_MIN_10_EXP				DBL_MIN_10_EXP
#define LDBL_MAX_EXP				DBL_MAX_EXP
#define LDBL_MAX_10_EXP				DBL_MAX_10_EXP


# define DBL_MAX		(__double_max.d)
# define DBL_EPSILON	(__double_epsilon.d)
# define DBL_MIN		(__double_min.d)

# define LDBL_MAX		(__extended_max.e)
# define LDBL_EPSILON	(__extended_epsilon.e)
# define LDBL_MIN		(__extended_min.e)

extern const __dcu_t __double_min, __double_max, __double_epsilon;
extern const __ecu_t __extended_min, __extended_max, __extended_epsilon;

__extern_c_end

#endif /* __FLOAT_H__ */

#endif /* else not __GNUC__ */
