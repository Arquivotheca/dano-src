// BTSClientSocket.cpp
/* A client socket allows for creation of a socket connection
   to a server. The necessary parameters needed to create the
   connection are passed to the constructor. The constructor 
   will block until either a connection has been made or the
   attempt has failed. The InitCheck() function should be used
   after construction to ensure the connection is valid.
   
*/
#define DEBUG 0

#include <Debug.h>
#include <stdlib.h>
#include <string.h>
#include "BTSBuffSocket.h"

// =============================================================================
//    • BTSBuffSocket
// =============================================================================
BTSBuffSocket::BTSBuffSocket()
	:	BTSSocket(0,0),
		fBufEnd(fBuffer + kBufferSize)
{
	fBufP = fBufEnd;
}

BTSBuffSocket::BTSBuffSocket(const int type, 
				const int protocol,
				const int family )
	:	BTSSocket(type, protocol, family),
		fBufEnd(fBuffer + kBufferSize)
{
	fBufP = fBufEnd;
}

BTSBuffSocket::BTSBuffSocket(const int socketID)
	:	BTSSocket(socketID),
		fBufEnd(fBuffer + kBufferSize)
{
	fBufP = fBufEnd;
}

long
BTSBuffSocket::ConnectToAddress(const BTSAddress& address)
{
	fBufEnd = fBuffer + kBufferSize;
	fBufP = fBufEnd;
	return BTSSocket::ConnectToAddress(address);
}
/**
BTSBuffSocket::BTSBuffSocket(const unsigned short port, 
									const char* serverHostName, 
									const int type , 
									const int protocol, 
									const int family) :
									BTSSocket(type, protocol, family),
									mAddress(port, serverHostName, family) 
{
	ConnectToAddress(mAddress);
	return;
}
**/

// =============================================================================
//    • InitCheck
// =============================================================================
//bool
//BTSClientSocket::InitCheck()
//{
	//return inherited::InitCheck() && IsConnected();
//}

// =============================================================================
//    • FillBuffer
// =============================================================================
//long
//BTSClientSocket::FillBuffer()
//{
//
//}

status_t	BTSBuffSocket::WriteString(const char *buf)
{
	int len = strlen(buf);
	if (len)
		return Send(buf,len);
	else
		return B_NO_ERROR;
}

status_t	BTSBuffSocket::WriteLine(const char *buf)
{
	WriteString(buf);
	return WriteString("\r\n");
}

status_t
BTSBuffSocket::Read(char *buf, int bytes, bool buffer)
{
	status_t res;
	int total = 0;
	if (fBufP < fBufEnd) {
		// stuff still left in buffer
		int amt = fBufEnd - fBufP;
		amt = min_c(amt,bytes);
		
		memcpy(buf,fBufP,amt);
		fBufP += amt;
		total += amt;
	}
	
	if (!buffer) {
		res = 1;
		if (bytes - total && res > 0) {
			res = Recv(buf+total,bytes-total);
			if (res < 0)
				return res;

			//	return ECONNABORTED;
				
			total += res;
		}
		return total;
	}
	
	return B_ERROR;
}

// accept CRLF, CR, LF

// return line size or negative error on error
// zero length line possible
status_t BTSBuffSocket::ReadLine(char *buf, int bufsize)
{
	status_t readErr = B_NO_ERROR;
	char *c = buf;
	char *max = buf + bufsize;

	while (1) {
		if (c >= max) {
			// buffer full
			// caller needs to invoke again with more buffer
			PRINT(("\n\nLINE BUFFER FULL length %d\n\n", c - buf));
			return c - buf;
		}
		readErr = GetChar(c);
		
		if (readErr == ECONNABORTED && c - buf) {
			// connection closed terminate the current line if not zero length
			*c = 0;
			PRINT(("line length is %d\n",c - buf));
			return c - buf;
		}
		if (readErr < B_NO_ERROR) {
			// other type of error	
			return readErr;
		}
		if (*c == '\r') {			
			// read ahead one character
			char e;
			readErr = GetChar(&e);
			
			if (readErr >= B_NO_ERROR  && e == '\n') {
				// we got extra linefeed, crlf
				// end of line
				*c = 0;
				// return line-length
				PRINT(("line length is %d\n",c - buf));
				return c - buf;
			}
			else {
				// no extra line feed next char is for next call
				// end of line
				
				*c = 0;
				
				if (readErr >= B_NO_ERROR) {
					// push back extra character
					UngetChar();
					// return line-length
					PRINT(("got newline, line is %d\n",c - buf));
					return c - buf;
				}
				else if (readErr == ECONNABORTED && c - buf) {
					// connection closed terminate the current line if not zero length
					PRINT(("line length is %d\n",c - buf));
					return c - buf;
				}
				else {
					return readErr;
				}
			}
		}
		else if (*c == '\n') {
			// end of line
			*c = 0;
			PRINT(("got newline, line is %d\n",c - buf));
			return c - buf;
		}
		c++;
	}
}


