#define DEBUG 0
// =============================================================================
//    ¥ BTSAddress.cpp
// =============================================================================

#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>
#include <Debug.h>
#include "BTSAddress.h"


BTSAddress::BTSAddress()
{
}

BTSAddress::BTSAddress(const unsigned short port, 
					const unsigned long address, 
					const int family)
{
	SetTo(port, address, family);
}

BTSAddress::BTSAddress(const unsigned short port, const char* hostName, 
						const int family )
{
	SetTo(port, hostName, family);
}
					
// =============================================================================
//    ¥ BTSAddress
// =============================================================================
status_t
BTSAddress::SetTo(const unsigned short port, 
					const unsigned long address, 
					const int family)
{
	// Define the connection information.
	memset(&fAddr,0,sizeof(fAddr));
	fAddr.sin_family = family;
	fAddr.sin_port = htons(port);	//htons gives portability.
	fAddr.sin_addr.s_addr = address;
	//memset(fAddr.sin_zero, 0, sizeof(fAddr.sin_zero));
	//fHostName = NULL;
	return B_NO_ERROR;
}

// =============================================================================
//    ¥ BTSAddress
// =============================================================================
status_t
BTSAddress::SetTo(const unsigned short port, const char* hostName, 
						const int family )
{
	PRINT(("BTSAddress::SetTo - ENTER\n"));
	// Define the connection information.
	PRINT(("BTSAddress::SetTo - getting host entry for %s\n", hostName));
	
	memset(&fAddr, 0, sizeof(fAddr));
	
	struct hostent* host = NULL;
	status_t result = B_NO_ERROR;
	
	int dotcount = 0;
	bool numeric = true;
	for (const char *c = hostName; *c; c++) {
		if (*c == '.')
			dotcount++;
		else if (!isdigit(*c))
			numeric = false;
	}
	numeric = numeric && (dotcount == 3);
			
	if (numeric) {
		ulong addr = inet_addr(hostName);
		PRINT(("BTSAddress::BTSAddres - setting up sockaddr\n"));
		fAddr.sin_addr.s_addr = addr;
		fAddr.sin_family = family;
		fAddr.sin_port = htons(port);	//htons gives portability.
			
		PRINT(("Net Address = %s\n", inet_ntoa(* (in_addr *)(&fAddr.sin_addr.s_addr))));
		//PRINT("addr is %d\n",addr);
		//host = gethostbyaddr((const char *)&addr,sizeof(struct in_addr),family);
		//perror("gethostbyaddr");
	}
	else {
		host = gethostbyname(hostName);
		if (host != NULL)
		{
			PRINT(("BTSAddress::BTSAddres - setting up sockaddr\n"));
			fAddr.sin_addr.s_addr = *(unsigned long*)(host->h_addr);
			fAddr.sin_family = family;
			fAddr.sin_port = htons(port);	//htons gives portability.
			
			PRINT(("Net Address = %s\n", inet_ntoa( *(in_addr *)(host->h_addr))));
		}
		else {
			// tried to use h_errno but it integrates poorly with
			// standard error codes
			//if (h_errno < 0)
			//	result = h_errno;
			//else
			result = EHOSTUNREACH;
		}
	}
	PRINT(("BTSAddress::BTSAddress - EXIT\n"));
	
	return result;
}

// =============================================================================
//    ¥ SockAddr
// =============================================================================
const sockaddr_in*	
BTSAddress::SockAddr() const
{
	return &fAddr;
}

// =============================================================================
//    ¥ HostEntry
// =============================================================================
//hostent*	
//BTSAddress::HostEntry() const
//{
//	struct hostent* entry = NULL;
//	if (fHostName != NULL)
//	{
//		entry = ::gethostbyname(fHostName);	// Convert host name to ip addr
//
//	}
//	else if (fAddr.sin_addr.s_addr != 0)
//	{
//		//entry = gethostbyaddr(fAddr.sin_addr.s_addr);
//	}
//	return entry;
//}
