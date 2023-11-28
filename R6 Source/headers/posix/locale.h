#ifndef _LOCALE_H_
#define _LOCALE_H_

#include <be_setup.h>
#include <null.h>


struct lconv {
	char	*decimal_point;
	char	*thousands_sep;
	char	*grouping;
	char	*int_curr_symbol;
	char	*currency_symbol;
	char	*mon_decimal_point;
	char	*mon_thousands_sep;
	char	*mon_grouping;
	char	*positive_sign;
	char	*negative_sign;
	char	  int_frac_digits;
	char	  frac_digits;
	char	  p_cs_precedes;
	char	  p_sep_by_space;
	char	  n_cs_precedes;
	char	  n_sep_by_space;
	char	  p_sign_posn;
	char	  n_sign_posn;
};


#if __GNUC__
# define LC_CTYPE       0
# define LC_NUMERIC     1
# define LC_TIME        2
# define LC_COLLATE     3
# define LC_MONETARY    4
# define LC_MESSAGES    5
# define LC_ALL			6
#elif __POWERPC__
# define LC_FIRST		0
# define LC_COLLATE		0x1
# define LC_CTYPE		0x2
# define LC_MONETARY	0x4
# define LC_NUMERIC		0x8
# define LC_TIME		0x10
# define LC_MESSAGES	0x20
# define LC_ALL			0x3f
# define LC_LAST		LC_ALL
#endif /* __POWERPC__ */

__extern_c_start

char		 *setlocale(int category, const char *locale);
struct lconv *localeconv(void);

__extern_c_end

#endif /* _LOCALE_H_ */
