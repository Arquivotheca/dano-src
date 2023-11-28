/* Copyright (C) 1996, 1997 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

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

#define	__NO_WCTYPE
#include <wctype.h>

/* Provide real-function versions of all the wctype macros.  */

#define	func(name, type) \
  int name (wint_t wc, __locale_t locale)				      \
  { return __iswctype_l (wc, type, locale); }

func (__iswalnum_l, _ISwalnum)
func (__iswalpha_l, _ISwalpha)
func (__iswcntrl_l, _ISwcntrl)
func (__iswdigit_l, _ISwdigit)
func (__iswlower_l, _ISwlower)
func (__iswgraph_l, _ISwgraph)
func (__iswprint_l, _ISwprint)
func (__iswpunct_l, _ISwpunct)
func (__iswspace_l, _ISwspace)
func (__iswupper_l, _ISwupper)
func (__iswxdigit_l, _ISwxdigit)

wint_t
(__towlower_l) (wint_t wc, __locale_t locale)
{
  return __towctrans_l (wc, locale->__ctype_tolower, locale);
}

wint_t
(__towupper_l) (wint_t wc, __locale_t locale)
{
  return __towctrans_l (wc, locale->__ctype_toupper, locale);
}
