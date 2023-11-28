// TCPMessenger

#include "TCPMessenger.h"
#include "auto_delete.h"
#include <app/Message.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#ifdef DEBUG
#define dfprintf fprintf
#else
static inline void dfprintf(FILE*, const char*, ...) { }
#endif

TCPMessenger::TCPMessenger(const char *hostname, uint16 port)
{
	memset(&mTarget, 0, sizeof(mTarget));
	mTarget.sin_len = sizeof(mTarget);
	mTarget.sin_family = AF_INET;
	mTarget.sin_port = htons(port);

	// we require the the hostname lookup return at least one IPv4 address
	struct hostent* he = gethostbyname(hostname);
	if (he && (he->h_addr_list != NULL))
	{
		mIsValid = true;
		memcpy(&mTarget.sin_addr.s_addr, *he->h_addr_list, 4);		// grab the first returned address
	}
	else
	{
		mIsValid = false;
	}
}

TCPMessenger::TCPMessenger(const struct sockaddr_in& targetAddress)
{
	memcpy(&mTarget, &targetAddress, sizeof(mTarget));
	mIsValid = true;
}

TCPMessenger::~TCPMessenger()
{
}

TCPMessenger::TCPMessenger(const TCPMessenger &other)
{
	memcpy(&mTarget, &other.mTarget, sizeof(mTarget));
	mIsValid = other.mIsValid;
}

TCPMessenger &
TCPMessenger::operator=(const TCPMessenger &rhs)
{
	memcpy(&mTarget, &rhs.mTarget, sizeof(mTarget));
	mIsValid = rhs.mIsValid;
	return *this;
}

bool 
TCPMessenger::IsValid() const
{
	return mIsValid;
}

status_t 
TCPMessenger::SendMessage(uint32 command) const
{
	// we could let the other SendMessage() variant take care of mIsValid checking,
	// but then we'd just wind up allocating and deleting a superfluous BMessage,
	// which would be silly
	if (!mIsValid) return B_NO_INIT;

	BMessage msg(command);
	return this->SendMessage(&msg);
}

status_t 
TCPMessenger::SendMessage(const BMessage *msg) const
{
	dfprintf(stderr, "* TCPMessenger::SendMessage(%p what = 0x%08x = '%c%c%c%c')\n", msg, (unsigned)msg->what,
		(int)(msg->what & 0xFF000000) >> 24,
		(int)(msg->what & 0x00FF0000) >> 16,
		(int)(msg->what & 0x0000FF00) >> 8,
		(int)(msg->what & 0x000000FF));

	if (!mIsValid)
	{
		return B_NO_INIT;
	}

	// aha, we have a legitimate destination, at least.  flatten the message and get
	// ready to send it.
#if B_BEOS_VERSION_DANO
	ssize_t flatSize = msg->FlattenedSize(B_MESSAGE_VERSION_1);
#else
	ssize_t flatSize = msg->FlattenedSize();
#endif
	if (flatSize < 0) return flatSize;
	char* flattenedMsg = new char[flatSize];
	// guarantee deletion of the flattened msg by using an auto_array_delete object
	auto_array_delete<char> flatMsgDeletor(flattenedMsg);

#if B_BEOS_VERSION_DANO
	status_t err = msg->Flatten(B_MESSAGE_VERSION_1, flattenedMsg, flatSize);
#else
	status_t err = msg->Flatten(flattenedMsg, flatSize);
#endif
	if (err < 0)
	{
		return err;
	}

	// open the outgoing TCP socket
dfprintf(stderr, "\t. opening outgoing socket\n");
	int sock = socket(AF_INET, SOCK_STREAM, 0);
dfprintf(stderr, "\t. socket() returned %d\n", sock);

	if (sock < 0)
	{
		int n = errno;
dfprintf(stderr, "\t. returning ERROR 0x%08x (%s)\n", n, strerror(n));
		return n;
	}

	// connect to the destination
dfprintf(stderr, "\t. connecting to destination\n");
	err = connect(sock, (const sockaddr*) &mTarget, sizeof(mTarget));
dfprintf(stderr, "\t. connect() returned %d\n", (int)err);
	if (err < 0)
	{
		int n = errno;
dfprintf(stderr, "\t. returning ERROR 0x%08x (%s)\n", n, strerror(n));
		return n;
	}

	// send the payload size == the size of the flattened BMessage, in network byte order, followed
	// by the flattened message itself.
	ssize_t networkFlatSize = htonl(flatSize);
	assert(sizeof(networkFlatSize) == 4);

dfprintf(stderr, "\t. sending size & payload\n");
	err = send(sock, &networkFlatSize, sizeof(networkFlatSize), 0);
	if (err == sizeof(networkFlatSize))
	{
		err = send(sock, flattenedMsg, flatSize, 0);
	}
dfprintf(stderr, "\t. send() sequence returned %d\n", (int)err);

	// done -- close the socket and return an appropriate error
	close(sock);				// !!! BONE only !!!
	if (err != flatSize)
	{
dfprintf(stderr, "\t. sent wrong byte count; returning B_IO_ERROR\n");
		return B_IO_ERROR;
	}

	dfprintf(stderr, "\t* TCPMessenger done sending msg 0x%08x = '%c%c%c%c'\n", (unsigned)msg->what,
		(int)(msg->what & 0xFF000000) >> 24,
		(int)(msg->what & 0x00FF0000) >> 16,
		(int)(msg->what & 0x0000FF00) >> 8,
		(int)(msg->what & 0x000000FF));

	return B_OK;
}
