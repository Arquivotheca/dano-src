// =============================================================================
//    ¥ BTSAddress.h
// =============================================================================
#ifndef _B_BTSADDRESS_
#define _B_BTSADDRESS_

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// =============================================================================

class BTSAddress
{
	public:
	
									BTSAddress();
	
									BTSAddress(const unsigned short port, 
				 						const unsigned long address = INADDR_ANY,
				 						const int family = AF_INET);
									BTSAddress(const unsigned short port, const char* hostName, 
												const int family );
				 						
						status_t	SetTo(const unsigned short port, 
				 						const unsigned long address = INADDR_ANY,
				 						const int family = AF_INET);
				 						
				 		status_t	SetTo(const unsigned short port,
										const char* hostName,
										const int family = AF_INET);
										
		virtual const sockaddr_in*	SockAddr() const;
		//struct	hostent* 			HostEntry() const;
	private:
	
		struct sockaddr_in 			fAddr;
		//const char*					fHostName;
};

#endif
