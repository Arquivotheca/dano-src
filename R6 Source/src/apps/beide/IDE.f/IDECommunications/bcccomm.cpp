//	bcccomm.cpp
//	Communications from the Be compiler to the Be IDE
//	Copyright 1995 Metrowerks Corporation. All rights reserved.
//	Jon Watte

#include <Debug.h>
#include <string.h>

#include "bcccomm.h"


//	When the compiler sees an include file, it should call this
//	function to find the location of the include file, or get an
//	error code if the file can't be found. If the include is with
//	brackets, inSysTreeIn should be TRUE.
//	Returns a pathname to use for opening the file.
//
long
DoHeaderQuery(
	port_id wrPort,
	port_id rdPort,
	const char * fileNameIn,
	bool inSysTreeIn,
	char * filePathOut)
{
	HeaderQuery message;
	HeaderReply reply;

	ASSERT(strlen(fileNameIn) < B_FILE_NAME_LENGTH);
	strncpy(message.fileName, fileNameIn, B_FILE_NAME_LENGTH);
	message.fileName[B_FILE_NAME_LENGTH-1] = 0;
	message.inSysTree = inSysTreeIn;

	long err = write_port(wrPort, kHeaderQuery, &message, sizeof(message));
	long id = kNullQuery;
	if (!err) {
		err = read_port(rdPort, &id, &reply, sizeof(reply));
	}
	if (!err) {
		ASSERT(id == kHeaderReply);
		err = reply.errorCode;
	}
	if (!err) {
		ASSERT(strlen(reply.filePath) < MAX_PATH_LENGTH);
		strncpy(filePathOut, reply.filePath, MAX_PATH_LENGTH);
		filePathOut[MAX_PATH_LENGTH-1] = 0;
	}
	return err;
}


//	When the compiler has a warning, error or note to give,
//	it should call this function. If severityIn is kFatalError,
//	it is expected that the compiler as the next message will
//	send a StatusNotification and quit.
//	The file and line arguments are displayed together with the
//	message like "<FILE> line ##: Message"
//
long
DoMessageNotification(
	port_id wrPort,
	long errorCodeIn,
	long fileLineIn,
	Severity severityIn,
	const char * messageIn,
	const char * fileNameIn,
	bool inSysTreeIn)
{
	MessageNotification message;

	ASSERT(strlen(messageIn) < MAX_PATH_LENGTH);
	ASSERT(strlen(fileNameIn) < B_FILE_NAME_LENGTH);
	message.errorCode = errorCodeIn;
	message.fileLine = fileLineIn;
	message.severity = severityIn;
	strncpy(message.message, messageIn, MAX_PATH_LENGTH);
	message.message[MAX_PATH_LENGTH-1] = 0;
	strncpy(message.fileName, fileNameIn, B_FILE_NAME_LENGTH);
	message.fileName[B_FILE_NAME_LENGTH-1] = 0;
	message.inSysTree = inSysTreeIn;

	return write_port(wrPort, kMessageNotification,
		&message, sizeof(message));
}


//	When the compiler is done, it should send one and only one
//	of these messages, stating whether an object file was produced,
//	and giving an error code if not.
//
long
DoStatusNotification(
	port_id wrPort,
	long errorCodeIn,
	bool objProducedIn)
{
	StatusNotification message;

	message.errorCode = errorCodeIn;
	message.objProduced = objProducedIn;

	return write_port(wrPort, kStatusNotification,
		&message, sizeof(message));
}