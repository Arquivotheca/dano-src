/*****************************************************************************/
/*
**	X_API.h
**
**	This provides for platform specfic functions that need to be rewitten,
**	but will provide a API that is stable across all platforms
**
**	© Copyright 1995-1997 Headspace, Inc, All Rights Reserved.
**	Written by Steve Hales
**
**	Headspace products contain certain trade secrets and confidential and
**	proprietary information of Headspace.  Use, reproduction, disclosure
**	and distribution by any means are prohibited, except pursuant to
**	a written license from Headspace. Use of copyright notice is
**	precautionary and does not imply publication or disclosure.
**
**	Restricted Rights Legend:
**	Use, duplication, or disclosure by the Government is subject to
**	restrictions as set forth in subparagraph (c)(1)(ii) of The
**	Rights in Technical Data and Computer Software clause in DFARS
**	252.227-7013 or subparagraphs (c)(1) and (2) of the Commercial
**	Computer Software--Restricted Rights at 48 CFR 52.227-19, as
**	applicable.
**
**	Confidential-- Internal use only
**
** Overview
**	Machine dependent code and equates
**
**	History	-
**	9/25/95		Created
**	10/15/95	Added XFiles
**	10/20/95	Added XMemory support
**	2/11/96		Added X_WEBTV system
**				Added XSound Manager stuff
**	2/17/96		Added more platform compile arounds
**	2/21/96		Changed XGetAndDetachResource to return a size
**	3/29/96		Added XPutLong & XPutShort
**	6/28/96		Added BeBox stuff
**	6/30/96		Changed font and re tabbed
**				Added XSetHardwareSampleRate and XGetHardwareSampleRate
**				Moved some types here ABS, DEBUG_STR
**				Added BeBox stuff and we are now dependent upon X_API.h to set
**				the machine type only
**	7/1/96		Added XCreateAccessCache
**				Added caching system to XFILERESOURCE
**	7/2/96		Merged Machine.h into X_API.h
**	7/3/96		Added XStrCmp
**	7/7/96		Added XStrnCmp
**	7/23/96		Added XMicroseconds
**	8/6/96		Added XGetIndexedType & XCountTypes
**	8/19/96		Added XGetCompressionName, XExpandMacADPCMto16BitLinear, 
**				XExpandULawto16BitLinear
**	9/22/96		Added XRandom & XSeedRandom & XRandomRange
**				Changed XExpandMacADPCMto16BitLinear to XExpandMacADPCMtoXBitLinear
**				to support 8 bit expansion
**	10/8/96		Added XStrCpy
**	10/9/96		Removed PASCAL usage
**	10/11/96	Added XConvertNativeFileToXFILENAME
**				Added XFileSetPositionRelative
**				Added XDetermineByteOrder
**				Added XSwapLong & XSwapShort
**	10/15/96	Added XFileOpenResourceFromMemory
**	11/14/96	Added XGetResourceName & XGetNamedResource
**				Added XWaitMicroseocnds
**	12/4/96		Added XStrLen
**	12/15/96	Added X_SOLARIS
**	12/18/96	Added more Solaris stuff
**	12/19/96	Added create flag in XFileOpenForWrite
**	12/19/96	Added Sparc pragmas
**	12/30/96	Changed copyright
**	1/2/97		Moved USE_MOD_API and USE_STREAM_API into this file
**				Added XCtoPstr & XPtoCstr
**	1/12/97		Added XDecryptData and XEncryptData
**				Renamed DecompressPtr to XDecompressPtr
**	1/13/97		Added XDuplicateStr
**	1/20/97		Added XEncryptedStrCpy && XEncryptedStrLen && XLStrCmp
**	1/21/97		Added XLStrnCmp
**	1/24/97		Added LONG_TO_FIXED & FIXED_TO_LONG
**	1/28/97		Added some Navio changes
**				Added XGetIndexedResource
**	1/29/97		Changed XFileOpenResourceFromMemory to support resource duplication
**				Added XCompressPtr
**	1/30/97		Added XDecryptAndDuplicateStr
**	2/2/97		Added XStringToLong
**	2/5/97		Added XFileOpenForReadFromMemory
*/
/*****************************************************************************/

#ifndef __X_API__
#define __X_API__

#include <BeBuild.h>

// some common types

// types for X_PLATFORM
#define X_MACINTOSH			0		// MacOS
#define X_WIN95				1		// Windows 95 OS
#define X_WEBTV				2		// WebTV OS
#define X_BE				3		// BeOS
#define X_MAGIC				4		// MagicCap OS. Note: This probably won't work yet
#define X_SOLARIS			5		// SunOS
#define X_NAVIO				6		// NaviOS
#define X_WIN_HARDWARE		7		// Windows 95 only direct to Sound Blaster hardware

// types for CODE_TYPE
#define INTEL				0
#define MACINTOSH			1
#define BEBOX				2
#define MAGIC				3
#define WEBTV				4
#define SUN					5
#define NAVIO				6

// types for CPU_TYPE
#define k68000				0
#define kRISC				1
#define k80X86				2
#define kSPARC				3

// **********************************
// Make sure you set the X_PLATFORM define correctly. Everything depends upon this 
// flag being setup correctly.
// **********************************
//
//#define X_PLATFORM		X_MACINTOSH
//#define X_PLATFORM		X_WIN95
//#define X_PLATFORM		X_WIN_HARDWARE
//#define X_PLATFORM		X_NAVIO
#define X_PLATFORM		X_BE
//#define X_PLATFORM		X_WEBTV
//#define X_PLATFORM		X_SOLARIS


typedef void *			XPTR;
typedef short			XERR;
typedef unsigned char	XBYTE;
typedef unsigned short	XWORD;
typedef unsigned long	XDWORD;
typedef char			XBOOL;

typedef unsigned char *	G_PTR;
typedef char			BOOL_FLAG;
typedef unsigned char	UBYTE;
typedef signed char		SBYTE;
typedef long			LOOPCOUNT;
#if X_PLATFORM != X_NAVIO
typedef short			INT16;
typedef unsigned short	UINT16;
typedef long			INT32;
typedef unsigned long	UINT32;
#endif
typedef unsigned long	FIXED_VALUE;

#undef LONG_TO_FIXED
#undef FIXED_TO_LONG
#define LONG_TO_FIXED(x)	(FIXED_VALUE)((((long)(x)) * 65536L))
#define FIXED_TO_LONG(x)	(long)(((long)(x)) / 65536L)

#undef ABS
#define ABS(x)					(((x) < 0) ? -(x) : (x))

#ifndef TRUE
	#define TRUE	1
#endif

#ifndef FALSE
	#define FALSE	0
#endif

#ifndef NULL
	#define NULL	0L
#endif

// **********************************
// MacOS via Sound Manager 3.1
#if X_PLATFORM == X_MACINTOSH
	#include <Sound.h>
	#include <Files.h>

	#define X_WORD_ORDER		FALSE
	#define CODE_TYPE			MACINTOSH
	#define FAR
	#define FP_OFF(x)			(x)
	#ifndef THINK_C
		#define CPU_TYPE		kRISC
	#else
		#define CPU_TYPE		k68000
	#endif
	#define INLINE				inline


	#ifndef DEBUG_STR
		#if USE_DEBUG == 0
			#define DEBUG_STR(x)
		#endif
	
		#if USE_DEBUG == 1
			#define DEBUG_STR(x)	DebugStr((void *)(x))
		#endif
	
		#if USE_DEBUG == 2
			extern short int drawDebug;
			void DPrint(short int d, ...);
			#define DEBUG_STR(x)	DPrint(drawDebug, "%p\r", x)
		#endif
	
		#if USE_DEBUG == 3
			#define DEBUG_STR(x)
			extern short int drawDebug;
			void DPrint(short int d, ...);
			#define DEBUG_STR2(x)	DPrint(drawDebug, "%p\r", x)
		#else
			#define DEBUG_STR2(x)
		#endif
	#endif

// API features
	#define USE_MOD_API				TRUE
	#define USE_SUN_AU_API			TRUE
	#define USE_STREAM_API			TRUE
	#define USE_SMALL_MEMORY_REVERB	FALSE
#endif

// **********************************
// NavioOS via direct hardware
#if X_PLATFORM == X_NAVIO
	#ifndef DEBUG_STR
		#define DEBUG_STR(x)
	#endif	
	#define X_WORD_ORDER		TRUE		// do swap words
	#define CODE_TYPE			NAVIO
	#define FAR
	#define FP_OFF(x)			(x)
	#define CPU_TYPE			k80X86
	#define INLINE	
	#include <kernel/cFile.h>

	#define USE_MOD_API				FALSE
	#define USE_STREAM_API			TRUE
	#define USE_SUN_AU_API			TRUE
	#define USE_SMALL_MEMORY_REVERB	TRUE
#endif

// **********************************
// WebTVOS via direct hardware
#if X_PLATFORM == X_WEBTV
	#ifndef DEBUG_STR
		#define DEBUG_STR(x)
	#endif	
	#define X_WORD_ORDER		FALSE		// don't swap words
	#define CODE_TYPE			WEBTV
	#define FAR
	#define FP_OFF(x)			(x)
	#define CPU_TYPE			kRISC
	#define INLINE	

// API features
	#define USE_MOD_API				FALSE
	#define USE_SUN_AU_API			TRUE
	#define USE_STREAM_API			FALSE
	#define USE_SMALL_MEMORY_REVERB	TRUE
#endif

// **********************************
// Windows 95 and NT via DirectSound
#if X_PLATFORM == X_WIN95
	#ifndef WIN32_EXTRA_LEAN
		#define WIN32_EXTRA_LEAN
	#endif
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>

	typedef struct tagWIN95INIT
	{
		int cbSize;
		HWND hwndOwner;
	} WIN95INIT;

	#include <mmsystem.h>
	#include <dsound.h>
	#include <process.h>

	#ifndef DEBUG_STR
		#define DEBUG_STR(x)
	#endif	
	#define X_WORD_ORDER		TRUE		// swap words
	#define CODE_TYPE			INTEL
	#define FP_OFF(x)			(x)
	#define INLINE				_inline
	#define CPU_TYPE			k80X86

// API features
	#define USE_MOD_API				TRUE
	#define USE_SUN_AU_API			TRUE
	#define USE_STREAM_API			TRUE
	#define USE_SMALL_MEMORY_REVERB	FALSE
#endif


// **********************************
// Windows 95 only and direct to Sound Blaster hardware
#if X_PLATFORM == X_WIN_HARDWARE
	#ifndef WIN32_EXTRA_LEAN
		#define WIN32_EXTRA_LEAN
	#endif
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>

	#include "Vxdintf.h"

	#include <mmsystem.h>
	#include <dsound.h>
	#include <process.h>

	#ifndef DEBUG_STR
		#define DEBUG_STR(x)
	#endif	
	#define X_WORD_ORDER		TRUE		// swap words
	#define CODE_TYPE			INTEL
	#define FP_OFF(x)			(x)
	#define INLINE				_inline
	#define CPU_TYPE			k80X86

// API features
	#define USE_MOD_API				TRUE
	#define USE_SUN_AU_API			TRUE
	#define USE_STREAM_API			TRUE
	#define USE_SMALL_MEMORY_REVERB	FALSE
#endif


// **********************************
// BeOS via low level audio stream threads
#if X_PLATFORM == X_BE
	#ifndef DEBUG_STR
		#define DEBUG_STR(x)
	#endif	

	#define CODE_TYPE			BEBOX
	#define FAR
	#define FP_OFF(x)			(x)
	#if __INTEL__
		#define CPU_TYPE		k80X86
		#define X_WORD_ORDER	TRUE
	#elif __POWERPC__
		#define CPU_TYPE		kRISC
		#define X_WORD_ORDER	FALSE
	#else
		#error - Unsupported procecssor type -
	#endif
	#define INLINE

// API features
	#define USE_MOD_API				FALSE
	#define USE_SUN_AU_API			FALSE
	#define USE_STREAM_API			FALSE
	#define USE_SMALL_MEMORY_REVERB	FALSE
#endif

// **********************************
// SunOS via dev/audio thread
#if X_PLATFORM == X_SOLARIS
	#ifndef DEBUG_STR
		#define DEBUG_STR(x)
	#endif	
	#define X_WORD_ORDER		FALSE
	#define CODE_TYPE	       	SUN
	#define FAR
	#define FP_OFF(x)			(x)
	#define CPU_TYPE			kSPARC
	#define INLINE

// API features
	#define USE_MOD_API				FALSE
	#define USE_SUN_AU_API			TRUE
	#define USE_STREAM_API			TRUE
	#define USE_SMALL_MEMORY_REVERB	FALSE
#endif

#ifdef __cplusplus
	extern "C" {
#endif


// Memory Manager
XPTR	XNewPtr(long size);
void	XDisposePtr(XPTR data);
long	XGetPtrSize(XPTR data);
void	XBlockMove(XPTR source, XPTR dest, long size);
XBOOL	XIsVirtualMemoryAvailable(void);
void	XLockMemory(XPTR data, long size);
void	XUnlockMemory(XPTR data, long size);
void	XSetMemory(void *pAdr, long len, char value);
void	XBubbleSortArray(short int *theArray, short int theCount);
void	XSetBit(void *pBitArray, long whichbit);
void	XClearBit(void *pBitArray, long whichbit);
XBOOL	XTestBit(void *pBitArray, long whichbit);

unsigned long XMicroseconds(void);
void XWaitMicroseocnds(unsigned long waitAmount);


// Resource Manager

struct XFILE_CACHED_ITEM
{
	long		resourceType;		// resource type
	long		resourceID;			// resource ID
	long		resourceLength;		// resource ID
	long		fileOffsetName;		// file offset from 0 to resource name
	long		fileOffsetData;		// file offset from 0 to resource data
};
typedef struct XFILE_CACHED_ITEM		XFILE_CACHED_ITEM;

struct XFILERESOURCECACHE
{
	long				totalResources;
	XFILE_CACHED_ITEM	cached[1];
};
typedef struct XFILERESOURCECACHE		XFILERESOURCECACHE;


struct XFILENAME
{
// public platform specific
#if X_PLATFORM == X_MACINTOSH
	short int			fileReference;
	FSSpec				theFile;
#endif
#if (X_PLATFORM == X_WIN95) || (X_PLATFORM == X_WIN_HARDWARE)
	long				fileReference;
	char				theFile[1024];
#endif
#if X_PLATFORM == X_BE
	long				fileReference;
	char				theFile[1024];
#endif
#if X_PLATFORM == X_SOLARIS
	long				fileReference;
	char				theFile[1024];
#endif
#if X_PLATFORM == X_NAVIO
	File				*fileReference;
	char				theFile[1024];
#endif

// private variables. Zero out before calling functions
	long				fileValidID;
	XBOOL				resourceFile;

	XPTR				pResourceData;	// if file is memory based
	long				resMemLength;	// length of memory resource file
	long				resMemOffset;	// current offset of memory resource file
	XBOOL				allowMemCopy;	// if TRUE, when a memory based resource is
										// read, a copy will be created otherwise
										// its just a pointer into the larger memory resource
										// file
	XFILERESOURCECACHE	*pCache;		// if file has been cached this will point to it
};
typedef struct XFILENAME	XFILENAME;
typedef long				XFILE;

#define XFILERESOURCE_ID	'IREZ'
#define XFILECACHE_ID		'CACH'

struct XFILERESOURCEMAP
{
	long		mapID;
	long		version;
	long		totalResources;
};
typedef struct XFILERESOURCEMAP		XFILERESOURCEMAP;

// Resource Entry
//	long	nextentry
//	long	resourceType
//	long	resourceID
//	pascal	string resourceName
//	long	resourceLength
//	data block

// Resource file works as follows:
//	MAP
//	ENTRIES
//
// You can assume that at the end of the file length you can tack new resources. Just update the map


// Open file as a resource file. Pass TRUE to 'readOnly' for read only.
XFILE	XFileOpenResource(XFILENAME *file, XBOOL readOnly);

// Open file as a read only resource file from a memory pointer. Assumes memory block
// is an exact copy of the resource file format. Don't dispose of pResource until you
// have closed the file. If allowCopy is TRUE, then when resources are read new copies
// will be created, otherwise just a pointer into the mapped resource file
XFILE	XFileOpenResourceFromMemory(XPTR pResource, unsigned long resourceLength, XBOOL allowCopy);

// Open file as a read only file from a memory pointer. Don't dispose of pMemoryBlock until you
// have closed the file. 
XFILE XFileOpenForReadFromMemory(XPTR pMemoryBlock, unsigned long memoryBlockSize);

// open file for reading and writing. Direct access.
XFILE	XFileOpenForRead(XFILENAME *file);
XFILE	XFileOpenForWrite(XFILENAME *file, XBOOL create);

// close file. Direct or resource
void	XFileClose(XFILE fileRef);
void	XFileUseThisResourceFile(XFILE fileRef);

void XConvertNativeFileToXFILENAME(void *file, XFILENAME *xfile);

XFILERESOURCECACHE * XCreateAccessCache(XFILE fileRef);

// search through open resource files
void	XGetResourceName(long resourceType, long resourceID, void *cName);
XPTR	XGetNamedResource(long resourceType, void *cName, long *pReturnedResourceSize);
XPTR	XGetAndDetachResource(long resourceType, long resourceID, long *pReturnedResourceSize);
XPTR	XGetIndexedResource(long resourceType, long *pReturnedID, long resourceIndex, 
								void *pName, long *pReturnedResourceSize);

// search through specific resource file
long	XAddFileResource(XFILE fileRef, long resourceType, long resourceID, void *pName, void *pData, long length);
XPTR	XGetFileResource(XFILE fileRef, long resourceType, long resourceID, void *pName, long *pReturnedResourceSize);
XPTR	XGetIndexedFileResource(XFILE fileRef, long resourceType, long *pReturnedID, long resourceIndex, 
									void *pName, long *pReturnedResourceSize);

long	XGetIndexedType(XFILE fileRef, long resourceIndex);
long	XCountTypes(XFILE fileRef);

// File Manager
long	XFileRead(XFILE fileRef, void * buffer, long bufferLength);
long	XFileWrite(XFILE fileRef, void *buffer, long bufferLength);
long	XFileSetPosition(XFILE fileRef, long filePosition);
long	XFileGetPosition(XFILE fileRef);
long	XFileSetPositionRelative(XFILE fileRef, long relativeOffset);
long	XFileGetLength(XFILE fileRef);

// standard strcmp
short int	XStrCmp(const char *s1, const char *s2);
// standard strcmp, but ignore case
short int	XLStrCmp(const char *s1, const char *s2);
short int	XLStrnCmp(const char *s1, const char *s2, long n);
short int	XStrnCmp(const char *s1, const char *s2, long n);
char *		XStrCpy(char *dest, char *src);
short int	XStrLen(char *src);
short int	XMemCmp(const void * src1, const void * src2, long n);
char *		XDuplicateStr(char *src);
// This will convert a string to a base 10 long value
long		XStrnToLong(char *pData, long length);

enum
{
	X_SOURCE_ENCRYPTED = 0,				// source encrypted, destination is not encrypted
	X_SOURCE_DEST_ENCRYPTED				// source and destination encrypted	
};
// standard strcpy, but with crypto controls
char		*XEncryptedStrCpy(char *dest, char *src, short int copy);
short int	XEncryptedStrLen(char *src);
char		*XDecryptAndDuplicateStr(char *src);



// convert a c string to a pascal string
void * XCtoPstr(void *cstr);

// convert a pascal string to a c string
void * XPtoCstr(void *pstr);

XDWORD	XFixedDivide(XDWORD divisor, XDWORD dividend);
XDWORD	XFixedMultiply(XDWORD prodA, XDWORD prodB);

// if TRUE, then motorola; if FALSE then intel
XBOOL				XDetermineByteOrder(void);
unsigned short int 	XGetShort(void *pData);
unsigned long		XGetLong(void *pData);
void				XPutShort(void *pData, unsigned long data);
void				XPutLong(void *pData, unsigned long data);

// These will swap bytes no matter the byte order
unsigned short		XSwapShort(unsigned short value);
unsigned long		XSwapLong(unsigned long value);

// First byte is a compression type.
// Next 3 bytes is uncompressed length.
//
// Type 0 - Delta encoded LZSS
void	* XDecompressSampleFormatPtr(void * pData, long dataSize);
void	LZSSDeltaUncompress(unsigned char * pSource, long size, unsigned char * pDest, long* pSize);

void	LZSSUncompress(unsigned char * pSource, long size, unsigned char * pDest, long* pSize);
// First 4 bytes is uncompressed length, followed by LZSS compressed data
void	* XDecompressPtr(void * pData, long dataSize);

void	LZSSCompress(unsigned char * pSource, long size, unsigned char * pDest, long * pSize);
// Given a block of data and a size, this will compress it and return a new pointer. The
// original pointer is not deallocated, and the new pointer must be deallocated when finished.
// Will return NULL if cannot compress. First 4 bytes is uncompressed length
void	* XCompressPtr(void *pData, long dataSize, long *pNewSize);

void	XPhase8BitWaveform(unsigned char * pByte, long size);

// Sound Support
XBOOL	XIs16BitSupported(void);
XBOOL	XIsStereoSupported(void);

#define X_FULL_VOLUME	256		// full volume (1.0)

short int		XGetHardwareVolume(void);
void			XSetHardwareVolume(short theVolume);
void			XSetHardwareSampleRate(unsigned long sampleRate);
unsigned long	XGetHardwareSampleRate(void);

void		XGetCompressionName(long compressionType, void *cName);

// Mac ADPCM decompression (IMA 4 to 1)
void		XExpandMacADPCMtoXBitLinear(char *pSource, long frames, short int channels, 
											void *pDest, short int bitSize);


// u law decompression
void		XExpandULawto16BitLinear(unsigned char *pSource, short int *pDest, long frames, long channels);

// MACE decompression
void		XExpandMace1to6(void *inBuffer, void *outBuffer, unsigned long cnt, 
					void * inState, void * outState, 
					unsigned long numChannels, unsigned long whichChannel);
void		XExpandMace1to3(void *inBuffer, void *outBuffer, unsigned long cnt, 
					void * inState, void * outState, 
					unsigned long numChannels, unsigned long whichChannel);

// Decrypt a block of data. This should be U.S. munitions safe. ie below 40 bit
void		XDecryptData(void *pData, unsigned long size);
// Encrypt a block of data. This should be U.S. munitions safe. ie below 40 bit
void		XEncryptData(void *pData, unsigned long size);

// Random numbers
short int	XRandom(void);					// return pseudo-random from 0 to 32767
void		XSeedRandom(unsigned long n);	// set pseudo-random generator
short int	XRandomRange(short int max);	// return pseudo-random from 0 to max - 1

#ifdef __cplusplus
	}
#endif

#endif	// __X_API__


