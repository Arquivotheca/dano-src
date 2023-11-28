// Code for communications between BeIDe and mwcc

// into bcccomm.h

// this enum is just being modified
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
	kAreaReply
} QueryType;

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

// sent at the end of each source file in a compile
typedef struct CompilerStatusNotification {
	int32		errorCode;
	int32 		codeSize;
	int32		dataSize;
	int32		count; // the order of this file in the batch of files (zero based)
	char 		fileName[B_FILE_NAME_LENGTH];
} CompilerStatusNotification;

// sent at the end of all source files
typedef struct StatusNotification {
	status_t	errorCode;
	bool		objProduced;
} StatusNotification;

status_t	
DoSendText(
			port_id 		wrPort,
			port_id			rdPort,
			const char *	inText,
			int32			inTextLength,
			QueryType		inMessageType);


status_t
DoCopyTextToArea(
	port_id 		wrPort,
	port_id			rdPort,
	const void *	inData,
	int32			inTextLength);

// into bcccomm.cpp

#include <OS.h>
#include <string.h>
#include <Debug.h>

// This function is used to communicate arbitrary sized text to the IDE
// This will be in response to requests for preprocess info and eventually
// for disassembly info.  It works by allocating an area and copying the
// text into that area.  The area_id is passed to the IDE, which copies
// the text.  The IDE then sends back a kProcessReply message.  When this
// is received the compiler deletes the area.  A status notification should
// be sent after this reporting any errors.
// inMessageType should be kPreprocessResult for the response to a preprocess
// command line request.

status_t
DoSendText(
	port_id 		wrPort,
	port_id			rdPort,
	const char *	inText,
	int32			inTextLength,
	QueryType		inMessageType)
{
	void*			areaAddr = (void*) B_PAGE_SIZE;
	int32			areaSize = ((inTextLength / B_PAGE_SIZE) * B_PAGE_SIZE) + B_PAGE_SIZE;
	int32			id = kNullQuery;
	status_t		err = B_NO_ERROR;
	SendTextMessage	message;
	SendTextReply	reply;

	// Generate the area
	message.textArea = create_area("mwcc area", &areaAddr,
					B_ANY_ADDRESS, areaSize, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (message.textArea < B_NO_ERROR)
		err = message.textArea;

	// Send the text to the IDE
	if (err == B_NO_ERROR)
	{
		memmove(areaAddr, inText, inTextLength);
		message.textSize = inTextLength;

		err = write_port(wrPort, inMessageType, &message, sizeof(message));
	}
	
	// Wait for the reply
	if (err == B_NO_ERROR)
	{
		err = read_port(rdPort, &id, &reply, sizeof(reply));
		ASSERT(id == kAreaReply);
	}

	if (message.textArea >= B_NO_ERROR)
		delete_area(message.textArea);

	return err;
}


status_t
DoCopyTextToArea(
	port_id 		wrPort,
	port_id			rdPort,
	const void *	inData,
	int32			inDataLength)
{
	BrowseDataArea*	areaAddress = (BrowseDataArea*) B_PAGE_SIZE;
	int32			id = kNullQuery;
	status_t		err = B_NO_ERROR;
	area_id			clone = B_ERROR;
	GetAreaReply	reply;
		
	reply.area = -1;

	// Get the area_id from the IDE
	err = write_port(wrPort, kGetArea, &reply, sizeof(reply));

	if (err == B_NO_ERROR)
	{
		err = read_port(rdPort, &id, &reply, sizeof(reply));
		if (err > 0)
			err = B_NO_ERROR;
		if (reply.area < B_NO_ERROR)
			err = B_ERROR;
	}

	// Clone the area
	if (err == B_NO_ERROR)
	{
		clone = clone_area("mwccarea", &areaAddress, B_ANY_ADDRESS, B_WRITE_AREA, reply.area);

		if (clone < B_NO_ERROR)
			err = clone;
	}

	// Is the area big enough?
	if (err == B_NO_ERROR)
	{
		area_info	info;
		
		err = get_area_info(clone, &info);
	
		if (err == B_NO_ERROR && info.size < inDataLength + sizeof(long))
		{
			long	len = (inDataLength + sizeof(long) + B_PAGE_SIZE - 1) / B_PAGE_SIZE * B_PAGE_SIZE;
			err = resize_area(clone, len);
		}
	}

	// Copy the data to the area
	if (err == B_NO_ERROR)
	{
		areaAddress->length = inDataLength;
		memmove(&areaAddress->data[0], inData, inDataLength);
	}

	// Remove the area from our address space
	if (clone >= B_NO_ERROR)
		delete_area(clone);

	return err;
}