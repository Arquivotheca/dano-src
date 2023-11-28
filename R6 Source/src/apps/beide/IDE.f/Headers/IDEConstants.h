//========================================================================
//	IDEConstants.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
// BDS
// Project-wide constants

#ifndef _IDECONSTANTS_H
#define _IDECONSTANTS_H

#ifndef _SUPPORT_DEFS_H
#include <SupportDefs.h>
#endif
#ifndef _STORAGE_DEFS_H
#include <StorageDefs.h>
#endif
#ifndef _GRAPHICS_DEFS_H
#include <GraphicsDefs.h>
#endif

// File types and creators
const ulong kIDECreator			= 'MIDE';
const ulong kProjectType		= 'MMPr';
const ulong kTextType			= 'TEXT';
const ulong kMWSourceType		= 'TEXT';
const ulong kSharedLibType		= 'shlb';
const ulong kCWLibType			= 'MPLF';
const ulong kXCOFFType			= 'XCOF';
const ulong kPreCompiledType	= 'MMCH';
const ulong kAppType			= 'BAPP';
const ulong kNULLType			= 0;				// The default file type on Be
const ulong kUnknownType		= 0xFFFFFFFF;		// if we don't recognize the type
const ulong kDebuggerCreator	= 'MBDB';


typedef signed long CommandT;
typedef signed long MessageT;

enum {
	kOK			= 1,
	kSave 		= kOK,
	kCancel,
	kDontSave
};

const long PCHMagicWord = 0xbeefface;


// Window constants
const float kBorderWidth = 5.0;

const long kPathSize = 256;
typedef  char FileNameT[B_FILE_NAME_LENGTH];
typedef  char PathNameT[kPathSize];

const unsigned char MAC_RETURN = 0x0D;
const unsigned char NEWLINE = 0x0A;
const unsigned char SPACE = ' ';
const unsigned char TAB = '\t';

// How many concurrent compiles can we do?
const long kMinConcurrentCompiles = 1;
const long kMaxConcurrentCompiles = 4;

// An enum for seeking in a data BFile
enum PositionModeT {
	fsFromBeginning,
	fsFromMark,
	fsFromEnd
};

const bigtime_t kSlowPulseRate = 0;				// Off
const bigtime_t kNormalPulseRate = 500000;		// half second

// If we are using Metrowerks, allow the STL to provide
// min/max.  If not, use our own versions.
#ifdef __MWERKS__
#include <algobase.h>
#else

template <class T, class U>
inline T min(T a, U b)
{
	return a < b ? a : b;
}
template <class T, class U>
inline T max(T a, U b)
{
    return  a < b ? b : a;
}

#endif

// constants & typedefs from IDEHeadersCommon.h

// the following causes problems for gcc compiler
// ...it complains about type matching with the ulong
// I will replace nil with NULL. (John Dance)
// const ulong nil = 0;
#define nil NULL
const unsigned char EOL_CHAR = '\n';

typedef char char64[64];
typedef char mime_t[B_MIME_TYPE_LENGTH];
typedef char attr_name_t[B_ATTR_NAME_LENGTH];

#define LOG(x) do { } while (0)

const long B_FILE_NAME_LENGTH_DR8 = 64;

#if __POWERPC__
#define MW_FOUR_CHAR_CODE(x) (x)
#elif __INTEL__
#define MW_FOUR_CHAR_CODE(x) (((ulong) ((x) & 0x000000FF)) << 24) \
							| (((ulong) ((x) & 0x0000FF00)) << 8) \
							| (((ulong) ((x) & 0x00FF0000)) >> 8) \
							| (((ulong) ((x) & 0xFF000000)) >> 24)
#endif

// For use with bsearch and qsort
typedef  int (*compare_members)(const void *, const void *);

// mac compatibility stuff
#define pascal
typedef char Str255[256];
typedef char *Ptr;
inline Ptr StripAddress(Ptr x) { return x; }

#endif

