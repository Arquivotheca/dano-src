/*************************************************************
 * File: baseconfig.h
 *
 * This file loads the baseline definitions which are used
 * to build etcsw stuff.  It is normally included via datatype.h,
 * but it should be kept clean so that things which don't include
 * datatype (because of conflicts with types defined there)
 * can still use this file.
 */

#ifdef CONFIG
#include CONFIG
#endif

