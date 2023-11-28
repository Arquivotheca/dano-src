/*
	MSNTransport.cpp
*/
#include "MSNTransport.h"
#include <stdio.h>

MSNTransport::MSNTransport()
	:	IMTransport()
{
	printf("MSN: Created new MSNTransport\n");
}


MSNTransport::~MSNTransport()
{

}

status_t MSNTransport::Login(const char *username)
{
	status_t result = B_OK;
	
	result = OpenSocketConnection("64.4.13.57", 1863);
	if (result != B_OK)
		return result;
	
	return result;
}

status_t MSNTransport::Logout()
{
	return B_OK;
}

extern "C" _EXPORT IMTransport* make_nth_transport(int32 n, image_id , uint32 , ...)
{
	if (n == 0)
		return new MSNTransport();
	return 0;
}

// End of MSNTransport.cpp


