#ifndef _PARAMETERS_H
#define _PARAMETERS_H

const int kMaxConnections = 16;
const int kMaxWorkerThreads = 10;
const bigtime_t kIdlePoolThreadDeath = 10000000ll;
const bigtime_t kResourceExpireAge = 30LL * 60LL * 1000000LL;
const bigtime_t kMaxDNSTries = 15;
const bigtime_t kDNSRetryTimeout = 10000000;
const int32 kMaxHistoryEntries = 20;
const int32 kMaxCookies = 300; // Minimum according to <http://www.netscape.com/newsref/std/cookie_spec.html>
const bigtime_t kCacheResizeInterval = 500000;
const int32 kFreeMemoryLowWater = 2 * 1024 * 1024;	
const int32 kFreeMemoryHighWater = 3 * 1024 * 1024;

// This is the amount of memory that the Resource object can cache
// while reading data into a content.  The minimum size for this is
// currently being controlled by how much the JPEG content add-on
// needs to buffer.
const uint kReadBufferSize = 0x8000;

#endif
