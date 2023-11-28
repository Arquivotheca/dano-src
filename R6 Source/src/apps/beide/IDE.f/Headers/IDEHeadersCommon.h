//========================================================================
//	IDEHeadersCommon.h
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#if __POWERPC__
#define MW_FOUR_CHAR_CODE(x) (x)
#elif __INTEL__
#define MW_FOUR_CHAR_CODE(x) (((ulong) ((x) & 0x000000FF)) << 24) \
							| (((ulong) ((x) & 0x0000FF00)) << 8) \
							| (((ulong) ((x) & 0x00FF0000)) >> 8) \
							| (((ulong) ((x) & 0xFF000000)) >> 24)
#endif

// Don't need everything in Be.h
#include <AppKit.h>
#include <InterfaceKit.h>
#include <KernelKit.h>
#include <MediaKit.h>
#include <StorageKit.h>
#include <SupportKit.h>
#include <byteorder.h>

#include <Debug.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <new.h>

// undocumented kernel call
extern "C" int sync(void);

// Project headers that don't change much
#include "IDEConstants.h"
#include "MExceptions.h"
#include "MAlert.h"
#include "MLocker.h"
#include "CString.h"

// not sure where this is supposed to be
#include <alloca.h>

const ulong nil = 0;
const unsigned char EOL_CHAR = '\n';

typedef char char64[64];
typedef char mime_t[B_MIME_TYPE_LENGTH];
typedef char attr_name_t[B_ATTR_NAME_LENGTH];

// For use with bsearch and qsort
typedef  int (*compare_members)(const void *, const void *);

#define LOG(x) do { } while (0)

const long B_FILE_NAME_LENGTH_DR8 = 64;

// mac compatibility stuff
#define Boolean bool
#define pascal
typedef char Str255[256];
typedef char *Ptr;
inline Ptr StripAddress(Ptr x) { return x; }
