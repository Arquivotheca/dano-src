
#include <www/mail/MailServer.h>

#include <www/mail/MailDebug.h>
#include <www/DNSCache.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/net_control.h>

#include <www/mail/IMAP.h>
#include <www/mail/POP3.h>


const char *	IMAP_SERVER = "IMAP";
const char *	POP3_SERVER = "POP3";

MailServer *
MailServer::MakeServer(const char *serverType)
{
	MDB(MailDebug md);
	if (strcasecmp(serverType, IMAP_SERVER) == 0)
		return new IMAP;
	else if (strcasecmp(serverType, POP3_SERVER) == 0)
		return new POP3;
	else
		return NULL;
}


MailServer::MailServer() :
	fSocket(-1),
	fAmountBuffered(0),
	fBufferReadPos(0),
	fTotalMessageCount(0),
	fRecentMessageCount(0),
	fUnseenMessageCount(0),
	fCapability(kNoCapability),
	fMailboxSize(0),
	fMailboxQuota(0),
	fLoggedIn(false)
{
}


MailServer::~MailServer()
{
}

status_t 
MailServer::Connect(const char *server, int port)
{
	MDB(MailDebug md);
	DB(md.Print("connect to %s:%ld\n", server, port));
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

	return B_OK;
}

void 
MailServer::Disconnect()
{
	if (fLoggedIn) 
		Logout();
	if (fSocket >= 0) {
		close(fSocket);
		fSocket = -1;
	}
}

status_t 
MailServer::Capability()
{
	return B_ERROR;
}

int32 
MailServer::GetTotalMessageCount() const
{
	return fTotalMessageCount;
}

int32 
MailServer::GetRecentMessageCount() const
{
	return fRecentMessageCount;
}

int32 
MailServer::GetUnseenMessageCount() const
{
	return fUnseenMessageCount;
}

uint32 
MailServer::GetCapability() const
{
	return fCapability;
}

int32 
MailServer::GetMailboxSize() const
{
	return fMailboxSize;
}

int32 
MailServer::GetMailboxQuota() const
{
	return fMailboxQuota;
}

bool 
MailServer::IsLoggedIn() const
{
	return fLoggedIn;
}

status_t 
MailServer::CreateMailbox(const char *)
{
	MDB(MailDebug md);
	return B_NOT_ALLOWED;
}

status_t 
MailServer::SelectMailbox(const char *mailbox, bool writable)
{
	MDB(MailDebug md);
	if (strcmp(mailbox, "inbox") == 0 && !writable)
		return B_OK;
	else
		return B_ERROR;
}

status_t 
MailServer::QuotaRoot(const char *)
{
	MDB(MailDebug md);
	return B_NOT_ALLOWED;
}

status_t 
MailServer::Append(const char *, size_t)
{
	MDB(MailDebug md);
	return B_NOT_ALLOWED;
}

status_t 
MailServer::SetFlags(const char *, uint32)
{
	MDB(MailDebug md);
	return B_NOT_ALLOWED;
}

status_t 
MailServer::Expunge()
{
	MDB(MailDebug md);
	return B_NOT_ALLOWED;
}

status_t 
MailServer::Close()
{
	MDB(MailDebug md);
	return B_NOT_ALLOWED;
}

status_t 
MailServer::GetStructure(const char *, MimeMessage *)
{
	MDB(MailDebug md);
	return B_NOT_ALLOWED;
}

status_t 
MailServer::GetEnvelope(const char *, MimeMessage *)
{
	MDB(MailDebug md);
	return B_NOT_ALLOWED;
}

status_t 
MailServer::GetRecentMessagesList(BList *)
{
	MDB(MailDebug md);
	return B_NOT_ALLOWED;
}

const char *
MailServer::GetFolderUid() const
{
	MDB(MailDebug md);
	return "";
}

const char *
MailServer::GetNextUid() const
{
	MDB(MailDebug md);
	return "";
}

const char *
MailServer::GetCurrentFolder() const
{
	MDB(MailDebug md);
	return "inbox";
}



ssize_t 
MailServer::ReadSocketBuffered(void *buffer, size_t count)
{
	if (fSocket < 0)
		return ENOTCONN;

	if (count == 0)
		return 0;
		
	//	If there is buffered data, copy that first
	if (fBufferReadPos < fAmountBuffered) {
		size_t sizeToCopy = MIN(count, fAmountBuffered - fBufferReadPos);
		memcpy(buffer, fBuffer + fBufferReadPos, sizeToCopy);
		fBufferReadPos += sizeToCopy;
		return sizeToCopy;
	}

	if (count < static_cast<size_t>(kLargeReadThreshold)) {
		//	This is a small read.  It would be inefficient to do a lot
		// 	of these, so read into a larger buffer and copy smaller chunks
		// 	out of that.
		ssize_t sizeReceived = ReadSocketUnbuffered(fBuffer, kLargeReadSize);
		if (sizeReceived <= 0)
			return sizeReceived;
		
		fAmountBuffered = sizeReceived;
		size_t sizeToCopy = MIN(count, fAmountBuffered);
		memcpy(buffer, fBuffer, sizeToCopy);
		fBufferReadPos = sizeToCopy;
		return sizeToCopy;
	}

	//	This is a fairly large read, so it just wastes time to
	//	copy into a temporary buffer.  Read from the socket directly into
	//	the user buffer
	return ReadSocketUnbuffered(buffer, count);
}

ssize_t 
MailServer::ReadSocketUnbuffered(void *buffer, size_t size)
{
	ssize_t retval = read(fSocket, buffer, size);
	if (retval < 0)
		retval = errno;
	else if (retval == 0)
		retval = -1;	// a zero byte read signifies the end of the stream

	return retval;
}

ssize_t 
MailServer::WriteSocket(const void *buffer, size_t count)
{
	if (fSocket < 0)
		return ENOTCONN;

	size_t sent = 0;
	while (sent < count) {
		ssize_t result = -1;
		result = write(fSocket, (char*) buffer + sent, count - sent);
		if (result <= 0)
			break;
		
		sent += result;
	}
	return sent;
}

status_t 
MailServer::SendCommand(StringBuffer &request)
{
	if (WriteSocket(request.String(), request.Length()) < request.Length())
		return B_ERROR;

	return B_OK;
}


