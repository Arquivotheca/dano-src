#define DEBUG 0
// =============================================================================
//    ¥ BTSSocket.cpp
// =============================================================================
#include "BTSSocket.h"
#include <stdio.h>
#include <support/Debug.h>
#include <errno.h>
#include <string.h>


// =============================================================================
//    ¥ BTSSocket
// =============================================================================

BTSSocket::BTSSocket(const int type, const int protocol, const int family) :
			fFamily(family)
{
	if (protocol == 0) fProtocol = IPPROTO_TCP;
	else fProtocol = protocol;

	if (type == 0) fType  = SOCK_STREAM; 
	else fType = type;
		
	Init();
	Open();
	//result = SetOption(SOL_SOCKET, SO_NONBLOCK, (char*)&ones, sizeof(int));
}

// =============================================================================
//    ¥ BTSSocket
// =============================================================================
BTSSocket::BTSSocket(const int socketID) :
			fFamily(AF_INET),
			fType(-1),
			fProtocol(-1)
{
	Init();
	fID = socketID;
}

BTSSocket::~BTSSocket()
{
	Close();
}

// =============================================================================
//    ¥ Init
// =============================================================================
void
BTSSocket::Init()
{
	fID = -1;
	fConnected = FALSE;
	fBound = FALSE;
	fListening = FALSE;
	fInterrupted = FALSE;
	fTimeout = 5*60;
}

// =============================================================================
//    ¥ SetTimeout
// =============================================================================
//void
//BTSSocket::SetTimeout(int seconds)
//{
	//fTimeout = seconds;
//}

// =============================================================================
//    ¥ SetOption
// =============================================================================
long
BTSSocket::SetOption(const int level, const int option, char* data, 
					const unsigned int size) const
{
	long result = setsockopt(fID, level, option, data, size);	
	PRINT(("Result of socket option setting is %d\n", result));
	return result;
}

// =============================================================================
//    ¥ Open
// =============================================================================
long
BTSSocket::Open()
{
	if (fID > -1)
	{
		close(fID);
	}
	fBound = FALSE;
	fListening = FALSE;
	fInterrupted = FALSE;
	
	fID = socket(fFamily,  fType, fProtocol);
	return 0;
}

// =============================================================================
//    ¥ SendLock
// =============================================================================
//long
//BTSSocket::SendLock() const 
//{
//	return ::acquire_sem(fSendSem);
//}

// =============================================================================
//    ¥ SendUnlock
// =============================================================================
//long
//BTSSocket::SendUnlock() const 
//{
//	return ::release_sem(fSendSem);
//}

// =============================================================================
//    ¥ RecvLock
// =============================================================================
//long
//BTSSocket::RecvLock() const 
//{
//	return ::acquire_sem(fRecvSem);
//}

// =============================================================================
//    ¥ RecvUnlock
// =============================================================================
//long
//BTSSocket::RecvUnlock() const 
//{
//	return ::release_sem(fRecvSem);
//}

// =============================================================================
//    ¥ Interrupt
// =============================================================================
void
BTSSocket::Interrupt()
{
	if (fID >= 0)
	{
		// set the flag first
 		fInterrupted = true;
		struct sockaddr 	addr;
		int 				size;
 		int 				result = getsockname(fID, &addr, &size);
	}
	return;
}

// =============================================================================
//    ¥ ConnectToAddress
// =============================================================================
long
BTSSocket::ConnectToAddress(const BTSAddress& address)
{
	long 				result;
	const sockaddr_in* 	sockAddr = address.SockAddr();
	
	PRINT(("BTSSocket::ConnectToAddress - ENTER, id= %d port= %d, family= %d rem addr = %ud\n", 
	 	fID, sockAddr->sin_port, sockAddr->sin_family, 
	 	sockAddr->sin_addr.s_addr));
	/* perform select here? */
	errno = 0;
	result = connect(fID, (struct sockaddr*)sockAddr, 
						sizeof(*sockAddr));
	PRINT(("BTSSocket::ConnectToAddress - EXIT, result %d, errno %d\n",result,errno));
	if (result < 0) result = errno;
	fConnected =  (result >= B_NO_ERROR);
	if (!fConnected && fInterrupted) {
		result = B_CANCELED;
	}
		
	return result;
}

// =============================================================================
//    ¥ BindTo
// =============================================================================
long	
BTSSocket::BindTo(const BTSAddress& address)
{
	long	result;
	const 	sockaddr_in* sockAddr  = address.SockAddr();
	PRINT(("BTSSocket::BindTo - ENTER, id= %d port= %d, family= %d rem addr = %ud\n", 
	 	fID, sockAddr->sin_port, sockAddr->sin_family, 
	 	sockAddr->sin_addr.s_addr));
	
	errno = 0;
	result = bind(fID, (struct sockaddr*)sockAddr, 
						sizeof(*sockAddr));
	PRINT(("BTSSocket::BindTo - EXIT, result is %s\n", strerror(result)));
	if (result < 0) result = errno;
	fBound = (result >= B_NO_ERROR);
	if (!fBound && fInterrupted)
		result = B_CANCELED;
	return result;
}

// =============================================================================
//    ¥ Accept
// =============================================================================

/// should change this to returns an error instead of a BTSSocket
BTSSocket*	
BTSSocket::Accept(struct sockaddr *addr, int *size) const
{
	BTSSocket* 	newClient = NULL;
	int			clientSock = -1;
	if (fID >= 0 && fBound && fListening)
	{
		clientSock = accept(fID, addr,size);
		if (clientSock >= 0)
		{
			newClient = new BTSSocket(clientSock);
			newClient->fType = fType;
			newClient->fProtocol = fProtocol;
			newClient->fMaxConnections = fMaxConnections;
			newClient->fConnected = true;
		}
	}
	return newClient;
}

// =============================================================================
//    ¥ Listen
// =============================================================================
long	
BTSSocket::Listen(const int maxConnections)
{
	PRINT(("BTSSocket::Listen - ENTER\n"));
	fMaxConnections = maxConnections;
	errno = 0;
	long result = listen(fID, maxConnections);
	PRINT(("BTSSocket::Listen - EXIT, resut is %s\n", strerror (result)));
	if (result < 0) result = errno;
	fListening = (result >= B_NO_ERROR);
	return result;
}

// =============================================================================
//    ¥ Close
// =============================================================================
long
BTSSocket::Close()
{
	PRINT(("BTSSocket::Close - ENTER, socket %d\n", fID));
	Interrupt();
	//SendLock();
	//RecvLock();
	if (fID > -1)
	{
 		close(fID);
 	}
	Init();
 	
 	//SendUnlock();
 	//RecvUnlock();
	PRINT(("BTSSocket::Close - EXIT, socket %d\n", fID));
 	return 0;
}

// =============================================================================
//    ¥ Send
// =============================================================================
long
BTSSocket::Send(const char* buf, const size_t bufSize) const
{
	if (fInterrupted)
		return B_CANCELED;
	
	int 	numBytes = -1;
	int		sentBytes = 0;
	PRINT(( "SOCKET SEND - ENTER, %ld bytes on socket %d\n", bufSize, fID));
	if (bufSize > 0 && buf != NULL && fID >= 0)
	{
		while (sentBytes < bufSize)
		{
			PRINT(("SOCKET SEND - Sending %ld/%ld bytes on socket %d..\n", 
					bufSize-sentBytes, bufSize, fID));
			errno = 0;
			numBytes = send(fID, buf+sentBytes, bufSize-sentBytes, 0);
			if (numBytes < 0) {
				if (numBytes != EINTR) {
					sentBytes = errno;
					break;
				}
				else if (fInterrupted) {
					sentBytes = B_CANCELED;
					break;
				}
			}
			else
				sentBytes += numBytes;
				
			// if (numBytes == 0) ???
		}
	}	
	
	#if DEBUG
	else
	{
		PRINT(("------------>BTSSocket::Send - Bad Info, socket %d\n", fID));
	}
	//if (sentBytes > 0) UpdateSendCount(sentBytes);
	//PRINT( ("SOCKET SEND - EXIT, sent %ld bytes on %d result is %s\n", sentBytes, 
	//			fID, strerror(result)));
	#endif
	
	return sentBytes;
}

// =============================================================================
//    ¥ Recv
// =============================================================================
long
BTSSocket::Recv(const char* buf, const size_t bufSize) const
{
	// Receives a network data buffer of a certain size. Does not return until
	// the buffer is full or if the socket returns 0 bytes (meaning it was 
	// closed) or returns an error besides EINTR. (EINTR can be generated when a
	// send() occurs on the same socket.
	
	if (fInterrupted)
		return B_CANCELED;
		
	int  receivedBytes = 0;	
	
	PRINT(("BTSSocket::Receive - ENTER, socket %d\n", fID));
	if (buf != NULL && bufSize > 0 && fID >= 0)
	{
		while (receivedBytes < bufSize)
		{
			//PRINT(("Receiving %ld/%ld bytes on %d\n", bufSize - receivedBytes, bufSize, 
			//		fID));
			/* perform select here */
			//if (select fails)
			//	result = B_TIMED_OUT;
			errno = 0;
			int numBytes = 0;
			numBytes = recv(fID, (char*)(buf+receivedBytes), 
								bufSize - receivedBytes, 0);
			
			if (numBytes == 0)
			{
				
				PRINT(("BTSSocket::Receive - got 0 bytes on %d at %ld/%ld bytes\n", 
							fID, receivedBytes, bufSize));
				break;
			}
			else if (numBytes < 0) 
			{
				PRINT(("BTSSocket::Receive - error when receiving data on socket %d - %s\n", 
							fID, strerror(errno)));
				if (errno != EINTR) {
					receivedBytes = errno;
					break;
				}
				else if (fInterrupted) {
					receivedBytes = B_CANCELED;
					break;
				}
			}
			else 
			{
				receivedBytes += numBytes;
				#if DEBUG
					UpdateReceiveCount(numBytes);
				#endif
			}
		}
		//PRINT(("SOCKET %d RECEIVE - Received %ld bytes result is %s\n", fID, numBytes,
		//			strerror(result)));
	}
	else
		receivedBytes = B_ERROR;
	PRINT(("BTSSocket::Receive - EXIT, socket %d\n", fID));
	return receivedBytes;	
}

// =============================================================================
//    ¥ UpdateSendCount
// =============================================================================
void BTSSocket::UpdateSendCount(const long numBytes)
{
	static long sendCount = 0;
	sendCount += numBytes;
	//PRINT(("Total bytes sent: %ld\n", sendCount));
	return;
}

// =============================================================================
//    ¥ UpdateReceiveCount
// =============================================================================
void BTSSocket::UpdateReceiveCount(const long numBytes)
{
	static long receiveCount = 0;
	receiveCount += numBytes;
	//PRINT(("Total bytes received: %ld\n", receiveCount));
	return;
}
