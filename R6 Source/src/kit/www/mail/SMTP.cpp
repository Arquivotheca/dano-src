/*
	SMTP.cpp
*/
#include <Binder.h>
#include <ctype.h>
#include <DNSCache.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <StringBuffer.h>
#include "MailDebug.h"
#include "SendMessageContainer.h"
#include "SMTP.h"
#include "www/util.h"

using namespace Wagner;

static const char *kHostName = "earthlink.net";

SmtpDaemon::SmtpDaemon()
	:	fSocket(-1),
		fReadBufferOffset(0),
		fReadBufferLength(0),
		fWriteBufferLength(0)
{

}

SmtpDaemon::~SmtpDaemon()
{
	if (fSocket >= 0)
		Disconnect();
}

status_t SmtpDaemon::Connect(SendMessageContainer *container)
{
	MDB(MailDebug md);
	status_t result = B_OK;
	// First try making a connection to the smtp server...
	fSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fSocket < 0) {
		DB(md.Print("Error creating socket\n"));
		return errno;
	}
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

	DNSCache::AddrList addrs;
	
	result = dnsCache.GetHostByName(container->GetSmtpServer(), addrs);
	if (result != B_OK) {
		DB(md.Print("Error getting host name '%s' from DNS cache.\n", container->GetSmtpServer()));
		return result;
	}
	
	addr.sin_len = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(container->GetSmtpPort());
	int32 idx = 0;
	for (idx = 0; idx < addrs.CountItems(); idx++) {
		addr.sin_addr.s_addr = addrs[idx];
		if (connect(fSocket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == 0) {
			// Successfull connection
			break;
		}
	}
	// Failed to connect
	if (idx == addrs.CountItems()) {
		DB(md.Print("Error connecting to socket.\n"));
		return errno;
	}
	
	int32 code = 0;
	ParseResponse(&code);
	if (code != 220) {
		DB(md.Print("Error, server returned code '%d' instead of 220 on connect.\n", code));
		return B_ERROR;
	}
	return result;
}

status_t SmtpDaemon::Login(SendMessageContainer *container)
{
	MDB(MailDebug md);
	status_t result = B_OK;
	// Say hello to the SMTP server [ ** Insert Proper SMTP Authentication Here ** ]
	if ((result = Helo(kHostName)) != B_OK)
		return result;
	return result;
}

status_t SmtpDaemon::Helo(const char *domain)
{
	MDB(MailDebug md);
	StringBuffer request;
	request << "HELO " << domain << "\r\n";
	int32 resultCode;
	status_t result;
	if ((result = SendCommand(&request, &resultCode)) != B_OK) {
		DB(md.Print("Error sending command 'HELO'\n"));
		return result;
	}
	if (resultCode != 250) {
		DB(md.Print("Result code error from HELO, code '%ld' returned instead of 250.\n", resultCode));
		return B_ERROR;
	}
	return B_OK;
}

status_t SmtpDaemon::PreFlightSmtp(SendMessageContainer *container)
{
	MDB(MailDebug md);
	status_t result = B_OK;
	StringBuffer command;
	command << "MAIL FROM:<" << container->GetFromAddress() << ">\r\n";
	int32 resultCode;
	if ((result = SendCommand(&command, &resultCode)) != B_OK) {
		DB(md.Print("Error sending MAIL FROM command='%s'\n", command.String()));
		return result;
	}
	if (resultCode != 250) {
		DB(md.Print("Result code from MAIL FROM was '%ld' not 250.\n", resultCode));
		return B_ERROR;
	}
	int32 count = container->CountRecipients();
	for (int32 i = 0; i < count; i++) {
		const recipient_container *recipient = container->RecipientAt(i);
		command.Clear();
		command << "RCPT TO:<" << recipient->fAddress.String() << ">\r\n";
		if ((result = SendCommand(&command, &resultCode)) != B_OK) {
			DB(md.Print("Error sending RCPT command='%s'\n", command.String()));
			break;
		}
		if ((resultCode != 250) && (resultCode != 251)) {
			DB(md.Print("Result code from RCPT was '%ld' not 250 or 251.\n", resultCode));
			break;
		}
	}
	return ((resultCode == 250) || (resultCode == 251) ? B_OK : B_ERROR);
}

status_t SmtpDaemon::StartData()
{
	MDB(MailDebug md);
	// StartData() and EndData() should probably be combined...
	StringBuffer command; command << "DATA\r\n";
	int32 resultCode;
	status_t result;
	if ((result = SendCommand(&command, &resultCode)) != B_OK) {
		DB(md.Print("Error sending command starting 'DATA'\n"));
		return result;
	}
	if (resultCode != 354) {
		DB(md.Print("Result code for DATA was '%ld' not 354.\n", resultCode));
		return B_ERROR;
	}
	return B_OK;
}

status_t SmtpDaemon::EndData()
{
	MDB(MailDebug md);
	// StartData() and EndData() should probably be combined...
	StringBuffer command; command << ".\r\n";
	int32 resultCode;
	status_t result;
	if ((result = SendCommand(&command, &resultCode)) != B_OK) {
		DB(md.Print("Error sending command ending 'DATA'\n"));
		return result;
	}
	if (resultCode != 250) {
		DB(md.Print("Result code for DATA was '%ld' not 250.\n", resultCode));
		return B_ERROR;
	}
	return B_OK;
}

status_t SmtpDaemon::SendCommand(StringBuffer *command, int32 *resultCode)
{
	MDB(MailDebug md);
	DB(md.Print("smtp:out-> '%s'\n", command->String()));
	if (SocketWrite((void *)command->String(), command->Length()) < command->Length()) {
		DB(md.Print("Error. SocketWrite did not write as much as we asked for.\n"));
		return B_ERROR;
	}
	status_t result;
	if ((result = ParseResponse(resultCode)) != B_OK) {
		DB(md.Print("Error parsing response.\n"));
		return result;
	}
	return B_OK;
}

status_t SmtpDaemon::Disconnect()
{
	MDB(MailDebug md);
	status_t result = B_OK;
	// Quit connection to smtp server
	if (fSocket >= 0) {
		StringBuffer command; command << "QUIT\r\n";
		int32 code = 0;
		if ((result = SendCommand(&command, &code)) != B_OK) {
			DB(md.Print("Error sending 'quit' command.\n"));
			return result;
		}
		if (code != 221) {
			DB(md.Print("Wrong code returned from quit. We got '%d' but expected 221.\n", code));
			return B_ERROR;
		}
		close(fSocket);
		fSocket = -1;
	}
	return B_OK;
}

ssize_t SmtpDaemon::Write(const void *buffer, size_t size)
{
	ssize_t totalWritten = 0;
	const char *out = (const char*) buffer;
	enum {
		kScanText,
		kScanNewline,
		kScanDot
	} state = kScanText;
	fWriteBufferLength = 0;
	for (int i = 0; i < size; i++) {
		if (out[i] == '\r')
			continue;
		else if (out[i] == '\n') {
			state = kScanNewline;
			fWriteBuffer[fWriteBufferLength++] = '\r';
		} else if (out[i] == '.' && state == kScanNewline) {
			fWriteBuffer[fWriteBufferLength++] = '.';
			state = kScanText;
		} else
			state = kScanText;
			
		fWriteBuffer[fWriteBufferLength++] = out[i];
		if (fWriteBufferLength >= kSMTPBufferSize - 2) {
			ssize_t written = SocketWrite(fWriteBuffer, fWriteBufferLength);
			if (written > 0)
				totalWritten += written;
			else {
				return B_ERROR;
			}
	
			fWriteBufferLength = 0;
		}
	}

	if (fWriteBufferLength) {
		ssize_t written = SocketWrite(fWriteBuffer, fWriteBufferLength);
		if (written > 0) {
			totalWritten += written;
		} else {
			return B_ERROR;
		}
	}

	return totalWritten;
}

ssize_t SmtpDaemon::SocketWrite(void *buffer, int size)
{
	if (fSocket < 0)
		return ENOTCONN;

	size_t sent = 0;
	while (sent < size) {
		ssize_t result = -1;
		result = write(fSocket, (char*) buffer + sent, size - sent);
		if (result <= 0) {
			if (errno == EINTR)
				continue;
			break;
		}
		
		sent += result;
	}

	return sent;
}

inline int SmtpDaemon::GetNextChar()
{
	if (fReadBufferOffset == fReadBufferLength) {
		ssize_t retval;
		do {
			retval = read(fSocket, fReadBuffer, kSMTPBufferSize);
		} while (retval == EINTR);
		
		if (retval <= 0)
			return -1;

		fReadBufferLength = retval;
		fReadBufferOffset = 0;
	}

	return fReadBuffer[fReadBufferOffset++];
}

inline void SmtpDaemon::EatRestOfLine()
{
	int c = 0;
	while ((c = GetNextChar()) != '\n') {
		// Skip to end of line
	}
}
	
status_t SmtpDaemon::ParseResponse(int32 *outResultCode)
{
	int8 digitsGot = 0;
	int32 resultCode = 0;

	// We're looking for 3 digits and then a space.
	// If this is a multi-line response, then it will have
	// a hyphen '-' after the three digits on every line,
	// except the last, where the hyphen will be a space.
	// We don't care about any text after the return code
	for (;;) {
		int c = GetNextChar();
		if (c < 0)
			break;
		
		if ((digitsGot == 3) && (isspace(c))) {
			EatRestOfLine();
			break;
		}
			
		if (isdigit(c)) {
			resultCode = resultCode * 10 + c - '0';
			digitsGot++;
		} else {
			resultCode = 0;
			digitsGot = 0;
			EatRestOfLine();
			continue;
		}

	}
	*outResultCode = resultCode;	
	return B_OK;
}

ssize_t SmtpDaemon::Read(void *, size_t )
{
	// Sorry, you ain't reading shiii_awt from this daemon...
	return B_ERROR;
}

