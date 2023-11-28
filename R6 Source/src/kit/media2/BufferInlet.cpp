#include "BufferInlet.h"
#include <media2/Buffer.h>

#include <support2/Autolock.h>
#include <support2/CallStack.h>
#include <support2/Debug.h>
#include <support2/StdIO.h>

#define checkpoint \
berr << "thid " << find_thread(0) << " -- " << __FILE__ << ":" << __LINE__ << " -- " << __FUNCTION__ << endl;

#define callstack \
checkpoint \
berr->BumpIndentLevel(1); \
B::Support2::BCallStack cs; cs.Update(); cs.LongPrint(berr); berr << endl; \
berr->BumpIndentLevel(-1);

namespace B {
namespace Private {

using namespace Media2;
using namespace Support2;

BBufferInlet::BBufferInlet(
	size_t maxConnections, size_t maxQueued, size_t maxInfoSize)
	:	mThread(-1),
		mInPort(-1),
		mMaxQueued(maxQueued),
		mMaxInfoSize(maxInfoSize),
		mMaxConnections(maxConnections),
		mConnectionCount(0),
		mConnections(0),
		mDisconnections(0),
		mDiscoCount(0),
		mDiscoHead(0),
		mDiscoTail(0)
{
}

BBufferInlet::~BBufferInlet()
{
	_StopThread();
}

status_t 
BBufferInlet::Acquired(const void *id)
{
	return BAtom::Acquired(id);
}

status_t 
BBufferInlet::Released(const void *id)
{
	return BAtom::Released(id);
}

status_t 
BBufferInlet::Connect(
	buffer_transport_type type,
	IBufferOutlet::arg sender,
	BMediaInput::arg input,
	int32 * outToken)
{
	status_t err;
	if (type != B_MEDIA_QUEUEING_TRANSPORT) return B_UNSUPPORTED;

	// kick off dispatcher thread if not already running
	BAutolock _l(mStartStopLock.Lock());

	// find a free connection slot and fetch token (slot index)
	int32 token = -1;
	connection * c = 0;
	{
		BAutolock(mConnectionLock.Lock());
		if (mConnectionCount == mMaxConnections) return B_NO_MEMORY;
		if (mConnectionCount)
		{
			for (token = 0;
				 mConnections[token].input != 0 && token < (int32)mMaxConnections;
				 token++)
			{}
			ASSERT(token < (int32)mMaxConnections);
		}
		else
		{
			token = 0;
			err = _StartThread();
			if (err < B_OK)
			{
berr << "BBufferInlet::Connect(): _StartThread(): " << strerror(err) << endl;
				return err;
			}
		}
		c = &mConnections[token];
		c->input = input;
		++mConnectionCount;
	}

	// set up IBufferOutlet's side of the affair
	c->reply = create_port(mMaxQueued, "outlet.reply");
	if (c->reply < B_OK) return c->reply;

	// confirm connection
	BValue info;
	info.Overlay("TOKN", BValue::Int32(token));
	info.Overlay("BUFS", BValue::Int32(mMaxQueued));
	info.Overlay("INFS", BValue::Int32(mMaxInfoSize));
	info.Overlay("QUEP", BValue::Int32(mInPort));
	info.Overlay("REPP", BValue::Int32(c->reply));
	err = sender->AcceptConnect(B_MEDIA_QUEUEING_TRANSPORT, input, info);

	if (err < B_OK)
	{
		// clean up
		port_id p = c->reply;
		mConnectionLock.Lock();
		*c = connection();
		--mConnectionCount;
		mConnectionLock.Unlock();
		delete_port(p);
		
		if (!mConnectionCount)
		{
			// we started the thread, but no connections are left to use it.
			_StopThread();
		}
		return err;
	}

	if (outToken) *outToken = token;

	return B_OK;
}

status_t 
BBufferInlet::Disconnect(int32 token)
{
	if (token < 0 || token >= (int32)mMaxConnections) return B_BAD_VALUE;
	
	BAutolock _l(mStartStopLock.Lock());
	mConnectionLock.Lock();

	connection & c = mConnections[token];
	if (c.input == 0) return B_NOT_ALLOWED;
	size_t headPrev = (mDiscoHead > 0) ? mDiscoHead - 1 : mMaxConnections - 1;
	if (mDiscoTail == headPrev ||
		(atomic_or(&c.state, CONNECTION_CLOSING) & CONNECTION_CLOSING))
	{
		// already disconnected at the API level
		return B_NOT_ALLOWED;
	}
	// add slot to disconnection queue
	mDisconnections[mDiscoTail] = &c;
	if (++mDiscoTail == mMaxConnections) mDiscoTail = 0;
	++mDiscoCount;
	
	bool stop = (mConnectionCount == mDiscoCount);

	mConnectionLock.Unlock();
	
	if (stop)
	{
		// we're closing the final connection: block until the thread finishes
		// processing buffer requests (since no requests are considered valid
		// at this point, it should be a short wait.)
		_StopThread();
	}
	
	return B_OK;
}

status_t 
BBufferInlet::_StartThread()
{
	if (mThread >= 0) return B_NOT_ALLOWED;

	mConnections = new connection[mMaxConnections];
	mDisconnections = new connection*[mMaxConnections];
	memset(mDisconnections, 0, sizeof(connection*) * mMaxConnections);

	mInPort = create_port(mMaxQueued, "outlet.in");
	if (mInPort < B_OK) return mInPort;

	mThread = spawn_thread(&ThreadEntry, "outlet.service", 120, this);
	if (mThread < B_OK) return mThread;

	status_t err = resume_thread(mThread);
	if (err < B_OK)
	{
		_StopThread();
		return err;
	}
	
	return B_OK;
}

void 
BBufferInlet::_StopThread()
{
	if (mInPort >= 0)
	{
		write_port(mInPort, QUIT, 0, 0);
		close_port(mInPort);
	}
	if (mThread >= 0)
	{
		status_t err;
		while (wait_for_thread(mThread, &err) == B_INTERRUPTED) {}
		mThread = -1;
	}
	if (mInPort >= 0)
	{
		delete_port(mInPort);
		mInPort = -1;
	}

	if (mConnections)
	{
		BAutolock _l(mConnectionLock.Lock());
		for (size_t n = 0; n < mMaxConnections; n++)
		{
			if (mConnections[n].input != 0)
			{
	berr << "BBufferInlet::StopThread(): connection " << n << " still open!" << endl;
				mConnections[n] = connection();
			}
		}
		delete [] mConnections;
		mConnections = 0;
		delete [] mDisconnections;
		mDisconnections = 0;
		mConnectionCount = 0;
		mDiscoCount = 0;
		mDiscoHead = 0;
		mDiscoTail = 0;
	}
}

status_t 
BBufferInlet::ThreadEntry(void *cookie)
{
	((BBufferInlet*)cookie)->Thread();
	return B_OK;
}

void 
BBufferInlet::Thread()
{
	const port_id port = mInPort;
	int32 code;
	const size_t data_size = sizeof(flat_buffer) + mMaxInfoSize;

	int8 * data = new int8[data_size];

	status_t local_err, err;
	ssize_t read_result;
	int64 timeout = B_INFINITE_TIMEOUT;
	while (true)
	{
		read_result = read_port_etc(port, &code, data, data_size, B_TIMEOUT, timeout);
		if (read_result < 0)
		{
			if (read_result != B_TIMED_OUT) break;
			BAutolock _l(mConnectionLock.Lock());
			if (mDiscoHead != mDiscoTail)
			{
				// the port has emptied, and we have connections to close:
				mConnectionCount -= mDiscoCount;
				mDiscoCount = 0;
				for (;
					mDiscoHead != mDiscoTail;
					mDiscoHead = (mDiscoHead + 1 < mMaxConnections) ? mDiscoHead + 1 : 0)
				{
					*(mDisconnections[mDiscoHead]) = connection();
				}
			}
			timeout = B_INFINITE_TIMEOUT;
			continue;
		}

		// read_result >= 0

		if (code == QUIT) break;

		switch (code)
		{
			case DISCO_FLUSH:
			{
				timeout = 0LL;
				break;
			}
			case BUFFER_TRANSPORT_QUEUE:
			{
				const flat_buffer & flat = *(flat_buffer*)data;
				BBuffer b(flat);
				if (read_result > (ssize_t)sizeof(flat_buffer))
				{
					b.EditInfo().Unflatten(data + sizeof(flat_buffer), read_result - sizeof(flat_buffer));
				}
				if (flat.token >= 0 &&
					flat.token < (int32)mMaxConnections &&
					mConnections[flat.token].input != 0)
				{
					connection & c = mConnections[flat.token];
					if (c.state & CONNECTION_CLOSING)
					{
						// +++ probably a dumb error code
						local_err = B_NOT_ALLOWED;
					}
					else
					{
						local_err = c.input->HandleBuffer(&b);
					}
					// +++ send modified buffer in reply... always? when requested? +++
					err = write_port(c.reply, local_err, 0, 0);
					if (err < B_OK)
					{
berr << "BBufferInlet: write_port(c.reply == " << c.reply << "): " << strerror(err) << endl;
					}
				}
#if DEBUG
				else
				{
berr << "BBufferInlet: bad token " << flat.token << endl;
				}
#endif
				break;
			}
		}
	}
	delete [] data;
}

} } // B::Private
