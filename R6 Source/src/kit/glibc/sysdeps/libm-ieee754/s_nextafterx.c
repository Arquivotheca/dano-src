/* s_nextafterx.c
 * Conversion freom s_nextafter.c by Ulrich Drepper, Cygnus Support,
 * drepper@cygnus.com.
 */

/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

#if defined(LIBM_SCCS) && !defined(lint)
static char rcsid[] = "$NetBSD: $";
#endif

/* IEEE functions
 *	nextafterx(x,y)
 *	return the next machine floating-point number of x in the
 *	direction toward y.
 *   Special cases:
 */

#include "math.h"
#include "math_private.h"

#ifdef __STDC__
	double __nextafterx(double x, long double y)
#else
	double __nextafterx(x,y)
	double x;
	long double y;
#endif
{
	int32_t hx,ix,iy;
	u_int32_t lx,hy,ly,esy;

	EXTRACT_WORDS(hx,lx,x);
	GET_LDOUBLE_WORDS(esy,hy,ly,y);
	ix = hx&0x7fffffff;		/* |x| */
	iy = esy&0x7fff;		/* |y| */

	if(((ix>=0x7ff00000)&&((ix-0x7ff00000)|lx)!=0) ||   /* x is nan */
	   ((iy>=0x7fff)&&((hy|ly)|-(hy|ly))!=0))           /* y is nan */
	   return x+y;
	if((long double) x==y) return x;	/* x=y, return x */
	if((ix|lx)==0) {			/* x == 0 */
	    double x2;
	    INSERT_WORDS(x,esy&0x8000?0x80000000:0,1);/* return +-minsub */
	    x2 = x*x;
	    if(x2==x) return x2; else return x;	/* raise underflow flag */
	}
	if(hx>=0) {				/* x > 0 */
	    if (esy>=0x8000||((ix>>20)&0x7ff)>iy
		|| (((ix>>20)&0x7ff)==iy
		    && (((hx<<11)|(lx>>21))>(hy&0x7fffffff)
			|| (((hx<<11)|(lx>>21))==(hy&0x7fffffff)
			    && (lx<<11)>ly)))) {	/* x > y, x -= ulp */
		if(lx==0) hx -= 1;
		lx -= 1;
	    } else {				/* x < y, x += ulp */
		lx += 1;
		if(lx==0) hx += 1;
	    }
	} else {				/* x < 0 */
	    if (esy<0x8000||((ix>>20)&0x7ff)>iy
		|| (((ix>>20)&0x7ff)==iy
		    && (((hx<<11)|(lx>>21))>(hy&0x7fffffff)
			|| (((hx<<11)|(lx>>21))==(hy&0x7fffffff)
			    && (lx<<11)>ly))))	{/* x < y, x -= ulp */
		if(lx==0) hx -= 1;
		lx -= 1;
	    } else {				/* x > y, x += ulp */
		lx += 1;
		if(lx==0) hx += 1;
	    }
	}
	hy = hx&0x7ff00000;
	if(hy>=0x7ff00000) return x+x;	/* overflow  */
	if(hy<0x00100000) {		/* underflow */
	    double x2 = x*x;
	    if(x2!=x) {		/* raise underflow flag */
	        INSERT_WORDS(x2,hx,lx);
		return x2;
	    }
	}
	INSERT_WORDS(x,hx,lx);
	return x;
}
weak_alias (__nextafterx, nextafterx)
#ifdef NO_LONG_DOUBLE
strong_alias (__nextafterx, __nextafterxl)
weak_alias (__nextafterx, nextafterxl)
#endif
