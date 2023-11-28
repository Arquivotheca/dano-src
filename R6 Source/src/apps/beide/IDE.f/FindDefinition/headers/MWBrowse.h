/*
 *  MWBrowse.h
 *
 *  Copyright © 1993-96 Metrowerks, Inc.  All rights reserved.
 *	
 *	Types and constants needed for
 */

#ifndef _MWBROWSE_H
#define _MWBROWSE_H

#include "MWLangDefs.h"

#pragma options align=mac68k

#ifdef __cplusplus
extern "C" {
#endif


#define BROWSE_HEADER	0xBEABBAEB
#define BROWSE_VERSION	2
#define BROWSE_EARLIEST_COMPATIBLE_VERSION 2

typedef struct BrowseHeader {
	long	browse_header;				// always set to BROWSE_HEADER
	long	browse_version;				// always set to BROWSE_VERSION
	short	browse_language;			// the language of this translation unit, enum ELanguage
	short   uses_name_table;			// TRUE: uses name table from CW object code
	long	earliest_compatible_version;// always set to BROWSE_EARLIEST_COMPATIBLE_VERSION
	long	reserved[15];				// for future extensions
} BrowseHeader;


typedef enum EBrowserItem {
		browseFunction,		// function, procedure, or method
		browseGlobal,		// global variable
		browseClass,		// class, struct, or union
		browseMacro,		// macro
		browseEnum,			// enum, enumerated type member
		browseTypedef,		// user-defined type other than class
		browseConstant,		// constant value
		browseTemplate,		// C++ template
		browsePackage,		// Java package
		browseEnd = 0xFF	// used to denote end-of-list
} EBrowserItem;


// flag constants for functions, member functions, data members
enum
{
	kAbstract	= 1,		// abstract/pure virtual
	kStatic		= 2,		// static member
	kFinal		= 4,		// final Java class, method, or data member
	kMember		= 8,		// item is a class member
	// reserve flags 0x10, 0x20, and 0x40 for other general flags
	
	// flags specific to classes
	kInterface	= 0x80,		// class is Java interface
	kPublic		= 0x100,	// class is public Java class

	// flags specific to functions and member functions
	kInline		= 0x80,		// inline function
	kPascal		= 0x100,	// pascal function
	kAsm		= 0x200, 	// asm function
	kVirtual	= 0x400,	// virtual member function
	kCtor		= 0x800,	// is constructor
	kDtor		= 0x1000,	// is destructor
	kNative		= 0x2000,	// native Java method
	kSynch		= 0x4000,	// synchronized Java method
	
	// flags specific to data members
	kTransient	= 0x80,		// transient Java data member
	kVolatile	= 0x100 	// volatile Java data member
};


typedef enum EAccess
{		// can be used as mask values
	accessNone 		= 0,
	accessPrivate	= 1,
	accessProtected = 2,
	accessPublic 	= 4,
	accessAll		= accessPrivate+accessProtected+accessPublic
} EAccess;

typedef enum EMember
{
	memberFunction,			// member function/method
	memberData,				// data member/field	
	memberEnd = 0xFF		// denotes end-of-list
} EMember;

typedef enum ETemplateType 
{ 	// templates are either class or function templates
	templateClass,
	templateFunction
} ETemplateType;


/********************************************************************************/
/*	Old (pre-CW9) browse data support definitions
/********************************************************************************/

#define BROWSE_OLD_VERSION		0

enum {
	BROWSE_SCOPE_STATIC,			//	local to this file
	BROWSE_SCOPE_EXTERN				//	global to all files
};

enum {
	BROWSE_OBJECT_FUNCTION,			//	function object
	BROWSE_OBJECT_DATA				//	data object
};

enum {								//	browse data types
	BROWSE_END,						//	end of browse data
	BROWSE_OBJECT					//	a function/data definition
};

typedef struct BrowseObjectDef {
	short	type;					//	always BROWSE_OBJECT
	short	size;					//	size of following data
	char	object_type;			//	one of: (BROWSE_OBJECT_FUNCTION,BROWSE_OBJECT_DATA)
	char	object_scope;			//	one of: (BROWSE_SCOPE_STATIC,BROWSE_SCOPE_EXTERN)
	long	source_offset;			//	offset of declartation in source code
//	char	name[...];					followed by padded object name (c-string)
//	char	classname[...];				followed by padded class name string (c-string)
}	BrowseObjectDef;

typedef struct BrowseHeader_Old {
	long	browse_header;			//	always set to BROWSE_HEADER
	long	browse_version;			//	always set to BROWSE_VERSION
	long	reserved[14];			//	for future extensions
}	BrowseHeader_Old;


#ifdef __cplusplus
}
#endif

#pragma options align=reset

#endif	// __MWBROWSE_H__
