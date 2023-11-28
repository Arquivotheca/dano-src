// into bcccomm.cpp

#include <OS.h>
#include <string.h>
#include <Debug.h>

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