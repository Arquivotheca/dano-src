// TCPLooper.h

#ifndef TCPLooper_H
#define TCPLooper_H 1

#include <MessageQueue.h>
#include <OS.h>

// This is *not* a BLooper descendent, but it acts much like one.

class TCPLooper
{
public:
	// listenPort == 0 means bind() will choose an ephemeral port TCP port
	TCPLooper(uint16 listenPort, int32 priority = B_NORMAL_PRIORITY);

	// what port are we listening on?
	uint16 Port() const;

	// send a message directly to this looper, no need to go through TCP
	status_t PostMessage(uint32 command);
	status_t PostMessage(BMessage* message);

	// Override to supply message handling.  The default DispatchMessage calls
	// MessageReceived() for the message, then deletes it.  If your subclass provides
	// its own DispatchMessage(), then you are responsible for deleting the messsage
	// yourself.
	virtual void DispatchMessage(BMessage* msg);
	virtual void MessageReceived(BMessage* msg);
	BMessage* CurrentMessage();
	BMessage* DetachCurrentMessage();

	// you must call this to start the looper thread going
	virtual thread_id Run();

	// you must call this to halt the looper; derived implementations *MUST* call
	// this parent method in order to shut down cleanly.
	virtual void Quit();

protected:
	// Never delete a TCPLooper directly, always call Quit(), which deletes the
	// object itself
	virtual ~TCPLooper();

private:
	void WakeUpLooper() const;

	// the actual thread function for the TCPLooper
	static int32 LooperThread(void*);
	friend void tcpl_sig_handler(int, TCPLooper*, void*);

	uint16 mPort;			// TCP port that we're listening at
	thread_id mTID;		// thread ID of the looper thread
	int mSocket;				// listen socket
	BMessage* mCurrentMessage;
	BMessageQueue mQueue;	// messages to be processed
};

// The BMessage will have the following named fields, in network byte order:
extern const char * const B_TCP_ORIGIN_IP;			// 32 bits
extern const char * const B_TCP_ORIGIN_PORT;	// 16 bits

#endif
