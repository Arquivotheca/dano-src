#ifndef _DNS_CACHE_H
#define _DNS_CACHE_H

#include <kernel/OS.h>
#include <support2/Locker.h>
#include <support2/Vector.h>
#include <www2/Queue.h>

using namespace B::Support2;

const int kMaxHostNameLength = 255;	// according to the RFC.
const int kDNSCacheSize = 15;	// This is also the limit on concurrent requests
const uint kDNSHashSize = 17;

class DNSCache {
public:
	typedef BVector<uint32> AddrList;

	DNSCache();
	virtual ~DNSCache();
	status_t GetHostByName(const char *hostname, uint32 *outSockAddr);
	status_t GetHostByName(const char *hostname, AddrList& outList);
	void KillPendingRequests();
	status_t ClearCache();
	
private:
	struct DNSCacheEntry : public QueueEntry {
		DNSCacheEntry *fHashNext, **fHashPrev;
		char hostname[kMaxHostNameLength + 1];
		thread_id resolver;
		AddrList ips;
		status_t error;
		bigtime_t requestTime;
		bigtime_t expiration;
		int retryCount;
		bool pending;
		int32 requestCount;
	};

	void SendRequest(DNSCacheEntry *entry, bool autodial);
	bool WaitForResponse();
	void SignalEntry(DNSCacheEntry *entry);
	bool IsNumericAddress(const char *addr);
	void Initialize();
	static void TimeoutRequest(int);
	void DumpEntries();
	int ParseResolvConf(struct sockaddr_in *servers, int maxserv);

	DNSCacheEntry fCacheEntries[kDNSCacheSize];
	BNestedLocker fLock;
	Queue fLRU;
	DNSCacheEntry *fHash[kDNSHashSize];
	int fSocket;
	bool fWaitingOnResponse;
	class Condition *fCondition;
	bool fShutdown;
	bool fInitialized;

#if PRINT_STATISTICS
	int32 fHits;
	int32 fRequests;
#endif
};

extern DNSCache dnsCache;

#endif
