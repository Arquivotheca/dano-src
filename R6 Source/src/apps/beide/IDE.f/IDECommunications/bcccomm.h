//========================================================================
//	BeIDEComm.h
//	Copyright 1995 - 97 Metrowerks Corporation. All rights reserved.
//========================================================================	
//	Header for communicating between the compiler and Be IDE
//	Jon Watte, BS

#pragma once

#define MWBROWSER 1
#define USINGFILECACHING 1

#include <OS.h>

#define MAX_PATH_LENGTH 256
#define MAX_ERROR_LENGTH 2048

#pragma options align=mac68k

struct FSSpec {
	short	v;
	long	d;
	char	name[MAX_PATH_LENGTH];
};

#pragma options align=reset

#include "CompilerErrorRef.h"

struct FileHandle
{
	area_id		id;
	long		offset;
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
	long		errorCode;
	char		filePath[MAX_PATH_LENGTH];
	bool		alreadyIncluded;
#ifdef MWBROWSER
	bool		recordbrowseinfo;		/* [->] record browse info for this file?		*/
	short		browserfileID;			/* [->] fileID to use in browse records		*/
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
	long		errorCode;
	long		browseErrorCode;
	long 		codeSize;
	long		dataSize;
	long		count; // the order of this file in the batch of files (zero based)
	char 		fileName[B_FILE_NAME_LENGTH];
} CompilerStatusNotification;

// sent at the end of all source files
typedef struct StatusNotification {
	long		errorCode;
	bool		objProduced;
} StatusNotification;

typedef struct SendTextMessage {
	area_id		textArea;
	long		textSize;
} SendTextMessage;

typedef struct SendTextReply {
	long		doneWithArea;
} SendTextReply;

typedef struct GetAreaReply {
	area_id		area;
} GetAreaReply;

typedef struct BrowseDataArea {
	long		length;
	char		data[1024];
} BrowseDataArea;

// The following is from DropInCompilerLinker.h
typedef struct CWBrowseOptions {
	Boolean recordClasses;				/* [<-] do we record info for classes				*/
	Boolean recordEnums;				/* [<-] do we record info for enums					*/
	Boolean recordMacros;				/* [<-] do we record info for macros				*/
	Boolean recordTypedefs;				/* [<-] do we record info for typedefs				*/
	Boolean recordConstants;			/* [<-] do we record info for constants				*/
	Boolean recordTemplates;			/* [<-] do we record info for templates				*/
	Boolean recordUndefinedFunctions;	/* [<-] do we record info for undefined functions	*/
	long	reserved1;					/* reserved space									*/
	long	reserved2;					/* reserved space									*/
} CWBrowseOptions;

typedef struct SourceQuery {
	char		fileName[MAX_PATH_LENGTH]; /* full name of project file (this will be the same name you pass to the compiler */
} SourceQuery;

typedef struct SourceReply {
	long			errorCode;
	CWBrowseOptions	browseoptions;		/* [->]	enabled browse information				*/
	bool			recordbrowseinfo;	/* [->] record browse info for this file?		*/
	short			browserfileID;		/* [->] fileID to use in browse records			*/
} SourceReply;


/*
 * These handy functions take care of the actual communication
 * from the compiler to the IDE
 */

long	DoHeaderQuery(
			const char * fileNameIn,
			bool inSysTreeIn,
			char * filePathOut,
			bool * alreadyIncluded);

long	DoMessageNotification(
			const CompilerErrorRef * error,
			const char * message,
			bool	warn);

long	DoCompilerStatusNotification(
			long errorCodeIn,
			long objcodesize,
			long objdatasize,
			long count,
			const char *name);

long	DoStatusNotification(
			long errorCodeIn,
			bool objProducedIn);

long	DoSendText(
			port_id 		wrPort,
			port_id			rdPort,
			const char *	inText,
			long			inTextLength,
			QueryType		inMessageType);

long	DoCopyTextToArea(
			port_id 		wrPort,
			port_id			rdPort,
			const void *	inData,
			long			inTextLength);

extern bool gIDE;
extern int	gWRPort;
extern int	gRDPort;
