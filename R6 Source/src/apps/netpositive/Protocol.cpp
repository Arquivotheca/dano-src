// ===========================================================================
//	ProgressView.cpp
//  Copyright 1998 by Be Incorporated.
// ===========================================================================

#include "Protocols.h"
#include "UResource.h"
#include "Cookie.h"
#include "NPApp.h"
#include "BasicAuth.h"
#include "Cache.h"
#include "MIMEType.h"
#include "Strings.h"
#include "MessageWindow.h"

#include <parsedate.h>
#include <netdb.h>
#include <malloc.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <Autolock.h>

//#ifndef NOSSL
#undef ASSERT
#include "BeSSL.h"
//#endif

#define closesocket close

extern bool gDumpData;
extern bool gCleanDump;
TLocker sDumpLock("Dump locker");

BLooper *Protocol::mDNSLooper = 0;

DNSLooper::DNSLooper()
	: BLooper("DNS Resolver", B_DISPLAY_PRIORITY - 1)
{
}

void DNSLooper::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case msg_GetHostByName: {
			const char *host = msg->FindString("host");
			BMessage reply(msg_Failure);
			if (host) {
				ulong addr = 0;					
				hostent* h = gethostbyname(host);
				
				if (h && h->h_addr) {
					addr = ntohl(*(long *)(h->h_addr));
					reply.what = msg_Success;
					reply.AddInt32("addr", *((int32*)(&addr)));
				}			
			}
			
			msg->SendReply(&reply);
			break;
		}
			
		default:
			BLooper::MessageReceived(msg);
			break;
	}
}

ulong DNSLooper::GetHostByName(const char *host)
{
	static TLocker lock("DNS Locker");
	static BList previousHosts;
	static BList previousAddrs;
	#define hostCacheSize 32

	BAutolock autolock(lock);
	int32 cacheSize = previousHosts.CountItems();
	for (int i = cacheSize - 1; i >= 0; i--) {
		BString* hostnameItem = (BString*)previousHosts.ItemAt(i);
		if (hostnameItem && hostnameItem->Compare(host) == 0) {
			ulong retval = (ulong)previousAddrs.ItemAt(i);
			if (i != cacheSize - 1) {
				previousHosts.RemoveItem(i);
				previousAddrs.RemoveItem(i);
				previousHosts.AddItem(hostnameItem);
				previousAddrs.AddItem((void*)retval);
			}
			return retval;
		}
	}

	// You can call this routine as a thread-safe version of
	// gethostbyname.  You may call this routine re-entrantly
	// as the interface to the DNS looper; it does not rely
	// on the looper being locked or otherwise not busy.
	BMessage msg(msg_GetHostByName);
	BMessage reply;
	ulong retval = 0;

	msg.AddString("host", host);
	BMessenger messenger(NULL, Protocol::mDNSLooper);
	messenger.SendMessage(&msg, &reply);
	ulong hostnumber = 0;
	if (reply.what == msg_Success && reply.FindInt32("addr", (int32*)&hostnumber) == B_OK)
		retval = hostnumber;

	previousHosts.AddItem(new BString(host));
	previousAddrs.AddItem((void*)retval);
	if (cacheSize == hostCacheSize) {
		delete ((BString *)previousHosts.ItemAt(0));
		previousHosts.RemoveItem((int32)0);
		previousAddrs.RemoveItem((int32)0);
	}
	return retval;
}

//=======================================================================
//	Ring buffer for streaming data

Buffer::Buffer(uint32 bufferSize) : mHead(0),mTail(0)
{
	ulong size = 1;
	while (size < bufferSize)
		size <<= 1;
//	NP_ASSERT(size == bufferSize);	// Must be a power of two
	mBufferSize = size;
	mBufferMask = size - 1;
	mBuffer = (char *)malloc(size);
}

Buffer::~Buffer()
{
	if (mBuffer)
		free(mBuffer);
}

//	

void Buffer::Reset()
{
	mHead = 0;
	mTail = 0;
}

//	Returns address of data to read from buffer and its size

void* Buffer::GetReadBuffer(uint32 *maxSize)
{
	if ((mTail & mBufferMask) > (mHead & mBufferMask))	// Only return contig space
		*maxSize = mBufferSize - (mTail & mBufferMask);
	else
		*maxSize = mHead - mTail;

//	NP_ASSERT(mBuffer);
//	NP_ASSERT(mTail <= mHead);
//	NP_ASSERT(*maxSize <= mBufferSize);
	return mBuffer + (mTail & mBufferMask);
}

void Buffer::ReadBuffer(uint32 size)
{
//	NP_ASSERT(mBuffer);
	mTail += size;
}

//	Returns address of write position in buffer and size of contiguous space

void* Buffer::GetWriteBuffer(uint32 *maxSize)
{
//	NP_ASSERT(mBuffer);
	if ((mHead & mBufferMask) >= (mTail & mBufferMask))	// Only return contiguous space
		*maxSize = mBufferSize - (mHead & mBufferMask);
	else
		*maxSize = mBufferSize - (mHead - mTail);
		
//	NP_ASSERT(mTail <= mHead);
//	NP_ASSERT(*maxSize <= mBufferSize);
	return mBuffer + (mHead & mBufferMask);
}

void Buffer::WriteBuffer(uint32 size)
{
//	NP_ASSERT(mBuffer);
	mHead += size;
}

//	Read a line of data, line end is defined by CRLF, CR or LF

long Buffer::ReadLine(char *line, int maxSize)
{
	char *lineEnd = line + maxSize;
	
	char	c,other;
	uint32	p;
//	NP_ASSERT(mBuffer);
	p = mTail;
	while (p < mHead) {
		c = (*line++ = mBuffer[p++ & mBufferMask]);
		// If line == lineEnd, we've reached the end of the buffer but haven't found a line break.
		// It's probably an illegal line.  Let's treat it as EOL and let the caller deal with it.
		if ((c == 0x0D) || (c == 0x0A) || line == lineEnd) {
			line[-1] = 0;		// Got a complete line, strip the 0x0D
			if (p > mHead)
//			if (p == mHead)		// This prevents us from reading the last byte in the buffer?
				break;			// Bounary case, can't have readTail > readHead
			
			other = mBuffer[p & mBufferMask];
			if (((other == 0x0D) || (other == 0x0A)) && other != c)	// Don't strip 0x0A,0x0A or 0x0D,0x0D
				p++;			// And the 0x0A in in 0x0D,0x0A
			mTail = p;		
			return 0;
		}
	}
	return 1;					// No line yet, but not an error
}


bool IsPluginType(const char *contentType)
{
	return strstr(contentType, "application/x-shockwave-flash");
}

bool CanHandleFile(const char *contentType)
{
	if (!contentType)
		return false;
	if (IsPluginType(contentType))
		return true;
		
	for (const FileTypeSpec *candidate = GetCanHandleList(); candidate && candidate->mimeType && *candidate->mimeType; candidate++) {
		if (strcasecmp(candidate->mimeType, contentType) == 0)
			return true;
	}
	
	return false;
}

bool ShouldSniffType(const char *contentType)
{
	return (!contentType || (CanHandleFile(contentType) && !IsPluginType(contentType)) || strstr(contentType, "application/x-scode-UPkg"));
}

Proxy::Proxy()
{
	mActive = false;
	mPort = 0;
	mName = "";
}


void
Proxy::SetActive(
	bool	active)
{
	mActive = active;
}


bool
Proxy::Active() const
{
	return (mActive);
}


void
Proxy::SetHostPort(
	int	port)
{
	mPort = port;
}

void Proxy::SetSecurePort(
	int port)
{
	mSecurePort = port;
}


int
Proxy::HostPort(
	URLParser	&url, bool secure) const
{
	if (mActive)
		return (secure ? mSecurePort : mPort);
	else 
		return (url.HostPort());
}


void
Proxy::SetHostName(
	const BString	&name)
{
	mName = name;
}


const char*
Proxy::HostName(
	URLParser	&url) const
{
	if (mActive)
		return (mName.String());
	else
		return (url.HostName());
}


void
Proxy::GetNameAndPort(
	BString	&name,
	int		*port,
	bool	secure) const
{
	name = mName;
	*port = secure ? mSecurePort : mPort;
}


//=======================================================================

#ifdef DEBUGMENU
const char* NETRESULTSTR(NetResult result)
{
	switch (result) {
		case NET_NO_ERROR:			return "NET_NO_ERROR";
		case NET_NO_SOCKETS:		return "NET_NO_SOCKETS";
		case NET_UNKNOWN_HOST:		return "NET_UNKNOWN_HOST";
		case NET_CANT_CONNECT:		return "NET_CANT_CONNECT";
		case NET_CANT_SEND:			return "NET_CANT_SEND";
		case NET_CANT_RECV:			return "NET_CANT_RECV";
		case NET_TIMEOUT:			return "NET_TIMEOUT";
		case NET_IS_CLOSED:			return "NET_IS_CLOSED";
		case NET_ALREADY_CLOSED:	return "NET_ALREADY_CLOSED";
		case NET_DONT_DOWNLOAD:		return "NET_DONT_DOWNLOAD";
		case NET_FILE_NOT_FOUND:	return "NET_FILE_NOT_FOUND";
		default:
			static char s[256];
			sprintf(s,"Network Error: %d",result);
			return s;		
	}
}
#else
#define NETRESULTSTR(a) ""
#endif

//=======================================================================
//=======================================================================
//	Generic net protocol

Proxy Protocol::sProxy;


Protocol::Protocol(ConnectionManager *mgr, UResourceImp *resImp) : BLooper("Protocol", B_NORMAL_PRIORITY + 2), mConnection(NULL), mResImp(resImp),mDataCount(0), mConnectionMgr(mgr)
{
	mDownload = false;
	mAbortRequested = false;
	mSecure = false;
	mSocketLocked = false;
	mResImp->RefCount(1);	// Protocol is using resource
	if (mConnectionMgr)
		mConnectionMgr->Reference();
}

Protocol::~Protocol()
{
	CloseSocket();
	if (mResImp && mAbortRequested) {
		if (mResImp->LockWithTimeout(TEN_SECONDS) == B_OK && mResImp->GetStatus() != kComplete) {
			mResImp->SetStatus(kAbort);
			mResImp->MarkForDeath();
			pprint("Aborting, marking resource imp 0x%x for death", (unsigned int)mResImp);
		}
		mResImp->Unlock();
	}
	pprint("Protocol 0x%x - deleting", (unsigned int)this);

	if(mResImp)
		mResImp->RefCount(-1);	// Protocol is done with resource

	if (mConnectionMgr)
		mConnectionMgr->Dereference();

	NetPositive::RemoveThread(Thread());
}


// There's a BeOS R3 bug that causes us to leak around 8K of memory
// if we call gethostbyname() from within a BLooper's thread.  This is
// because BLooper's Quit mechanism kills the thread without giving
// it a chance to do its thread cleanup, which, if gethostbyname() is
// called, includes freeing some thread-local data storage.  Until
// this bug is fixed, let's send all gethostbyname() calls through a
// dedicated BLooper, so it will leak this memory only once.  This code
// initializes and tears down this BLooper.

void Protocol::Init()
{
	if (!mDNSLooper) {
		mDNSLooper = new DNSLooper;
		mDNSLooper->Run();
	}
}

void Protocol::Cleanup()
{
	// If the looper is hung up on a request, it won't respond to
	// B_QUIT_REQUESTED.  Since it's fairly harmless, let's kill
	// it hard.  This routine is only called on program exit, so
	// we're not worried about the consequences.
	if (mDNSLooper)
		kill_thread(mDNSLooper->Thread());
}

/*
bool Protocol::QuitRequested()
{
	if (mResImp && mAbortRequested) {
		if (mResImp->LockWithTimeout(TEN_SECONDS) == B_OK && mResImp->GetStatus() != kComplete) {
			mResImp->SetStatus(kAbort);
			mResImp->MarkForDeath();
			pprint("Aborting, marking resource imp 0x%x for death", (unsigned int)mResImp);
		}
		mResImp->Unlock();
	}
	pprint("Protocol 0x%x - quit requested", (unsigned int)this);
	NetPositive::RemoveThread(Thread());
	return true;
}
*/

//	Set the status of the resource

void Protocol::SetStatus(StoreStatus status)
{
	if (!mResImp)
		return;
	
	if (mResImp->LockWithTimeout(TEN_SECONDS) != B_OK) return;
	mResImp->SetStatus(status);
	mResImp->Unlock();
}

//	Incoming message loop

//	Close the socket if a problem arises
				
void Protocol::CloseSocket()
{
	if (mConnection == NULL) return;

	if (mSocketLocked) {
		mConnection->UnlockSocket();
		mSocketLocked = false;
	}
	
	mConnection->Dereference();
	mConnection = NULL;
	if (mConnectionMgr)
		mConnectionMgr->ConnectionFinished(this);
}

void Protocol::Abort()
{
	mConnectionMgr = NULL;
	mAbortRequested = true;
	if (Thread() != B_ERROR) {
		PostMessage(B_QUIT_REQUESTED);
	}
}

void Protocol::RequestConnection()
{
	if (mConnectionMgr)
		mConnectionMgr->RequestConnection(this, mSecure);
}

void Protocol::SetConnectionManager(ConnectionManager *mgr)
{
	if (mConnectionMgr)
		mConnectionMgr->Dereference();
	mConnectionMgr = mgr;
	if (mConnectionMgr)
		mConnectionMgr->Reference();
}


// Get a connection

NetResult Protocol::Connect(bool *reused, const char *proxyName, int proxyPort, bool secure)
{
	if (mConnection == NULL)
		return NET_CANT_CONNECT;
		
	if (mConnection->IsOpen()) {
		if (reused)
			*reused = true;
		return NET_NO_ERROR;
	} else if (reused)
		*reused = false;
			
	const Proxy &theProxy = ProxyObject();

	BString host;
	int port;
	
	if (!proxyName || !(*proxyName) || !proxyPort) {
		BString pName;
		int pPort;
		theProxy.GetNameAndPort(pName, &pPort, secure);
		if (theProxy.Active() && pName.Length() > 0 && pPort) {
			port = theProxy.HostPort(mURL, mSecure);
			host = theProxy.HostName(mURL);
			pprint("Using preference-set proxy %s (%d)", theProxy.HostName(mURL), theProxy.HostPort(mURL, mSecure));
		} else {
			port = mURL.HostPort();
			host = mURL.HostName();
		}
	} else {
		pprint("Using passed proxy %s (%d)", proxyName, proxyPort);
		port = proxyPort;
		host = proxyName;
	}

	SetStatus(kDNS);
	if (host.Length() == 0) {
		pprint("Protocol::Connect Null Host Name");
		return NET_CANT_CONNECT;
	}

//	Do DNS Lookup, only one at a time, please...
	NetResult result = mConnection->Connect(host, port, this);
	if (result != NET_NO_ERROR)
		return result;
		
	SetStatus(kRequest);
	return NET_NO_ERROR;			// Got a connection on the correct port
}

//===========================================================================
//	Read body of data

bool Protocol::SkipBody()
{
	return false;
}

NetResult HTTP::ReadBody()
{
	if (!mResImp)
		return NET_CANT_RECV;
		
	if (!mDownload || mAuthenticating || mRedirecting) {
		pprint("ReadBody - Don't download");
		mResImp->CreateStore();		// need to do this so that others think all is well
		if (!mRedirecting && !mAuthenticating)
			mResImp->NotifyListeners(msg_ResourceChanged);
		if (mConnectionMgr)
			mConnectionMgr->SetConnectionReuse(mConnection, false);
		return (NET_DONT_DOWNLOAD);
	}

	char* data;
	uint32 size;
	int32 recvSize;

	enum {kReadingHeader, kReadingBody, kReadingFooter, kReadingEpilogue, kFinished} chunkState = kReadingHeader;
	
	long bytesRead = 0;
	uint32 chunkSize = 0;
	uint32 chunkRead = 0;
	uint32 chunkHeaderPos = 0;
	char chunkData[256];
	
	if (mFirstByte > 0 && mResImp->GetContentLength() > 0)
		mResImp->SetContentLength(mResImp->GetContentLength() + mFirstByte);
	
	for (;;) {
		do {
			data = (char *)mBuffer.GetReadBuffer(&size);	// Send data from buffer
			if (!data)
				return NET_CANT_RECV;
			if (!mChunked && mResImp->GetContentLength() > 0)
				size = MIN(size, mResImp->GetContentLength() - bytesRead - mFirstByte);
			else if (mChunked && chunkSize > 0 && chunkState == kReadingBody) {
					size = MIN(size, chunkSize - chunkRead);
			}
			if (size) {
				if (!SkipBody())	{	// Dispose Redirected data (or authenticate data)
					status_t status = mResImp->LockWithTimeout(TEN_SECONDS);
//					if (status != B_OK && status != B_TIMED_OUT)
						// If we got B_ERROR, then the resource may have switched backing stores and
						// the old one (with its locker) may have been deleted.  Try again.
//						status = mResImp->LockWithTimeout(TEN_SECONDS);
					if (status != B_OK)
						return NET_CANT_RECV;

					if (mDataCount == 0 && (!mChunked || chunkState == kReadingBody)) {					// Set the file type
						char str[1024];
						str[0] = 0;
						str[1023] = 0;
//						NP_ASSERT(mURL.Path());
						if (ShouldSniffType(mResImp->GetContentType()) && mFirstByte == 0) {
							DetermineFileType(mURL.Path(),0,data,size,str);
							// Don't ever remap text/html to text/plain.  The sniffer could be wrong.
							// Rendering actual plain text as HTML is (relatively) harmless.  Rendering
							// HTML as plain text is very harmful, especially in a WWW browser.

							pprint("Resource is %s, sniff results are %s", mResImp->GetContentType(), str);
							if (mResImp->GetContentType() == NULL ||
								(strcasecmp(mResImp->GetContentType(), "text/html") != 0 ||
								strcasecmp(str, "text/plain") != 0) &&
								strcasecmp(mResImp->GetContentType(), str) != 0) {
									mResImp->SetContentType(str);
									pprint("Resetting content type of %s to %s", mURL.Path(), str);
							}
							
							// See if we can handle the new content-type.  Despite what CanHandleFile
							// says, we can't handle application/octet-stream; it returns true so that
							// we can download it and MIME sniff it to see if it's really a GIF or something
							// we can handle, but if the MIME sniffer throws up its hands, we should just stuff
							// it in a file.  
							if (!CanHandleFile(str) || strstr(str, "application/octet-stream")) {
								mResImp->SetCacheOnDisk(false);
							}
						}
						if (mFirstByte > 0) {
							pprint("Seeking output file to %d\n", mFirstByte);
							mResImp->Seek(mFirstByte);
						}
					}
					
					long written = 0;
					
					if (mChunked && chunkState != kReadingBody) {
						uint32 dataSize = MIN(255 - chunkHeaderPos, size);
						memcpy(chunkData + chunkHeaderPos, data, dataSize);
						chunkData[dataSize + chunkHeaderPos] = 0;
						char *crlfPos = strstr(chunkData, "\r\n");
						if (!crlfPos) {
							// We coudn't find a CRLF in the data.  We'll try to keep reading until we get
							// one.  Ideally, we should return 0 for the size, let the buffer refill, and then
							// try reading again, but the buffer is a ring buffer, and if we've hit the end of
							// it, we won't get any more data from it until we read up to the end of the buffer
							// and then let it refill so it can tell us to start reading from the beginning of it
							// again.  Sooooo, let's grab all of the bytes that are left in the buffer, store them
							// temporarily, and then have another go at it, appending the bits we get the next time
							// to the end of the bits we're remembering here.
							if (dataSize < 255) {
								size = dataSize;
								chunkHeaderPos = dataSize;
							} else {
								// We're in real trouble here.  We should have found a CRLF but couldn't.
								size = 0;
								chunkHeaderPos = 0;
							}
						} else {
							size = (crlfPos - chunkData + 2 - chunkHeaderPos);
							chunkHeaderPos = 0;
							*crlfPos = 0;
	
							if (chunkState == kReadingHeader) {							
								// The header for a chunk consists of a hex length string (and optional chunk
								// extensions, which we ignore, and a CRLF.  If the buffer doesn't contain a
								// CRLF, then punt on reading until we get more data in.
								chunkState = kReadingBody;
								
								// Later on, we will add size to chunkRead, but this is incorrect, since size
								// represents the size of this header.  Subtract it off now to make it come
								// out even.
								chunkRead = -size;
	
								// Strip off any chunk extensions.  We don't care.
								char *semicolonPos = strchr(chunkData, ';');
								if (semicolonPos)
									*semicolonPos = 0;
									
								chunkSize = 0;		
								char *c = chunkData;
								while (*c) {
									char hexDigit = tolower(*c++);
									if (!((hexDigit >= '0' && hexDigit <= '9') || (hexDigit >= 'a' && hexDigit <= 'f')))
										break;
									chunkSize = chunkSize * 16 + ((hexDigit >= 'a') ? (hexDigit - 'a' + 10) : (hexDigit - '0'));
								}
								if (chunkSize == 0)
									chunkState = kReadingEpilogue;
							} else if (chunkState == kReadingFooter)
								chunkState = kReadingHeader;	
							else if (chunkState == kReadingEpilogue) {
	
								// Throw away all of the epilogue data and scan until we hit a bare CRLF.
								if (crlfPos == chunkData)
									chunkState = kFinished;
							}
						}
					} else
//						NP_ASSERT(mResImp->Write(data,size, false) == 0);
						written = mResImp->Write(data,size, false);

					mDataCount += size;
					mResImp->Unlock();
					
					if (written < 0) {
						mAbortRequested = true;
						return NET_CANT_RECV;
					}
				}
				mBuffer.ReadBuffer(size);
				bytesRead += size;
				if (mChunked) {
					chunkRead += size;
					if (chunkState == kReadingBody && chunkSize > 0 && chunkRead >= chunkSize)
						chunkState = kReadingFooter;
				}
			}
		} while (size && (!mChunked || chunkState != kFinished));
		
		mResImp->NotifyListeners(msg_ResourceChanged);
//		If nobody wants this resource, stop loading it, don't cache it
		status_t status = mResImp->LockWithTimeout(TEN_SECONDS);
//		if (status != B_OK && status != B_TIMED_OUT)
			// If we got B_ERROR, then the resource may have switched backing stores and
			// the old one (with its locker) may have been deleted.  Try again.
//			status = mResImp->LockWithTimeout(TEN_SECONDS);
		if (status != B_OK)
			return NET_CANT_RECV;
		if (mResImp->RefCount(0) == 1) {
			pprint("Cancel Download of '%s'",mResImp->GetURL());
			mResImp->MarkForDeath();
			mResImp->Unlock();
			return NET_CANT_RECV;
		}
		mResImp->Unlock();

//		Read more data from net, write it into the buffer
		long contentLength = mResImp->GetContentLength();
		if ((!mChunked && contentLength > 0 && bytesRead >= contentLength - mFirstByte) ||
			(mChunked && chunkState == kFinished)) {
			return NET_NO_ERROR;
		}

		recvSize = mConnection->FillBuffer(mBuffer);					// Read data into buffer
		if (recvSize == 0 && mFirstByte == 0) {
			if (mChunked)
				// We didn't receive a valid end block, but give up and display what we have.
				mResImp->SetContentLength(mDataCount + mFirstByte);
			else if (mResImp->GetContentLength() == 0)
				mResImp->SetContentLength(mDataCount + mFirstByte);
			return NET_NO_ERROR; 
		}
		if (recvSize < 0)
			return NET_CANT_RECV;			
	}
}

//	Note.. FillBuffer returns number of bytes read, not an error

long Protocol::FillBuffer(int socket, Buffer& buffer)
{
	char* data;
	uint32 bufSize;
	data = (char *)buffer.GetWriteBuffer(&bufSize);	// read data into buffer
	if (data == NULL || bufSize == 0) 
		return 0;
	
	if (mAbortRequested) {
		pprint("Abort request for socket %d", socket);
		return -1;
	}
	
	int32 size = bufSize;
	size = recv(socket,data,size,0);
	if (size == -1)
		pprint("Errno %x", errno);

#if 0
	char *buff = (char *)malloc(size + 1);
	memcpy(buff, data, size);
	buff[size] = 0;
char *foo = buff;
while (*foo) {
	if (*foo == '\n') *foo = 'N';
	if (*foo == '\r') *foo = 'R';
	foo++;
}
	printf("\nRaw data: %s\n\n", buff);
	free(buff);
#endif

	
	if (size > 0) {
		buffer.WriteBuffer(size);
	}
	return size;
}

// Wait until a complete line of data arrives

NetResult Protocol::ReadLine(char *str, int maxLen)
{
	for (;;) {
		if (mBuffer.ReadLine(str, maxLen) == 0)	// Is there a complete line in buffer?
			return NET_NO_ERROR;			// Yes
			
		if (!mConnection || mAbortRequested)
			return NET_ERROR;

		long size = mConnection->FillBuffer(mBuffer);	// Otherwise read some more data
		if (size <= 0)
			return NET_CANT_RECV;			// Can't read another line...
	}										// And try again..
}


const Proxy&
Protocol::ProxyObject()
{
	return (sProxy);
}

void Protocol::CreateErrorMessage(NetResult result, BString &message)
{
	const char *extra = mURL.HostName();
	switch (result) {
		case NET_UNKNOWN_HOST:		message = GetErrorUnknownHost(); break;
		case NET_TIMEOUT:			message = GetErrorTimeout(); break;
		case NET_REFUSED:			message = GetErrorRefused(); break;
		case NET_UNAUTHORIZED:		message = GetErrorUnauthorized(); break;
		case NET_FILE_NOT_FOUND:	message = GetErrorFileNotFound(); extra = mURL.Path(); break;
		default:					message = GetErrorGeneric(); break;
	}
	
	if (extra == NULL)
		extra = "";

	message.ReplaceFirst("%s", extra);
}


//=======================================================================
//=======================================================================
//	HTTP Protocol

Proxy HTTP::sProxy;

HTTP::HTTP(ConnectionManager *mgr, UResourceImp *res, bool neverReuse) : 
	Protocol(mgr, res),mRedirecting(false),mAuthenticating(false),mUseCache(false),mRealm(""),
	mFormData(""), mChunked(false), mNeverReuse(neverReuse) //, mRes(res)
{
	mSentAuthentication = false;
	mResourceIsCached = false;
	NetPositive::AddThread(Run());	// Protocol base class does not run by itself
}

void
HTTP::SetProxyActive(
	bool	active)
{
	sProxy.SetActive(active);
}

void
HTTP::SetProxyNameAndPort(
	const BString	&name,
	int		port,
	int		securePort)
{
	sProxy.SetHostName(name);
	sProxy.SetHostPort(port);
	sProxy.SetSecurePort(securePort);
}

bool
HTTP::ProxyActive()
{
	return (sProxy.Active());
}

void
HTTP::GetProxyNameAndPort(
	BString	&name,
	int		*port,
	int		*securePort)
{
	sProxy.GetNameAndPort(name, port, false);
	sProxy.GetNameAndPort(name, securePort, true);
}

//	Send a request

NetResult HTTP::Request(const char *formData, const char *authorization, const char *, const char *referrer, const char *proxyName, int proxyPort)
{
	static	char *HTTPVERS = 		" HTTP/1.1\r\n";
//	static	char *USER_AGENT =		"User-Agent: Mozilla/3.0 (compatible; NetPositive; BeOS)\r\n";
	static	char *ACCCEPT =			"Accept: image/gif, image/jpeg, image/pjpeg, image/png, */*\r\n";
	static 	char *HOST = 			"Host: ";
	static	char *AUTHORIZATION =	"Authorization: Basic ";

//	static	char *POST = 			"POST %s HTTP/1.0\r\n";
	static	char *CONTENT_TYPE = 	"Content-type: application/x-www-form-urlencoded\r\n";
	static	char *CONTENT_LENGTH =	"Content-length: %d\r\n";
	static	char *REFERRER =		"Referer: ";
	static	char *IF_MODIFIED =		"If-Modified-Since: %a, %d %b %Y %H:%M:%S GMT\r\n";
	static  char *RANGE =			"Range:bytes=%d-\r\n";

	long formDataSize = 0;
	BString	req;
	BString path;
	
	BString proxName(proxyName);
//	int proxyPort;
	if (!proxyName || !(*proxyName) || !proxyPort)
		ProxyObject().GetNameAndPort(proxName, &proxyPort, mSecure);
	mURL.WriteURL(path, ProxyObject().Active() && proxName.Length() > 0 && proxyPort);		// Only include host if this is going thru a proxy
	if (path.Length() == 0)
		path = "/";

	static BString USER_AGENT;
	if (USER_AGENT.Length() == 0) {
		const char *prefBrowserString = gPreferences.FindString("BrowserString");
		USER_AGENT = "User-Agent: ";
		if (prefBrowserString && *prefBrowserString)
			USER_AGENT += prefBrowserString;
		else {
			char browserString[128];
			sprintf(browserString, kDefaultBrowserString,GetVersionNumber());
			USER_AGENT += browserString;
		}
		USER_AGENT += "\r\n";
	}

//	GET or POST a request
//	if this was a redirected POST, it will now be a GET.  Against spec but copies other browsers.
	if (mHeadersOnly)
		req = "HEAD ";
	else if (formData == NULL || *formData == 0)
		req = "GET ";
	else
		req = "POST ";
		
	req += path;
	req += HTTPVERS;			// HTTP/1.0
	req += USER_AGENT;			// User-Agent: Mozilla/NetPositive0.75
	req += ACCCEPT;				// Accept: text/html, image/jpeg, image/gif
	req += HOST;
	req += mURL.HostName();
	req += "\r\n";

	if (referrer && *referrer) {
		req += REFERRER;
		req += referrer;
		req += "\r\n";
	}
	
	UResourceImp *resource = UResourceCache::Cached(mResImp->GetURL(), true);

	if (resource && (resource->GetCacheName() != NULL || resource != mResImp)) {
		char str[256];
		struct tm t;
		time_t timet = time(NULL);
		gmtime_r(&timet, &t);
		strftime(str, 256, IF_MODIFIED, &t);
		req += str;
	}

	if (resource)
		resource->RefCount(-1);
	
	if (mRangeBegin > 0) {
		char str[256];
		sprintf(str, RANGE, mRangeBegin);
		req += str;
	}
	
	if (authorization && *authorization) {
		req += AUTHORIZATION;	// Authorization: Basic %s
		req += authorization;

		// Nibble off any trailing \r or \n characters to get it in a consistent state.
		char c = 0;
		int length;
		do {
			length = req.Length();
			if (length > 0) {
				c = req[length - 1];
				if (c == '\r' || c == '\n')
					req.Truncate(--length);
			}
		} while ((c == '\r' || c == '\n') && length);

		req += "\r\n";
//		mRealm = realm;
	}
	
	Cookie::Add(mURL,req);		// Add any cookies for this domain/path

	if (formData != NULL && *formData) {
		req += CONTENT_TYPE; 
		char str[256];
		formDataSize = strlen(formData);
		sprintf(str,CONTENT_LENGTH,formDataSize);	// Content-length: %d
		req += str;
		req += "\r\n";
		req += formData;
		
/*
		if (!mURL.Query() || !(*(mURL.Query()))) {
			// This isn't right.  In POST requests, we mustn't append the form data
			// to the end of the URL, because it will then show up in the URL of the following
			// page, which it isn't supposed to do.  Also, if you clone the current page, it
			// will re-send the request (as a GET request, however), which it isn't supposed to do --
			// POST requests aren't idempotent, and we shouldn't submit them more than once.
			// However, we need to have a unique URL attached to the resource, or caching and the
			// history won't work right.  For now, we'll do it wrong here and special-case it
			// elsewhere.
			BString fullURL;
			mURL.WriteURL(fullURL, true);
			fullURL += "?";
			fullURL += formData;
			pprint("Setting resource URL to %s", fullURL.String());
			UResourceCache::ChangeURL(mResImp, fullURL.String());
		}
*/
	} else
	req += "\r\n";

	pprint("HTTP Request: Protocol 0x%x, connection 0x%x", this, mConnection);
	pprint("HTTP Request: %s", req.String());
	
	NetResult result = NET_NO_ERROR;
	int foo;
//	foo = send(mConnection->mSocket, req, req.Length(), 0);
	foo = mConnection->Send(req.String(), req.Length());
	if (foo < 0) {
			pprint("Socket send error %d", errno);
		result = NET_CANT_SEND;
	}
	return result;
}

//===========================================================================

//	Incoming message loop

void HTTP::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case NET_GET_URL: {
			bool download = false;
			if (msg->FindBool("download", &download) == B_NO_ERROR)
				mDownload = download;

			const char *url = msg->FindString("url");
			if (url)
				mURL.SetURL(url);

			pprint("NET_GET_URL %s", url);

			const char *formData = msg->FindString("formData");
			const char *authorization = msg->FindString("authorization");
			const char *realm = msg->FindString("realm");
			const char *referrer = msg->FindString("referrer");
			const char *proxyName = msg->FindString("proxyName");
			mHeadersOnly = msg->FindBool("headersOnly");
			mProxyPort = msg->FindInt32("proxyPort");
			mSecure = msg->FindBool("secure");
			mRangeBegin = msg->FindInt32("rangeBegin");
			mNeverReuse = msg->FindBool("neverReuse");
			
			if (formData != NULL)
				mFormData = formData;
				
			if (authorization != NULL)
				mAuthorization = authorization;
				
			if (realm != NULL)
				mRealm = realm;
				
			if (referrer != NULL)
				mReferrer = referrer;
				
			if (proxyName != NULL)
				mProxyName = proxyName;
			else
				mProxyPort = 0;

			RequestConnection();
			break;
		}
			
		case NET_CONNECTION_AVAIL: {
			pprint("Received NET_CONNECTION_AVAIL");

			msg->FindPointer("ConnectionInfo", (void **)&mConnection);
//			mConnection->Reference();
			
			NetResult result = NET_NO_ERROR;
			mBuffer.Reset();
			
			bool reused = false;
			result = Connect(&reused, mProxyName.String(), mProxyPort, mSecure);
			if (result == NET_NO_ERROR) {
				result = ProcessRequest(mFormData.String(), mAuthorization.String(), mRealm.String(), mReferrer.String(), mProxyName.String(), mProxyPort);
			} else {
				BString message;
				CreateErrorMessage(result, message);
				mResImp->SetErrorMessage(message);
				pprint("HTTP: Connect - %s",NETRESULTSTR(result));
			}
			
			if ((result == NET_CANT_SEND || result == NET_CANT_RECV) && reused && mConnectionMgr) {
				mConnectionMgr->SetConnectionReuse(mConnection, false);
				CloseSocket();
				RequestConnection();
				break;
			}
				
			if ((mRedirecting || mAuthenticating) && result == NET_NO_ERROR)
				;						// Wait for redirection before reporting result
			else if (mResImp) {
				mResImp->Flush(result == NET_NO_ERROR ? kComplete : kError);
				mResImp->SetUpToDate(result == NET_NO_ERROR);
			}

			CloseSocket();

			if (!(mRedirecting || mAuthenticating)) {
				Quit();
			}
			break;
		}
		default:
			BLooper::MessageReceived(msg);
	}
}

NetResult HTTP::ProcessRequest(const char *formData, const char *authorization, const char *realm, const char *referrer, const char *proxyName, int proxyPort)
{
	NetResult result;
	
	if (!mConnection->LockSocket())
		return NET_CANT_SEND;
		
	mSocketLocked = true;

	result = Request(formData,authorization,realm, referrer, proxyName, proxyPort);
//	if (!mConnection->LockSocket())
//		return NET_CANT_CONNECT;
		
	if (result == NET_NO_ERROR) {
		result = Response();
		if (result == NET_NO_ERROR) {
			if (authorization && realm)
				gBasicAuthFolder->Good(realm);
			if (mHeadersOnly || mResourceIsCached) {
				SetStatus(kComplete);
				return NET_NO_ERROR;
			}
			result = ReadBody();
			if (result != NET_NO_ERROR) {
				pprint("HTTP: ReadBody - %s",NETRESULTSTR(result));
				if (result == NET_DONT_DOWNLOAD)
					result = NET_NO_ERROR;	// this is the same, really...
				else
					mResImp->MarkForDeath();
			}
		} else {
			pprint("HTTP: Response - %s",NETRESULTSTR(result));
			if ((mRedirecting) || (mUseCache))
				result = NET_NO_ERROR;	// Ignore response error if we are going to redirect anyway
		}
	} else
		pprint("HTTP: Request - %s",NETRESULTSTR(result));
		
	return result;
}

NetResult HTTP::Response()
{
	char response[MAX_URL_LENGTH];
	char *field;
	NetResult result;
	
	bool setContentType = false;

//	Read response code

	mRedirecting = false;
	mAuthenticating = false;
	mSentAuthentication = false;
	mFirstByte = 0;
	bool setLastModified = false;
	
	mResponseCode = 0;
	
	while (mResponseCode == 0 || mResponseCode == 100) {
		result = ReadLine(response, MAX_URL_LENGTH);
		if (result != NET_NO_ERROR) {
//			CloseSocket();
			return result;
		}
		pprint("HTTP Response: %s",response);	
	
		// I think if the reponse code is 100 then we can go ahead and try pipelining requests.
		// However, this isn't implemented yet.

		mResImp->SetLastVisited(time(NULL));
	
		long majorV = 1;
		long minorV = 0;
		if (sscanf(response,"HTTP/%ld.%ld %ld",&majorV,&minorV,&mResponseCode) != 3 &&
			sscanf(response,"HTTP %ld",&mResponseCode) != 1 && mResponseCode != 100) {
			if (strlen(response) == 0)
				continue;
			mDownload = true;
//printf("%5d SetType   0x%x\n", find_thread(NULL), mResImp);
			mResImp->SetContentType("text/html");
			setContentType = true;
			return NET_NO_ERROR;							// Might just be sending the data?
		}
		if (majorV >= 1 && minorV >= 1) {
			pprint("HTTP 1.1 server.  Setting connection reuse to true");
			if (mConnectionMgr && !mNeverReuse)
				mConnectionMgr->SetConnectionReuse(mConnection, true);
		}	
	}
	
	pprint("Response code: %d", mResponseCode);

	if (mResponseCode >= 400) {
		if (mResponseCode == 401) {
			// Authentication
			mAuthenticating = true;			// WWW-Authenticate

			if (mRealm.Length() > 0) {		
				// Get the realm this request is authorized for (if any)
//				char str[1024];
//				sprintf(str,GetErrorUnauthorized(),mRealm.String());
				BString str = GetErrorUnauthorized();
				str.ReplaceFirst("%s", mRealm.String());

				//stuff the error message into the resource so that HTMLView::QualifyNewDoc() will display it
				if(mResImp)
					mResImp->SetErrorMessage(str.String());

				// Tried to use a password on a realm, it failed
				gBasicAuthFolder->Bad(mRealm.String());
				
				mRealm = "";

				mAuthenticating = false;	

				UResourceCache::Purge(mResImp->GetURL());
			}
		} 
/* Allow download of all 40x error pages, including 404's.  The server usually sends
   some sort of error HTML page that we want to display.
		else {
			if (mResponseCode != 404) {				
				BString path;
				mURL.WriteURL(path);
				if (path.Length() == 0)
					path = "/";
				pprint("HTTP %d For: '%s'",mResponseCode,path.String());					// Don't cache errors
				mResImp->MarkForDeath();
			}
		}
*/
		
	} else if (mResponseCode == 302) {
		pprint("REDIRECTION");
		mRedirecting = true;
	} else if (mResponseCode == 304) {
		// This is in reponse to a conditional GET set up with a If-Modified-Since field.
		// If the server sends us back a 304, then the object hasn't changed, so get the
		// cached version.  Do this by setting the modification date of the resource we're
		// reading into to the modification date of the cached version; we will automatically
		// pick this up, and swap in the cached resource.
		UResourceImp *resource = UResourceCache::Cached(mResImp->GetURL(), true);
		if (resource && (resource->GetCacheName() != NULL || resource != mResImp) && mResImp->LockWithTimeout(TEN_SECONDS) == B_OK) {
			NetResult result = SetLastModified(resource->GetLastModified());
			if (mResImp)
				mResImp->Unlock();
			resource->RefCount(-1);
			mResourceIsCached = true;
			return result;
		} else
			return NET_CANT_RECV;
	}

//	Read rest of header

	BString contentType = "";

	for (;;) {
		result = ReadLine(response, MAX_URL_LENGTH);

		if (result != NET_NO_ERROR)
			return result;

		if (response[0] == 0) {	// "" string means start of body 
			if (!setContentType) {
				pprint("Content type was never set.  Assuming text/html and setting Download to true");
				contentType = "text/html";
				mDownload = true;
			}
			
			if (contentType.Length() > 0 && mRangeBegin == 0)
{
//printf("%5d SetType   0x%x\n", find_thread(NULL), mResImp);
				mResImp->SetContentType(contentType.String());
}
			if (!setLastModified && !mHeadersOnly && mResImp->LockWithTimeout(TEN_SECONDS) == B_OK) {
				SetLastModified(time(NULL));
				if (mResImp)
					mResImp->Unlock();
			}
			
			return NET_NO_ERROR;
		}

		pprint("HTTP: %s",response);
			
//		Convert Header field types to lower case, split field from response

		field = response;
		while (field[0] && field[0] != ':') {
			if (isspace(*field))
				*field = 0;
			else
				*field = tolower(*field);
			field++;
		}
		if (field[0] == ':') {
			*field++ = 0;
			while (isspace(*field)) field++;
		}
		
//		Handle relocation....

		if (strstr(response,"location")) {
			if (mResponseCode >= 300 && mResponseCode < 400)	// Redirection
				Redirect(field);
		}
		
//		Handle authentication

		if (mAuthenticating && strstr(response,"www-authenticate"))
			if(Authenticate(field) == NET_AUTH_CANCELED){
				pprint("HTTP: Received a cancel from Authenticate");
				if(mResImp)
					mResImp->SetErrorMessage(SHOW_NO_ERROR);
			}

		if (mRedirecting || mAuthenticating) {				// Ignore rest of redirection body
			if (strstr(response,"set-cookie"))	{			// Remember a cookie on redirection
				pprint("Redirection Cookie: %s",field);
				Cookie::Set(mURL,field);
			} else 
				pprint("Ignoring: %s",response);
			continue;
		}
		
//		Interpret header field

		if (mResImp->LockWithTimeout(TEN_SECONDS) != B_OK) return NET_CANT_RECV;

		if (strstr(response,"date"))
			mResImp->SetDate(parsedate(field, -1));			// date
			
		else if (strstr(response,"last-modified")) {
			NetResult err = SetLastModified(parsedate(field, -1));
			setLastModified = true;
			if (err != NET_NO_ERROR) {
				if (mResImp)
					mResImp->Unlock();
				return (err);
			}
		}

		else if (strstr(response,"expires"))
			mResImp->SetExpires(parsedate(field, -1));			// expires
			
		else if (strstr(response,"content-type")) {			// content-type
		pprint("HTTP: %s: %s",response, field);
			contentType = MimeType::RepairMimeType(field);
			setContentType = true;

			if (CanHandleFile(contentType.String()))
				mDownload = true;	// force the download
			else
				mResImp->SetCacheOnDisk(false);
		}
		
		else if (strstr(response,"content-range")) {
			if (mRangeBegin == 0)
				pprint("Got content-range when we didn't request one!");
			else {
				// The content-range will look something like 500-7999/8000,
				// meaning you're getting bytes 500-7999 out of an 8000-byte file
				// (where the file starts at zero; 7999 is the last byte).  A
				// sscanf will pick up the 500 portion of this range, which is
				// what we're interested in.
		pprint("HTTP: %s: %s",response, field);
				sscanf(field,"%*s %ld-",&mFirstByte);
				pprint("Got content-range.  Setting mFirstByte to %d", mFirstByte);
			}
		}
		
		else if (strstr(response,"pragma:no-cache") ||
				 strstr(response,"cache-control:no-cache"))
					mResImp->SetExpires(time(NULL));
					
		else if (strstr(response,"cache-control:no-store"))
					mResImp->SetCacheOnDisk(false);

		else if (strstr(response,"content-length")) {		// content-length
		pprint("HTTP: %s: %s",response, field);
			long length;
			sscanf(field,"%ld",&length);
			mResImp->SetContentLength(length);
		}
		
		else if (strstr(response,"set-cookie"))				// Remember a cookie
			Cookie::Set(mURL,field);
		
//		else if (strstr(response,"mime-version"))			// Don't take notice of these yet
//			;
//		else if (strstr(response,"server"))
//			;
//		else if (strstr(response,"message-id"))
//			;
		else if (strstr(response,"connection") && strstr(field,"close") && mConnectionMgr) {
			mConnectionMgr->SetConnectionReuse(mConnection, false);
		} else if (strstr(response,"transfer-encoding") && strstr(field, "chunked"))
			mChunked = true;
		else
			pprint("Protocol: %s: %s",response,field);		// Mention any strange ones

		mResImp->Unlock();
	}

	if (!setContentType) {
		pprint("Content type was never set.  Assuming text/html and setting Download to true");
		contentType = "text/html";
		mDownload = true;
	}
	
	if (contentType.Length() > 0 && mFirstByte == 0)
{
//printf("%5d SetType   0x%x\n", find_thread(NULL), mResImp);
		mResImp->SetContentType(contentType.String());
}
		
	if (!setLastModified && !mHeadersOnly)
		SetLastModified(time(NULL));

	return (NET_NO_ERROR);
}

//	Response has redrected the request elsewhere...
//	We need to start all over...

NetResult HTTP::Redirect(char *url)
{
	pprint("HTTP::Redirect %s", url);

	if (url[0] == 0) {
		mRedirecting = false;
		return NET_CANT_CONNECT;
	}
	
	BString URL = url;
	// If the URL contains a question mark, then discard it and
	// everything that follows -- we still have the form data and
	// will add it separately.  Only discard it if it differs from
	// our form data, though.
	char *questionPos = strchr(URL.String(), '?');
	if (questionPos && mFormData.Length() > 0 && mFormData == (questionPos + 1))
		URL.Truncate(questionPos - URL.String());

	//we will copy other browsers and turn a redirect from a POST into a GET
	//this is against the spec but compliant with the rest of the world
	mFormData = "";
	
	URLParser parser;
	parser.SetURL(URL.String());
	
	if (parser.Scheme() != kNOSCHEME)
		mSecure = (parser.Scheme() == kHTTPS);
	else {
		pprint("Inherit old security of %d\n", mSecure);
	}
	pprint("Security is %d\n", mSecure);
	
	parser.BuildURL(URL,mURL);
	if (URL.Length() == 0)
		return NET_CANT_CONNECT;
	
	pprint("Redirecting to '%s'",URL.String());
	mRedirecting = true;
	
//	Post a request for the redirected data...
	BMessage msg(NET_GET_URL);
	msg.AddString("url",URL.String());
	msg.AddBool("secure", mSecure);
	if (mRealm.Length() > 0) {
		const char *auth = gBasicAuthFolder->Get(mRealm.String());

		if (auth != NULL) {
			msg.AddString("authorization",auth);
			msg.AddString("realm",mRealm.String());
		}
	}

//  Bug 7713
//  This will cause us to incorrectly deal with sites that do password
//  authentication with forms, cookies, and HTTP redirect.  developer.javasoft.com
//  is such a site.
//	if (mFormData.Length() > 0)
//		msg.AddString("formData", (char *)mFormData);
	PostMessage(&msg);
	
//	Update the URL in the resource...
//	Redirected data may be in the cache and won't work...

	if (mResImp->LockWithTimeout(TEN_SECONDS) != B_OK)
		return NET_CANT_CONNECT;
	UResourceCache::ChangeURL(mResImp, URL.String());
	mResImp->Unlock();
	
//	Should the respose be requeued at a top level? ¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥¥

	return NET_NO_ERROR;
}

//	Authentication has been requested, ask the user for a password and start again...
//	We need to start all over...

NetResult HTTP::Authenticate(char *field)
{
	char* realm = NULL;
	if (mSentAuthentication)
		return NET_NO_ERROR;
	
	while ((field[0] != '\0') && (field[0] != '='))
		field++;

	if (field[0] == '=') {
		field++;

		if (field[0] == '"')
			field++;

		realm = field;

		while ((field[0] != '\0') && (field[0] != '"'))
			field++;

		if (field[0] == '"') 
			field[0] = 0;

		const char *auth = gBasicAuthFolder->Get(realm);
		if (auth) {
			// Post a request for the authorized data...
			BMessage msg(NET_GET_URL);
				
			BString URL;
			mURL.WriteURL(URL);
			
			// If the URL contains a question mark, then discard it and
			// everything that follows -- we still have the form data and
			// will add it separately.
// This is no longer necessary.  We now do a better job of discriminating between
// GET and POST and no longer include the POST data in the URL, at least here.
//			char *questionPos = strchr(URL.String(), '?');
//			if (questionPos && mFormData.Length() > 0)
//				URL.Truncate(questionPos - URL.String());
				
			msg.AddString("url",URL.String());

			pprint("Authenticating '%s'",realm);
			msg.AddString("authorization",auth);
			msg.AddString("realm",realm);
			if (mFormData.Length() > 0)
				msg.AddString("formData", mFormData.String());
			PostMessage(&msg);
			mSentAuthentication = true;
				
			return (NET_NO_ERROR);
		}
	}
	
	pprint("Authentication of '%s' was cancelled",realm);
	mResImp->MarkForDeath();
	mAuthenticating = false;
	mSentAuthentication = false;

	return (NET_AUTH_CANCELED);
}

//	If a rediection or an authetication failure occurs, we want to ignore the body data

bool HTTP::SkipBody()
{
	return mAuthenticating || mRedirecting;
}


NetResult
HTTP::SetLastModified(
	long	modified)
{
	bool stillAlive = true;

	mResImp->SetLastModified(modified);

	UResourceImp *resource = UResourceCache::Cached(mResImp->GetURL(), true);
	if (resource != NULL) {
		UResourceImp *cachedImp = resource;

		if (cachedImp && (cachedImp->GetCacheName() != NULL || cachedImp != mResImp)) {
			if (cachedImp->GetLastModified() < mResImp->GetLastModified()) {
				cachedImp->MarkForDeath();
				UResourceCache::Add(mResImp);
			} else {
				stillAlive = mResImp->RefCount(-1) > 0;	// unref as per Protocol::~Protocol
				if (stillAlive) {
					UResourceImp *oldImp = mResImp;
					if (mResImp->IsLocked())
						mResImp->Unlock();			// mResImp locked before this function
					mResImp = cachedImp;		// we want the cached imp
					if (mResImp->LockWithTimeout(TEN_SECONDS) != B_OK) return NET_CANT_RECV;			// mResImp locked before this function
					mResImp->RefCount(1);		// Protocol::Protocol
// 					mRes->SetImp(mResImp);		// let resource know about new imp
					mUseCache = true;
					
					oldImp->NotifyListenersOfSwitch(mResImp);
					mResImp->InheritListeners(oldImp);
					oldImp->RefCount(-1);
					mResImp->NotifyListeners(msg_ResourceChanged);
					mResImp->NotifyListeners(msg_ResourceFlushed);
					mResourceIsCached = true;
					
					// Reset the expiration date on the resource.  Try to set it to an arbitrarily large number;
					// the resource will use its own heuristics to determine a proper expiration date.
					mResImp->SetExpires(0x7fffffff);
//					mResImp->Unlock();
				} else
					mResImp = NULL;
			}
		}
		//delete (resource);
		resource->RefCount(-1);
	}
	if (stillAlive)
		return ((mUseCache) ? NET_ERROR : NET_NO_ERROR);	

	return (NET_CANT_RECV);
}

const Proxy&
HTTP::ProxyObject()
{
	return (sProxy);
}

//=======================================================================
//=======================================================================
//	FTP Protocol

Proxy FTP::sProxy;


FTP::FTP(ConnectionManager *mgr, UResourceImp *resImp) : Protocol(mgr, resImp), mResponseCode(0),mPASVAddr(0),mPASVPort(0),mDataSize(0)
{
	NetPositive::AddThread(Run());	// Protocol base class does not run by itself
}

void
FTP::SetProxyActive(
	bool	active)
{
	sProxy.SetActive(active);
}

void
FTP::SetProxyNameAndPort(
	const BString	&name,
	int		port)
{
	sProxy.SetHostName(name);
	sProxy.SetHostPort(port);
}

bool
FTP::ProxyActive()
{
	return (sProxy.Active());
}

void
FTP::GetProxyNameAndPort(
	BString	&name,
	int		*port)
{
	sProxy.GetNameAndPort(name, port, false);
}

void FTP::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case NET_GET_URL: {
			bool download = false;
			if (msg->FindBool("download", &download) == B_NO_ERROR)
				mDownload = download;

			mRangeBegin = msg->FindInt32("rangeBegin");

			const char *url = msg->FindString("url");
			if (url)
				mURL.SetURL(url);

			RequestConnection();
			break;
		}
			
		case NET_CONNECTION_AVAIL: {
			msg->FindPointer("ConnectionInfo", (void **)&mConnection);
//			mConnection->Reference();
			
			NetResult result = Connect(NULL);
			if (result == NET_NO_ERROR)
				result = Request();
			mResImp->Flush(result == NET_NO_ERROR ? kComplete : kError);
			CloseSocket();

			Quit();
			break;
		}
		default:
			BLooper::MessageReceived(msg);
	}
}

//	Send a FTP string command

NetResult FTP::Command(char *str)
{
	if (!mConnection || mAbortRequested)
		return NET_CANT_SEND;
		
	pprint("FTP Command: %s",str);
	char* command = (char *)malloc(strlen(str) + 2 + 1);
	strcpy(command,str);
	strcat(command,"\r\n");
	
//	long result = send(mConnection->mSocket,command,strlen(command),0);
	long result = mConnection->Send(command,strlen(command));
	free(command);
	
	if (result < 0)  {
		CloseSocket();
		return NET_CANT_SEND;
	}
	return NET_NO_ERROR;
}

//	Bail

NetResult FTP::CloseSocket()
{	
	if (mConnection && mConnection->IsOpen()) {
		const char *quitCmd = "QUIT\r\n";
//		send(mConnection->mSocket, quitCmd, strlen(quitCmd), 0);
		mConnection->Send(quitCmd, strlen(quitCmd));
	}
	Protocol::CloseSocket();

	return (NET_NO_ERROR);
}

//	Open Data socket for transfer 

NetResult FTP::OpenDataSocket()
{
	sockaddr_in	addr;				// Try to connect
	addr.sin_family = AF_INET;
	addr.sin_port = htons(mPASVPort);
	addr.sin_addr.s_addr = htonl(mPASVAddr);
	
	int s = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if (s < 0)
		return NET_NO_SOCKETS;		// Can't get a socket
		
	pprint("%6d: Connecting on '%d'...",Thread(),s);
	if (connect(s,(sockaddr *)&addr,sizeof(sockaddr_in)) < 0) {
		closesocket(s);
		return NET_REFUSED;	// Can't connect! Geez!
	}
	
	mDataSocket = s;
	return NET_NO_ERROR;			// Got a connection on the correct port
}

//	Log on, deal with passwords if required

NetResult FTP::LogOn()
{
	if (Response() != 220)			//	Just connected... does the site want to deal with us?
		return NET_CANT_CONNECT;

//	Log in (parse userid and password?)

	char user[1024];
	char pass[1024];
	strcpy(user,"USER XXX");
	strcpy(pass,"PASS XXX");
	
//	Get a username and password from url

	if (mURL.User() && *mURL.User()) {
		strcpy(user + 5,mURL.User());
		if (mURL.Password() && *mURL.Password())
			strcpy(pass + 5,mURL.Password());
	} else
		gBasicAuthFolder->GetFTP(mURL.HostName(),user + 5,pass + 5,false);	// Or see if we can reuse an old password
		
	if (strlen(pass) == 5)
		strcat(pass,kAnonymousPassword);	// Guess at a password
		
//	Try the best guess passwords

	Command(user);
	if (Response() == 230)			// 331 password required or 530 Login incorrect
		return NET_NO_ERROR;		// Hmm.  The server let us in without a password.
	Command(pass);					// 230 user logged in or 530 Login incorrect
	
//	Try again with a real password if login failed using the defaults

	if (Response() != 230) {
		if (gBasicAuthFolder->GetFTP(mURL.HostName(),user + 5,pass + 5,true) == false)
			return NET_UNAUTHORIZED;					// User cancelled
		
		Command(user);
		Response();			// 331 password required or 530 Login incorrect
		Command(pass);		// 230 user logged in or 530 Login incorrect
		
		if (Response() != 230) {
			gBasicAuthFolder->Bad(mURL.HostName());		// Tried to use a password on a ftp site, it failed, don't use it again
			return NET_UNAUTHORIZED;
		} else
			gBasicAuthFolder->Good(mURL.HostName());
	}
	
	return NET_NO_ERROR;
}

//	Send the request

NetResult FTP::Request()
{
	mFirstByte = 0;
	NetResult result = NET_NO_ERROR;
	char str[1024];
	
//	Try and log on

	result = LogOn();
	if (result != NET_NO_ERROR)
		return result;

//	Not really sure if it is a directory request or a file request...
//	Assume it's a file, try and get if first

	const char *path = mURL.Path();
	if (path == NULL || *path == 0)
		path = "/";
	pprint("FTP: Path is '%s'",path);
	
	if (path[strlen(path)-1] != '/') {	// Does not end in a slash, think its a file
		sprintf(str,"SIZE %s",path);		// Try and determine the file's size
		Command(str);							// ..If it is a file
		result = Response();

		if ((result == 213) || (result == 500) || (result == 502)) {
			// 500/502 .. not understood/not implemented: Server may not understand size command (might be a bad pathname)
			Command("TYPE I");					// Binary transfers
			Response();

			Command("PASV");					// Put the host into passive mode
			if (Response() != 227)
				return NET_CANT_RECV;			// Will only do passive transfers

			if (mRangeBegin > 0) {
				sprintf(str, "REST %ld", mRangeBegin);
				Command(str);
				if (Response() == 350)
					mFirstByte = mRangeBegin;
			}
			sprintf(str,"RETR %s",path);	// Send the get command
			Command(str);

			result = OpenDataSocket();			// Try and open the socket to get data
			if (result == NET_NO_ERROR) {
				int i = Response();
				if ((i >= 100) && (i < 200)) {	// To the Get command - 150 (125 works too)
					if (ReadBody() == NET_DONT_DOWNLOAD) 	// Read data from the connection
						return (NET_NO_ERROR);		// mDownload == false!
					else
						Response();					// Complete - 226
				} else {
					closesocket(mDataSocket);
					result = NET_CANT_RECV;		// Usually 550: No such file or directory
				}
			}
	
			if (result == NET_NO_ERROR)
				return result;					// Fall thru to dir if we failed here
		}
	}
	
//	Pathname ended with a '/' or failed to get a file by that name
//	Get Directory

	strcat(str,path);
	mWorkingDir = path;
	sprintf(str,"CWD %s",path);				// Set the working directory to the path
	Command(str);
	if (Response() != 250)					// If it fails it just gets us back to the default
		return NET_CANT_RECV;
		
	Command("PWD");				// Get default directory
	Response();
	
	Command("PASV");			// Put the host into passive mode
	if (Response() != 227)
		return NET_CANT_RECV;	// Will only do passive transfers
	
//	Connect to the passive data post

	Command("LIST");
	result = OpenDataSocket();			// Try and open the socket to get data
	if (result == NET_NO_ERROR) {
		int response = Response();
		if (response == 125)
			response = 150;			// Connection already in place
			
		if (response == 150) {		// To the List command - 150
			ReadDir();					// Read list from the connection
			Response();					// Complete - 226
		} else {
			closesocket(mDataSocket);
			result = NET_CANT_RECV;		// Usually 550: No such file or directory
		}
	}
	return result;
}
		
//	Scan the response to the command, handle mulitline responses

NetResult FTP::Response()
{
	char response[1024];
	NetResult result;
	
	bool multiLine = false;
	do {
		do {
			result = ReadLine(response, 1024);
		} while (result == NET_NO_ERROR && !(*response));

		if (result != NET_NO_ERROR) {
			CloseSocket();
			return result;
		}
		pprint("FTP: %s",response);
		
		if (multiLine) {
			if (sscanf(response,"%d",&mResponseCode) == 1 && response[3] == ' ')
				multiLine = false;
		} else {
//			NP_ASSERT(sscanf(response,"%d",&mResponseCode) == 1);
			sscanf(response,"%d",&mResponseCode);
			if (response[3] == '-')						// Check for multiline
				multiLine = true;
		}
	} while (multiLine);
	
//	Handle response code

//	if (responseCode >= 500) {		// Error, possibly login
//		return -1;
//	}

	long code;
	char *s;
	if (mResponseCode >= 200) {
		switch (mResponseCode) {
			case 213:
				if (sscanf(response,"%ld %ld",&code,&mDataSize) == 2)
					pprint("FTP: Determined size to be %d",mDataSize);
				break;
				
			case 227: {								// Passive mode response
				s = strstr(response,"(");
				long p[6];
				if (s && sscanf(s,"(%ld,%ld,%ld,%ld,%ld,%ld)",p,p+1,p+2,p+3,p+4,p+5) == 6) {
					pprint("FTP: Scanned passive mode response");
					mPASVAddr = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
					mPASVPort = (p[4] << 8) | p[5];
				}
				break;
			}
				
			case 257: {
				char wd[256];
				int i = 0;
				s = response;					// Scan the working directory
				while (s[0] && s[0] != '"')
					s++;
				if (s[0]) {
					s++;
					while (s[0] && s[0] != '"')
						wd[i++] = *s++;
				}
				if (wd[i-1] != '/')
					wd[i++] = '/';
				wd[i++] = 0;
				pprint("FTP: Working Directory is '%s'",wd);
				mWorkingDir = wd;
				break;
			}
				
			case 150:
				pprint("FTP: Data connection open");
				break;
				
			case 550: {
				BString message;
				CreateErrorMessage(NET_FILE_NOT_FOUND, message);
				mResImp->SetErrorMessage(message);
			}
		}
	}

	return (NetResult)mResponseCode;
}

//	Read the body of the data from the data socket

NetResult FTP::ReadBody()
{
	char* data;
	int32 recvSize;
	uint32 size;
	Buffer buffer;

	for (;;) {
		do {
			data = (char *)buffer.GetReadBuffer(&size);	// Send data from buffer
			if (size) {
				status_t status;
				status = mResImp->LockWithTimeout(TEN_SECONDS);
				if (status == B_TIMEOUT)
					return NET_CANT_RECV;
				else if (status != B_OK)
					continue;
				
				if (mDataCount == 0) {					// Set the file type
					char str[1024];
//					NP_ASSERT(mURL.Path());
					if (mFirstByte == 0) {
						DetermineFileType(mURL.Path(),0,data,size,str);
//printf("%5d SetType   0x%x\n", find_thread(NULL), mResImp);
						mResImp->SetContentType(str);
						if (CanHandleFile(str))
							mDownload = true;	// force the download
						else
							mResImp->SetCacheOnDisk(false);
					}
					if ((mDataSize) && (mDownload)) {
						mResImp->SetContentLength(mDataSize);	// We know data size from 'SIZE' command
						
						if (mFirstByte > 0)
							mResImp->Seek(mFirstByte);
					}
					
					if (!mDownload) {
						mResImp->CreateStore();					// all is well...
						mResImp->Unlock();
						mResImp->NotifyListeners(msg_ResourceChanged);
						return (NET_DONT_DOWNLOAD);
					}
				}
//				NP_ASSERT(mResImp->Write(data,size,false) == 0);
				long written = mResImp->Write(data,size,false);
				mDataCount += size;
				mResImp->Unlock();
				if (written < 0) {
					mAbortRequested = true;
					return NET_CANT_RECV;
				}
				buffer.ReadBuffer(size);
			}
		} while (size);
		mResImp->NotifyListeners(msg_ResourceChanged);

		recvSize = FillBuffer(mDataSocket,buffer);		// Read data into buffer
		if (recvSize == 0) {
			mResImp->SetContentLength(mDataCount + mFirstByte);
			mResImp->NotifyListeners(msg_ResourceFlushed);
			pprint("FTP: Set Size to %d",mDataCount);
			closesocket(mDataSocket);
			mDataSocket = -1;
			return NET_NO_ERROR; 
		}
		if (recvSize < 0)
			return NET_CANT_RECV;
	}
}

//	Read the body of the data from the data socket

NetResult FTP::ReadDir()
{
	char	str[1024];
	Buffer	buffer;
	
//	Update the url to reflect the real path

	if (mResImp->LockWithTimeout(TEN_SECONDS) != B_OK) return NET_CANT_RECV;
	sprintf(str,"ftp://%s%s",mURL.NetLocation(),mWorkingDir.String());
	UResourceCache::ChangeURL(mResImp, str);
	mResImp->Unlock();

	FileSystemHTML	dir(mResImp);	// New directory object

	dir.WriteHeader();
	for (;;) {
		while (buffer.ReadLine(str, 1024) == 0)	// Is there a complete line in buffer?
			AddLine(dir,str);
			
		long size = FillBuffer(mDataSocket,buffer);	// Otherwise read some more data
		if (size <= 0)
			break;									// Can't read another line...
	}
	dir.WriteTrailer();
	
	closesocket(mDataSocket);
	mDataSocket = -1;

	return (NET_NO_ERROR);
}


//	Extact the next token from the string

const char* FTPToken(const char* s, char *token)
{
	token[0] = 0;
	if (s[0] == 0)
		return NULL;	// end of string
	
	while (s[0] && isspace(s[0]))
		s++;
	if (s[0]) {
		while (s[0] && !isspace(s[0]))
			*token++ = *s++;
		*token++ = 0;
	}
	return s;
}

//	Parse a ftp response line

//	flags	id	owner	group	size	month	day	time	name

void FTP::AddLine(FileSystemHTML& dir, const char* line)
{
	char token[256];
	
	line = FTPToken(line,token);
	if (strlen(token) != 10)
		return;									// Could be 'total xxx' for first line
		
	char ftype = token[0];						// 'd' for directory, 'l' for link, '-' for file
	
	int i;
	for (i = 0; i < 4; i++) {
		line = FTPToken(line,token);			// id	owner	group size
		if (line == NULL || token[0] == 0)
			return;
	}
	
	long size;
//	NP_ASSERT(sscanf(token,"%ld",&size) == 1);	//	Determine size of file
	sscanf(token,"%ld",&size);	//	Determine size of file

//	Read the month, day and time (year)

	char month[128];
	char day[128];
	char time[128];
	line = FTPToken(line,month);
	if (line == NULL || token[0] == 0)
		return;
	line = FTPToken(line,day);
	if (line == NULL || token[0] == 0)
		return;
	line = FTPToken(line,time);
	if (line == NULL || token[0] == 0)
		return;
	long date = 0;								// Extract date in a more useful format (Sep 20 18:01 or Aug  7  1994)
	
//	Name is the rest of the line

	while (line[0] && isspace(line[0]))
		line++;
	if (line[0])
		dir.AddFile(ftype,size,date,line);		// Add a file to the HTML
}


const Proxy&
FTP::ProxyObject()
{
	return (sProxy);
}

void printHex(void *data, long count)
{
	unsigned short *d = (unsigned short *)data;
	long addr = 0;
	char s[80];
	short x;
	
	while (count > 0) {
		char str[20];
		str[16] = 0;
		memcpy(str,d,16);
		for (x = 0; x < 16; x++)
			if (str[x] < 32)
				str[x] = '.';
		if (count < 16)
			str[count] = 0;

		sprintf(s,"%8lX..%4X.%4X.%4X.%4X..%4X.%4X.%4X.%4X %s",addr,d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[7],str);
		for (x = 0; x < 50; x++) {
			if (s[x] == ' ')
				s[x] = '0';
			else if (s[x] == '.')
				s[x] = ' ';
		}

//		Clip line if count < 16

		if (count < 16) {
			str[count] = 0;
			short clip = (count >> 1) * 5;
			clip += count < 8 ? 10 : 11;
			if (count & 1)
				clip -= 2;
			for (x = clip; x < 50; x++)
				s[x] = ' ';
		}
		
		printf("%s\n", s);
		d += 8;
		addr += 16;
		count -= 16;
	}
}

ConnectionInfo::ConnectionInfo() : mReuse(false), mSecure(false), mSocket(-1), mSocketLocker("Socket lock")
{
	mDontUse = false;
}

ConnectionInfo::~ConnectionInfo()
{
	pprint("ConnectionInfo 0x%x Closing socket %d", this, mSocket);
	if (gDumpData) {
		BAutolock lock(sDumpLock);
		printf("***** Connection 0x%x closed *****\n", (unsigned int)this);
	}
	if (mSocket != -1)
		closesocket(mSocket);
}


bool ConnectionInfo::LockSocket()
{
	bool val = mSocketLocker.Lock();
	return val;
}

void ConnectionInfo::UnlockSocket()
{
	if (!mReuse) {
		mDontUse = true;
		if (mSocket != -1) {
			closesocket(mSocket);
			mSocket = -1;
		}
	}
	mSocketLocker.Unlock();
}

NetResult ConnectionInfo::Connect(const BString& host, int port, Protocol *protocol)
{
	ulong a;
	sockaddr_in	addr;				// Try to connect
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

//	hostent* h = gethostbyname(host.String());
//	if (h && h->h_addr)
//		a = ntohl(*(long *)(h->h_addr));
//	else
//			return NET_UNKNOWN_HOST;				// Nope, and Can't find that host
	a = DNSLooper::GetHostByName(host.String());
	if (!a)
		return NET_UNKNOWN_HOST;


//	pprint("%6d: Connecting to '%d.%d.%d.%d'...",Thread(),(a >> 24),(a >> 16) & 0xFF,(a >> 8) & 0xFF,a & 0xFF);

	protocol->SetStatus(kConnect);
	addr.sin_addr.s_addr = htonl(a);
	int s = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if (s < 0)
		return NET_NO_SOCKETS;		// Can't get a socket
		
	//pprint("%6d: Connecting on socket '%d'...",Thread(),s);
	if (connect(s,(sockaddr *)&addr,sizeof(sockaddr_in)) < 0) {
		closesocket(s);
		return NET_REFUSED;	// Can't connect! Geez!
	}

	if (gDumpData) {
		BAutolock lock(sDumpLock);
		printf("***** Connection 0x%x to %s:%d established *****\n", (unsigned int)this, host.String(), port);
	}
	mSocket = s;
	return NET_NO_ERROR;
}

bool ConnectionInfo::IsOpen()
{
	return mSocket != -1;
}

long ConnectionInfo::Send(const void *data, long length)
{
	if (gDumpData) {
		BAutolock lock(sDumpLock);
		BString string((const char *)data);
		string.Truncate(length);
		printf("***** Connection 0x%x sending %ld bytes *****\n%s\n\n", (unsigned int)this, length, string.String());
	}
	if (mDontUse)
		return -1;
	return send(mSocket, data, length, 0);
}

void DoDumpData(const char *description, unsigned int pointer, char *data, int32 size)
{
	if (gCleanDump){
		printf("***** %s 0x%x received %ld bytes of data *****\n", description, pointer, size);
		for(int i=0; i<size; ++i){
			if((uchar)data[i] > 127 || (uchar)data[i] == 0)
				break;
			else
				printf("%c", data[i]);
		}
		printf("\n");
	}
	else {
		printf("***** %s 0x%x received %ld bytes of binary data *****\n", description, pointer, size);
		printHex(data, MIN(size, 64));
		printHex(data, size);
	}
}

void DumpData(const char *description, unsigned int pointer, char *data, int32 size)
{
	uchar *foo = (uchar *)data;
	int32 firstHighByte = 0;
	while (foo < (uchar *)(data + size) && firstHighByte == 0) {
		if (*foo > 127 || *foo == 0) {
			firstHighByte = foo - (uchar *)data;
			break;
		}
		foo++;
	}
	
	if (firstHighByte > size / 4) {
		DoDumpData(description, pointer, data, firstHighByte - 1);
		DoDumpData(description, pointer, data + firstHighByte, size - firstHighByte);
	} else
		DoDumpData(description, pointer, data, size);
}

long ConnectionInfo::FillBuffer(Buffer& buffer)
{
	char* data;
	uint32 bufSize;
	data = (char *)buffer.GetWriteBuffer(&bufSize);	// read data into buffer
	if (data == NULL || bufSize == 0) 
		return 0;

	int32 size = bufSize;

	size = recv(mSocket,data,size,0);
	if (size == -1)
		pprint("Errno %x", errno);

	if (gDumpData) {
		BAutolock lock(sDumpLock);
		DumpData("Connection", (unsigned int)this, data, size);
	}

	
	if (size > 0) {
		buffer.WriteBuffer(size);
	}
	return size;
}

#ifndef NOSSL
SSLConnectionInfo::SSLConnectionInfo()
{
	mIsOpen = false;
	mSecure = true;
	mBeSSL = new BeSSL();
	if (mBeSSL->InitCheck() != 0) {
		delete mBeSSL;
		mBeSSL = 0;
	}
}

SSLConnectionInfo::~SSLConnectionInfo()
{
	if (gDumpData) {
		BAutolock lock(sDumpLock);
		printf("***** SSL Connection 0x%x closed *****\n", (unsigned int)this);
	}
	delete mBeSSL;
	mBeSSL = 0;
}

NetResult SSLConnectionInfo::Connect(const BString& host, int port, Protocol *protocol)
{
	if (mBeSSL) {
		protocol->SetStatus(kConnect);
		switch (mBeSSL->Connect(host.String(), port)) {
			case 0:
				mIsOpen = true;
				if (gDumpData) {
					BAutolock lock(sDumpLock);
					printf("***** SSL Connection 0x%x to %s:%d established *****\n", (unsigned int)this, host.String(), port);
				}
				return NET_NO_ERROR;
			default:
				return NET_CANT_CONNECT;
		}
	}
	return NET_CANT_CONNECT;
}


bool SSLConnectionInfo::IsOpen()
{
	return mIsOpen;
}


long SSLConnectionInfo::Send(const void *data, long length)
{
	uint32 writtenLength = length;
	if (gDumpData) {
		BAutolock lock(sDumpLock);
		BString string((const char *)data);
		string.Truncate(length);
		printf("***** SSL Connection 0x%x sending %ld bytes *****\n%s\n\n", (unsigned int)this, length, string.String());
	}
	if (mDontUse)
		return -1;
	if (mBeSSL->Write((unsigned char *)data, writtenLength) == 0) {
		return writtenLength;
	} else {
		return 0;
	}
}

long SSLConnectionInfo::FillBuffer(Buffer& buffer)
{
	char* data;
	uint32 size;
	data = (char *)buffer.GetWriteBuffer(&size);	// read data into buffer
	if (data == NULL || size == 0) 
		return 0;

	uint32 sslSize = size;
	sslSize = mBeSSL->Read((unsigned char *)data, sslSize);
	if (sslSize <= 0) {
		return 0;
	} 
	
	if (gDumpData) {
		BAutolock lock(sDumpLock);
		DumpData("SSL Connection", (unsigned int)this, data, sslSize);
	}

	if (sslSize > 0) {
		buffer.WriteBuffer(sslSize);
	}
	return sslSize;
}
#endif

ConnectionManager::ConnectionManager()
	: mLocker("ConnectionMgr lock")
{
	mMaxConnections = gPreferences.FindInt32("MaxConnections");
}

ConnectionManager::~ConnectionManager()
{
	pprint("Deleting manager 0x%x", this);
	KillAllConnections();
}

void ConnectionManager::RequestConnection(Protocol *protocol, bool secure)
{
	BAutolock lock(mLocker);
	pprint("Requested connection from manager 0x%x", this);
	ConnectionInfo *ci = NULL;
	
	const char *hostName = protocol->GetParser()->HostName();
	int hostPort = protocol->GetParser()->HostPort();
	
	for (int j = 0; j < mPendingConnections.CountItems(); j++) {
		ci = (ConnectionInfo *)mPendingConnections.ItemAt(j);
		if (ci->mReuse == true && ci->mHostName == hostName && hostPort == ci->mHostPort && secure == ci->mSecure) {
			pprint("Reusing connection 0x%x", ci);

			BMessage msg(NET_CONNECTION_AVAIL);
			msg.AddPointer("ConnectionInfo", ci);
			ci->Reference();
			
			ci->mProtocols.AddItem(protocol);			
//			if (protocol->Thread() == B_ERROR) {
//				NetPositive::AddThread(protocol->Run());	// Protocol base class does not run by itself
//			}
			protocol->PostMessage(&msg);
			return;
		}
	}
	
	pprint("Secure is %d", secure);
#ifndef NOSSL
	if (secure)
		ci = new SSLConnectionInfo;
	else
#endif
		ci = new ConnectionInfo;
	ci->mProtocols.AddItem(protocol);
	ci->mHostName = hostName;
	ci->mHostPort = hostPort;

	if ((uint32)mPendingConnections.CountItems() < mMaxConnections) {
		pprint("Requested connection 0x%x for protocol 0x%x.  NumConnections = %d.  Queue size = %d.  Connecting...",ci,protocol, mPendingConnections.CountItems(), mQueuedConnections.CountItems());

		BMessage msg(NET_CONNECTION_AVAIL);
		msg.AddPointer("ConnectionInfo", ci);
		ci->Reference();
		mPendingConnections.AddItem(ci);
		
//		if (protocol->Thread() == B_ERROR) {
//			NetPositive::AddThread(protocol->Run());	// Protocol base class does not run by itself
//		}
		protocol->PostMessage(&msg);
	} else {
		pprint("Requested connection 0x%x for protocol 0x%x.  NumConnections = %d.  Queue size = %d.  Queueing...",ci,protocol, mPendingConnections.CountItems(), mQueuedConnections.CountItems());
		mQueuedConnections.AddItem(ci);
	}
}

void ConnectionManager::ConnectionFinished(Protocol *protocol)
{
	BAutolock lock(mLocker);
	pprint("ConnectionFinished for manager 0x%x", this);
	
	ConnectionInfo *ci = NULL;
	bool found = false;
	
	for (int j = 0; j < mPendingConnections.CountItems() && !found; j++) {
		ci = (ConnectionInfo *)mPendingConnections.ItemAt(j);
		for (int k = 0; k < ci->mProtocols.CountItems() && !found; k++) {
			if (ci->mProtocols.ItemAt(k) == protocol) {
				found = true;
				ci->mProtocols.RemoveItem(protocol);
				if (ci->mProtocols.CountItems() == 0) {
					mPendingConnections.RemoveItem(ci);
					ci->Dereference();
				}
			}
		}
	}
	
	pprint("Done with connection 0x%x for protocol 0x%x.  NumConnections = %d.", ci, protocol, mPendingConnections.CountItems());

	if (!found)
		pprint("Unable to find connection 0x%x for protocol 0x%x in queue!", ci, protocol);
		
	ActivateQueuedConnection();
}

bool ConnectionManager::ActivateQueuedConnection()
{
	BAutolock lock(mLocker);
	ConnectionInfo *ci;
	bool retval = false;
	
	if ((uint32)mQueuedConnections.CountItems() > 0 && (uint32)mPendingConnections.CountItems() < mMaxConnections) {
		retval = true;
		ci = (ConnectionInfo *)mQueuedConnections.ItemAt(0);
		pprint("Processing queued connection 0x%x.  Queue size = %d", ci, mQueuedConnections.CountItems());
		mQueuedConnections.RemoveItem(ci);
		mPendingConnections.AddItem(ci);

		BMessage msg(NET_CONNECTION_AVAIL);
		msg.AddPointer("ConnectionInfo", ci);
		ci->Reference();
		for (int k = 0; k < ci->mProtocols.CountItems(); k++) {
			Protocol *p = (Protocol *)ci->mProtocols.ItemAt(k);
//			if (p->Thread() == B_ERROR) {
//				NetPositive::AddThread(p->Run());	// Protocol base class does not run by itself
//			}
			p->PostMessage(&msg);
		}
	}
	return retval;
}

void ConnectionManager::SetMaxConnections(uint32 max)
{
	mMaxConnections = max;
	while (ActivateQueuedConnection())
		;
}

void ConnectionManager::SetConnectionReuse(ConnectionInfo *connection, bool reuse)
{
	BAutolock lock(mLocker);
	pprint("Setting reuse of connection 0x%x to %d", connection, reuse);
	
	ConnectionInfo *ci = NULL;
	
	for (int j = 0; j < mPendingConnections.CountItems(); j++) {
		ci = (ConnectionInfo *)mPendingConnections.ItemAt(j);
		if (ci == connection) {
//		for (int k = 0; k < ci->mProtocols.CountItems(); k++)
//			if (ci->mProtocols.ItemAt(k) == protocol) {
				ci->mReuse = reuse;
pprint("Found connection, setting reuse");
				return;
			}
	}
pprint("Couldn't find connection");
}

bool ConnectionManager::AdoptResource(UResourceImp* resource, ConnectionManager *fromMgr)
{
	BAutolock lock(mLocker);

	if (mPendingConnections.CountItems() >= (int32)mMaxConnections) {
		return false;
	}
		
	BAutolock lock2(fromMgr->mLocker);
	
	for (int i = 0; i < fromMgr->mPendingConnections.CountItems(); i++) {
		ConnectionInfo *ci = (ConnectionInfo*)fromMgr->mPendingConnections.ItemAt(i);
		for (int j = 0; j < ci->mProtocols.CountItems(); ci++) {
			Protocol *p = (Protocol*)ci->mProtocols.ItemAt(j);
			if (p->GetResource() == resource) {
				p->SetConnectionManager(this);
				fromMgr->SetConnectionReuse(ci, false);
				fromMgr->mPendingConnections.RemoveItem(ci);
				fromMgr->ActivateQueuedConnection();
				mPendingConnections.AddItem(ci);
				return true;
			}
		}
	}
	return false;
}

void ConnectionManager::KillAllConnections()
{
	BAutolock lock(mLocker);
	pprint("Killing all connections for manager 0x%x.", this);
	ConnectionInfo *ci;

	if (mQueuedConnections.CountItems() > 0)
		for (int j = mQueuedConnections.CountItems() - 1; j >= 0; j--) {
			ci = (ConnectionInfo *)mQueuedConnections.ItemAt(j);
			for (int k = 0; k < ci->mProtocols.CountItems(); k++)
				((Protocol *)ci->mProtocols.ItemAt(k))->Abort();
			mQueuedConnections.RemoveItem(ci);
			ci->Dereference();
		}

	if (mPendingConnections.CountItems() > 0)
		for (int j = mPendingConnections.CountItems() - 1; j >= 0; j--) {
			ci = (ConnectionInfo *)mPendingConnections.ItemAt(j);
			for (int k = 0; k < ci->mProtocols.CountItems(); k++) {
				Protocol *p = (Protocol *)ci->mProtocols.ItemAt(k);
				p->Abort();
				// The connection will try to remove itself when Abort() is called.
				// However, we might want to dispose of the ConnectionManager before
				// then, so let's get rid of it now.  Let's make the protocol forget
				// about its manager so that we can delete it right away if need be.
				p->SetConnectionManager(NULL);
			}
			mPendingConnections.RemoveItem(ci);
			ci->Dereference();
		}
		
	SetReferrer(NULL);
}

void ConnectionManager::GetFTP(UResourceImp *resource, const BString& url, const char *downloadPath, bool forceDownload, int32 rangeBegin)
{
	BAutolock lock(mLocker);
	BString proxyName;
	int proxyPort;
	FTP::GetProxyNameAndPort(proxyName, &proxyPort);
	if (FTP::ProxyActive() && proxyName.Length() > 0 && proxyPort) {
		GetHTTP(resource, url.String(), NULL, NULL, downloadPath, false, proxyName.String(), proxyPort);
		return;
	}

	BMessage msg(NET_GET_URL);
	msg.AddString("url", url.String());
	if (downloadPath != NULL || forceDownload) {
		msg.AddBool("download", true);
	}
	
	msg.AddInt32("rangeBegin", rangeBegin);

//	BMessenger *m = new BMessenger(new FTP(this, resource));
//	m->SendMessage(&msg);
	BMessenger m(new FTP(this, resource));
	m.SendMessage(&msg);
}

void ConnectionManager::GetHTTP(UResourceImp *resource, const BString& url, BString* formData, BString *referrer,const char *downloadPath, bool secure, const char *proxyName, int proxyPort, bool headersOnly, bool forceDownload, int32 rangeBegin, bool neverReuse)
{
	BAutolock lock(mLocker);
	BMessage msg(NET_GET_URL);

	pprint("ConnectionManager::GetHTTP %s", url.String());
	
	msg.AddString("url", url.String());
	if (formData) {
		const char *theData = (formData->Length() > 0) ? (*formData).String() : "";		
		msg.AddString("formData", theData);
	}
	if (downloadPath != NULL || forceDownload) {
		msg.AddBool("download", true);
	}
	if (referrer != NULL) {
		const char *theData = (referrer->Length() > 0) ? (*referrer).String() : "";		
		msg.AddString("referrer", theData);
	}
	msg.AddBool("secure", secure);
	msg.AddPointer("resource", resource);
	if (proxyName && *proxyName && proxyPort) {
		msg.AddString("proxyName", proxyName);
		msg.AddInt32("proxyPort", proxyPort);
	}
	
	msg.AddBool("headersOnly", headersOnly);

	msg.AddInt32("rangeBegin", rangeBegin);
	msg.AddBool("neverReuse", neverReuse);

//	BMessenger *m = new BMessenger(new HTTP(this, resource));
//	m->SendMessage(&msg);
	BMessenger m(new HTTP(this, resource));
	m.SendMessage(&msg);
}

long ConnectionManager::GetTotalNumConnections()
{
	return mPendingConnections.CountItems() + mQueuedConnections.CountItems();
}
