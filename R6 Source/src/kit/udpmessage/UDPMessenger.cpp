// UDPMessenger

#include "UDPMessenger.h"
#include "auto_delete.h"
#include "UDPMessage_p.h"
#include <app/Message.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <stdio.h>
#include <errno.h>

#ifdef DEBUG
#define dfprintf fprintf
#else
static inline void dfprintf(FILE*, const char*, ...) { }
#endif

UDPMessenger::UDPMessenger(const char *hostname, uint16 port)
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
	
#ifdef DEBUG
	target = hostname;
	target += ":";
	target << (int)port;
#endif

}

UDPMessenger::UDPMessenger(const struct sockaddr_in& targetAddress)
{
	memcpy(&mTarget, &targetAddress, sizeof(mTarget));
	mIsValid = true;
}

UDPMessenger::~UDPMessenger()
{
}

UDPMessenger::UDPMessenger(const UDPMessenger &other)
{
	memcpy(&mTarget, &other.mTarget, sizeof(mTarget));
	mIsValid = other.mIsValid;
}

UDPMessenger &
UDPMessenger::operator=(const UDPMessenger &rhs)
{
	memcpy(&mTarget, &rhs.mTarget, sizeof(mTarget));
	mIsValid = rhs.mIsValid;
	return *this;
}

bool 
UDPMessenger::IsValid() const
{
	return mIsValid;
}

status_t 
UDPMessenger::SendMessage(uint32 command) const
{
	// we could let the other SendMessage() variant take care of mIsValid checking,
	// but then we'd just wind up allocating and deleting a superfluous BMessage,
	// which would be silly
	if (!mIsValid) return B_NO_INIT;

	BMessage msg(command);
	return this->SendMessage(&msg);
}

status_t 
UDPMessenger::SendMessage(const BMessage *msg) const
{
	dfprintf(stderr, "* UDPMessenger::SendMessage(%p what = 0x%08x = '%c%c%c%c')\n", msg, msg->what,
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

	// open the outgoing UDP socket
dfprintf(stderr, "\t. opening outgoing socket\n");
	int sock = socket(AF_INET, SOCK_DGRAM, 0);

dfprintf(stderr, "\t. socket() returned %d\n", sock);

	if (sock < 0)
	{
		int n = errno;
dfprintf(stderr, "\t. returning ERROR 0x%08x (%s)\n", n, strerror(err));
		return n;
	}

	// enable BROADCAST.
	int one = 1;	
	setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one));

	// bind to a response socket
	struct sockaddr_in server;
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(0);
dfprintf(stderr, "\t. binding to response socket\n");
	bind(sock, (struct sockaddr*) &server, sizeof(server));
dfprintf(stderr, "\t. bind() returned, setting up to transmit\n");

	// okay, now we bundle up the flattened message into a series of one or more
	// UDP packets.
	uint32 totalPackets = (flatSize / UDPM_MAX_PAYLOAD) + 1;		// always at least one
	uint64 txnNum = system_time();
	uint32 totalBytesRemaining = flatSize;
	char* udpm_base = new char[sizeof(UDPMessageHeader) + UDPM_MAX_PAYLOAD];
	memset(udpm_base, 0, sizeof(UDPMessageHeader));
	auto_array_delete<char> udpmDeletor(udpm_base);
	UDPMessageHeader* udpm = (UDPMessageHeader*) udpm_base;

	// send the flattened message, one packet at a time, waiting for
	// each packet to be acked before sending the next one
	udpm->transaction_number = txnNum;		// fixed for all packets in this transaction
	udpm->total_packets = totalPackets;
	udpm->total_payload = flatSize;

	struct timeval tv;		// used for timeout detection when listening for acks
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	int retries = 0;

dfprintf(stderr, "\t. starting to send %lu packets\n", totalPackets);

	for (uint32 packet = 0; packet < totalPackets; )
	{
dfprintf(stderr, "\t. building packet %lu\n", packet);
		// build the outgoing packet
		udpm->this_packet = packet;
		udpm->this_payload = (totalBytesRemaining > UDPM_MAX_PAYLOAD)
			? UDPM_MAX_PAYLOAD : totalBytesRemaining;
		totalBytesRemaining -= udpm->this_payload;
		memcpy(udpm_base + sizeof(UDPMessageHeader),
			flattenedMsg + (packet * UDPM_MAX_PAYLOAD),
			udpm->this_payload);

		// send it
dfprintf(stderr, "\t. sending packet %lu\n", packet);
		sendto(sock, udpm_base, sizeof(UDPMessageHeader) + udpm->this_payload, 0,
			(struct sockaddr*) &mTarget, sizeof(mTarget));
dfprintf(stderr, "\t. done sending packet %lu, listening for ack\n", packet);

		// read back the ack for this packet, using select() to impose a timeout
		fd_set sockset;
		FD_ZERO(&sockset);
		FD_SET(sock, &sockset);
dfprintf(stderr, "\t. calling select() for packet %lu ack\n", packet);
		int nReady = select(sock + 1, &sockset, NULL, NULL, &tv);
dfprintf(stderr, "\t. select() for packet %lu returned %d\n", packet, nReady);
		if (nReady > 0)
		{
			// we got the ack; read it out and proceed to the next packet
			UDPMessageHeader ack;
			struct sockaddr_in sender;
			int inSize = sizeof(sender);
dfprintf(stderr, "\t. calling recvfrom() to read packet %lu ack\n", packet);
			recvfrom(sock, &ack, sizeof(ack), 0, (struct sockaddr*) &sender, &inSize);
dfprintf(stderr, "\t. recvfrom() for packet %lu ack returned; bumping packet count up\n", packet);
			retries = 0;
			packet++;
		}
		else if ((err == 0) && (retries < 3))
		{
			// timeout -- resend the packet until we run out of retries
			retries++;
			dfprintf(stderr, "...TIMEOUT waiting for packet %lu ack, retries now %d\n", packet, retries);
		}
		else
		{
			dfprintf(stderr, "\t. returning hard error 0x%08x\n", nReady);
			// hard error or too many retries - bail here and don't continue
			err = (!nReady) ? B_TIMED_OUT : nReady;
			break;		// break out of the for() loop and return immediately
		}
	}

	dfprintf(stderr, "\t* UDPMessenger done sending msg 0x%08x = '%c%c%c%c'\n", msg->what,
		(int)(msg->what & 0xFF000000) >> 24,
		(int)(msg->what & 0x00FF0000) >> 16,
		(int)(msg->what & 0x0000FF00) >> 8,
		(int)(msg->what & 0x000000FF));

	close(sock);		// !!! BONE only !!!
	return err;
}
