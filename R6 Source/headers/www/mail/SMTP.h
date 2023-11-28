/*
	SMTP.h
*/

#ifndef _SMTP_H_
#define _SMTP_H_
#include "StringBuffer.h"

namespace Wagner {

const int kSMTPBufferSize = 4096;

// Forward Declarations
class SendMessageContainer;

class SmtpDaemon : public BDataIO {
	public:
								SmtpDaemon();
		virtual					~SmtpDaemon();

		status_t 				Connect(SendMessageContainer *container);
		status_t				Login(SendMessageContainer *container);
		status_t				SendCommand(StringBuffer *command, int32 *resultCode);
		status_t				Helo(const char *domain);
		status_t				PreFlightSmtp(SendMessageContainer *container);
		status_t				StartData();
		status_t				EndData();
		status_t				Disconnect();

		virtual	ssize_t			Read(void*, size_t);
		virtual	ssize_t			Write(const void*, size_t);

	private:
		ssize_t 				SocketWrite(void*, int);
		inline void				EatRestOfLine();
		inline int 				GetNextChar();
		status_t 				ParseResponse(int32 *outResultCode);
	
		
		int fSocket;
		char fReadBuffer[kSMTPBufferSize];
		int fReadBufferOffset;
		int fReadBufferLength;
		char fWriteBuffer[kSMTPBufferSize];
		int fWriteBufferLength;

};
}
#endif
