/***************************************************************
  PROPRIETARY NOTICE: The software information contained
  herein is the sole property of Eastman Kodak Company and is
  provided to Eastman Kodak Company users under license for use on
  their designated equipment only.  Reproduction of this matter in
  whole or in part is forbidden without the express written
  consent of Eastman Kodak Company.

  COPYRIGHT (c) 1991-1994 Eastman Kodak Company.
  As an unpublished work pursuant to Title 17 of the United
  States Code.  All rights reserved.
****************************************************************
*/

/*
 * functions to calculate input, output, and grid tables from
 * user defined functions.
 */
#include "fut.h"
#include "fut_util.h"		/* internal interface file */

/* Add a little extra precision to the itable ramp */
#define	PRECISION 8

/*
 * fut_calc_itbl computes the values of an input table from a user
 * defined function.  Ifun must be a pointer to a function accepting a
 * double and returning a double, both in the range (0.0,1.0).  (NULL is
 * a legal value - it returns leaving the table uninitialized)
 * fut_calc_itbl returns 0 (FALSE) if an error occurs (ifun returned
 * value out of range) and  1 (TRUE) otherwise.
 *
 * NOTE: if ifun == fut_iramp, we generate the ramp inline to save time.
 *	 A change to this inline code must be accompanied by a change
 *	 to fut_iramp.
 *
 * Another NOTE:
 *    To avoid referencing off-grid values (possibly resulting in a
 *    memory violation error) we clip the input table entries to the
 *    value ((grid size-1)<<FUT_INP_DECIMAL_PT)-1.  Thus, when
 *    interpolating a value at the very last grid point, we will get:
 *
 *        val = g[n-2] + (1-1/64K)*(g[n-1]-g[n-2])
 *            = g[n-1] + (1/64K) * (g[n-2]-g[n-1])
 *
 *    instead of:
 *
 *        val = g[n-2] + 0 * (g[n]-g[n-1])
 *            = g[n-1] (or error because g[n] is not defined)
 *
 *    Since the grid table entries are 12-bits, this causes an
 *    interpolation * error of at most 1/16 in the * result,
 *    which when rounded, disappears completely.
 */
int
fut_calc_itbl (fut_itbl_ptr_t itbl, double (*ifun) (double))
{
	fut_itbldat_ptr_t	tbl_ptr;
	int				i;
	double			val;
	double			norm;
	fut_itbldat_t	increment, max_val, itbl_val;
	
	if ( ! IS_ITBL(itbl) )
		return (0);

	tbl_ptr = itbl->tbl;

	if ( ifun == 0 ) {
		return (1);

	} else if ( ifun == fut_iramp ) {
		itbl->id = -(itbl->size);

		increment = (fut_itbldat_t) ((itbl->size - 1) << (FUT_INP_DECIMAL_PT + PRECISION));
		max_val = (increment >> PRECISION) - 1;
		increment /= (fut_itbldat_t) (FUT_INPTBL_ENT-1);
		
		itbl_val = 0;
		for ( i=0; i < FUT_INPTBL_ENT; i++ ) {
			*tbl_ptr++ = (itbl_val >> PRECISION);
			itbl_val += increment;
		}

		/* last entry must be clipped (see note above) */
		if (*(tbl_ptr-1) > max_val) {
			*(tbl_ptr-1) = max_val;
		}
	} else {
		itbl->id = fut_unique_id();

		norm = (double) ((itbl->size - 1) << FUT_INP_DECIMAL_PT);
		for ( i=0; i<FUT_INPTBL_ENT; i++ ) {
			val = (*ifun) ((double) i / (double) (FUT_INPTBL_ENT-1));
			if ( val < 0.0 || val > 1.0 ) {
				DIAG("fut_calc_itbl: bad return value %f\n", val);
				return (0);
			}
			val = val * norm + .5;

					/* clip value to 1 less than
						norm ( see not above) */
			if ( val >= norm ) val = norm - 1;

			*tbl_ptr++ = (fut_itbldat_t) val;
		}
	}

	/*
	 * set the very last (generally invisible) input table entry to the
	 * value of the previous one.  This will perform automatic clipping
	 * of input greater than 4080 to the valid gridspace, which is defined
	 * only for input in the range (0,255) or (0<<4,255<<4).
	 */

	*tbl_ptr = *(tbl_ptr-1);

	return (1);

} /* fut_calc_itbl */

/*
 * fut_calc_otbl computes the values of an output table from a user defined
 * function.  Ofun must be a pointer to a function accepting a fut_gtbldat_t
 * in the range (0,FUT_GRD_MAXVAL) and returning a fut_otbldat_t in
 * the same interval (NULL is a legal value - it just returns
 * leaving the table uninitialized).
 * fut_calc_otbl returns 0 (FALSE) if an error occurs (ofun returned
 * value out of range) and 1 (TRUE) otherwise.
 *
 * NOTE: if ofun == fut_oramp, we generate the ramp inline to save time.
 *	 A change to this inline code must be accompanied by a change
 *	 to fut_oramp.
 */
int
fut_calc_otbl (fut_otbl_ptr_t otbl, fut_otbldat_t (*ofun) (fut_gtbldat_t))
{
	fut_otbldat_ptr_t	tbl_ptr;
	fut_gtbldat_t		i;
	fut_otbldat_t		val;

	if ( ! IS_OTBL(otbl) )
		return (0);

	tbl_ptr = otbl->tbl;

	if ( ofun == 0 ) {
		return (1);

	} else if ( ofun == fut_oramp ) {
		otbl->id = -1;

		for ( i=0; i<FUT_OUTTBL_ENT; i++ ) {
			*tbl_ptr++ = i;
		}

	} else {
		otbl->id = fut_unique_id();

		for ( i=0; i<FUT_OUTTBL_ENT; i++ ) {
			val = (*ofun) (i);
			if ( (unsigned) val > FUT_GRD_MAXVAL ) {
				DIAG("fut_calc_otbl: bad return value %d\n", val);
				return (0);
			}
			*tbl_ptr++ = val;
		}
	}

	return (1);

} /* fut_calc_otbl */

/*
 * fut_calc_gtblA computes the values of a grid table from a user
 * defined function.  Gfun must be a pointer to a function accepting
 * doubles in the range (0.0,1.0) and returning a fut_gtbldat_t in the
 * interval (0,FUT_GRD_MAXVAL). (NULL is a legal value - it just returns
 * leaving the table uninitialized).
 * fut_calc_gtblA returns 0 (FALSE) if an error occurs (gfun returned
 * value outof range), 1 (TRUE) otherwise.
 *
 * NOTE: if gfun == fut_gramp, we generate the ramp inline to save time.
 *	 A change to this inline code must be accompanied by a change
 *	 to fut_gramp.
 */

#define GCVARS(x) double norm##x; KpInt32_t num##x, i##x;
#define GCINIT(x) norm##x = 1.0 / (double) ((num##x = n[FUT_##x##CHAN])-1);
#define GCLOOP(x) for ( i##x=0; i##x<num##x; i##x++ ) { \
		    cList[FUT_##x##CHAN] = (double) i##x * norm##x;

int
	fut_calc_gtblA (fut_gtbl_ptr_t gtbl, fut_gtbldat_t (*gfun) (double FAR*))
{
int32	n[FUT_NICHAN];
int32	ndim, i;
fut_gtbldat_ptr_t	grid;
double		cList[FUT_NICHAN];
fut_gtbldat_t	val;
GCVARS(X)
GCVARS(Y)
GCVARS(Z)
GCVARS(T)
GCVARS(U)
GCVARS(V)
GCVARS(W)
GCVARS(S)

#ifndef QUIET
	char	*errmsg = "fut_calc_gtblA: bad return value %d\n";
#endif /* QUIET */

	if ( ! IS_GTBL(gtbl) )
		return (0);

	if ( gfun == FUT_NULL_GFUN ) {
		return (1);
	}

					/* find grid size in each dimension */
	for ( i=0, ndim = 0; i<FUT_NICHAN; i++ ) {
		if ( gtbl->size[i] > 1 ) {
			n[ndim++] = gtbl->size[i];
		}
	}

					/* initialize grid table with gfun() */
	gtbl->id = fut_unique_id();
	grid = gtbl->tbl;

	switch (ndim) {
	    case 0 :	/* function of no variables (constant) */
	    cList[FUT_XCHAN] = 0.0;
		val = (*gfun)(cList);
		if ( (unsigned) val > FUT_GRD_MAXVAL ) {
			DIAG(errmsg, val);
			return (0);
		}
		*grid = val;
		break;
	    case 1:	/* function of one variable (x), (y), (z), or (t) */
		if ( gfun == fut_gramp ) {
			numX = n[FUT_XCHAN];			/* x|y|z|t */
			for ( iX=0; iX<numX; iX++ ) {
			    *grid++ =(fut_gtbldat_t)(((int32)FUT_MAX_PEL12*iX) / (numX -1));
			}

		} else {
			GCINIT(X)

			GCLOOP(X)
				val = (*gfun)(cList);
				if ( (unsigned) val > FUT_GRD_MAXVAL ) {
					DIAG(errmsg, val);
					return (0);
				}
				*grid++ = val;
			}
		}
		break;
	    case 2:	/* function of two variables (x,y), (x,z), etc. */
		GCINIT(X)
		GCINIT(Y)

		GCLOOP(X)
			GCLOOP(Y)
				val = (*gfun)(cList);
				if ( (unsigned) val > FUT_GRD_MAXVAL ) {
					DIAG(errmsg, val);
					return (0);
				}
				*grid++ = val;
		    }
		}
		break;
		
	    case 3:	/* function of three variables (x,y,z), (x,y,t), etc. */
		GCINIT(X)
		GCINIT(Y)
		GCINIT(Z)

		GCLOOP(X)
			GCLOOP(Y)
				GCLOOP(Z)
					val = (*gfun)(cList);
					if ( (unsigned) val > FUT_GRD_MAXVAL ) {
						DIAG(errmsg, val);
						return (0);
					}
					*grid++ = val;
				}
		    }
		}
		break;
		
	    case 4:	/* function of four variables (x,y,z,t), etc. */
		GCINIT(X)
		GCINIT(Y)
		GCINIT(Z)
		GCINIT(T)

		GCLOOP(X)
			GCLOOP(Y)
				GCLOOP(Z)
					GCLOOP(T)
						val = (*gfun)(cList);
						if ( (unsigned) val > FUT_GRD_MAXVAL ) {
							DIAG(errmsg, val);
							return (0);
						}
						*grid++ = val;
				    }
				}
		    }
		}
		break;
		
	    case 5:	/* function of five variables (x,y,z,t,u), etc. */
		GCINIT(X)
		GCINIT(Y)
		GCINIT(Z)
		GCINIT(T)
		GCINIT(U)

		GCLOOP(X)
			GCLOOP(Y)
				GCLOOP(Z)
					GCLOOP(T)
						GCLOOP(U)
							val = (*gfun)(cList);
							if ( (unsigned) val > FUT_GRD_MAXVAL ) {
								DIAG(errmsg, val);
								return (0);
							}
							*grid++ = val;
						}
				    }
				}
		    }
		}
		break;
		
	    case 6:	/* function of six variables (x,y,z,t,u,v), etc. */
		GCINIT(X)
		GCINIT(Y)
		GCINIT(Z)
		GCINIT(T)
		GCINIT(U)
		GCINIT(V)

		GCLOOP(X)
			GCLOOP(Y)
				GCLOOP(Z)
					GCLOOP(T)
						GCLOOP(U)
							GCLOOP(V)
								val = (*gfun)(cList);
								if ( (unsigned) val > FUT_GRD_MAXVAL ) {
									DIAG(errmsg, val);
									return (0);
								}
								*grid++ = val;
							}
						}
				    }
				}
		    }
		}
		break;
		
	    case 7:	/* function of seven variables (x,y,z,t,u,v,w), etc. */
		GCINIT(X)
		GCINIT(Y)
		GCINIT(Z)
		GCINIT(T)
		GCINIT(U)
		GCINIT(V)
		GCINIT(W)

		GCLOOP(X)
			GCLOOP(Y)
				GCLOOP(Z)
					GCLOOP(T)
						GCLOOP(U)
							GCLOOP(V)
								GCLOOP(W)
									val = (*gfun)(cList);
									if ( (unsigned) val > FUT_GRD_MAXVAL ) {
										DIAG(errmsg, val);
										return (0);
									}
									*grid++ = val;
								}
							}
						}
				    }
				}
		    }
		}
		break;
		
	    case 8:	/* function of eight variables (x,y,z,t,u,v,w,s), etc. */
		GCINIT(X)
		GCINIT(Y)
		GCINIT(Z)
		GCINIT(T)
		GCINIT(U)
		GCINIT(V)
		GCINIT(W)
		GCINIT(S)

		GCLOOP(X)
			GCLOOP(Y)
				GCLOOP(Z)
					GCLOOP(T)
						GCLOOP(U)
							GCLOOP(V)
								GCLOOP(W)
									GCLOOP(S)
										val = (*gfun)(cList);
										if ( (unsigned) val > FUT_GRD_MAXVAL ) {
											DIAG(errmsg, val);
											return (0);
										}
										*grid++ = val;
									}
								}
							}
						}
				    }
				}
		    }
		}
		break;
		
	    default:
		return (0);
	} /* switch */

	return (1);

} /* fut_calc_gtblA */

/*
 * ramp functions for initializing and calculating tables.
 *
 * NOTE: these are intercepted and emulated by fut_calc routines
 * so a change here must be accompanied by a change there (above).
 */

double
	fut_iramp	(double x)
{
	return (x);
}

fut_otbldat_t
	fut_oramp	(fut_gtbldat_t x)
{
	return ((fut_otbldat_t) x);
}

fut_gtbldat_t
	fut_gramp	(double FAR* dP)
{
	return ((fut_gtbldat_t) (FUT_MAX_PEL12*dP[0]));
}

#if !defined (NO_FUT_GCONST)
/*
 * fut_set_const and fut_gconst are used together to initialize
 * a 1*1*1 grid.  The macro FUT_GCONST(x) is defined as
 * (fut_set_gconst(x), fut_gconst) so that one may initialize the
 * small grid with:
 *
 *	gtbl = fut_new_gtblA (0,FUT_GCONST(value));
 */
static fut_gtbldat_t	gconst;

void
fut_set_gconst (fut_gtbldat_t x)
{
	gconst = x;
}

fut_gtbldat_t
	fut_gconst	(double FAR* dP)
{
	if (dP) {}	/* avoid compiler warnings */
	return (gconst);
}
#else
/*
 * Use this one with the color processor 
 * and get rid of the global
 * 
 *	PS: these functions should never be call from KCMS
 */

void
fut_set_gconst (fut_gtbldat_t x)
{
 fut_gtbldat_t	gconst;
 
 gconst = x;
 if (gconst) {}
 
}

fut_gtbldat_t
	fut_gconst (double FAR* dP)
{
	if (dP) {}	/* avoid compiler warnings */
	return (0);
}
#endif

#ifdef NOTYET
/*
 * fut_calc calculates the grid tables of a fut given a user defined
 * function which accepts real input (0.0,255.0) and returns real output
 * (0.0,255.0).  The input and output tables are taken in to account when
 * computing the grid values.
 */
int
fut_calc (fut_t	* fut, fut_t * fut_fun)
{
}
#endif /* NOTYET */

