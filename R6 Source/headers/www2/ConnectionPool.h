#ifndef _CONNECTION_POOL_H
#define _CONNECTION_POOL_H

#include <www2/Timer.h>
#include <support2/Locker.h>
#include "parameters.h"

namespace B {
namespace WWW2 {

class Connection;
class ConnectionHandle;

class ConnectionPool
{
	public:
		ConnectionPool();
		virtual ~ConnectionPool();

		status_t GetConnection(const char *hostname, int port,
		        bool tryToReuse, Connection **connection, bool *is_new_connection = NULL);

		// Returning a Connection to the wrong ConnectionPool (one which doesn't have the
		//  Connection in its list) returns B_BAD_VALUE, but is safe.
		// Otherwise, ReturnConnection() returns B_OK.
		// ReturnConnection() assumes that only one thread will be trying to return
		//  a given Connection at a time.  Basically, if you want several threads to
		//  share a single Connection, you'll have to implement your own access control.
		//  Note that several threads CAN share a Connection _Pool_ safely.
		status_t ReturnConnection(Connection *connection, bigtime_t timeout, int maxRequests);

		// Subclasses override this to return the desired type of (Open) Connection:
		virtual status_t CreateConnection(const char *hostname, int port,
		        Connection **new_connection) = 0;

	private:
		static void Cleanup(const BMessage &message);
		void ReapStaleConnections();
		inline static bool IsConnectionStale(const ConnectionHandle &slot);

		ConnectionHandle *fConnections;
		// fLock guards access to fConnections[].  In particular,
		//  you can't believe that fConnections[i].fIsAllocated == false
		//  unless you read that value while holding this lock.  You _can_,
		//  however, believe that fIsAllocated == true without holding the
		//  fLock; you may be "wrong" (the Connection may have already been
		//  Returned, and its associated fIsAllocated may be nanoseconds
		//  away from being cleared to false), but that particular
		//  misunderstanding is, at least, safe.
		// You can clear fIsAllocated to false without holding fLock, but
		//  you must hold fLock if you want to set fIsAllocated to true.
		BLocker fLock;
		atom_ptr<Timer> fCleanupTimer;

#if ENABLE_CONNECTION_STATISTICS
		int fConnectionsServed;
		int fConnectionsReused;
		int fConnectionsPreempted;
#endif
};

} } // namespace B::WWW2

#endif

