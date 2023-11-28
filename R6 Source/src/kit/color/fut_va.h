#ifndef FUT_VA_H
#define FUT_VA_H

 /* @(#)fut_va.h	1.1   09/09/94  */

#if defined(SABR) || defined(KPWIN) || defined(KPWATCOM) || defined(KPMAC) || defined (KPTHINK) || defined (__STDC__)
#define GOT_STD_VARARGS 1
#endif

/*
 * KCMS_VAR_xxx are macros which are used by those library functions which allow
 * an array to be specified in place of a variable arglist.  This is indicated
 * by setting the use_array parameter to true in KCMS_VA_ARRAY.  Sometimes it is necessary
 * (or at least damn convenient) to use an array because the number of args is
 * not known at compile time.  This feature has been changed from the fut_va stuff since
 * a generic library does not need the iomask parameter.  The fut library can either
 * be changed to have additional paramters to all the fucntions that want it (yeah, right)
 * or more likely the fut_va stuff can be implemented by calling the KCMS stuff.
 *
 * These macros are also used to hide the differences between
 * ANSI and Unix versions of C.
 *
 * Two pointers are maintained: `list' and `array'.  `list' always points
 * into the stack frame (or however varargs does it) while `array' is
 * either NULL or points into an array of arguments passed in as one of
 * the args.  If `array' is set to NULL, then subsequent args will be
 * taken from `list' using va_arg(), otherwise they will be taken from
 * `array' which is then incremented to point to the next arg.
 *
 * KCMS_VA_START is used in place of va_start.  It initially sets array to NULL
 *		so that unless KCMS_VA_ARRAY is called, KCMS_VA_ARG will always
 *		take arguments from the arglist.
 *
 * KCMS_VA_LIST	sets array to NULL so that subsequent calls to KCMS_VA_ARG will
 *		always take values from the arg list.
 *
 * KCMS_VA_ARRAY checks the use_array parameter and if true, it unpacks
 *		the next arg from the arg list into the array.  Subsequent calls
 *		to KCMS_VA_ARG will get args from the array instead of the list.
 *		If KCMS_VARRGS is not set, then array is set to NULL and subsequent
 *		calls to KCMS_VA_ARG get values from the arg list.
 *
 * KCMS_VA_ARG	is used in place of va_arg and returns either the next arg
 *		in the variable arg list or the next item in the array,
 *		depending on whether array is NULL or not.
 *
 * KCMS_VARIABLE_ARGS is used to help handle differences between the ANSI C
 *		and Unix C versions of the variable argument calling.  It is
 *		used by putting it into the argument list where either ...(ANSI C)
 *		or va_alist(Unix C) is used.
 *
 * KCMS_VA_DCL	is used to help handle differences between the ANSI C
 *		and Unix C versions of the variable argument calling.  It is
 *		used as part of the declaration of a function with a variable
 *		number of arguments in the place where va_dcl is normally
 *		used in the Unix C version of variable argument passing.
 *
 * KCMS_VA_ARG_PTR	data type for pointer to variable arguments.
 *
 * KCMS_VA_END	is used in place of va_end.
 *
 * Note that the primary advantage to using these macros is to allow the loops
 * which unpack the args to be ignorant of which calling convention is being
 * used.  We thus avoid having to code two, almost identical, loops.
 */

/*
 *   Now include the file for the variable
 *   number of argument stuff.  It is either stdarg.h or
 *   varargs.h
 */
#ifdef GOT_STD_VARARGS  /* for standard C (ANSI C) */
#include <stdarg.h>
#else
#ifndef SABR
#include <varargs.h>
#endif  /* GOT_STD_VARARGS */
#endif

 


/*
  * Make up special variable argument list macros for when creating a DLL.  This is needed to get
  * far pointers into the pointers for the argument lists.  DLLs are defined when the symbol
  * _WINDLL is defined. This is not necessary if using a 32-bit compiler(i.e., Watcom)
  */
#if defined(_WINDLL) && !defined(KPWATCOM)
typedef char _far *gen_va_list;

#define gen_va_start(ap,v)      (ap = (gen_va_list)(&v) + sizeof(v))
#define gen_va_arg(ap,t)        (((t _far*)(ap += sizeof(t)))[-1])
#define gen_va_end(ap)          (ap=NULL)

#else				/* defined(_WINDLL) && !defined(KPWATCOM) */

 /*
  * Define generic variable argument macros if not defined already(e.g., for Windows DLLs).  If not
  * already defined, just default to the standard ANSI stuff.
  */

#if  defined(GOT_STD_VARARGS) || defined(SABR)
typedef va_list gen_va_list;

#define gen_va_start(ap,v)      va_start(ap,v)
#define gen_va_arg(ap,t)        va_arg(ap,t)
#define gen_va_end(ap)          va_end(ap)
#endif				/* GOT_STD_VARARGS */
#endif				/* !defined(_WINDLL) && !defined(KPWATCOM) */



 /*
  * Define the variable argument list pointer data type.  This needs to be different for MS Windows
  * DLLs and so two definitions are required.  
  */
#ifdef GOT_STD_VARARGS		       /* for standard C (ANSI C) */
typedef gen_va_list KCMS_VA_ARG_PTR;

#else
typedef va_list KCMS_VA_ARG_PTR;

#endif




 /*
  * for standard C (ANSI C)
  */
#ifdef GOT_STD_VARARGS

#define KCMS_VA_START(list,array,first)	{gen_va_start (list,first); array = NULL;}
#define KCMS_VA_DCL
#define KCMS_VARIABLE_ARGS ...
#define KCMS_VA_ARG(list,array,mode) ((array == NULL) ?		\
				gen_va_arg (list, mode) :		\
				((mode FLAT_FAR*)(array += sizeof(mode)))[-1])
#define KCMS_VA_ARRAY(list,array,use_array) array = (use_array) ? \
						gen_va_arg(list, char_p) : NULL
#define KCMS_VA_END(list,array)		gen_va_end (list)

 /*
  * I think this is buggy since there is no KCMS_VA_LIST macro up here, nor was there one in the fut
  * library.  fut only used it in new.c I guess this will break something sooner or later... -pZ
  */

#else

 /*
  * for Unix System V style C
  */

#define KCMS_VA_START(list,array,first) {va_start (list); array = NULL;}
#define KCMS_VA_DCL	va_dcl
#define KCMS_VARIABLE_ARGS va_alist
#define KCMS_VA_ARG(list,array,mode) ((array == NULL) ?		\
				va_arg (list, mode) :		\
				((mode *)(array += sizeof(mode)))[-1])
#define KCMS_VA_ARRAY(list,array,use_array) array = (use_array) ? \
						va_arg(list, char_p) : NULL
#define KCMS_VA_END(list,array)	va_end (list)
#endif				/* GOT_STD_VARARGS */

#define KCMS_VA_LIST(list,array)		array = NULL



#endif				/* FUT_VA_H */

