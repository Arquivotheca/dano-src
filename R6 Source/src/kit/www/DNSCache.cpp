#include <netdb.h>
#include <signal.h>
#include <string.h>
#include <Debug.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/net_control.h>
#include <errno.h>
#include <ctype.h>
#include <StopWatch.h>
#include <support/Binder.h>
#include <Autolock.h>
#include <NodeMonitor.h>
#include <unistd.h>
#include "Condition.h"
#include "DNSCache.h"
#include "util.h"
#include "parameters.h"
#include "priv_syscalls.h"

static atom<GHandler> fResolvconfWatcher;
class ResolvconfWatcher : public GHandler
{
	public:
	
	void SetupWatching()
	{
		printf("*\n*\nSetupWatching*\n*\n");
		struct stat st;
		_kstop_notifying_(Port(),Token());
		lstat("/boot/beos/etc/resolv.conf",&st);
		_kstart_watching_vnode_(
			st.st_dev,
			st.st_ino,
			B_WATCH_ALL,
			Port(),
			Token());
		lstat("/boot/beos/etc",&st);
		_kstart_watching_vnode_(
			st.st_dev,
			st.st_ino,
			B_WATCH_DIRECTORY,
			Port(),
			Token());
	}
	
	ResolvconfWatcher() {
		SetupWatching();
	}
	
	status_t HandleMessage(BMessage *) {
		SetupWatching();
		dnsCache.ClearCache();
		return B_OK;
	}
};

const ushort kNameServicePort = 53;
DNSCache dnsCache;

// Response Types
const uint kHostAddress = 1;

// Error Codes
const uint kNoError = 0;
const uint kFormatError = 1;
const uint kServerFailure = 2;
const uint kNameError = 3;
const uint kNotImplemented = 4;
const uint kRefused = 5;

struct message_header {
	short id;
	char flags1, flags2;
	short question_count;
	short answer_count;
	short authority_count;
	short additional_count;
} PACKED;

const uint kMaxMessageLength = 512;	// according to the RFC

// This is a private net kit function
extern "C" {
	int get_dns_ipaddrs(unsigned long **dns);
	void free_dns_ipaddrs(unsigned long *dns);
}

DNSCache::DNSCache()
	:	fLock("DNS cache lock"),
		fWaitingOnResponse(false),
		fCondition(new Condition("DNS event", &fLock)),
		fShutdown(false),
		fInitialized(false),
		fInBlackout(false)
{
	for (int i = 0; i < kDNSCacheSize; i++) {
		fCacheEntries[i].fHashPrev = 0;
		fLRU.Enqueue(&fCacheEntries[i]);
	}

	memset(fHash, 0, sizeof(DNSCacheEntry*) * kDNSHashSize);
}

DNSCache::~DNSCache()
{
	//note that KillPendingRequests() may have closed 'fSocket' for us
	if (fShutdown == false)
		close(fSocket);

#if PRINT_STATISTICS
	DumpEntries();
#endif

	delete fCondition;
}

status_t DNSCache::ClearCache()
{
	if (!fLock.Lock()) return B_ERROR;

	printf("*\n*\n*\n*\n*\n*\nClearing cache*\n*\n*\n*\n*\n*\n");
	for (int32 i=0;i<kDNSHashSize;i++) fHash[i] = NULL;
	for (int32 i=0;i<kDNSCacheSize;i++) fCacheEntries[i].fHashPrev = NULL;

	fLock.Unlock();
	return B_OK;
}

void DNSCache::Blackout(bool enterBlackout)
{
	BAutolock _lock(fLock);
	
	if (enterBlackout == fInBlackout) {
		//we're already in the desired state - do nothing
		return;
	}

	//switch to new state
	fInBlackout = enterBlackout;

	//update the default interface's autodial flag as necessary
	if (fDialer.CanAutodial()) {
		struct sockaddr_in addr;
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_len = sizeof(addr);
	
		ifreq_t ifreq;
		if (get_interface_by_route((struct sockaddr *)&addr, &ifreq) == 0) {
	
			if (fInBlackout) {
				if (ifreq.ifr_flags & IFF_AUTOUP)
					clr_iface_flag_by_name(ifreq.ifr_name, IFF_AUTOUP);
			} else {
				if (~ifreq.ifr_flags & IFF_AUTOUP)
					set_iface_flag_by_name(ifreq.ifr_name, IFF_AUTOUP);
			}
		}	
	}
}

status_t DNSCache::GetHostByName(const char *hostname, uint32 *outSockAddr)
{
	status_t retval;
	AddrList list;

	//not too efficient, but...
	if ((retval = GetHostByName(hostname, list)) != B_OK)
		return retval;
	
	*outSockAddr = list[0];
	return B_OK;
}

status_t DNSCache::GetHostByName(const char *hostname, AddrList& outList)
{
#if PRINT_STATISTICS
	fRequests++;
#endif

//HACK!!!
if (fInBlackout) {
	//we're currently blacked out (failing all DNS requests to prevent
	//autodials) - don't bother
	return ENOTCONN;
}

	if ((int)strlen(hostname) > kMaxHostNameLength) {
		//this hostname is too long - don't bother
		return ERANGE;
	}

	if (IsNumericAddress(hostname)) {
		ulong addr = inet_addr(hostname);
		if (addr == 0xfffffffful)
			return B_ERROR;		// Bad address or 255.255.255.255 (oops)
			
		outList.MakeEmpty();
		outList.InsertItem(addr, 0);
		return B_OK;
	}

	if (!fLock.Lock())
		return B_ERROR;

	if (!fInitialized)
		Initialize();

	DNSCacheEntry *entry = 0;

	// See if this entry is in the hash table
	DNSCacheEntry **bucket = &fHash[HashStringI(hostname) % kDNSHashSize];
	for (entry = *bucket; entry; entry = entry->fHashNext)
		if (strcasecmp(hostname, entry->hostname) == 0)
			break;
			
	if (entry == 0 || (!entry->pending && entry->requestCount == 0
		&& (system_time() > entry->expiration || entry->error < 0))) {
		// It is not hashed, or the hashed entry was stale, recycle an old entry
		// and use it for this host
		if (entry == 0)
			entry = (DNSCacheEntry*) fLRU.Dequeue();
		else
			fLRU.RemoveEntry(entry);	// Request count is now positive, make sure
										// it isn't in LRU anymore.  Note that this
										// will check and do nothing if the entry
										// isn't queued

		if (entry == 0) {
			// If this happens,  you have more than kDNSHashSize
			// concurrent requests pending.
			return ENOBUFS;
		}
		
		// Remove this from the hash.  If entry->fHashPrev is 0, it is not
		// hashed.
		if (entry->fHashPrev != 0) {
			*entry->fHashPrev = entry->fHashNext;
			if (entry->fHashNext)
				entry->fHashNext->fHashPrev = entry->fHashPrev;
		}

		// Stick it in the new slot.
		entry->fHashPrev = bucket;
		entry->fHashNext = *bucket;
		if (entry->fHashNext)
			entry->fHashNext->fHashPrev = &entry->fHashNext;
			
		*bucket = entry;

		// Set up the entry
		strcpy(entry->hostname, hostname);
		entry->error = 0;
		entry->retryCount = 0;
		entry->pending = true;
		entry->requestCount = 0;
		
		// Send a request.  This shouldn't block, but it may cause
		// the device to autodial if not currently connected.
		SendRequest(entry, true);
	} else {
		// Request count is now positive, make sure it isn't in LRU anymore.
		// Same as above.
		fLRU.RemoveEntry(entry);
#if PRINT_STATISTICS	
		fHits++;
#endif
	}
	
	entry->requestCount++;

	//
	//	Any thread that is the first to request a DNS entry that isn't cached
	//	will add it to the hash and send a request to the DNS server.  Only one
	//	thread waits on the connection until its entry gets signalled.  It
	//	signals other threads when it receives responses for their entries.
	//
	bool imTheMan = false;
	while (entry->pending) {
		if (!fWaitingOnResponse) {
			imTheMan = true;
			fWaitingOnResponse = true;
		}

		if (imTheMan) {
			fLock.Unlock();
			bool gotResponse = WaitForResponse();
			fLock.Lock();
					
			if (gotResponse)
				fCondition->Signal();
				
			if (fShutdown) {
				entry->requestCount--;
				fLock.Unlock();
				return B_ERROR;
			}
		} else {
			// Someone else is handing socket messages, I have to wait.
			status_t err = fCondition->Wait();	// Releases/Reacquires lock.  See Condition.h
			if (err < B_OK)
				return err;

			if (fShutdown) {
				entry->requestCount--;
				fLock.Unlock();
				return B_ERROR;
			}
		}

		if (system_time() - entry->requestTime >= kDNSRetryTimeout) {
			if (++entry->retryCount == kMaxDNSTries) {
				entry->error = B_TIMED_OUT;
				SignalEntry(entry);
			} else {
				// Try another server - note that we pass 'false' to
				// SendRequest(), which will prevent the machine from
				// autodialing (it's possible that we're not receiving
				// responses because the user downed our connection, in
				// which case we shouldn't re-up it)
				PRINT(("DNS request timed out, retry #%d\n", entry->retryCount));
				SendRequest(entry, false);
			}
		}
	}

	if (imTheMan)
		fWaitingOnResponse = false;

	status_t err = B_OK;
	if (entry->error == 0)
		outList = entry->ips;
	else
		err = entry->error;

	if (--entry->requestCount == 0)
		fLRU.Enqueue(entry);

	fLock.Unlock();
	return err;
}

bool DNSCache::IsNumericAddress(const char *addr)
{
	while (*addr) {
		if (!isdigit(*addr) && *addr != '.')
			return false;
		else
			addr++;
	}
			
	return true;
}

// At some point, this could take multiple names and perform real compression
uchar* compress_qname(uchar */* message */, int /* message_len */, uchar *compressed,
	uchar *name)
{
	while (*name) {
		uchar len = 0;
		while (name[len] != '.' && name[len] != '\0')
			len++;

		*compressed++ = len;		// length of this label
		while (len-- > 0)
			*compressed++ = *name++;

		if (*name == '.')
			name++;
	}

	*compressed++ = '\0';	// terminate with zero length label
	return compressed;
}

uchar* uncompress_qname(uchar *message, int message_len, uchar *compressed, uchar *outname,
	bool indirected = false)
{
	bool first = true;
	for (int len = 0; len < kMaxHostNameLength; len++) {
		if ((*compressed & 0xc0) == 0xc0) {
			if (indirected)
				break;
		
			// Pointer to another part of the message
			ushort index = ((compressed[0] << 8) | compressed[1]) & 0x3fff;
			compressed += 2;
			if (index > message_len) {
				PRINT(("Bad reference.\n"));
				break;
			}

			if (first)
				first = false;
			else
				*outname++ = '.';

			uncompress_qname(message, message_len, message + index, outname, true);
			outname += strlen((const char*) outname);
			break;
		} else {
			// Normal label
			int len = *compressed++;
			if (len == 0)
				break;

			if (compressed + len > message + message_len) {
				PRINT(("Bad label size %d\n", len));
				break;
			}

			if (first)
				first = false;
			else
				*outname++ = '.';
				
			while (len-- > 0)
				*outname++ = *compressed++;
		}
	}
	
	*outname = '\0';
	return compressed;
}

uchar* parse_resource_record(uchar *buffer, uint size, uchar *start, uchar *out_name,
	uint *out_type, uint *out_class, uint *out_ttl, uint *out_len)
{
	start = uncompress_qname(buffer, size, start, out_name);
	*out_type = (start[0] << 8) | start[1];
	*out_class = (start[2] << 8) | start[3];
	*out_ttl = (start[4] << 24) | (start[5] << 16) | (start[6] << 8) | start[7];
	*out_len = (start[8] << 8) | start[9];
	return start + 10;
}

void DNSCache::SendRequest(DNSCacheEntry *entry, bool autodial)
{
	entry->requestTime = system_time();

	uchar message_buffer[kMaxMessageLength];
	memset(message_buffer, 0, kMaxMessageLength);
	message_header *header = (message_header*) message_buffer;
	header->id = htons(entry - fCacheEntries); // Request ID, index into entry array.
	header->flags1 = 1;		// Ask server to recursively look up name for us.  This is important
	header->question_count = htons(1);
	uchar *c = (uchar*)(header + 1);
	c = compress_qname(message_buffer, 0, c, (uchar*) entry->hostname);

	*(short*) c = htons(1);	// type for query (a host address)
	c += 2;
	*(short*) c = htons(1);	// class for query; 1 for Internet.
	c += 2;


#if 0
	// Look up some servers (currently, the binder can hold only two
	// DNS addresses, called "primary" and "secondary")
	const int MAX_SERVERS = 2;
	static const char *serverNames[MAX_SERVERS] = {
		"primary",
		"secondary",
	};

	BinderNode::property_ref dns = BinderNode::Root()/"service"/"network"/"DNS";
	int serverCount = 0;

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_len = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(kNameServicePort);

	for (int i = 0; i < MAX_SERVERS; i++) {

		//note that we don't check whether the binder property is
		//valid/exists, since if it doesn't, inet_addr() will recognize
		//that it's not a valid address
		if ((int)(addr.sin_addr.s_addr =
					inet_addr(dns[serverNames[i]].String().String())) != -1)
		{
			//we have a valid (well-formed) server address - remember
			serverCount++;

			//query the server
			if (sendto(fSocket, message_buffer, c - message_buffer,
							(autodial ? 0 : MSG_DONTWAIT),
							(struct sockaddr*) &addr, sizeof(addr)) < 0)
			{
#if DEBUG
				debugger("Warning: sendto() failed in DNSCache::SendRequest");
#endif
			}
		}
	}

	if (serverCount == 0) {
		entry->error = B_NAME_NOT_FOUND;
		SignalEntry(entry);
	}
#else
	// Look up some servers 
	sockaddr_in servers[2];
	int serverCount;

	if ((serverCount = ParseResolvConf(servers, 2)) == 0) {
		entry->error = B_NAME_NOT_FOUND;
		SignalEntry(entry);
	}

	// Send requests to all servers.  A little faster.
	for (int i = 0; i < serverCount; i++) {
		if (sendto(fSocket, message_buffer, c - message_buffer,
					(autodial ? 0 : MSG_DONTWAIT),
					(struct sockaddr*) &servers[i], sizeof(servers[i])) < 0)
		{
#if DEBUG
			debugger("Warning: sendto() failed in DNSCache::SendRequest");
#endif
		}
	}
#endif
}

bool DNSCache::WaitForResponse()
{
	uchar message_buffer[kMaxMessageLength];
	uchar name_buffer[kMaxHostNameLength + 1];

	struct sockaddr_in addr;
	int addrSize = sizeof(addr);

	//we gave the socket a small receive timeout in Initialize(), so must
	//loop over recvfrom() calls to ensure we wait the proper amount
	//of time
	bigtime_t timeout = system_time() + kDNSRetryTimeout;

	ssize_t responseSize;
	do {
		if (fShutdown == true) {
			//we've been shutdown - bail out early
			return false;
		}

		responseSize = recvfrom(fSocket, message_buffer, 512, 0,
									(struct sockaddr*) &addr, &addrSize);

	} while (responseSize < 0 && errno == B_TIMED_OUT &&
												system_time() < timeout);

	if (responseSize <= 0)
		return false;

	message_header *header = (message_header*) message_buffer;
	int id = (unsigned short) ntohs(header->id);
	if (id > kDNSCacheSize) {
		PRINT(("Bad response ID\n"));
		return false;
	}

	DNSCacheEntry *entry = &fCacheEntries[id];
	if (!entry->pending) {
		// This can happen if we timeout and send another request and
		// the server was just slow.
		PRINT(("Received response for entry that isn't pending\n"));
		return false;
	}

	int questionRecords = ntohs(header->question_count);
	int answerRecords = ntohs(header->answer_count);
	uchar *c = (uchar*) (header + 1);

	int validAnswers = 0;
	uint32 maxTTL = 0xffffffff;

	// Skip questions records
	while (questionRecords-- > 0) {
		c = uncompress_qname(message_buffer, responseSize, c, name_buffer);
		if (strcasecmp((char*) name_buffer, entry->hostname) != 0) {
			PRINT(("Received DNS response for removed entry \"%s\"\n", name_buffer));
			return false;
		} 
			
		c += 4;	// skip type and class
	}

	//assume all answer records are gonna be valid, and reserve enough
	//space for them all
	entry->ips.AssertSize(answerRecords);

	// Check the result code *after* we've verified that the hostname from the
	// response matches that from the entry.
	switch (header->flags2 & 0x0f) {
		case kNoError:
			break;
		case kNotImplemented:
			TRESPASS();
			return false;
		case kFormatError:
			TRESPASS();
			return false;
		case kServerFailure:
			if (++entry->retryCount == kMaxDNSTries) {
				entry->error = B_NAME_NOT_FOUND;
				SignalEntry(entry);
				return true;
			}
			
			PRINT(("Internal server error, retry #%d\n", entry->retryCount));
			SendRequest(entry, false);	// Should try another server.
			return false;
		case kNameError:
			PRINT(("Host \"%s\" not found\n", entry->hostname));
			entry->error = B_NAME_NOT_FOUND;
			SignalEntry(entry);
			return true;
		case kRefused:
			TRESPASS();
			return false;
		default:
			TRESPASS();
	}		

	for (int i = 0; i < answerRecords; i++) {
		uint type, cls, ttl, data_len;
		c = parse_resource_record(message_buffer, responseSize, c, name_buffer,
			&type, &cls, &ttl, &data_len);
		if (type == kHostAddress) {
			entry->ips[validAnswers++] = *(uint32 *)c;

			//we'll expire the entire record as soon as one of it's
			//answers times out
			if (maxTTL > ttl)
				maxTTL = ttl;
		}

		c += data_len;
	}

	if (validAnswers > 0) {
		//we successfully resolved the address - tidy up our cache record
		entry->expiration = system_time() + (maxTTL * 1000000LL);

		//trim unused slots from our array of addresses
		entry->ips.RemoveItems(validAnswers, (answerRecords - validAnswers));

		//let everyone know that we resolved successfully
		entry->error = B_OK;
		SignalEntry(entry);
		return true;

	} else {
		//we were unable to resolve the address
		entry->error = B_NAME_NOT_FOUND;
		SignalEntry(entry);
		return true;
	}
}

void DNSCache::SignalEntry(DNSCacheEntry *entry)
{
	entry->pending = false;
}

void DNSCache::KillPendingRequests(bool quitting)
{
	BAutolock _lock(&fLock);
	fShutdown = true;
	if (quitting) {
		//wagner is quitting for good - close the socket, thereby
		//unblocking any thread which is reading from it (when wagner isn't
		//quitting, we just have to wait for that thread's read() to timeout)
		close(fSocket);
	}
	fCondition->Signal();
}

void DNSCache::ResumeProcessingRequests()
{
	BAutolock _lock(&fLock);
	fShutdown = false;
}

void DNSCache::Initialize()
{

	fInitialized = true;
	fSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (fSocket < 0)
		PRINT(("Failed to create socket: %s\n", strerror(errno)));

	//set receive timeout (we make it short, so we can react to
	//KillPendingRequest() calls quickly)
	struct timeval tv = {1, 0};
	if (setsockopt(fSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) {
		PRINT(("Failed to set receive timeout: %s\n", strerror(errno)));
		close(fSocket);
		fSocket = -1;
	}

	fResolvconfWatcher = new ResolvconfWatcher();
}

void DNSCache::DumpEntries()
{
#if PRINT_STATISTICS
	printf("\nDNS Cache\n");
	printf("   hits/requests: %d/%d (%.2f%%)\n", fHits, fRequests,
		fRequests != 0 ? (float) fHits / (float) fRequests * 100.0: 0.0);
	for (int i = 0; i < kDNSHashSize; i++)
		for (DNSCacheEntry *entry = fHash[i]; entry; entry = entry->fHashNext) {
			uint32 firstAddr = (entry->Count() ? entry->ips[0] : 0);

			printf("   %-25s   %d.%d.%d.%d (rcnt=%d err=%d ttl=%ds)\n", entry->hostname,
				firstAddr & 0xff, (firstAddr >> 8) & 0xff, (firstAddr >> 16) & 0xff,
				firstAddr >> 24, entry->requestCount, entry->error, (entry->expiration
				- system_time()) / 1000000);
		}

#endif	
}

/* "borrow" this from libbind.so */
int DNSCache::ParseResolvConf(struct sockaddr_in *servers, int maxserv)
{
	int rc = 0, fd = -1;
	struct stat st;
	char *buf = 0, *save_ptr = 0, *token = 0;
	
	memset(&st, 0, sizeof(st));
	
	fd = open("/etc/resolv.conf", O_RDONLY);

	if(fd < 0)
		goto out;

	fstat(fd, &st);
	
	if(st.st_size == 0)
		goto out1;

	buf = (char *)malloc(st.st_size + 1);

	if(buf == 0)
		goto out1;
	
	if(read(fd, buf, st.st_size) != st.st_size)
		goto out2;

	//strtok() will actually modify 'buf', so using a non-null-terminated
	//string can cause strtok() to corrupt other memory
	buf[st.st_size] = 0x00;
	
	/*
	 * made it this far, resolv.conf is now in buf, time to parse
	 */

	token = strtok_r(buf, " \t\r\n", &save_ptr);
	
	rc = 0;
	while(token != 0)
	{			
		if(strcmp(token, "domain") == 0)
		{

			token = strtok_r(NULL, " \t\r\n", &save_ptr);

			//just discard the domain value
		}
		else if(strcmp(token, "nameserver") == 0)
		{
			struct sockaddr_in *cur = &(servers[rc]);
			token = strtok_r(NULL, " \t\r\n", &save_ptr);
			
			if (rc < maxserv) {
				cur->sin_len = sizeof(struct sockaddr_in);
				cur->sin_family = AF_INET;
				cur->sin_port = htons(53);
				cur->sin_addr.s_addr = inet_addr(token);
				rc++;
			}
		}
		
		token = strtok_r(NULL, " \t\r\n", &save_ptr);
	}
	
out2:
	free(buf);
out1:
	close(fd);		
out:
	return rc;
}
