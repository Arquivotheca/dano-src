/* Machine-independant string function optimizations.
   Copyright (C) 1997, 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by John C. Bowman <bowman@math.ualberta.ca>, 1995.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#if __GNUC__ 

#ifndef _STRINGINLINE_H_
#define _STRINGINLINE_H_

# ifndef _STRING_H_
#  error "Never include <stringinline.h> directly! Include <string.h> instead."
# endif /* _STRING_H_ */

#if !defined(__NO_STRING_INLINES) && (defined(__STRICT_ANSI__) || defined(__cplusplus)) && defined(__OPTIMIZE__)

#ifdef __cplusplus
# define __STRING_INLINE inline
#else
# define __STRING_INLINE extern __inline
#endif

/* These are a few types we need for the optimizations if we cannot
   use unaligned memory accesses.  */
# define __STRING2_COPY_TYPE(N) \
  typedef struct { unsigned char __arr[N]; }				      \
    __STRING2_COPY_ARR##N __attribute__ ((packed))
__STRING2_COPY_TYPE (2);
__STRING2_COPY_TYPE (3);
__STRING2_COPY_TYPE (4);
__STRING2_COPY_TYPE (5);
__STRING2_COPY_TYPE (6);
__STRING2_COPY_TYPE (7);
__STRING2_COPY_TYPE (8);

/* Dereferencing a pointer arg to run sizeof on it fails for the
   void pointer case, so we use this instead.  Note that the argument
   must not contain any side effects.  */
#define __string2_1bptr_p(x) (((size_t) ((x) + 1) - (size_t) (x)) == 1)

/* Copy SRC to DEST.  */
# define strcpy(dest, src) \
  (__extension__ (__builtin_constant_p (src)				      \
		  ? (__string2_1bptr_p (src) && strlen (src) + 1 <= 8	      \
		     ? __strcpy_small (dest, src, strlen (src) + 1)	      \
		     : (char *) memcpy (dest, src, strlen (src) + 1))	      \
		  : strcpy (dest, src)))

#  define __strcpy_small(dest, src, srclen) \
  (__extension__ ({ char *__dest = (char *) (dest);			      \
		    switch (srclen)					      \
		      {							      \
		      case 1:						      \
			*__dest = '\0';					      \
			break;						      \
		      case 2:						      \
			*((__STRING2_COPY_ARR2 *) __dest) =		      \
			  ((__STRING2_COPY_ARR2)			      \
			   { { ((__const char *) (src))[0],		      \
			       '\0' } });				      \
			break;						      \
		      case 3:						      \
			*((__STRING2_COPY_ARR3 *) __dest) =		      \
			  ((__STRING2_COPY_ARR3)			      \
			   { { ((__const char *) (src))[0],		      \
			       ((__const char *) (src))[1],		      \
			       '\0' } });				      \
			break;						      \
		      case 4:						      \
			*((__STRING2_COPY_ARR4 *) __dest) =		      \
			  ((__STRING2_COPY_ARR4)			      \
			   { { ((__const char *) (src))[0],		      \
			       ((__const char *) (src))[1],		      \
			       ((__const char *) (src))[2],		      \
			       '\0' } });				      \
			break;						      \
		      case 5:						      \
			*((__STRING2_COPY_ARR5 *) __dest) =		      \
			  ((__STRING2_COPY_ARR5)			      \
			   { { ((__const char *) (src))[0],		      \
			       ((__const char *) (src))[1],		      \
			       ((__const char *) (src))[2],		      \
			       ((__const char *) (src))[3],		      \
			       '\0' } });				      \
			break;						      \
		      case 6:						      \
			*((__STRING2_COPY_ARR6 *) __dest) =		      \
			  ((__STRING2_COPY_ARR6)			      \
			   { { ((__const char *) (src))[0],		      \
			       ((__const char *) (src))[1],		      \
			       ((__const char *) (src))[2],		      \
			       ((__const char *) (src))[3],		      \
			       ((__const char *) (src))[4],		      \
			       '\0' } });				      \
			break;						      \
		      case 7:						      \
			*((__STRING2_COPY_ARR7 *) __dest) =		      \
			  ((__STRING2_COPY_ARR7)			      \
			   { { ((__const char *) (src))[0],		      \
			       ((__const char *) (src))[1],		      \
			       ((__const char *) (src))[2],		      \
			       ((__const char *) (src))[3],		      \
			       ((__const char *) (src))[4],		      \
			       ((__const char *) (src))[5],		      \
			       '\0' } });				      \
			break;						      \
		      case 8:						      \
			*((__STRING2_COPY_ARR8 *) __dest) =		      \
			  ((__STRING2_COPY_ARR8)			      \
			   { { ((__const char *) (src))[0],		      \
			       ((__const char *) (src))[1],		      \
			       ((__const char *) (src))[2],		      \
			       ((__const char *) (src))[3],		      \
			       ((__const char *) (src))[4],		      \
			       ((__const char *) (src))[5],		      \
			       ((__const char *) (src))[6],		      \
			       '\0' } });				      \
			break;						      \
		    }							      \
		  __dest; }))


/* Copy no more than N characters of SRC to DEST.  */
#  define strncpy(dest, src, n) \
  (__extension__ (__builtin_constant_p (src) && __builtin_constant_p (n)      \
		  ? (strlen (src) + 1 >= ((size_t) (n))			      \
		     ? (char *) memcpy (dest, src, n)			      \
		     : strncpy (dest, src, n))				      \
		  : strncpy (dest, src, n)))


/* Append no more than N characters from SRC onto DEST.  */
#  define strncat(dest, src, n) \
  (__extension__ (__builtin_constant_p (src) && __builtin_constant_p (n)      \
		  ? (strlen (src) < ((size_t) (n))			      \
		     ? strcat (dest, src)				      \
		     : strncat (dest, src, n))				      \
		  : strncat (dest, src, n)))


/* Compare characters of S1 and S2.  */
# define strcmp(s1, s2) \
  (__extension__ (__builtin_constant_p (s1) && __builtin_constant_p (s2)      \
		  && (!__string2_1bptr_p (s1) || strlen (s1) >= 4)	      \
		  && (!__string2_1bptr_p (s2) || strlen (s2) >= 4)	      \
		  ? memcmp ((__const char *) (s1), (__const char *) (s2),     \
			    (strlen (s1) < strlen (s2)			      \
			     ? strlen (s1) : strlen (s2)) + 1)		      \
		  : (__builtin_constant_p (s1) && __string2_1bptr_p (s1)      \
		     && strlen (s1) < 4					      \
		     ? (__builtin_constant_p (s2) && __string2_1bptr_p (s2)   \
			? __strcmp_cc (s1, s2, strlen (s1))		      \
			: __strcmp_cg (s1, s2, strlen (s1)))		      \
		     : (__builtin_constant_p (s2) && __string2_1bptr_p (s2)   \
			&& strlen (s2) < 4				      \
			? (__builtin_constant_p (s1) && __string2_1bptr_p (s1)\
			   ? __strcmp_cc (s1, s2, strlen (s2))		      \
			   : __strcmp_gc (s1, s2, strlen (s2)))		      \
			: strcmp (s1, s2)))))

# define __strcmp_cc(s1, s2, l) \
  (__extension__ ({ register int __result =				      \
		      (((__const unsigned char *) (__const char *) (s1))[0]   \
		       - ((__const unsigned char *) (__const char *)(s2))[0]);\
		    if (l > 0 && __result == 0)				      \
		      {							      \
			__result = (((__const unsigned char *)		      \
				     (__const char *) (s1))[1]		      \
				    - ((__const unsigned char *)	      \
				       (__const char *) (s2))[1]);	      \
			if (l > 1 && __result == 0)			      \
			  {						      \
			    __result =					      \
			      (((__const unsigned char *)		      \
				(__const char *) (s1))[2]		      \
			       - ((__const unsigned char *)		      \
				  (__const char *) (s2))[2]);		      \
			    if (l > 2 && __result == 0)			      \
			      __result =				      \
				(((__const unsigned char *)		      \
				  (__const char *) (s1))[3]		      \
				 - ((__const unsigned char *)		      \
				    (__const char *) (s2))[3]);		      \
			  }						      \
		      }							      \
		    __result; }))

# define __strcmp_cg(s1, s2, l1) \
  (__extension__ ({ __const unsigned char *__s2 =			      \
		      (__const unsigned char *) (__const char *) (s2);	      \
		    register int __result =				      \
		      (((__const unsigned char *) (__const char *) (s1))[0]   \
		       - __s2[0]);					      \
		    if (l1 > 0 && __result == 0)			      \
		      {							      \
			__result = (((__const unsigned char *)		      \
				     (__const char *) (s1))[1] - __s2[1]);    \
			if (l1 > 1 && __result == 0)			      \
			  {						      \
			    __result = (((__const unsigned char *)	      \
					 (__const char *) (s1))[2] - __s2[2]);\
			    if (l1 > 2 && __result == 0)		      \
			      __result = (((__const unsigned char *)	      \
					  (__const char *)  (s1))[3]	      \
					  - __s2[3]);			      \
			  }						      \
		      }							      \
		    __result; }))

# define __strcmp_gc(s1, s2, l2) \
  (__extension__ ({ __const unsigned char *__s1 =			      \
		      (__const unsigned char *) (__const char *) (s1);	      \
		    register int __result =				      \
		      __s1[0] - ((__const unsigned char *)		      \
				 (__const char *) (s2))[0];		      \
		    if (l2 > 0 && __result == 0)			      \
		      {							      \
			__result = (__s1[1]				      \
				    - ((__const unsigned char *)	      \
				       (__const char *) (s2))[1]);	      \
			if (l2 > 1 && __result == 0)			      \
			  {						      \
			    __result =					      \
			      (__s1[2] - ((__const unsigned char *)	      \
					  (__const char *) (s2))[2]);	      \
			    if (l2 > 2 && __result == 0)		      \
			      __result =				      \
				(__s1[3]				      \
				 - ((__const unsigned char *)		      \
				    (__const char *) (s2))[3]);		      \
			  }						      \
		      }							      \
		    __result; }))

/* Compare N characters of S1 and S2.  */
# define strncmp(s1, s2, n) \
  (__extension__ (__builtin_constant_p (s1) && strlen (s1) < ((size_t) (n))   \
		  ? strcmp (s1, s2)					      \
		  : (__builtin_constant_p (s2) && strlen (s2) < ((size_t) (n))\
		     ? strcmp (s1, s2)					      \
		     : strncmp (s1, s2, n))))


/* Return the length of the initial segment of S which
   consists entirely of characters not in REJECT.  */
# define strcspn(s, reject) \
  (__extension__ (__builtin_constant_p (reject) && __string2_1bptr_p (reject) \
		  ? (((__const char *) (reject))[0] == '\0'		      \
		     ? strlen (s)					      \
		     : (((__const char *) (reject))[1] == '\0'		      \
			? __strcspn_c1 (s, ((__const char *) (reject))[0])    \
			: (((__const char *) (reject))[2] == '\0'	      \
			   ? __strcspn_c2 (s, ((__const char *) (reject))[0], \
					   ((__const char *) (reject))[1])    \
			   : (((__const char *) (reject))[3] == '\0'	      \
			      ? __strcspn_c3 (s,			      \
					      ((__const char *) (reject))[0], \
					      ((__const char *) (reject))[1], \
					      ((__const char *) (reject))[2]) \
			      : strcspn (s, reject)))))			      \
		  : strcspn (s, reject)))

__STRING_INLINE size_t __strcspn_c1 (__const char *__s, char __reject);
__STRING_INLINE size_t
__strcspn_c1 (__const char *__s, char __reject)
{
  register size_t __result = 0;
  while (__s[__result] != '\0' && __s[__result] != __reject)
    ++__result;
  return __result;
}

__STRING_INLINE size_t __strcspn_c2 (__const char *__s, char __reject1,
				     char __reject2);
__STRING_INLINE size_t
__strcspn_c2 (__const char *__s, char __reject1, char __reject2)
{
  register size_t __result = 0;
  while (__s[__result] != '\0' && __s[__result] != __reject1
	 && __s[__result] != __reject2)
    ++__result;
  return __result;
}

__STRING_INLINE size_t __strcspn_c3 (__const char *__s, char __reject1,
				     char __reject2, char __reject3);
__STRING_INLINE size_t
__strcspn_c3 (__const char *__s, char __reject1, char __reject2,
	      char __reject3)
{
  register size_t __result = 0;
  while (__s[__result] != '\0' && __s[__result] != __reject1
	 && __s[__result] != __reject2 && __s[__result] != __reject3)
    ++__result;
  return __result;
}


/* Return the length of the initial segment of S which
   consists entirely of characters in ACCEPT.  */
# define strspn(s, accept) \
  (__extension__ (__builtin_constant_p (accept) && __string2_1bptr_p (accept) \
		  ? (((__const char *) (accept))[0] == '\0'		      \
		     ? 0						      \
		     : (((__const char *) (accept))[1] == '\0'		      \
			? __strspn_c1 (s, ((__const char *) (accept))[0])     \
			: (((__const char *) (accept))[2] == '\0'	      \
			   ? __strspn_c2 (s, ((__const char *) (accept))[0],  \
					  ((__const char *) (accept))[1])     \
			   : (((__const char *) (accept))[3] == '\0'	      \
			      ? __strspn_c3 (s,				      \
					     ((__const char *) (accept))[0],  \
					     ((__const char *) (accept))[1],  \
					     ((__const char *) (accept))[2])  \
			      : strspn (s, accept)))))			      \
		  : strspn (s, accept)))

__STRING_INLINE size_t __strspn_c1 (__const char *__s, char __accept);
__STRING_INLINE size_t
__strspn_c1 (__const char *__s, char __accept)
{
  register size_t __result = 0;
  /* Please note that __accept never can be '\0'.  */
  while (__s[__result] == __accept)
    ++__result;
  return __result;
}

__STRING_INLINE size_t __strspn_c2 (__const char *__s, char __accept1,
				    char __accept2);
__STRING_INLINE size_t
__strspn_c2 (__const char *__s, char __accept1, char __accept2)
{
  register size_t __result = 0;
  /* Please note that __accept1 and __accept2 never can be '\0'.  */
  while (__s[__result] == __accept1 || __s[__result] == __accept2)
    ++__result;
  return __result;
}

__STRING_INLINE size_t __strspn_c3 (__const char *__s, char __accept1,
				    char __accept2, char __accept3);
__STRING_INLINE size_t
__strspn_c3 (__const char *__s, char __accept1, char __accept2, char __accept3)
{
  register size_t __result = 0;
  /* Please note that __accept1 to __accept3 never can be '\0'.  */
  while (__s[__result] == __accept1 || __s[__result] == __accept2
	 || __s[__result] == __accept3)
    ++__result;
  return __result;
}


/* Find the first occurrence in S of any character in ACCEPT.  */
# define strpbrk(s, accept) \
  (__extension__ (__builtin_constant_p (accept) && __string2_1bptr_p (accept) \
		  ? (((__const char  *) (accept))[0] == '\0'		      \
		     ? NULL						      \
		     : (((__const char *) (accept))[1] == '\0'		      \
			? strchr (s, ((__const char *) (accept))[0])	      \
			: (((__const char *) (accept))[2] == '\0'	      \
			   ? __strpbrk_c2 (s, ((__const char *) (accept))[0], \
					   ((__const char *) (accept))[1])    \
			   : (((__const char *) (accept))[3] == '\0'	      \
			      ? __strpbrk_c3 (s,			      \
					      ((__const char *) (accept))[0], \
					      ((__const char *) (accept))[1], \
					      ((__const char *) (accept))[2]) \
			      : strpbrk (s, accept)))))			      \
		  : strpbrk (s, accept)))

__STRING_INLINE char *__strpbrk_c2 (__const char *__s, char __accept1,
				     char __accept2);
__STRING_INLINE char *
__strpbrk_c2 (__const char *__s, char __accept1, char __accept2)
{
  /* Please note that __accept1 and __accept2 never can be '\0'.  */
  while (*__s != '\0' && *__s != __accept1 && *__s != __accept2)
    ++__s;
  return *__s == '\0' ? NULL : (char *) __s;
}

__STRING_INLINE char *__strpbrk_c3 (__const char *__s, char __accept1,
				     char __accept2, char __accept3);
__STRING_INLINE char *
__strpbrk_c3 (__const char *__s, char __accept1, char __accept2,
	      char __accept3)
{
  /* Please note that __accept1 to __accept3 never can be '\0'.  */
  while (*__s != '\0' && *__s != __accept1 && *__s != __accept2
	 && *__s != __accept3)
    ++__s;
  return *__s == '\0' ? NULL : (char *) __s;
}


/* Find the first occurrence of NEEDLE in HAYSTACK.  */
# define strstr(haystack, needle) \
  (__extension__ (__builtin_constant_p (needle) && __string2_1bptr_p (needle) \
		  ? (((__const char *) (needle))[0] == '\0'		      \
		     ? (char *) (haystack)				      \
		     : (((__const char *) (needle))[1] == '\0'		      \
			? strchr (haystack,				      \
				  ((__const char *) (needle))[0]) 	      \
			: strstr (haystack, needle)))			      \
		  : strstr (haystack, needle)))

# define __strtok_r(s, sep, nextp) \
  (__extension__ (__builtin_constant_p (sep) && __string2_1bptr_p (sep)	      \
		  ? (((__const char *) (sep))[0] != '\0'		      \
		     && ((__const char *) (sep))[1] == '\0'		      \
		     ? __strtok_r_1c (s, ((__const char *) (sep))[0], nextp)  \
		     : __strtok_r (s, sep, nextp))			      \
		  : __strtok_r (s, sep, nextp)))

__STRING_INLINE char *__strtok_r_1c (char *__s, char __sep, char **__nextp);
__STRING_INLINE char *
__strtok_r_1c (char *__s, char __sep, char **__nextp)
{
  char *__result;
  if (__s == NULL)
    __s = *__nextp;
  while (*__s == __sep)
    ++__s;
  if (*__s == '\0')
    __result = NULL;
  else
    {
      __result = __s;
      while (*__s != '\0' && *__s != __sep)
	++__s;
      if (*__s == '\0')
	*__nextp = __s;
      else
	{
	  *__s = '\0';
	  *__nextp = __s + 1;
	}
    }
  return __result;
}

# define strtok_r(s, sep, nextp) __strtok_r ((s), (sep), (nextp))

#undef __STRING_INLINE

#endif /* No string inlines, etc.  */

#endif /* _STRINGINLINE_H_ */

#endif /* __GNUC__ */
