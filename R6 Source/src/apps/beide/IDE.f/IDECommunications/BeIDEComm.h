//========================================================================
//	BeIDEComm.h
//	Copyright 1995 - 97 Metrowerks Corporation. All rights reserved.
//========================================================================	
//	Header for communicating between the compiler and Be IDE
//	Jon Watte, BS

#ifndef _BEIDECOMM_H
#define _BEIDECOMM_H


#define MWBROWSER 1
#define USINGFILECACHING 1

#include <OS.h>

#define MAX_PATH_LENGTH 256
#define MAX_ERROR_LENGTH 2048

#ifdef __MWERKS__
	#pragma options align=mac68k
	#define PACKED
#else
	#define PACKED __attribute__((packed))
#endif

struct FSSpec {
	int16	v PACKED;
	int32	d PACKED;
	char	name[MAX_PATH_LENGTH] PACKED;
};

#ifdef __MWERKS__
	#pragma options align=reset
#endif

#include "CompilerErrorRef.h"

struct FileHandle
{
	area_id		id;
	int32		offset;
};

// Header for each file in the area
struct SharedFileRec
{
	int32		length;
	char		name[256];
	bool		systemtree;
};

/*
 * The IDE will call the compiler with the first argument being
 * -beide 14,32 where "14" will be the OS port for the compiler
 * to write to, and the "32" being the OS port for the compiler
 * to read a reply from.
 *
 * The HeaderQuery is a synchronous query; after writing to the
 * write port, the compiler will read the reply from the read
 * port.
 * 
 * The compiler also waits for a reply for the SendText message 
 * so it knows when it can delete the area.
 *
 * The others are just notifications from the compiler to the
 * IDE; no reply is given or should be read.
 */

typedef enum QueryType {
	kNullQuery,
	kHeaderQuery,
	kHeaderReply,
	kMessageNotification,
	kStatusNotification,
	// CW8 additions
	kPreprocessResult,
	kPreprocessReply,
	kCompilerStatusNotification,
	// DR8 additions
	kGetArea,
	kAreaReply,
	kSourceQuery,
	kSourceReply
} QueryType;

typedef struct HeaderQuery {
	char		fileName[B_FILE_NAME_LENGTH];
	bool		inSysTree;
} HeaderQuery;

typedef struct HeaderReply {
	status_t	errorCode;
	char		filePath[MAX_PATH_LENGTH];
	bool		alreadyIncluded;
#ifdef MWBROWSER
	bool		recordbrowseinfo;		/* [->] record browse info for this file?		*/
	int16		browserfileID;			/* [->] fileID to use in browse records		*/
#endif
#ifdef USINGFILECACHING
	FileHandle	handle;					/* [->] FileHandle for locating this file in shared memory*/
#endif
} HeaderReply;

typedef struct ErrorNotificationMessage {
	CompilerErrorRef		errorRef;
	bool					hasErrorRef;
	bool					isWarning;
	char					errorMessage[MAX_ERROR_LENGTH];
} ErrorNotificationMessage;

// sent at the end of each source file in a compile
typedef struct CompilerStatusNotification {
	status_t	errorCode;
	status_t	browseErrorCode;
	int32 		codeSize;
	int32		dataSize;
	int32		count; // the order of this file in the batch of files (zero based)
	char 		fileName[B_FILE_NAME_LENGTH];
} CompilerStatusNotification;

// sent at the end of all source files
typedef struct StatusNotification {
	status_t		errorCode;
	bool		objProduced;
} StatusNotification;

typedef struct SendTextMessage {
	area_id		textArea;
	int32		textSize;
} SendTextMessage;

typedef struct SendTextReply {
	int32		doneWithArea;
} SendTextReply;

typedef struct GetAreaReply {
	area_id		area;
} GetAreaReply;

typedef struct BrowseDataArea {
	int32		length;
	char		data[1024];
} BrowseDataArea;

// The following is from DropInCompilerLinker.h
typedef struct CWBrowseOptions {
	bool recordClasses;				/* [<-] do we record info for classes				*/
	bool recordEnums;				/* [<-] do we record info for enums					*/
	bool recordMacros;				/* [<-] do we record info for macros				*/
	bool recordTypedefs;			/* [<-] do we record info for typedefs				*/
	bool recordConstants;			/* [<-] do we record info for constants				*/
	bool recordTemplates;			/* [<-] do we record info for templates				*/
	bool recordUndefinedFunctions;	/* [<-] do we record info for undefined functions	*/
	int32	reserved1;					/* reserved space									*/
	int32	reserved2;					/* reserved space									*/
} CWBrowseOptions;

typedef struct SourceQuery {
	char		fileName[MAX_PATH_LENGTH]; /* full name of project file (this will be the same name you pass to the compiler */
} SourceQuery;

typedef struct SourceReply {
	status_t		errorCode;
	CWBrowseOptions	browseoptions;		/* [->]	enabled browse information				*/
	bool			recordbrowseinfo;	/* [->] record browse info for this file?		*/
	int16			browserfileID;		/* [->] fileID to use in browse records			*/
} SourceReply;


/*
 * These handy functions take care of the actual communication
 * from the compiler to the IDE
 */

status_t	DoHeaderQuery(
				const char * fileNameIn,
				bool inSysTreeIn,
				char * filePathOut,
				bool * alreadyIncluded);

status_t	DoMessageNotification(
				const CompilerErrorRef * error,
				const char * message,
				bool	warn);

status_t	DoCompilerStatusNotification(
				long errorCodeIn,
				long objcodesize,
				long objdatasize,
				long count,
				const char *name);

status_t	DoStatusNotification(
				long errorCodeIn,
				bool objProducedIn);

status_t	DoSendText(
				port_id 		wrPort,
				port_id			rdPort,
				const char *	inText,
				long			inTextLength,
				QueryType		inMessageType);

status_t	DoCopyTextToArea(
				port_id 		wrPort,
				port_id			rdPort,
				const void *	inData,
				long			inTextLength);

extern bool gIDE;
extern int	gWRPort;
extern int	gRDPort;

#endif
