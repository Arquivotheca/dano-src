// TCPLooper.cpp

#include "TCPLooper.h"
#include "TCPMessenger.h"
#include "auto_delete.h"
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
const char * const B_TCP_ORIGIN_IP = "b:origin_ip";				// 32 bits
const char * const B_TCP_ORIGIN_PORT = "b:origin_port";		// 16 bits

// magic size token used to identify a "noop" non-payload TCP message
const ssize_t TCPLOOPER_MAGIC_NOOP = 0xFA7E5038;

// Utility function: read() exactly N bytes from a socket into the given buffer, taken from Stevens.
ssize_t						/* Read "n" bytes from a descriptor. */
readn(int fd, void *vptr, size_t n)
{
	size_t	nleft;
	ssize_t	nread;
	char	*ptr;

	ptr = (char*) vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR)
				nread = 0;		/* and call read() again */
			else
				return(-1);
		} else if (nread == 0)
			break;				/* EOF */

		nleft -= nread;
		ptr   += nread;
	}
	return(n - nleft);		/* return >= 0 */
}
/* end readn */

//#pragma mark -

TCPLooper::TCPLooper(uint16 listenPort, int32 priority)
	: mPort(listenPort), mCurrentMessage(NULL)
{
	mTID = spawn_thread(TCPLooper::LooperThread, "TCPLooper", priority, this);
}

TCPLooper::~TCPLooper()
{
}

uint16 
TCPLooper::Port() const
{
	return mPort;
}

status_t 
TCPLooper::PostMessage(uint32 command)
{
	mQueue.AddMessage(new BMessage(command));
	WakeUpLooper();
	return B_OK;
}

status_t 
TCPLooper::PostMessage(BMessage* message)
{
	mQueue.AddMessage(new BMessage(*message));
	WakeUpLooper();
	return B_OK;
}

void 
TCPLooper::DispatchMessage(BMessage *msg)
{
	mCurrentMessage = msg;
	this->MessageReceived(msg);
	delete mCurrentMessage;		// if detached, mCurrentMessage will be NULL
}

void 
TCPLooper::MessageReceived(BMessage* /*msg*/)
{
	// intentionally blank
}

BMessage *
TCPLooper::CurrentMessage()
{
	return mCurrentMessage;
}

BMessage *
TCPLooper::DetachCurrentMessage()
{
	BMessage* msg = mCurrentMessage;
	mCurrentMessage = NULL;
	return msg;
}

thread_id 
TCPLooper::Run()
{
	resume_thread(mTID);
	return mTID;
}

void 
TCPLooper::Quit()
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
// TCP message directly; this is the only reliable way to cause select() to
// trip even when the thread is in the brief interval between dispatching
// queued messages and calling select() again.
void 
TCPLooper::WakeUpLooper() const
{
	// address it to the loopback interface
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_len = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(mPort);
	addr.sin_addr.s_addr = htonl(0x7F000001);		// localhost

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock >= 0)
	{
		int err = connect(sock, (const struct sockaddr*) &addr, sizeof(addr));
		if (err >= 0)
		{
			send(sock, &TCPLOOPER_MAGIC_NOOP, sizeof(TCPLOOPER_MAGIC_NOOP), 0);
		}
		close(sock);
	}
}

// main looper thread, uses one signals for out-of-band communication:
// SIGINT == clean shutdown of the looper thread

void tcpl_sig_handler(int sig, TCPLooper* looper, void* /* vreg_ptr */)
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

int32 
TCPLooper::LooperThread(void* arg)
{
	TCPLooper* tcpl = (TCPLooper*) arg;		// surrogate "this" pointer

	// install the signal handler for SIGINT
	{
		struct sigaction action;
		action.sa_handler = (__signal_func_ptr) &tcpl_sig_handler;
		action.sa_flags = 0;
		action.sa_userdata = tcpl;			// pointer to the looper object as userdata to the handler
		sigemptyset(&action.sa_mask);
		sigaction(SIGINT, &action, NULL);
	}

	// set up the TCP listen socket
	struct sockaddr_in server;
	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);	// listen on all interfaces
	server.sin_port = htons(tcpl->mPort);					// may be zero

	// create the socket and bind to it
	tcpl->mSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (bind(tcpl->mSocket, (struct sockaddr*) &server, sizeof(server)) < 0)
	{
		int bindErr = errno;
		dfprintf(stderr, "ERROR 0x%x (%s) trying to bind to port %d\n", bindErr, strerror(bindErr), tcpl->mPort);
		return bindErr;
	}

	// if we bound to an ephemeral port, look up the actual one
	if (tcpl->mPort == 0)
	{
		struct sockaddr_in localAddr;
		int addrLen = sizeof(localAddr);
		if (getsockname(tcpl->mSocket, (struct sockaddr*) &localAddr, &addrLen) >= 0)
		{
			tcpl->mPort = ntohs(localAddr.sin_port);
		}
	}

	// mark the socket for listening
	if (listen(tcpl->mSocket, 5) < 0)		// !!! magic backlog count; see Stevens 2e sect 4.5 for a discussion of this
	{
		int listenErr = errno;
		dfprintf(stderr, "ERROR 0x%x (%s) trying to listen on port %d\n", listenErr, strerror(listenErr), tcpl->mPort);
		return listenErr;
	}

	// listen loop
	bool done = false;
	while (!done)
	{
		// dispatch any queued messages
		tcpl->mQueue.Lock();
		while (tcpl->mQueue.CountMessages())
		{
			BMessage* msg = tcpl->mQueue.FindMessage((int32) 0);
			tcpl->mQueue.RemoveMessage(msg);
			tcpl->DispatchMessage(msg);		// DispatchMessage() deletes the message
		}
		tcpl->mQueue.Unlock();

		// now wait for a connection
		struct sockaddr_in sender;
		int inSize = sizeof(sender);
		int conn = accept(tcpl->mSocket, (struct sockaddr*) &sender, &inSize);
		if (conn >= 0)			// aha, a connection!
		{
			ssize_t netFlatSize;
			ssize_t nRead = readn(conn, &netFlatSize, sizeof(netFlatSize));
dfprintf(stderr, "- TCPLooper got size field = 0x%08lx = 0x%08lx host\n", netFlatSize, ntohl(netFlatSize));
			if (nRead == sizeof(netFlatSize))
			{
dfprintf(stderr, "\t- read right number of bytes\n");
				if (netFlatSize != TCPLOOPER_MAGIC_NOOP)		// don't try to read a message if it's this
				{
					ssize_t flatSize = ntohl(netFlatSize);
					char* data = new char[flatSize];
					auto_array_delete<char> dataDeletor(data);

dfprintf(stderr, "\t- it's a real payload, reading flattened message = %ld bytes\n", flatSize);
					nRead = readn(conn, data, flatSize);
dfprintf(stderr, "\t- read %ld bytes of payload\n", nRead);

					if (nRead == flatSize)													// got it successfully
					{
dfprintf(stderr, "\t- got payload, unflattening\n");
						BMessage* msg = new BMessage;
						status_t err = msg->Unflatten(data);
						if (!err)
						{
							// add the sender IP & port, then queue it for dispatch
							msg->AddInt32(B_TCP_ORIGIN_IP, sender.sin_addr.s_addr);
							msg->AddInt16(B_TCP_ORIGIN_PORT, sender.sin_port);
							tcpl->mQueue.AddMessage(msg);
						}
						else
						{
							dfprintf(stderr, "*** error unflattening message: %ld (%s)\n", err, strerror(err));
							delete msg;
						}
					}
					else
					{
dfprintf(stderr, "\t- got wrong number of bytes of payload; discarding\n");
					}
				}
			}
			close(conn);
		}
		else			// error from accept()
		{
			int err = errno;
dfprintf(stderr, "\t- accept() returned error 0x%08x (%s)\n", err, strerror(err));
			if (err == B_FILE_ERROR)
			{
dfprintf(stderr, "\t- socket-close detected; quitting looper thread\n");
				done = true;		// socket was closed
			}
		}
	}

	delete tcpl;										// delete the looper object itself
	return 0;
}
