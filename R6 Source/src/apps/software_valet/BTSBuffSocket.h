// BTSClientSocket.h
/* A client socket allows for creation of a socket connection
   to a server. The necessary parameters needed to create the
   connection are passed to the constructor. The constructor 
   will block until either a connection has been made or the
   attempt has failed. The InitCheck() function should be used
   after construction to ensure the connection is valid.
   
*/


#ifndef _BTSBUFFSOCKET_H_
#define _BTSBUFFSOCKET_H_

#include "BTSSocket.h"

//class _EXPORT BTSBuffSocket: public BTSSocket
class BTSBuffSocket: public BTSSocket
{
	public:
						BTSBuffSocket();
						BTSBuffSocket(const int type, 
								const int protocol,
								const int family = AF_INET );
						BTSBuffSocket(const int socketID);
						
		virtual long	ConnectToAddress(const BTSAddress& address);
			
			status_t	WriteString(const char *);
			status_t	WriteLine(const char *);			
			status_t	ReadLine(char *buf, int bufsize);
			
			// support buffered or unbuffered reads
			status_t	Read(char *buf, int bufsize, bool buffer = false);
	private:
		enum {
			kBufferSize = 16384
		};

		inline status_t GetChar(char *c) {	
			if (fBufP >= fBufEnd) {
				status_t res = Recv(fBuffer, kBufferSize);
			
				if (res < 0) {
					return res;
				}
				else if (res == 0)
					return ECONNABORTED;
				
				fBufP = fBuffer;
				fBufEnd = fBuffer + res;
			
			}
			*c = *fBufP++;
			return B_NO_ERROR;
		}

		inline void UngetChar() {
			if (fBufP <= fBuffer) {
				// if we point at the beginning of the buffer, wrap around
				fBufP = fBufEnd - 1;
			} else {
				fBufP--;
			}
		}
										
				char	fBuffer[kBufferSize];
				char	*fBufP;
				char	*fBufEnd;
};

//inline void		BTSBuffSocket::UngetChar()
//{
//	if (fBufP <= fBuffer) {
//		// if we point at the beginning of the buffer, wrap around
//		fBufP = fBufEnd - 1;
//	}
//	else
//		fBufP--;
//}

// returns either error code or no-error
// if recv returns zero, connaborted is returned
//inline status_t	BTSBuffSocket::GetChar(char *c)
//{	
//	if (fBufP >= fBufEnd) {
//		status_t	res = Recv(fBuffer, kBufferSize);
//		
//		if (res < 0) {
//			return res;
//		}
//		else if (res == 0)
//			return ECONNABORTED;
//			
//		fBufP = fBuffer;
//		fBufEnd = fBuffer + res;
//		
//	}
//	*c = *fBufP++;
//	return B_NO_ERROR;
//}

#endif
