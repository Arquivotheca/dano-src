//========================================================================
//	MRegExpErrors.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#ifndef _MREGEXPERRORS_H
#define _MREGEXPERRORS_H

const char FindErrInternal[] = "Regular expression internal error.";
const char FindErrTooComplicated[] = "Expression too complicated.";
const char FindErrNoMem[] = "Out of memory.";
const char FindErrTooManySubs[] = "Too many sub-expressions.";
const char FindErrUnmatchedParenthesis[] = "Unmatching parenthesis.";
const char FindErrEmptyStarPlus[] = "Cannot use *+.";
const char FindErrCantNest[] = "Cannot nest *?+.";
const char FindErrInvalidRange[] = "Invalid [] range.";
const char FindErrUnmatchedBracket[] = "Unmatched bracket.";
const char FindErrBadRepeater[] = "Cannot use ?+*.";
const char FindErrTrailingBackslash[] = "Pattern ends with an extra \\.";
const char FindErrTooMuchRecursion[] = "We have passed our safe limit on recursion while trying to evaluate an expression.";

#endif
