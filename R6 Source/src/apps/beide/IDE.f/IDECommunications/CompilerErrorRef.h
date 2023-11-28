
/*
 *  CompilerErrorRef.h - Compiler Error File Reference for Metrowerks CodeWarriorª
 *
 *  Copyright © 1995 metrowerks inc.  All rights reserved.
 *
 *
 *	THEORY OF OPERATION
 *
 *	All "compiler" errors or warnings which occur at some specific location in some
 *	source file are identified by a CompilerErrorRef structure. This structure
 *	provides sufficient information for the development environment to locate
 *	and display the exact error position:
 *
 *		errorfile		FSSpec of file containing error
 *		linenumber		number of line containing error
 *		offset			character offset from start of file to error token
 *		length			number of characters in error token
 *		sync[]			some characters before and after the error token
 *		synclen			number of characters in the sync array
 *		syncoffset		offset to the error token in the sync array
 *		errorlength		length of error token in error message line
 *		erroroffset		offset of error token in error message line
 *		warning			this is a warning (vs. error) message
 *
 *	When an error is reported, the report will include a copy of the source
 *	line containing the error, as well as the error message itself. The
 *	development environment uses this data to display and select the
 *	erroneous character(s).
 *
 */

#ifndef __COMPILERERRORREF_H__
#define __COMPILERERRORREF_H__

#if defined(__POWERPC__)	/* FIXME: Should probably really be __METROWERKS__ or something */
#pragma options align=mac68k
#endif

#ifdef __cplusplus
	extern "C" {
#endif


typedef struct CompilerErrorRef {
	FSSpec			errorfile;		/*	error file reference		*/
	long			linenumber;		/*	error linenumber in file	*/
	long			offset;			/*	offset of error in file		*/
	short			length;			/*	length of error token (in program text)	*/
	char			sync[32];		/*	32 bytes of data to find the error		*/
	short			synclen;		/*	length of sync array	*/
	short			syncoffset;		/*	selection offset in sync array	*/	
	short			errorlength;	/*	length of error token in following errorstring data		*/
	short			erroroffset;	/*	offset of errorposition in following errorstring data	*/
	bool			warning;		/*	TRUE: is warning; FALSE: is error	*/
} CompilerErrorRef, *CompilerErrorRefPtr;


#ifdef __cplusplus
	}
#endif

#if defined(__POWERPC__)	/* FIXME: Should probably really be __METROWERKS__ or something */
#pragma options align=reset
#endif

#endif
