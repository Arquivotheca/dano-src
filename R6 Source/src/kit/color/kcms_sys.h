#define	KPBE

/*

   File:       kcms_sys.h	@(#)kcms_sys.h	1.73	8/4/97	

   Contains:
           This file contains utility definitions and function prototypes
           needed everywhere in the KCMS software.

   Written by: Drivin' Team

   Copyright:  (c) 1991-1996 by Eastman Kodak Company, all rights reserved.

   Change History (most recent first):

	Macintosh
		<12>	 5/18/94	nsb		Added thread storage mechanisms
	  	<11>	 2/24/94	sek		Added OSI library stuff
		<10>	11/04/91	rlc		Added io_file header stuff
        <9>      9/24/91    gbp     get :KCMS_sys:os.h from local when not ThinkC
        <8>      9/23/91    lcc     Integrate mods for port to think c
        <7>      9/13/91    pgt     Merge KCMS API version of memory.c and this version.
        <6>      8/28/91    pgt     Define bool type here.
        <5>      8/28/91    pgt     Change creator to MPS
        <4>       8/8/91    gbp     remove False/True definitions
        <3>      6/26/91    gbp     add allocate buffer and lock function, etc.
        <2>      6/26/91    gbp     os.h is not local
        <1>      6/24/91    gbp     first checked in

	Windows History
		$Header:   O:/pc_src/lib/kcms_sys/kcms_sys.h_v   1.13   28 Feb 1992 08:35:32   lsh  $

	Sun History
	<2>	12/03/93	rfp	put ARGS in other prototypes
	<1>	9/21/93		rfp	put ARGS in reallocBufferPtr prototype

   To Do:
*/

#ifndef KCMS_SYS_H
#define KCMS_SYS_H

#include "kcmsos.h"

/* The following pragma statement disables the MSVC 4.1 compiler warning:
 * "keyword is reserved for future use"
 */
#if defined (KPMSMAC) || defined (KPWIN32)
#pragma warning( disable : 4237 )
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* version number */
/*    Only define this sucker if we
 *    are on a Sun. It takes up too
 *    much space on anything else!
 */
#if defined(KP_USE_VERSIONSTRING)
static char libkcms_sysVer[] = "libkcms_sysV3.24";
#endif


/*
 * type definitions
 */
#ifndef BOOL_DEFINED
#if defined (KP_USE_KBOOL)
/* This definition is provided for use with XWindows.	*/
/* XWindows defines True and False which conflicts with	*/
/* the standard bool enum 				*/
	typedef enum { KFalse=0, KTrue=1 }    bool;  /* general boolean type */
#else
	typedef enum { False=0, True=1 }    bool;  /* general boolean type */
#endif

#define BOOL_DEFINED
#endif

typedef KpUInt32_t KpCrc32_t;

/* I/O return values */
#define KCMS_IO_ERROR		0
#define KCMS_IO_SUCCESS		1
#define KCMS_IO_ERR_SHARE	2

/* error return values for non-I/O functions */
#define KCMS_SUCCESS	0
#define KCMS_FAIL		1

#define KCMS_COMP_OPEN			10
#define KCMS_COMP_CLOSE			11
#define KCMS_BAD_STR			12
#define KCMS_MORE_DATA			13
#define KCMS_ACCESS_VIOLATION	14
#define KCMS_ENTRY_NOT_FOUND	15

/* Platform independent newline */
#define KP_NEWLINE 0xA

/**********************************************/
/* Prototypes for memory allocation functions */
/**********************************************/

/*------------------------------*/
/* application private versions */
/*------------------------------*/

KpHandle_t FAR allocBufferHandle (
			KpInt32_t	Size);

KpHandle_t FAR allocLargeBufferHandle (
			KpInt32_t	Size);

KpGenericPtr_t FAR allocBufferPtr (
			KpInt32_t	Size);

KpGenericPtr_t FAR reallocBufferPtr (
			KpGenericPtr_t	OldPtr,
			KpInt32_t		Size);

void FAR freeBuffer (
			KpHandle_t		Handle);

void FAR freeBufferPtr (
			KpGenericPtr_t	Ptr);

KpInt32_t FAR getBufferSize (
			KpHandle_t		Handle);

KpHandle_t FAR getHandleFromPtr (
			KpGenericPtr_t	Ptr);

KpInt32_t FAR getPtrSize (
			KpGenericPtr_t	Ptr);

KpGenericPtr_t FAR lockBuffer (
			KpHandle_t		Handle);

bool FAR unlockBuffer (
			KpHandle_t		Handle);

KpHandle_t FAR unlockBufferPtr (
			KpGenericPtr_t	Ptr);

/*-----------------------------*/
/* application shared versions */
/*-----------------------------*/

KpHandle_t FAR allocSysBufferHandle (
			KpInt32_t		Size);

KpHandle_t FAR allocSysLargeBufferHandle (
			KpInt32_t		Size);

KpGenericPtr_t FAR allocSysBufferPtr (
			KpInt32_t		Size);

void FAR freeSysBuffer (
			KpHandle_t		Handle);

void FAR freeSysBufferPtr (
			KpGenericPtr_t	Ptr);

KpInt32_t FAR getSysBufferSize (
			KpHandle_t		Handle);

KpHandle_t FAR getSysHandleFromPtr (
			KpGenericPtr_t	Ptr);

KpInt32_t FAR getSysPtrSize (
			KpGenericPtr_t	Ptr);

KpGenericPtr_t FAR lockSysBuffer (
			KpHandle_t		Handle);

bool FAR unlockSysBuffer (
			KpHandle_t		Handle);

KpHandle_t FAR unlockSysBufferPtr (
			KpGenericPtr_t	Ptr);

/****************************/
/* Timers and Delay support */
/****************************/
void FAR KpSleep (
			KpUInt32_t	MilliSeconds,
			bool		IntentIsDelayOnly);

/****************************/
/* Critical Section support */
/****************************/
void FAR KpInitializeCriticalSection (
			KpCriticalFlag_t FAR *CriticalFlag);

KpInt32_t FAR KpEnterCriticalSection (
			KpCriticalFlag_t FAR *CriticalFlag);

void FAR KpLeaveCriticalSection (
			KpCriticalFlag_t FAR *CriticalFlag);

void FAR KpDeleteCriticalSection (
			KpCriticalFlag_t FAR *CriticalFlag);

/*****************************************/
/* Read Modify Write function prototypes */
/*****************************************/
KpInt32_t KpInterlockedExchange (KpInt32_t FAR *address, KpInt32_t value);

/********************************************************/
/* Semaphore Prototypes, type definitions and constants */
/********************************************************/

#define KP_SEM_NO_WAIT		0
#define KP_SEM_INFINITE		0xFFFFFFFF

#define KP_SEM_DEF_INCR		1		/*	The default value to increment
										a semaphore by when releasing
										the semaphore					*/

#if defined (KPSOLARIS)
#define KCMS_SYS_SEM_SET	"Kp_kcms_sys.sem"
#define KCMS_SYS_NUM_SEMS	1
#define KCMS_SYS_SEMAPHORE	1
#endif

struct KpSemSet_tag { char dontuse; };
typedef struct KpSemSet_tag FAR * KpSemSet_t; 

typedef struct {
	KpInt32_t	SemaphoreNum;
	KpUInt32_t	InitialValue;
} KpSemInit_t;



KpSemSet_t FAR KpSemSetInit (
			char		FAR *Name,
			KpUInt32_t	NumSems,
			KpSemInit_t	FAR *SemInitArray);

KpUInt32_t	FAR KpSemSetFree (
			KpSemSet_t	FAR *SemSet);

KpUInt32_t	FAR KpSemSetDestroy (
			char FAR *name);

KpUInt32_t	FAR KpSemaphoreGet (
			KpSemSet_t	SemSet,
			KpUInt32_t	NumEntries,
			KpUInt32_t	FAR *SemList,
			KpUInt32_t	TimeOut);

KpUInt32_t	FAR KpSemaphoreRelease (
			KpSemSet_t	SemSet,
			KpUInt32_t	NumEntries,
			KpUInt32_t	FAR *SemList,
			KpUInt32_t	Increment);

#if defined (KPSOLARIS)
KpUInt32_t	getKeyFromName (char *name, key_t *key);
KpSemSet_t	acquireKcmsSysLock (void);
KpUInt32_t	releaseKcmsSysLock (KpSemSet_t *SemSet);		
#endif

/*************************************************/
/* Prototypes for thread/process specific memory */
/*************************************************/

/* Thread/Process memory flags */
#define KPTHREADMEM	1
#define KPPROCMEM	0

#if defined(KPMAC)
KpInt32_t FAR KpSetCurrentProcessId (KpInt32_t processId);
#endif
KpInt32_t FAR KpGetCurrentThreadId (void);
KpInt32_t FAR KpGetCurrentProcessId (void);

KpGenericPtr_t FAR KpThreadMemCreate (
			KpThreadMemHdl_t	FAR *RootId,
			KpInt32_t			ThreadFlag,
			KpUInt32_t			Size);

KpInt32_t FAR KpThreadMemDestroy (
			KpThreadMemHdl_t	FAR *RootId,
			KpInt32_t			ThreadFlag);

KpGenericPtr_t FAR KpThreadMemFind (
			KpThreadMemHdl_t	FAR *RootId,
			KpInt32_t			ThreadFlag);

void FAR KpThreadMemUnlock (
			KpThreadMemHdl_t	FAR *RootId,
			KpInt32_t			ThreadFlag);

/********************************************************/
/* System Info Prototypes, type definitions and constants */
/********************************************************/

typedef enum {
		KPOSTYPE_WIN32S,
		KPOSTYPE_WINNT,
		KPOSTYPE_WIN95,
		KPOSTYPE_WINUNKNOWN,
		KPOSTYPE_SYSTEM6,
		KPOSTYPE_SYSTEM7,
		KPOSTYPE_SYSTEM8,
		KPOSTYPE_SYSUNKNOWN,
		KPOSTYPE_BIG_ENUM1 = BIG_ENUM_NUMBER
} KpOsType_t;
typedef KpOsType_t	*KpOsType_p;

KpInt32_t KpGetSystemInfo (KpOsType_p KpOsType, KpInt32_p version);

/********************************************************/
/* Registry Preference Prototypes, type definitions and */
/* constants                                            */
/********************************************************/

#if defined(KPWIN)
#define KPPREF_BUF_MAX	MAX_PATH
#else
#define KPPREF_BUF_MAX	256
#endif

/* The Preference functions are currently only supported under 
   Macintosh and Windows */
#if !defined (KPUNIX)

KpInt32_t KpDeleteSectionPreference (KpChar_p prefsFile, ioFileChar_p fileProps,
										KpChar_p section);
KpInt32_t KpWriteBooleanPreference (KpChar_p prefsFile, ioFileChar_p fileProps, 
									KpUInt32_t resType, KpInt16_t resID,
									KpChar_p section, KpChar_p entry,
									KpBool_t theBoolean);
KpInt32_t KpReadBooleanPreference (KpChar_p prefsFile, ioFileChar_p fileProps, 
									KpUInt32_t resType, KpInt16_t resID,
									KpChar_p section, KpChar_p entry,
									KpBool_t theDefault, KpBool_p theBoolean);
KpInt32_t KpWriteInt16Preference (KpChar_p prefsFile, ioFileChar_p fileProps, 
									KpUInt32_t resType, KpInt16_t resID,
									KpChar_p section, KpChar_p entry,
									KpInt16_t theShort);
KpInt32_t KpReadInt16Preference (KpChar_p prefsFile, ioFileChar_p fileProps, 
									KpUInt32_t resType, KpInt16_t resID,
									KpChar_p section, KpChar_p entry,
									KpInt16_t theDefault, KpInt16_p theShort);
KpInt32_t KpWriteInt32Preference (KpChar_p prefsFile, ioFileChar_p fileProps, 
									KpUInt32_t resType, KpInt16_t resID,
									KpChar_p section, KpChar_p entry,
									KpInt32_t theLong);
KpInt32_t KpReadInt32Preference (KpChar_p prefsFile, ioFileChar_p fileProps, 
								KpUInt32_t resType, KpInt16_t resID,
								KpChar_p section, KpChar_p entry,
								KpInt32_t theDefault, KpInt32_p theLong);
KpInt32_t KpWriteStringPreference (KpChar_p prefsFile, ioFileChar_p fileProps, 
									KpUInt32_t resType, KpInt16_t resID,
									KpChar_p section, KpChar_p entry,
									KpChar_p theString);
KpInt32_t KpReadStringPreference (KpChar_p prefsFile, ioFileChar_p fileProps, 
									KpUInt32_t resType, KpInt16_t resID,
									KpChar_p section, KpChar_p entry,
									KpChar_p theDefault, KpChar_p theString,
									KpUInt32_t *theSize);
KpInt32_t KpWriteBlockPreference (KpChar_p prefsFile, ioFileChar_p fileProps,
									KpUInt32_t resType, KpInt16_t resID,
									KpChar_p section, KpChar_p entry,
									KpGenericPtr_t theBlock, KpUInt32_t numBytes);
KpInt32_t KpReadBlockPreference (KpChar_p prefsFile, ioFileChar_p fileProps,
									KpUInt32_t resType, KpInt16_t resID,
									KpChar_p section, KpChar_p entry,
									KpGenericPtr_t theDefault, KpUInt32_t defnumBytes,
									KpGenericPtr_t theBlock, KpUInt32_t *numBytes);
KpUInt32_t KpGetBlockSizePreference (KpChar_p prefsFile, ioFileChar_p fileProps,
									KpUInt32_t resType, KpInt16_t resID,
									KpChar_p section, KpChar_p entry);
KpUInt32_t KpGetStringSizePreference (KpChar_p prefsFile, ioFileChar_p fileProps,
									KpUInt32_t resType, KpInt16_t resID,
									KpChar_p section, KpChar_p entry);

KpInt32_t KpSetPreferenceSpec(KpChar_p prefsFile, ioFileChar_p fileProps);
KpInt32_t KpDeletePreferenceType( KpChar_p prefsFile, ioFileChar_p fileProps,
											KpUInt32_t resType);
KpInt32_t KpDeletePreferences (KpChar_p prefsFile, ioFileChar_p fileProps);

KpInt32_t KpDeleteSectionPreferenceEx (KpChar_p prefsFile, KpMainHive_t mainHive,
										KpFileProps_p fileProps, KpChar_p section);
KpInt32_t KpWriteBooleanPreferenceEx (KpChar_p prefsFile, KpMainHive_t mainHive,
									KpFileProps_p fileProps, KpUInt32_t resType,
									KpInt16_t resID, KpChar_p section,
									KpChar_p entry, KpBool_t theBoolean);
KpInt32_t KpReadBooleanPreferenceEx (KpChar_p prefsFile, KpMainHive_t mainHive,
									KpFileProps_p fileProps, KpUInt32_t resType,
									KpInt16_t resID, KpChar_p section,
									KpChar_p entry, KpBool_t theDefault,
									KpBool_p theBoolean);
KpInt32_t KpWriteInt16PreferenceEx (KpChar_p prefsFile, KpMainHive_t mainHive,
									KpFileProps_p fileProps, KpUInt32_t resType,
									KpInt16_t resID, KpChar_p section,
									KpChar_p entry, KpInt16_t theShort);
KpInt32_t KpReadInt16PreferenceEx (KpChar_p prefsFile, KpMainHive_t mainHive,
									KpFileProps_p fileProps, KpUInt32_t resType,
									KpInt16_t resID, KpChar_p section,
									KpChar_p entry, KpInt16_t theDefault,
									KpInt16_p theShort);
KpInt32_t KpWriteInt32PreferenceEx (KpChar_p prefsFile, KpMainHive_t mainHive,
									KpFileProps_p fileProps, KpUInt32_t resType,
									KpInt16_t resID, KpChar_p section,
									KpChar_p entry, KpInt32_t theLong);
KpInt32_t KpReadInt32PreferenceEx (KpChar_p prefsFile, KpMainHive_t mainHive,
									KpFileProps_p fileProps, KpUInt32_t resType,
									KpInt16_t resID, KpChar_p section,
									KpChar_p entry, KpInt32_t theDefault,
									KpInt32_p theLong);
KpInt32_t KpWriteStringPreferenceEx (KpChar_p prefsFile, KpMainHive_t mainHive,
									KpFileProps_p fileProps, KpUInt32_t resType,
									KpInt16_t resID, KpChar_p section,
									KpChar_p entry, KpChar_p theString);
KpInt32_t KpReadStringPreferenceEx (KpChar_p prefsFile, KpMainHive_t mainHive,
									KpFileProps_p fileProps, KpUInt32_t resType,
									KpInt16_t resID, KpChar_p section,
									KpChar_p entry, KpChar_p theDefault,
									KpChar_p theString, KpUInt32_t *theSize);
KpInt32_t KpWriteBlockPreferenceEx (KpChar_p prefsFile, KpMainHive_t mainHive,
									KpFileProps_p fileProps, KpUInt32_t resType,
									KpInt16_t resID, KpChar_p section,
									KpChar_p entry, KpGenericPtr_t theBlock,
									KpUInt32_t numBytes);
KpInt32_t KpReadBlockPreferenceEx (KpChar_p prefsFile, KpMainHive_t mainHive,
									KpFileProps_p fileProps, KpUInt32_t resType,
									KpInt16_t resID, KpChar_p section,
									KpChar_p entry, KpGenericPtr_t theDefault,
									KpUInt32_t defnumBytes, KpGenericPtr_t theBlock,
									KpUInt32_t *numBytes);
KpUInt32_t KpGetBlockSizePreferenceEx (KpChar_p prefsFile, KpMainHive_t mainHive,
									KpFileProps_p fileProps, KpUInt32_t resType,
									KpInt16_t resID, KpChar_p section,
									KpChar_p entry);
KpUInt32_t KpGetStringSizePreferenceEx (KpChar_p prefsFile, KpMainHive_t mainHive,
										KpFileProps_p fileProps, KpUInt32_t resType,
										KpInt16_t resID, KpChar_p section,
										KpChar_p entry);

KpInt32_t KpDeletePreferencesEx (KpChar_p prefsFile, KpMainHive_t mainHive,
									KpFileProps_p fileProps);

KpInt32_t KpSetPreferenceSpecEx(KpChar_p prefsFile, KpFileProps_p fileProps);
KpInt32_t KpDeletePreferenceTypeEx( KpChar_p prefsFile, KpFileProps_p fileProps,
											KpUInt32_t resType);

KpInt32_t KpWriteRegistryString (KpMainHive_t mainHive,	KpChar_p subRegPath,
									KpChar_p stringName, KpChar_p stringBuffer); 

KpInt32_t KpReadRegistryString (KpMainHive_t mainHive, KpChar_p subRegPath,
									KpChar_p stringName, KpChar_p stringBuffer,
									KpUInt32_p bufferSize); 

KpUInt32_t KpGetRegistryStringSize (KpMainHive_t mainHive, KpChar_p subRegPath,
									KpChar_p stringName);

KpInt32_t KpDeleteRegistry (KpMainHive_t mainHive, KpChar_p subKeyName);

#endif

/********************************************************/
/* Resource Prototypes, type definitions and */
/* constants                                            */
/********************************************************/
#if defined (KPMAC)
typedef	Component			KpModuleId;
typedef ComponentInstance	KpInstance;
#elif defined (KPWIN)
typedef	HINSTANCE			KpModuleId;
typedef HINSTANCE			KpInstance;
#else
typedef	KpInt32_t			KpModuleId;
typedef KpInt32_t			KpInstance;
#endif

#if defined(KPMAC) || defined(KPWIN)

#if defined(KPMAC)
void KpCopyPStr (Str255 first, Str255 second);
#endif

KpInt32_t KpGetCString (KpModuleId id,
						KpInt32_t strListId,
						KpInt32_t index,
						KpChar_p theString,
						KpInt32_t maxSize);
KpInt32_t KpGetProductVersion (KpModuleId id,
								KpChar_p verString,
								KpInt32_t maxSize);
#endif


/*************************************/
/* Time                              */
/*************************************/

/*
 * This structure is just a copy of
 * the ANSI one
 */
typedef struct kpTm {
	int sec;	/* seconds after the minute - [0,59] */
	int min;	/* minutes after the hour - [0,59] */
	int hour;	/* hours since midnight - [0,23] */
	int mday;	/* day of the month - [1,31] */
	int mon;	/* months since January - [0,11] */
	int year;	/* years since 1900 */
	int wday;	/* days since Sunday - [0,6] */
	int yday;	/* days since January 1 - [0,365] */
	int isdst;	/* daylight savings time flag */
} kpTm_t, FAR* kpTm_p, FAR* FAR* kpTm_h;

void KpGetLocalTime (
			struct kpTm	FAR *localTime);

/***********************************************/
/* Prototypes for directory handling functions */
/***********************************************/
typedef enum {
		DIR_NO_FILE,
		DIR_FOUND_SUBDIR,
		DIR_FOUND_FILE,
		DIR_FOUND_PT,
		DIR_NO_MEMORY,
		DIR_OK,
		DIR_ERROR,
		DIR_PARAM_ERROR
} dirStatus;

#define 	PT_EXTENSION	".PT"

dirStatus FAR KpFileDirCount (
			char		FAR *DirName,
			KpFileProps_t	FAR *FileProps,
			KpInt32_t	FAR *Count);


/*** ioFileFirstDirEntry and ioFileNextDirEntry
		have been obsoleted by ioFileFind			**/
dirStatus FAR ioFileFirstDirEntry (
			char	FAR *DirName,
			bool	SubDirsFlag,
			char	FAR * FAR *FileNameBuffer);

dirStatus FAR ioFileNextDirEntry (
			char FAR * FAR *FileNameBuffer);

/*** ioFileDirCount has been obsoleted by KpFileDirCount	**/
dirStatus FAR ioFileDirCount (
			char		FAR *DirName,
			ioFileChar	FAR *FileProps,
			KpInt32_t	FAR *Count);


/*****************************************************************************
 *
 * Function: KpFileFind
 *
 * Function obsoletes ioFileFind
 * Function obsoletes ioFileFirstDirEntry ioFileNextDirEntry.
 *
 * Windows 32 bit changed the interface for searching directories
 * which required the caller to close the search. To eliminate
 * possible application interface conflicts with older applications,
 * ioFileDirFirst and ioFileDirNext were removed for KPWIN32 builds.
 *
 * The user supplies a callback to ioFileFind which is called for all
 * files appropriately found within a directory. The user no longer has to
 * start the search with one function, coontinue the search with another and
 * finally ends the search with yet a thrd function. 
 *
 * See io_file.c for more details on this function.
 *
 ****************************************************************************/
/* Defined Operational Status of iofileFindNext */

#define IOFILE_NEXT		0x0
#define IOFILE_STARTED	0x1
#define IOFILE_FIRST	0x2
#define IOFILE_FINISHED 0x4
#define IOFILE_ERROR	0x10

#ifndef MAX_PATH
#define	MAX_PATH	260
#endif


/* Attribute Flags for a File Entry */
#if defined (KPMAC) || defined (KPMSMAC)

#include <Files.h>

#define ATTR_DIRECTORY		0x10		/* File is a (sub) Directory File */

typedef void FAR * Kp_FindOS_Data_p; 

/* osfPrivate is used in iofileDirEntry_t, osfFilePrivate is used in 
   KpfileDirEntry_t.  Do not make any changes to osfPrivate_t as that is 
   used by the ioFilexxx functions which are now obsolete.  All changes 
   should be made to osfFilePrivate_t										*/ 
typedef struct osfPrivate_tbl	{
	KpUInt16_t	vRefNum;			/* Mac volume reference number */
	KpUInt32_t	fileType;			/* Mac File Type */
	KpUInt32_t	fileCreator;		/* Mac Creator */
	KpUInt32_t	dirID;				/* Mac Directory ID */
	CInfoPBRec	parm;				/* Mac parameters for call to ROM */
} osfPrivate_t;

typedef struct osfFilePrivate_tbl	{
	KpUInt16_t	vRefNum;			/* Mac volume reference number */
	KpUInt32_t	fileType;			/* Mac File Type */
	KpUInt32_t	fileCreator;		/* Mac Creator */
	KpUInt32_t	dirID;				/* Mac Directory ID */
	CInfoPBRec	parm;				/* Mac parameters for call to ROM */
} osfFilePrivate_t;

#elif defined (KPWIN32)

#define ATTR_DIRECTORY		FILE_ATTRIBUTE_DIRECTORY	/* File is a (sub) Directory File */

typedef WIN32_FIND_DATA	  Kp_FindOS_Data;
typedef Kp_FindOS_Data	* Kp_FindOS_Data_p;

/* osfPrivate is used in iofileDirEntry_t, osfFilePrivate is used in 
   KpfileDirEntry_t.  Do not make any changes to osfPrivate_t as that is 
   used by the ioFilexxx functions which are now obsolete.  All changes 
   should be made to osfFilePrivate_t										*/ 
typedef struct osfPrivate_tbl	{
	char			base_Dir[MAX_PATH];
	void *			userPrivate;					/* keep it simple for now */
} osfPrivate_t;

typedef struct osfFilePrivate_tbl	{
	char			base_Dir[MAX_PATH];
	char			filter [MAX_PATH];
	Kp_FindOS_Data	*pFindData;
	void *			userPrivate;					/* keep it simple for now */
} osfFilePrivate_t;


#elif defined (KPWIN16)

#include <dos.h>

#define ATTR_DIRECTORY		_A_SUBDIR	/* File is a (sub) Directory File */

typedef struct find_t	  Kp_FindOS_Data;
typedef Kp_FindOS_Data	* Kp_FindOS_Data_p;

/* osfPrivate is used in iofileDirEntry_t, osfFilePrivate is used in 
   KpfileDirEntry_t.  Do not make any changes to osfPrivate_t as that is 
   used by the ioFilexxx functions which are now obsolete.  All changes 
   should be made to osfFilePrivate_t										*/ 
typedef struct osfPrivate_tbl	{
	char		base_Dir[MAX_PATH];
	void *		userPrivate;					/* keep it simple for now */
} osfPrivate_t;

typedef struct osfFilePrivate_tbl	{
	char		base_Dir[MAX_PATH];
	void *		userPrivate;					/* keep it simple for now */
} osfFilePrivate_t;


#else

#define ATTR_DIRECTORY		0x10		/* File is a (sub) Directory File */

typedef void FAR * Kp_FindOS_Data_p; 

/* osfPrivate is used in iofileDirEntry_t, osfFilePrivate is used in 
   KpfileDirEntry_t.  Do not make any changes to osfPrivate_t as that is 
   used by the ioFilexxx functions which are now obsolete.  All changes 
   should be made to osfFilePrivate_t										*/ 
typedef struct osfPrivate_tbl	{
	char		base_Dir[MAX_PATH];
	void FAR *	userPrivate;					/* keep it simple for now */
} osfPrivate_t;

typedef struct osfFilePrivate_tbl	{
	char		base_Dir[MAX_PATH];
	void FAR *	userPrivate;					/* keep it simple for now */
} osfFilePrivate_t;

#endif

#define ATTR_MASK			(ATTR_DIRECTORY)

typedef struct iofileDirEntry_tbl	{
/* (No attributes picked means all attributes desired in wAttr) */ 
	KpUInt32_t		structSize;			/* size of this structure according to user */
	KpUInt32_t		wAttr;				/* wanted directory, read protected, etc. attributes */
	KpUInt32_t		nwAttr;				/* attributes which don't want */
	KpInt8_t		fileName[MAX_PATH];	/* filename */
	KpInt16_t		opStatus;			/* starting, finished or opps! */
	osfPrivate_t	osfPrivate;			/* OS dependendent stuff */
} iofileDirEntry_t;

typedef struct KpfileDirEntry_tbl	{
/* (No attributes picked means all attributes desired in wAttr) */ 
	KpUInt32_t			structSize;			/* size of this structure according to user */
	KpUInt32_t			wAttr;				/* wanted directory, read protected, etc. attributes */
	KpUInt32_t			nwAttr;				/* attributes which don't want */
	KpInt8_t			fileName[MAX_PATH];	/* filename */
	KpInt16_t			opStatus;			/* starting, finished or opps! */
	KpBool_t			subDirSearch;		/* set to true if this is a recursive entry */
	osfFilePrivate_t	osfPrivate;			/* OS dependendent stuff */
} KpfileDirEntry_t;

typedef bool (FAR * iofileCallBack_t) (
			iofileDirEntry_t	FAR * fileSearch,
			void				FAR * user_ptr);

typedef KpBool_t (FAR * KpfileCallBack_t) (
			KpfileDirEntry_t	FAR * fileSearch,
			void				FAR * user_ptr);
			
/** ioFileFind obsoleted by KpFileFind */
bool ioFileFind (
			iofileDirEntry_t	FAR * fileSearch,
			void				FAR * user_ptr,
			iofileCallBack_t	user_fcn );

KpBool_t KpFileFind (
			KpfileDirEntry_t	FAR * fileSearch,
			void				FAR * user_ptr,
			KpfileCallBack_t	user_fcn );


/**********************************/
/* Prototypes for file/memory I/O */
/**********************************/
typedef int		ioFileId, KpFileId;

 /*
  * A kp memory/file descriptor is either an int (used for disk files)
  * or a more complex structure describing memory files.  The kp io library
  * does NOT maintain a global array of 32 of these like the fut library
  * used to do.
  */
typedef struct KpFd_s {
	enum {
		KCMS_IO_NULLFILE = 0x7aaa,	/* unused descriptor */
		KCMS_IO_SYSFILE,						/* normal file */
		KCMS_IO_MEMFILE,						/* memory file */
		KCMS_IO_CALCCRC,						/* calculate a crc */
		KCMS_IO_BIG_ENUM1 = BIG_ENUM_NUMBER
	} type;	       /* type of kcms file (union tag) */

	union {
		/* system file descriptor (if type==KCMS_IO_SYSFILE) */
		ioFileId          sys;	

		/* memory file descriptor (if type==KCMS_IO_MEMFILE) */
		struct {
			KpLargeBuffer_t buf;	/* pointer to memory buffer */
			KpInt32_t       size;	/* size of memory file */
			KpInt32_t       pos;	/* current position within memfile */
		} mem;

		/* cumulative crc */
		KpCrc32_t	crc32;

	} fd;

} KpFd_t, FAR * KpFd_p, FAR * FAR * KpFd_h;

typedef enum {
		FROM_START,
		FROM_CURRENT,
		FROM_END
} ioFileStart, KpFileStart;


#if defined(KPMAC) || defined (KPMSMAC)
short GetBlessed KPARGS((void));
int		KpGetVolAndName(
			char*		oldPath,
			KpFileProps_t	*oldProps, 
			char**		newPath, 
			KpFileProps_t	*newProps);
			
int KpGetBlessed (
			short	*foundVRefNum, 
			long	*foundDirID);

int 	KpGetDirRefNum(
			KpFileProps_t	*props, 
			char*		rPath,
			int		length, 
			KpFileProps_t	*newProps);
			
int KpPathNameFromDirID(
			short		vRefNum, 
			long		dirID, 
			char_p		fullPathName,
			KpUInt32_t	bufSize);

int		getVolAndName(char*,short*,char**);
int		getDirRefNum(short,char*,int);
#endif
int	FAR KpFileOpen (
			char		FAR *Name,
			char		FAR *Mode,
			KpFileProps_t	FAR *Props,
			KpFileId	FAR *FileId);

int	FAR KpFileRead (
			KpFileId FileId,
			KpLargeBuffer_t Buffer,
			KpInt32_t FAR *Size);

int	FAR KpFileWrite (
			KpFileId		FileId,
			KpLargeBuffer_t	Buffer,
			KpInt32_t		Size);

int	FAR KpFilePosition (
			KpFileId	FileId,
			KpFileStart	Whence,
			KpInt32_t	Offset);

int FAR KpFileTell (
			KpFileId	FileId,
			KpInt32_t	FAR *Position);

int	FAR KpFileSize (
			char		FAR *Name,
			KpFileProps_t	FAR *Props,
			KpInt32_t	FAR *Size);

int	FAR KpFileCopy (
			KpFileProps_t	FAR *Props,
			char		FAR *OldName,
			char		FAR *NewName);

int	FAR KpFileClose (
			KpFileId	FileId);

int FAR KpFileDelete (
			char		FAR *Name,
			KpFileProps_t	FAR *Props);

dirStatus FAR KpFileExists (
			char		FAR *Name,
			KpFileProps_t	FAR *Props,
			KpBool_t	FAR *Flag);

int	FAR KpFileRename (
			KpFileProps_t	FAR *fileProps,
 			char		FAR *OldName,
			char		FAR *NewName);

void KpFileStripPath (
			KpChar_p filePlusPath,
			KpChar_p theFile);

/*** The ioFilexxx functions are obsolete.  ***/
/*** Please use the KpFilexxx functions 	***/
int	FAR ioFileOpen (
			char		FAR *Name,
			char		FAR *Mode,
			ioFileChar	FAR *Props,
			ioFileId	FAR *FileId);

int	FAR ioFileRead (
			ioFileId FileId,
			KpLargeBuffer_t Buffer,
			KpInt32_t FAR *Size);

int	FAR ioFileWrite (
			ioFileId		FileId,
			KpLargeBuffer_t	Buffer,
			KpInt32_t		Size);

int	FAR ioFilePosition (
			ioFileId	FileId,
			ioFileStart	Whence,
			KpInt32_t	Offset);

int FAR ioFileTell (
			ioFileId	FileId,
			KpInt32_t	FAR *Position);

int	FAR ioFileSize (
			char		FAR *Name,
			ioFileChar	FAR *Props,
			KpInt32_t	FAR *Size);

int	FAR ioFileCopy (
			char	FAR *OldName,
			char	FAR *NewName);

int	FAR ioFileClose (
			ioFileId	FileId);

int FAR ioFileDelete (
			char		FAR *Name,
			ioFileChar	FAR *Props);

int FAR ioFileExists (
			char		FAR *Name,
			ioFileChar	FAR *Props,
			bool		FAR *Flag);

int	FAR ioFileRename (
			char	FAR *OldName,
			char	FAR *NewName);

void ioFileStripPath (
			KpChar_p filePlusPath,
			KpChar_p theFile);

/* Functions that use files or memory buffers */
int KpOpen (
			char		FAR *filename,
			char		FAR *mode,
			KpFd_t		FAR *fd,
			KpFileProps_t	FAR *fileProps,
			...);

	/** Please use Kp_openEx in place of Kp_open.	**/
	/** Kp_open is obsolete.						**/
int Kp_open (
			char		FAR *filename,
			char		FAR *mode,
			KpFd_t		FAR *fd,
			ioFileChar	FAR *fileProps,
			...);

int Kp_close (
			KpFd_t		FAR *fd);

int Kp_read (
			KpFd_t			FAR *fd,
			KpLargeBuffer_t	buf,
			KpInt32_t		nbytes);

int Kp_write (
			KpFd_t			FAR *fd,
			KpLargeBuffer_t	buf,
			KpInt32_t		nbytes);

int Kp_skip (
			KpFd_t		FAR *fd,
			KpInt32_t	nbytes);

int Kp_get_crc (
			KpFd_t		FAR *fd,
			KpCrc32_t	FAR *crc);

KpCrc32_t Kp_Crc32 (
			KpCrc32_t	crc32,
			KpInt32_t	size,
			KpChar_p	buf);

void Kp_swab16 (
			KpGenericPtr_t	buf,
			KpInt32_t		nitems);

void Kp_swab32 (
			KpGenericPtr_t	buf,
			KpInt32_t		nitems);

/*************************************/
/* buffered I/O                      */
/*	uses ioFile* and alloc* routines */
/*************************************/

typedef struct {
	KpFileId	fd;
    char		FAR *buff;
	char		mode;
	KpInt32_t	size;
	int			valid;
	int			offset;
	int			status;
	KpInt32_t	start_pos;
	KpBool_t	dirty;
	KpBool_t	empty;
	KpBool_t	read_only;
} KpBuffer_t;

int KpBufferFlush (
			KpBuffer_t	FAR *ioBuffer);

int KpBufferDestroy (
			KpBuffer_t	FAR *ioBuffer);

int KpBufferCreate (
			KpFileId	fd,
			char		FAR *mode,
			KpBuffer_t	FAR *ioBuffer,
			KpInt32_t	size);

int KpBufferOpen (
			char		FAR *filename,
			char		FAR *mode,
			KpFileProps_t	FAR *props,
			KpBuffer_t	FAR *ioBuffer,
			KpInt32_t	size);

int KpBufferClose (
			KpBuffer_t	FAR *ioBuffer);

int KpBufferSeek (
			KpBuffer_t	FAR *ioBuffer,
			KpFileStart	whence,
			KpInt32_t	offset);

int KpBufferRead (
			KpBuffer_t		FAR *ioBuffer,
			KpLargeBuffer_t	buffer,
			KpInt32_t		FAR *numBytes);

int KpBufferWrite (
			KpBuffer_t		FAR *ioBuffer,
			KpLargeBuffer_t	buffer,
			KpInt32_t		FAR *numBytes);

/*** The ioBufferxxx functions are obsolete.  ***/
/*** Please use the KpBufferxxx functions 	***/
typedef struct {
	ioFileId	fd;
    char		FAR *buff;
	char		mode;
	KpInt32_t	size;
	int			valid;
	int			offset;
	int			status;
	KpInt32_t	start_pos;
	KpBool_t	dirty;
	KpBool_t	empty;
	KpBool_t	read_only;
} ioBuffer_t;

int ioBufferFlush (
			ioBuffer_t	FAR *ioBuffer);

int ioBufferDestroy (
			ioBuffer_t	FAR *ioBuffer);

int ioBufferCreate (
			ioFileId	fd,
			char		FAR *mode,
			ioBuffer_t	FAR *ioBuffer,
			KpInt32_t	size);

int ioBufferOpen (
			char		FAR *filename,
			char		FAR *mode,
			ioFileChar	FAR *props,
			ioBuffer_t	FAR *ioBuffer,
			KpInt32_t	size);

int ioBufferClose (
			ioBuffer_t	FAR *ioBuffer);

int ioBufferSeek (
			ioBuffer_t	FAR *ioBuffer,
			ioFileStart	whence,
			KpInt32_t	offset);

int ioBufferRead (
			ioBuffer_t		FAR *ioBuffer,
			KpLargeBuffer_t	buffer,
			KpInt32_t		FAR *numBytes);

int ioBufferWrite (
			ioBuffer_t		FAR *ioBuffer,
			KpLargeBuffer_t	buffer,
			KpInt32_t		FAR *numBytes);

/*************************************/
/* File Mapping                      */
/*************************************/

typedef struct {
#if defined (KPWIN32) && !defined(KPMSMAC)
	HANDLE		hMapObject;
	HFILE		hFile;
#else
	ioFileId	Fd;
	char		Mode;
#endif
	KpInt32_t	NumBytes;
	void		FAR *Ptr;
} KpMapFile_t;

void FAR *KpMapFileEx (
			char		FAR *Filename,
			KpFileProps_t	FAR *FileProps,
			char		FAR *Mode,
			KpMapFile_t	FAR *MapFileCtl);

/** Please use KpMapFileEx.	**/
/** KpMapFile is obsolete.	**/
void FAR *KpMapFile (
			char		FAR *Filename,
			ioFileChar	FAR *FileProps,
			char		FAR *Mode,
			KpMapFile_t	FAR *MapFileCtl);

int KpUnMapFile (
			KpMapFile_t	FAR *MapFileCtl);


bool FAR Kp_IsBadHugeReadPtr (
			const void	KPHUGE *Ptr,
			KpUInt32_t	Size);

bool FAR Kp_IsBadHugeWritePtr (
			void		KPHUGE *Ptr,
			KpUInt32_t	Size);

bool FAR Kp_IsBadReadPtr (
			const void	FAR *Ptr,
			KpUInt32_t	Size);

bool FAR Kp_IsBadWritePtr (
			void		FAR *Ptr,
			KpUInt32_t	Size);

bool FAR Kp_IsBadStringPtr (
			const void	FAR *Ptr,
			KpUInt32_t	Size);

bool FAR Kp_IsBadCodePtr (
			FARPROC);

void * FAR KpMemCpy (
			void		KPHUGE *dest,
			void		KPHUGE *src,
			KpInt32_t	cnt);

int  FAR KpMemCmp (
			void		KPHUGE *s1,
			void		KPHUGE *s2,
			KpInt32_t	cnt);

void * FAR KpMemSet (
			void		KPHUGE *ptr,
			int			value,
			KpInt32_t	cnt);

/*****************************/
/* Numeric/String conversion */
/*****************************/

/* convert integer to ascii string */
char FAR * FAR KpItoa (
			KpInt32_t	Value,
			char		FAR *Buf);

/* convert ascii string to integer */
KpInt32_t FAR KpAtoi (
			char FAR *Buf);

/* convert integer to hex string */
char FAR * FAR KpLtos (
			KpInt32_t	Value,
			char		FAR *Buf);

#ifdef __cplusplus
}
#endif

/* Reset the MSVC 4.1 compiler warning:  "keyword is reserved for future use" */
#if defined (KPMSMAC) || defined (KPWIN32)
#pragma warning( default : 4237 )
#endif

#endif /* KCMS_SYS_H */


