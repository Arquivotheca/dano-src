/***************************************************************
  PROPRIETARY NOTICE: The software information contained
  herein is the sole property of Eastman Kodak Company and is
  provided to Eastman Kodak Company users under license for use on
  their designated equipment only.  Reproduction of this matter in
  whole or in part is forbidden without the express written
  consent of Eastman Kodak Company.

  COPYRIGHT (c) 1991-1997 Eastman Kodak Company.
  As an unpublished work pursuant to Title 17 of the United
  States Code.  All rights reserved.
****************************************************************
*/
/*
 * fut_iomask is a function which quickly determines iomask
 * from a string.  The syntax is as follows:
 *	iomask :=
 *		<out_list> ( <in_list> ) <interp_order>
 *
 *	out_list :=		- function outputs
 *		<> |
 *		<channel_spec> |
 *		<out_list> <channel_spec>
 *
 *	in_list :=		- function inputs
 *		<> |
 *		<channel_spec> |
 *		<in_list> <channel_spec> |
 *		<in_list> , <channel_spec> |
 *		[ <in_list> ]	- optional pass thru to output
 *
 *	interp_order :=		- interpolation type
 *		<> |		- default
 *		1  |		- (tri-/bi-)linear (2 point)
 *		2		- (tri-/bi-)cubic (4 point)
 *
 *	channel_spec :=		- defined by a table
 *		x | y | z | t |		- generic color space
 *		u | v | L | m |		- uvL* color space
 *		C | M | Y | K |		- ink space
 *		R | G | B | ...		- RGB space
 *		0 | 1 | 2 | 3 |		- numeric labels
 *
 * Examples: "CMYK(uvl)2"  "uvl([uv]L)"
 */

#include "fut.h"
#include "fut_util.h"		/* internal interface file */

int32
	fut_iomask (char *mask)
{
	int32	iomask = 0;

state1:
	switch (*mask++) {			/* collecting output channels */
	case 'x':
	case 'u':
	case 'C':
	case 'R':
	case '0':
		iomask |= FUT_OUT(FUT_X);
	case ' ':
		goto state1;
	case 'y':
	case 'v':
	case 'M':
	case 'G':
	case '1':
		iomask |= FUT_OUT(FUT_Y);
		goto state1;
	case 'z':
	case 'L':
	case 'Y':
	case 'B':
	case '2':
		iomask |= FUT_OUT(FUT_Z);
		goto state1;
	case 't':
	case 'm':
	case 'K':
	case '3':
		iomask |= FUT_OUT(FUT_T);
		goto state1;
	case 'U':
	case '4':
		iomask |= FUT_OUT(FUT_U);
		goto state1;
	case 'V':
	case '5':
		iomask |= FUT_OUT(FUT_V);
		goto state1;
	case 'W':
	case '6':
		iomask |= FUT_OUT(FUT_W);
		goto state1;
	case 'S':
	case '7':
		iomask |= FUT_OUT(FUT_S);
		goto state1;

	case '(':
		goto state2;

	default:
		return (0);
	}

state2:
	switch (*mask++) {			/* collecting input channels */
	case 'x':
	case 'u':
	case 'C':
	case 'R':
	case '0':
		iomask |= FUT_IN(FUT_X);
	case ' ':
	case ',':
		goto state2;
	case 'y':
	case 'v':
	case 'M':
	case 'G':
	case '1':
		iomask |= FUT_IN(FUT_Y);
		goto state2;
	case 'z':
	case 'L':
	case 'Y':
	case 'B':
	case '2':
		iomask |= FUT_IN(FUT_Z);
		goto state2;
	case 't':
	case 'm':
	case 'K':
	case '3':
		iomask |= FUT_IN(FUT_T);
		goto state2;
	case 'U':
	case '4':
		iomask |= FUT_IN(FUT_U);
		goto state2;
	case 'V':
	case '5':
		iomask |= FUT_IN(FUT_V);
		goto state2;
	case 'W':
	case '6':
		iomask |= FUT_IN(FUT_W);
		goto state2;
	case 'S':
	case '7':
		iomask |= FUT_IN(FUT_S);
		goto state2;

	case ')':
		goto state5;

	case '[':
		goto state3;
		
	default:
		return (0);
	}

state3:
	switch (*mask++) {			/* collecting pass through channels */
	case 'x':
	case 'u':
	case 'C':
	case 'R':
	case '0':
		iomask |= FUT_PASS(FUT_X);
	case ' ':
	case ',':
		goto state3;
	case 'y':
	case 'v':
	case 'M':
	case 'G':
	case '1':
		iomask |= FUT_PASS(FUT_Y);
		goto state3;
	case 'z':
	case 'L':
	case 'Y':
	case 'B':
	case '2':
		iomask |= FUT_PASS(FUT_Z);
		goto state3;
	case 't':
	case 'm':
	case 'K':
	case '3':
		iomask |= FUT_PASS(FUT_T);
		goto state3;
	case 'U':
	case '4':
		iomask |= FUT_PASS(FUT_U);
		goto state3;
	case 'V':
	case '5':
		iomask |= FUT_PASS(FUT_V);
		goto state3;
	case 'W':
	case '6':
		iomask |= FUT_PASS(FUT_W);
		goto state3;
	case 'S':
	case '7':
		iomask |= FUT_PASS(FUT_S);
		goto state3;

	case ']':
		goto state2;
		
	default:
		return (0);
	}

state5:
	switch (*mask++) {			/* collecting interpolation order */
	case ' ':
		goto state5;

	case '0':
		iomask |= FUT_ORDER(FUT_NEAREST);
		goto state6;
	case '1':
		iomask |= FUT_ORDER(FUT_LINEAR);
		goto state6;
	case '2':
		iomask |= FUT_ORDER(FUT_CUBIC);
		goto state6;
	case '3':
		iomask |= FUT_ORDER(FUT_BSPLINE);

	default:
		goto state6;
	}

state6:
	return (iomask);				/* finished */
}

/*
 * fut_iomask_to_int() and fut_int_to_iomask() convert between fut_iomask_t
 * int32 iomask data types.  The two should be equivalent except that the
 * fut_iomask_t is technically a structure and can not be assigned directly
 * to or from an int.
 *
 * The fut_iomask_t data type is used primarily internally by the library
 * and conversion is rarely necessary.
 */
union fut_iomask_u {	/* union for quick conversion */
	fut_iomask_t	structure;
	int32		integer;
};

int32
fut_iomask_to_int (fut_iomask_t iomask)
{
	union fut_iomask_u	temp;

	temp.structure = iomask;

	return (temp.integer);
}

fut_iomask_t
fut_int_to_iomask (int32 iomask)
{
	union fut_iomask_u	temp;

	temp.integer = iomask;

	return (temp.structure);
}

