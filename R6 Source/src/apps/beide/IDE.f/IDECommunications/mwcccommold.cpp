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
	kGetArea
} QueryType;

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
	char		data[1];
} BrowseDataArea;

long	DoSendText(
			port_id 		wrPort,
			port_id			rdPort,
			const char *	inText,
			long			inTextLength,
			QueryType		inMessageType);


long
DoCopyTextToArea(
	port_id 		wrPort,
	port_id			rdPort,
	const void *	inData,
	long			inTextLength);

// into bcccomm.cpp

#include <OS.h>
#include <string.h>

// This function is used to communicate arbitrary sized text to the IDE
// This will be in response to requests for preprocess info and eventually
// for disassembly info.  It works by allocating an area and copying the
// text into that area.  The area_id is passed to the IDE, which copies
// the text.  The IDE then sends back a kProcessReply message.  When this
// is received the compiler deletes the area.  A status notification should
// be sent after this reporting any errors.
// inMessageType should be kPreprocessResult for the response to a preprocess
// command line request.

long
DoSendText(
	port_id 		wrPort,
	port_id			rdPort,
	const char *	inText,
	long			inTextLength,
	QueryType		inMessageType)
{
	void*			areaAddr = (void*) B_PAGE_SIZE;
	long			areaSize = ((inTextLength / B_PAGE_SIZE) * B_PAGE_SIZE) + B_PAGE_SIZE;
	long			id = kNullQuery;
	long			err = B_NO_ERROR;
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
		ASSERT(id == kPreprocessReply);
	}

	if (message.textArea >= B_NO_ERROR)
		delete_area(message.textArea);

	return err;
}

long
DoCopyTextToArea(
	port_id 		wrPort,
	port_id			rdPort,
	const void *	inData,
	long			inDataLength)
{
	BrowseDataArea*	areaAddress = (BrowseDataArea*) B_PAGE_SIZE;
	long			id = kNullQuery;
	long			err = B_NO_ERROR;
	area_id			clone = B_ERROR;
	GetAreaReply	reply;
		
	reply.area = -1;

	// Get the area_id from the compiler
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
			long	len = (inDataLength + B_PAGE_SIZE - 1) / B_PAGE_SIZE * B_PAGE_SIZE;
			err = resize_area(clone, len);
		}
	}

	// Copy the data to the area
	if (err == B_NO_ERROR)
	{
		memmove(&areaAddress->data, inData, inDataLength);
		areaAddress->length = inDataLength;
	}

	// Remove the area from our address space
	if (clone >= B_NO_ERROR)
		delete_area(clone);

	return err;
}