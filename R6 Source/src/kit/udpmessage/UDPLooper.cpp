// UDPLooper.cpp

#include "UDPLooper.h"
#include "UDPMessenger.h"
#include "auto_delete.h"
#include "UDPMessage_p.h"
#include <List.h>
#include <DataIO.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#ifdef DEBUG
#define dfprintf fprintf
#else
static inline void dfprintf(FILE*, const char*, ...) { }
#endif

// The BMessage will have the following named fields, in network byte order:
const char * const B_UDP_ORIGIN_IP = "b:origin_ip";				// 32 bits
const char * const B_UDP_ORIGIN_PORT = "b:origin_port";		// 16 bits

// magic sequence used to identify a "noop" non-payload UDP packet
const uint8 UDPLOOPER_MAGIC_NOOP[] = "MaGiCNooP";

UDPLooper::UDPLooper(uint16 listenPort, int32 priority)
	: mPort(listenPort), mCurrentMessage(NULL)
{
	mTID = spawn_thread(UDPLooper::LooperThread, "UDPLooper", priority, this);
}

UDPLooper::~UDPLooper()
{
	Quit();
}

uint16 
UDPLooper::Port() const
{
	return mPort;
}

status_t 
UDPLooper::PostMessage(uint32 command)
{
	mQueue.AddMessage(new BMessage(command));
	WakeUpLooper();
	return B_OK;
}

status_t 
UDPLooper::PostMessage(BMessage* message)
{
	mQueue.AddMessage(new BMessage(*message));
	WakeUpLooper();
	return B_OK;
}

void 
UDPLooper::DispatchMessage(BMessage *msg)
{
	mCurrentMessage = msg;
	this->MessageReceived(msg);
	delete mCurrentMessage;		// if detached, mCurrentMessage will be NULL
}

void 
UDPLooper::MessageReceived(BMessage* /*msg*/)
{
	// intentionally blank
}

BMessage *
UDPLooper::CurrentMessage()
{
	return mCurrentMessage;
}

BMessage *
UDPLooper::DetachCurrentMessage()
{
	BMessage* msg = mCurrentMessage;
	mCurrentMessage = NULL;
	return msg;
}

thread_id 
UDPLooper::Run()
{
	resume_thread(mTID);
	return mTID;
}

void 
UDPLooper::Quit()
{
	// any pending messages in the queue are automatically
	// deleted when the object itself is deleted, so we need not
	// do anything about it here, just kill off the listener thread
	// once we hold the lock to guarantee that we're not leaving
	// the queue lock in a bad state
	mQueue.Lock();
	send_signal(mTID, SIGINT);
}

// We force the looper to recheck its queue by sending it a special noop
// UDP message directly; this is the only reliable way to cause select() to
// trip even when the thread is in the brief interval between dispatching
// queued messages and calling select() again.
void 
UDPLooper::WakeUpLooper() const
{
	// address it to the loopback interface
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_len = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(mPort);
	addr.sin_addr.s_addr = htonl(0x7F000001);		// localhost

	const uint32 DATALEN = sizeof(UDPLOOPER_MAGIC_NOOP);
	sendto(mSocket, UDPLOOPER_MAGIC_NOOP, DATALEN, 0, (struct sockaddr*) &addr, sizeof(addr));
}

// main looper thread, uses one signals for out-of-band communication:
// SIGHUP == break out of signal() and dispatch any messages in the queue
// SIGINT == clean shutdown of the looper thread

void udpl_sig_handler(int sig, UDPLooper* looper, void* /* vreg_ptr */)
{
	// throw the "shutdown" flag if this was a SIGINT; if it wasn't,
	// then it must have been a SIGHUP, which doesn't need any
	// special handling other than letting the main recvfrom() loop
	// recheck the message queue
	if (sig == SIGINT)
	{
		dfprintf(stderr, "--- SIGINT received; setting shutdown flag on %p\n", looper);
		close(looper->mSocket);
	}
}

struct PendingMessage
{
	uint32 transaction_number;		// common to all packets in a given BMessage
	uint32 total_packets;				// total number of packets in this message, to avoid overruns
	BMallocIO flatMessage;
};

// utility function - clear the PendingMessage list
static void EmptyMessageList(BList& list)
{
	for (int32 i = 0; i < list.CountItems(); i++)
	{
		PendingMessage* pm = (PendingMessage*) list.ItemAt(i);
		delete pm;
	}
	list.MakeEmpty();
}

int32 
UDPLooper::LooperThread(void* arg)
{
	UDPLooper* udpl = (UDPLooper*) arg;		// surrogate "this" pointer

	// install the signal handler for SIGINT
	{
		struct sigaction action;
		action.sa_handler = (__signal_func_ptr) &udpl_sig_handler;
		action.sa_flags = 0;
		action.sa_userdata = udpl;			// pointer to the looper object as userdata to the handler
		sigemptyset(&action.sa_mask);
		sigaction(SIGINT, &action, NULL);
	}

	// set up the UDP listen socket
	struct sockaddr_in server;
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);	// listen on all interfaces
	server.sin_port = htons(udpl->mPort);					// may be zero

	// create the socket and bind to it
	udpl->mSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (bind(udpl->mSocket, (struct sockaddr*) &server, sizeof(server)) < 0)
	{
		int bindErr = errno;
		dfprintf(stderr, "ERROR 0x%x (%s) trying to bind to port %d\n", bindErr, strerror(bindErr), udpl->mPort);
		return bindErr;
	}

	// if we bound to an ephemeral port, look up the actual one
	if (udpl->mPort == 0)
	{
		struct sockaddr_in localAddr;
		int addrLen = sizeof(localAddr);
		if (getsockname(udpl->mSocket, (struct sockaddr*) &localAddr, &addrLen) >= 0)
		{
			udpl->mPort = ntohs(localAddr.sin_port);
		}
	}

	// list of incoming message buffers
	BList messageList;		// pointers to PendingMessage structs

	// incoming datagram buffer
	const uint32 DATALEN = sizeof(UDPMessageHeader) + UDPM_MAX_PAYLOAD;
	char* data = new char[DATALEN];
	auto_array_delete<char> dataDeletor(data);
	UDPMessageHeader* udpm = (UDPMessageHeader*) data;

	// timeout structure for select()
	struct timeval tv;
	tv.tv_sec = 30;
	tv.tv_usec = 0;

	bool done = false;
	while (!done)
	{
		// dispatch any queued messages
		udpl->mQueue.Lock();
		while (udpl->mQueue.CountMessages())
		{
			BMessage* msg = udpl->mQueue.FindMessage((int32) 0);
			udpl->mQueue.RemoveMessage(msg);
			udpl->DispatchMessage(msg);		// DispatchMessage() deletes the message
		}
		udpl->mQueue.Unlock();

		// now wait for a datagram.  if we go 30 seconds without receiving any data, we
		// discard any partially-received messages on the theory that the "connection" with
		// the sender is toast.
		fd_set sockset;
		FD_ZERO(&sockset);
		FD_SET(udpl->mSocket, &sockset);
		int err = select(udpl->mSocket + 1, &sockset, NULL, NULL, &tv);
		if (err > 0)		// someone sent us a datagram!
		{
			struct sockaddr_in sender;
			int inSize = sizeof(sender);
			ssize_t nRead = recvfrom(udpl->mSocket, data, DATALEN, 0, (struct sockaddr*) &sender, &inSize);
			if (nRead > 0)
			{
				dfprintf(stderr, "\t. got a datagram\n");
				// a "no-op" packet is our signal to fall through and check the queue again,
				// not to actually ack & dispatch it.  such a packet is indicated by matching
				// the first N bytes with UDPLOOPER_MAGIC_NOOP.
				if (memcmp(data, UDPLOOPER_MAGIC_NOOP, sizeof(UDPLOOPER_MAGIC_NOOP)))
				{
					dfprintf(stderr, "\t. it's a data packet; sending ack\n");
					// send the ack for this packet
					UDPMessageHeader udpmAck;
					memset(&udpmAck, 0, sizeof(udpmAck));
					udpmAck.transaction_number = udpm->transaction_number;
					udpmAck.this_packet = udpm->this_packet;
					sendto(udpl->mSocket, &udpmAck, sizeof(udpmAck), 0, (struct sockaddr*) &sender, sizeof(sender));
					dfprintf(stderr, "\t. sent ack\n");

					// if this is the first packet of a message, create a buffer for it and
					// add it to the list
					PendingMessage* pm = NULL;
					if (udpm->this_packet == 0)
					{
						pm = new PendingMessage;
						pm->transaction_number = udpm->transaction_number;
						pm->total_packets = udpm->total_packets;
						pm->flatMessage.SetSize(udpm->total_payload);
						pm->flatMessage.Seek(0, SEEK_SET);
						pm->flatMessage.Write(data + sizeof(UDPMessageHeader), udpm->this_payload);
						messageList.AddItem(pm);
					}
					else
					{
						// find the message that this packet belongs to.  If it's a middle packet of
						// a transaction that doesn't exist in our list, we just drop it.
						for (int32 i = 0; i < messageList.CountItems(); i++)
						{
							PendingMessage* qMsg = (PendingMessage*) messageList.ItemAt(i);
							if (qMsg &&
								(qMsg->transaction_number == udpm->transaction_number) &&
								(qMsg->total_packets > udpm->this_packet))
							{
								pm = qMsg;
								pm->flatMessage.Seek(udpm->this_packet * UDPM_MAX_PAYLOAD, SEEK_SET);
								pm->flatMessage.Write(data + sizeof(UDPMessageHeader), udpm->this_payload);
							}
						}
					}

					// if we just finished this message, unflatten it and post it to the queue
					if (pm && (udpm->this_packet == pm->total_packets - 1))
					{
						dfprintf(stderr, "\t. whole message received; unflattening\n");
						BMessage* msg = new BMessage;
						status_t err = msg->Unflatten((const char*) pm->flatMessage.Buffer());
						if (!err)
						{
							// add the sender IP & port, then queue it for dispatch
							msg->AddInt32(B_UDP_ORIGIN_IP, sender.sin_addr.s_addr);
							msg->AddInt16(B_UDP_ORIGIN_PORT, sender.sin_port);
							udpl->mQueue.AddMessage(msg);
						}
						else
						{
							dfprintf(stderr, "*** error unflattening message: %ld (%s)\n", err, strerror(err));
							delete msg;
						}
						messageList.RemoveItem(pm);
						delete pm;
					}
				}
			}
		}
		else if (err == 0)		// timeout!
		{
			dfprintf(stderr, "* TIMEOUT waiting for data; flushing partial messages\n");
			EmptyMessageList(messageList);
		}
		else
		{
			if (errno == B_FILE_ERROR)
			{
				// this means that the socket was closed out from under us, i.e. Quit() was called
				done = true;
			}
		}
	}

	EmptyMessageList(messageList);	// make sure we're not leaking
	delete udpl;										// delete the looper object itself
	return 0;
}
