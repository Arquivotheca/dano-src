#ifndef _CTYPE_H_
#define _CTYPE_H_

#include <be_setup.h>

__extern_c_start

int isalnum (int);
int isalpha (int);
int iscntrl (int);
int isdigit (int);
int isgraph (int);
int islower (int);
int isprint (int);
int ispunct (int);
int isspace (int);
int isupper (int);
int isxdigit(int);
int tolower (int);
int toupper (int);

#if __GNUC__

# define _ISbit(bit)	(bit < 8 ? ((1 << bit) << 8) : ((1 << bit) >> 8))
enum 
{
  _ISupper = _ISbit (0),	/* UPPERCASE.  */
  _ISlower = _ISbit (1),	/* lowercase.  */
  _ISalpha = _ISbit (2),	/* Alphabetic.  */
  _ISdigit = _ISbit (3),	/* Numeric.  */
  _ISxdigit = _ISbit (4),	/* Hexadecimal numeric.  */
  _ISspace = _ISbit (5),	/* Whitespace.  */
  _ISprint = _ISbit (6),	/* Printing.  */
  _ISgraph = _ISbit (7),	/* Graphical.  */
  _ISblank = _ISbit (8),	/* Blank (usually SPC and TAB).  */
  _IScntrl = _ISbit (9),	/* Control character.  */
  _ISpunct = _ISbit (10),	/* Punctuation.  */
  _ISalnum = _ISbit (11)	/* Alphanumeric.  */
};

extern const unsigned short int *__ctype_b;	/* Characteristics.  */
extern const int *__ctype_tolower; /* Case conversions.  */
extern const int *__ctype_toupper; /* Case conversions.  */

# define __isctype(c, type) \
  (__ctype_b[(int) (c)] & (unsigned short int) type)

# define	isascii(c)	(((c) & ~0x7f) == 0)	/* If C is a 7 bit value.  */
# define	toascii(c)	((c) & 0x7f)		/* Mask off high bits.  */

# define	tolower(c)	((int) __ctype_tolower[(int) (c)])
# define	toupper(c)	((int) __ctype_toupper[(int) (c)])

# define isalnum(c)	__isctype((c), _ISalnum)
# define isalpha(c)	__isctype((c), _ISalpha)
# define iscntrl(c)	__isctype((c), _IScntrl)
# define isdigit(c)	__isctype((c), _ISdigit)
# define islower(c)	__isctype((c), _ISlower)
# define isgraph(c)	__isctype((c), _ISgraph)
# define isprint(c)	__isctype((c), _ISprint)
# define ispunct(c)	__isctype((c), _ISpunct)
# define isspace(c)	__isctype((c), _ISspace)
# define isupper(c)	__isctype((c), _ISupper)
# define isxdigit(c)	__isctype((c), _ISxdigit)

#elif __POWERPC__

extern unsigned char	__ctype_map[];
extern unsigned char	__lower_map[];
extern unsigned char	__upper_map[];

#define __control_char	0x01
#define __motion_char	0x02
#define __space_char	0x04
#define __punctuation	0x08
#define __digit			0x10
#define __hex_digit		0x20
#define __lower_case	0x40
#define __upper_case	0x80

#define __letter		(__lower_case   | __upper_case  )
#define __alphanumeric	(__letter       | __digit       )
#define __graphic		(__alphanumeric | __punctuation )
#define __printable		(__graphic      | __space_char  )
#define __whitespace	(__motion_char  | __space_char  )
#define __control       (__motion_char  | __control_char)

#define __zero_fill(c)	((int) (unsigned char) (c))

#ifdef	__cplusplus

inline int isalnum(int c)		{ return __ctype_map[__zero_fill(c)] & __alphanumeric; }
inline int isalpha(int c)		{ return __ctype_map[__zero_fill(c)] & __letter      ; }
inline int iscntrl(int c)		{ return __ctype_map[__zero_fill(c)] & __control     ; }
inline int isdigit(int c)		{ return __ctype_map[__zero_fill(c)] & __digit       ; }
inline int isgraph(int c)		{ return __ctype_map[__zero_fill(c)] & __graphic     ; }
inline int islower(int c)		{ return __ctype_map[__zero_fill(c)] & __lower_case  ; }
inline int isprint(int c)		{ return __ctype_map[__zero_fill(c)] & __printable   ; }
inline int ispunct(int c)		{ return __ctype_map[__zero_fill(c)] & __punctuation ; }
inline int isspace(int c)		{ return __ctype_map[__zero_fill(c)] & __whitespace  ; }
inline int isupper(int c)		{ return __ctype_map[__zero_fill(c)] & __upper_case  ; }
inline int isxdigit(int c)		{ return __ctype_map[__zero_fill(c)] & __hex_digit   ; }
inline int isascii(int c)		{ return (((c) & ~0x7f) == 0); }

/*
 * -1 case added for EOF handling
 */
inline int tolower(int c)	{ return ((c == -1) ? -1:((int)  __lower_map[__zero_fill(c)])); }
inline int toupper(int c)	{ return ((c == -1) ? -1:((int)  __upper_map[__zero_fill(c)])); }
inline int toascii(int c) 	{ return (c & 0x7f); }

#else /* __cplusplus */

#define isalnum(c)	((int) (__ctype_map[__zero_fill(c)] & __alphanumeric))
#define isalpha(c)	((int) (__ctype_map[__zero_fill(c)] & __letter      ))
#define iscntrl(c)	((int) (__ctype_map[__zero_fill(c)] & __control     ))
#define isdigit(c)	((int) (__ctype_map[__zero_fill(c)] & __digit       ))
#define isgraph(c)	((int) (__ctype_map[__zero_fill(c)] & __graphic     ))
#define islower(c)	((int) (__ctype_map[__zero_fill(c)] & __lower_case  ))
#define isprint(c)	((int) (__ctype_map[__zero_fill(c)] & __printable   ))
#define ispunct(c)	((int) (__ctype_map[__zero_fill(c)] & __punctuation ))
#define isspace(c)	((int) (__ctype_map[__zero_fill(c)] & __whitespace  ))
#define isupper(c)	((int) (__ctype_map[__zero_fill(c)] & __upper_case  ))
#define isxdigit(c)	((int) (__ctype_map[__zero_fill(c)] & __hex_digit   ))
#define toascii(c) 	((int) (c & 0x7f))
#define	isascii(c)	(((c) & ~0x7f) == 0)

#endif /* __cplusplus */

#endif  /* __POWERPC__ */

__extern_c_end

#endif /*  _CTYPE_H_ */
