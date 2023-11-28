/*
	IMTransport.cpp
*/
#include <DNSCache.h>
#include <errno.h>
#include "IMTransport.h"
#include <netinet/in.h>
#include <sys/socket.h>

IMTransport::IMTransport()
	:	BinderNode(),
		fSocket(-1)
{

}


IMTransport::~IMTransport()
{

}

status_t IMTransport::OpenSocketConnection(const char *server, int port)
{
	fSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fSocket < 0)
		return errno;

	// Set a 60 second timeout
	struct timeval tv ={60, 0};
	//set the receive timeout
	if (setsockopt(fSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) {
		//that didn't work, should we do something here?
	}
	// set the send timeout (usually not necessary, since send() returns before the data is
	// actually sent)
	setsockopt(fSocket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	status_t result = dnsCache.GetHostByName(server, &addr.sin_addr.s_addr);
	if (result < 0)
		return result;

	addr.sin_len = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (connect(fSocket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
		return errno;

	printf("MSNTransport::OpenSocketConnection connected\n");
	return B_OK;
}

// End of IMTransport.cpp
