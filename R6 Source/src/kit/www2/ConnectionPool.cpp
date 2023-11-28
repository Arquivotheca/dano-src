#include <support2/Autolock.h>
#include <support2/Debug.h>
#include <support2/Message.h>
#include <support2/String.h>
#include <www2/ConnectionPool.h>
#include <www2/Connection.h>

#include <OS.h>

#include <signal.h>

using namespace B::WWW2;

#if ENABLE_CONNECTION_TRACE
	#define CONNECTION_NORMAL_COLOR "\e[0m"
	#define CONNECTION_STATUS_COLOR "\e[0;35m"

	#define CONNECTION_TRACE(x) printf x
#else
	#define CONNECTION_TRACE(x) ;
#endif

namespace B {
namespace WWW2 {

struct ConnectionHandle {
	Connection *fConnection;
	BString fHostName;
	ushort fPort;
	bool fIsAllocated;
	bigtime_t fTimeout;
	int fMaxRequests;
	
	ConnectionHandle()
		:	fConnection(NULL),
			fIsAllocated(false)
	{ }
};

const bigtime_t kCleanupInterval = 30000000;
const bigtime_t kMinimumTTL = 100000;
const bigtime_t kMaxKeepAlive = 60000000;

} } // namespace B::WWW2


ConnectionPool::ConnectionPool()
	:	fLock("Connection Pool Lock")
{
	fConnections = new ConnectionHandle[kMaxConnections];

	BMessage msg;
	atom_ptr<BAtom> ptr(dynamic_cast<BAtom *>(this));
	msg.SetData(BValue::Atom(ptr));
	fCleanupTimer = new Timer();
	fCleanupTimer->Start(Cleanup, msg, kCleanupInterval, 0x7fffffff);

#if ENABLE_CONNECTION_STATISTICS
	fConnectionsServed = 0;
	fConnectionsReused = 0;
	fConnectionsPreempted = 0;
#endif
}

ConnectionPool::~ConnectionPool()
{
#if ENABLE_CONNECTION_STATISTICS
	printf("\nConnection Stats:\n");
	printf("  Served: %d\n", fConnectionsServed);
	printf("  Reused: %d (%.2f%%)\n", fConnectionsReused, fConnectionsReused
		? static_cast<float>(fConnectionsReused) / static_cast<float>(fConnectionsServed)
		* 100.0 : 0.0);
	printf("  Preempted: %d\n", fConnectionsPreempted);
#endif

	for (int index = 0; index < kMaxConnections; index++) {
		delete fConnections[index].fConnection;
	}
	delete [] fConnections;
}


// ConnectionPool::IsConnectionStale():
//  A NULL fConnection is considered stale.
//  Returns true if the connection is
//   (1) Unallocated, and
//   (2) At least one of
//        (a) fConnection is NULL, or
//        (b) the Connection has disconnected, or
//        (c) the Connection has timed out, or
//        (d) fMaxRequests <= 0. I don't know what this last one
//             means (Laz, 2001-02-19).

inline bool ConnectionPool::IsConnectionStale(const ConnectionHandle& handle)
{
	return !handle.fIsAllocated
		&& (!handle.fConnection
			|| system_time() > handle.fTimeout - kMinimumTTL
			|| handle.fMaxRequests <= 0
			|| handle.fConnection->HasUnreadData());
}

void ConnectionPool::Cleanup(const BMessage &msg)
{
//	ConnectionPool *pool;
//	BValue value = msg->Data();
//	status_t status = B_ERROR;
//	
//	pool = reinterpret_cast<ConnectionPool *>(value.AsAtom(&status));
//
//	if (status == B_OK)
//		pool->ReapStaleConnections();
}

void ConnectionPool::ReapStaleConnections()
{
	// Some threads seem to get here with the wrong signal
	// handler for SIGPIPE.
	signal(SIGPIPE, SIG_IGN);
	
	fLock.Lock();
	
	for (int index = 0; index < kMaxConnections; index++) {
		if (IsConnectionStale(fConnections[index])) {
			delete fConnections[index].fConnection;
			fConnections[index].fConnection = NULL;
			fConnections[index].fIsAllocated = false;
		}
	}
	
	fLock.Unlock();
}

status_t ConnectionPool::GetConnection(const char *hostname, int port,
	bool tryToReuse, Connection **connection, bool *is_new_connection)
{
	fLock.Lock();
#if ENABLE_CONNECTION_STATISTICS
	fConnectionsServed++;
#endif

	if (is_new_connection)
		*is_new_connection = false;

	CONNECTION_TRACE(("Getting Connection to %s:%d, tryToReuse=%d.\n",
		hostname, port, tryToReuse));
	if (tryToReuse) {
		// See if there is already a connection
		for (int index = 0; index < kMaxConnections; index++) {
			ConnectionHandle &handle = fConnections[index];
			if (handle.fConnection
				&& !handle.fIsAllocated
				&& handle.fPort == port
				&& handle.fHostName.ICompare(hostname) == 0
				&& !IsConnectionStale(handle))
			{
				CONNECTION_TRACE((CONNECTION_STATUS_COLOR "Reusing connection %d (%p) [%s:%d] ttl = %Ld ms.\n" CONNECTION_NORMAL_COLOR,
					index, handle.fConnection, hostname, port, (handle.fTimeout - system_time()) / 1000));
#if ENABLE_CONNECTION_STATISTICS
				fConnectionsReused++;
#endif

				handle.fIsAllocated = true;
				*connection = handle.fConnection;
				fLock.Unlock();
				return B_OK;
			}

#if ENABLE_CONNECTION_TRACE
			else {
				CONNECTION_TRACE(("Not reusing Connection %d (%p)", index, handle.fConnection));
				if (!handle.fConnection)
					CONNECTION_TRACE(("because it's NULL.\n"));
				else if (handle.fIsAllocated)
					CONNECTION_TRACE(("because it's already allocated.\n"));
				else if (handle.fPort != port)
					CONNECTION_TRACE(("because it's using the wrong port (using %d, need %d).\n",
						handle.fPort, port));
				else if (handle.fHostName.ICompare(hostname) != 0)
					CONNECTION_TRACE(("because its hostname is wrong (using %s, need %s.\n",
						handle.fHostName.String(), hostname));
				else if (IsConnectionStale(handle)) {
					bigtime_t syst = system_time();
					CONNECTION_TRACE(("because it's stale (system_time() = %Ld %s Timeout = %Ld, "
						"MaxRequests = %d, HasUnreadData() = %d).\n", syst,
						(syst > handle.fTimeout ? ">" : "<="),
						handle.fTimeout, handle.fMaxRequests,
						handle.fConnection->HasUnreadData()));
				} else
					CONNECTION_TRACE(("for reasons unknown.\n"));
			}
#endif // ENABLE_CONNECTION_TRACE
		}
	}

	// Allocate a new one, but skip over connections that are still open
	for (int index = 0; index < kMaxConnections; index++) {
		ConnectionHandle &handle = fConnections[index];
		if (IsConnectionStale(handle)) {
			delete handle.fConnection;
			handle.fConnection = NULL;
			
			CONNECTION_TRACE((CONNECTION_STATUS_COLOR "Opening connection %d (%s:%d).\n" CONNECTION_NORMAL_COLOR,
				index, hostname, port));
			handle.fIsAllocated = true;
			fLock.Unlock();	// don't lock while opening

			status_t err = CreateConnection(hostname, port, &(handle.fConnection));
			if (err >= B_OK) {
				handle.fHostName.SetTo(hostname);
				handle.fPort = port;
				*connection = handle.fConnection;
				if (is_new_connection)
					*is_new_connection = true;
			} else {
				delete handle.fConnection;
				handle.fConnection = NULL;
				handle.fIsAllocated = false;
			}
			
			CONNECTION_TRACE(("Allocated Connection %p.\n", handle.fConnection));
			return err;
		}
	}

	// Allocate a new one, closing some other one that's already open
	for (int index = 0; index < kMaxConnections; index++) {
		ConnectionHandle &handle = fConnections[index];
		if (!handle.fIsAllocated) {
			CONNECTION_TRACE((CONNECTION_STATUS_COLOR "Killing connection %d (%p) [%s].\n" CONNECTION_NORMAL_COLOR,
				index, handle.fConnection, hostname));
#if ENABLE_CONNECTION_STATISTICS
			fConnectionsPreempted++;
#endif

			handle.fIsAllocated = true;
			fLock.Unlock();	// don't lock while opening
			delete handle.fConnection;
			handle.fConnection = NULL;
			
			status_t err = CreateConnection(hostname, port, &(handle.fConnection));
			if (err >= B_OK) {
				handle.fHostName.SetTo(hostname);
				handle.fPort = port;
				*connection = handle.fConnection;
				if (is_new_connection)
					*is_new_connection = true;
			} else {
				delete handle.fConnection;
				handle.fConnection = NULL;
				handle.fIsAllocated = false;
			}

			CONNECTION_TRACE(("Allocated Connection %p.\n", handle.fConnection));
			return err;
		}
	}

	debugger("Out of connections");
	return B_ERROR;
}

status_t ConnectionPool::ReturnConnection(Connection *connection, bigtime_t timeout,
	int maxRequests)
{
	CONNECTION_TRACE(("Attempting to return Connection %p, timeout = %Ld, maxRequests = %d.\n",
		connection, timeout, maxRequests));
	if (!connection)
		return B_BAD_VALUE;

	int index;
	for (index = 0; index < kMaxConnections; ++index) {
		if (fConnections[index].fConnection == connection)
			break;
	}
	if (index >= kMaxConnections)
		return B_BAD_VALUE; // We didn't find the Connection in our list.

	CONNECTION_TRACE(("Found Connection %p at index %d.\n",
		connection, index));

	ConnectionHandle &handle = fConnections[index];

	if (timeout == 0LL) {
		//handle special "expire now" value (passed when a connection's
		//Abort() is called)
		delete connection;
		handle.fConnection = NULL;
		handle.fIsAllocated = false;

	} else {
		handle.fTimeout = system_time() + (timeout < kMaxKeepAlive ? timeout : kMaxKeepAlive);
		handle.fMaxRequests = maxRequests;
		handle.fIsAllocated = false;
	}

	return B_OK;
}
