#include <CookieManager.h>
#include <ctype.h>
#include <Debug.h>
#include <Protocol.h>
#include <Resource.h>
#include <ResourceCache.h>
#include <DNSCache.h>
#include <ContentView.h>	//for USER_ACTION
#include <stdlib.h>
#include <StringBuffer.h>
#include <util.h>
#include <parsedate.h>
#include "BufferedConnectionPool.h"
#include "SSLConnection.h"
#include "Connection.h"
#include "SSLConnectionPool.h"
#include "HTTP.h"
#include "HTTPUtil.h"
#include "BArray.h"

enum HttpResult {
	kHttpContinue = 100,
	kHttpOk = 200,
	kHttpNoContent = 204,
	kHttpPartialContent = 206,
	kHttpRedirectBase = 300,
	kHttpRedirectTop = 399,
	kHttpErrorBase = 400,
	kHttpAuthenticationError = 401
};

// used in SendRequestMultipartEncoded as it lays out the request
typedef struct {
	StringBuffer sectionText;
	Protocol *protocol;
	ssize_t streamBytes;
} requestSection;

// Think hard before you change this.  Lots of sites pick out different parts
// of this string with primitive scripts, and any change can turn a friendly
// site into an error message.
static const char *kDefaultUserAgentString = "Mozilla/4.72 [en] (Win98; I)";

static const char *kHttpVersion = "HTTP/1.1";
static const int kUnknownLength = 0x7fffffff;

static BufferedConnectionPool unencryptedConnectionPool;
static SSLConnectionPool encryptedConnectionPool;
static EncryptedProxiedHTTPConnectionPool encryptedProxiedConnectionPool;

// We initialize fServerDate
// only in the constructor assuming
// that it is more likely that the
// servers will have the right date
// set.
Http::Http(void *handle)
	:	Protocol(handle),
		fConnection(NULL),
		fAborted(false),
		fUnencryptedUsingProxy(false),
		fServerDate(-1),
		fMetaCallback(0)
{
}

Http::~Http()
{
	//return 'fConnection' to the pool
	SetConnection(NULL);
}

status_t Http::Open(const URL &url, const URL &referrer, BMessage *outErrorInfo, uint32 flags)
{
	fFullContentLength = kUnknownLength;
	fExpectedContentLength = fContentRead = 0;
	fFlags = flags;
	fIsChunked = false;
	fUrl = url;
	fCurrentPosition = 0;
	fCachePolicy = CC_CACHE;
	fReferrer = referrer;
	fAcceptsRanges = false;
	fKeepAlive = false;
	fKeepAliveTimeout = B_INFINITE_TIMEOUT;
	fCurrentChunkSize = 0;
	fChunkReadState = kScanChunkSize;
	fMaxKeepAliveRequests = 0x7fffffff;

	return OpenInternal(outErrorInfo, true);
}

ssize_t Http::GetContentLength()
{
	return fFullContentLength;
}

void Http::GetContentType(char *type, int size)
{
	strncpy(type, fContentType.String(), size - 1);
	type[MIN(fContentType.Length(), size - 1)] = '\0';
}

CachePolicy Http::GetCachePolicy()
{
	return fCachePolicy;
}

ssize_t Http::Read(void *out, size_t size)
{
	return ReadCommon(fCurrentPosition, out, size);
}

ssize_t Http::ReadAt(off_t pos, void *out, size_t size)
{
	if (!(fFlags & RANDOM_ACCESS))
		return B_NO_RANDOM_ACCESS;
	
	return ReadCommon(pos, out, size);
}

off_t Http::Seek(off_t position, uint32 seekMode)
{
	if (!(fFlags & RANDOM_ACCESS))
		return 0;
	
	switch (seekMode) {
		case SEEK_SET:
			fCurrentPosition = position;
			break;

		case SEEK_CUR:
			fCurrentPosition += position;
			break;

		case SEEK_END:
			fCurrentPosition = fFullContentLength + position;
			break;
	}
	
	return fCurrentPosition;
}

off_t Http::Position() const
{
	if (!(fFlags & RANDOM_ACCESS))
		return 0;

	return fCurrentPosition;
}

bool Http::GetRedirectURL(URL &outRedirect, bigtime_t *outDelay)
{
	if (fRedirectUrl.IsValid()) {
		outRedirect = fRedirectUrl;
		*outDelay = fRedirectDelay;
		return true;
	}
	
	return false;		
}

void Http::Abort()
{
	//note that a Protocol object is generally used by only a single thread,
	//with the exception of the Abort() method - as such, we must synchronize
	//here, but don't really need to anywhere else...
	GehnaphoreAutoLock _lock(fConnectionLock);

	//remember that we've been aborted (this prevents OpenInternal() from
	//actually opening a connection if it is called after we return)
	fAborted = true;

	//abort any existing connection
	if (fConnection)
		fConnection->Abort();
}

status_t Http::SetMetaCallback(void *cookie, MetaCallback metaCallback)
{
	fMetaCookie = cookie;
	fMetaCallback = metaCallback;
	return B_OK;
}

ssize_t Http::ReadCommon(int pos, void *out, int size)
{
	if (pos >= fFullContentLength)
		return 0; 	// At the end of the file.
	else if (pos + size > fFullContentLength)
		size = fFullContentLength - pos;

	if (fFlags & RANDOM_ACCESS)
		return ReadByteRange(pos, out, size);
	else if (fIsChunked)
		return ReadChunked(out, size);
	else
		return ReadRaw(out, size);
}

ssize_t Http::ReadByteRange(int pos, void *out, int size)
{
	status_t error = B_OK;
	for (int errorCount = 0; errorCount < 2; errorCount++) {
		BMessage errorInfo;
		if (errorCount != 0 || !fKeepAlive) {
			// Either this connection is not being kept alive, or an error occured
			// below in the SendRequest() or ParseResponseHeader(). The most likely
			// error is that the keep-alive period for the connection has expired.
			// OpenInternal() has its own retry loop, which is why this loop only
			// tries to reconnect once.
			HTTP_TRACE((HTTP_STATUS_COLOR"Byte range access connection closed, reopen.\n"HTTP_NORMAL_COLOR));
			error = OpenInternal(&errorInfo, false);
			if (error < 0)
				break;
		}

		error = SendRequest(fUrl.GetQueryMethod() == GET_QUERY ? kHttpGet : kHttpPost, pos,
			pos + size - 1);
		if (error >= B_OK) {
			do {
				error = ParseResponseHeader(&errorInfo);
			} while (fResultCode == kHttpContinue && error == B_OK);
		}
		
		if (error == B_OK)
			break;
	}
	
	if (error != B_OK) {
		fKeepAlive = false;	
		return error;
	} else if (fResultCode != kHttpPartialContent && fResultCode != kHttpOk) {
		fAcceptsRanges = false;
		fKeepAlive = false;
		return B_NO_RANDOM_ACCESS;
	}

	int totalRead = 0;
	do {
		int bytesRead = 0;
		if (fIsChunked)
			bytesRead = ReadChunked(out, size);
		else
			bytesRead = ReadRaw(out, size);
	
		if (bytesRead <= 0)
			break;

		totalRead += bytesRead;
		out = reinterpret_cast<uint8*>(out) + bytesRead;
		size -= bytesRead;
	} while (size > 0 && fFullContentLength != kUnknownLength);
	return totalRead;
}

ssize_t Http::ReadChunked(void *out, int size)
{
	int ret = B_ERROR;
	for (;;) {
		if (fChunkReadState == kScanChunkData) {
			if (fCurrentChunkSize > 0) {
				// Read the data portion of the chunk
				ret = fConnection->Read(out, MIN(size, fCurrentChunkSize));
				if (ret > 0) {
					fCurrentChunkSize -= ret;
					fCurrentPosition += ret;
					fContentRead += ret;
				}
				break;
			} else
				fChunkReadState = kScanChunkDelimiter;
		}

		char c;
		ret = fConnection->Read(&c, 1);
		if (ret <= 0)
			break;

		switch (fChunkReadState) {
			case kScanChunkSize:
				if (isdigit(c))
					fCurrentChunkSize = fCurrentChunkSize * 16 + c - '0';
				else if (c >= 'a' && c <= 'f')
					fCurrentChunkSize = fCurrentChunkSize * 16 + c - 'a' + 10;
				else if (c >= 'A' && c <= 'F')
					fCurrentChunkSize = fCurrentChunkSize * 16 + c - 'A' + 10;
				else {
					if (fCurrentChunkSize == 0)
						return B_ERROR;	// End of stream
					else
						fChunkReadState = kScanChunkExtension;
				}
					
				break;
	
			case kScanChunkExtension:
				if (c == '\n')
					fChunkReadState = kScanChunkData;
					
				break;
	
			case kScanChunkDelimiter:
				if (c == '\n')
					fChunkReadState = kScanChunkSize;		// next chunk

				break;

			default:
				TRESPASS();
		}
	}
	
	return ret;
}

ssize_t Http::ReadRaw(void *out, int size)
{
	ssize_t bytes_read;

	if (fCurrentPosition < fSimpleResponseLen) {
		//we have a simple-response, and its beginning was already
		//read from 'fConnection' by ParseResponseHeader() - return these
		//already-read characters now
		bytes_read = min_c(size, fSimpleResponseLen - fCurrentPosition);
		memcpy(out, &fSimpleResponse[fCurrentPosition], bytes_read);

	} else
		bytes_read = fConnection->Read(out, size);

	if (bytes_read > 0) {
		fCurrentPosition += bytes_read;
		fContentRead += bytes_read;
	}

	HTTP_TRACE((HTTP_STATUS_COLOR
		"HTTP Protocol %p reading Connection %p: Attempted %d, read %d\n"
		HTTP_NORMAL_COLOR,
		this, fConnection, size, bytes_read));

	return bytes_read;
}

status_t Http::OpenInternal(BMessage *outErrorInfo, bool sendInitialRequest)
{
	status_t error = B_OK;
	bool encrypt = strcasecmp(fUrl.GetScheme(), "https") == 0;
	int port = fUrl.GetPort();
	if (port == 0) {
		// Default the port number.
		port = (encrypt ? 443 : 80);
	}
	
	char proxyHost[255];
	int proxyPort;

	get_proxy_server(proxyHost, &proxyPort);
	bool using_proxy = (proxyHost[0] != '\0');
	fUnencryptedUsingProxy = (using_proxy && !encrypt);
	
	const char *resolvedHost;
	int resolvedPort;
	ConnectionPool *connectionPool;
	if (encrypt) {
		// EncryptedProxiedHTTPConnections have to find the proxy on their
		// own, since they have to know the names of both their proxy (in
		// order to connect) and the host we actually want to talk to
		// (which is included in the one-time header we send when the
		// EncryptedProxiedHTTPConnection Open()s).  So in this case
		// we will tell the ConnectionPool just the URL's host and
		// port, not the proxy host and port.
		resolvedHost = fUrl.GetHostName();
		resolvedPort = port;
		if (using_proxy) {
			connectionPool = &encryptedProxiedConnectionPool;
		} else {
			connectionPool = &encryptedConnectionPool;
		}
	} else {
		connectionPool = &unencryptedConnectionPool;
		if (using_proxy) {
			resolvedHost = proxyHost;
			resolvedPort = proxyPort;
		} else {
			resolvedHost = fUrl.GetHostName();
			resolvedPort = port;
		}
	}
	
	if (fFlags & USER_ACTION) {
		//cancel any current DNS blackout (we always allow explicit user
		//actions to trigger autodials)
		dnsCache.Blackout(false);
	}

	for (int retry = 0; retry < 3; retry++) {

		// Note that we only try to reuse another connection
		// if this is the first try (second to last parameter)

		Connection *newConnection = NULL;
		error = connectionPool->GetConnection(resolvedHost, resolvedPort,
			(retry == 0), &newConnection);
		
		if (error == B_NAME_NOT_FOUND) {
			// Couldn't find host for the given url.
			outErrorInfo->AddString(S_ERROR_TEMPLATE, "Errors/hostunknown.html");
			outErrorInfo->AddString(S_ERROR_MESSAGE, strerror(error));
			outErrorInfo->AddString("host", fUrl.GetHostName());
			return error;
		} else if (error == B_TIMED_OUT) {
			// Network connection timed out.
			outErrorInfo->AddString(S_ERROR_TEMPLATE, "Errors/timeout.html");
			outErrorInfo->AddString(S_ERROR_MESSAGE, strerror(error));
			outErrorInfo->AddString("host", fUrl.GetHostName());
			return error;
		} else if (error < 0) {
			// Couldn't connect
			outErrorInfo->AddString(S_ERROR_TEMPLATE, "Errors/connect.html");
			outErrorInfo->AddString(S_ERROR_MESSAGE, strerror(error));
			outErrorInfo->AddString("host", fUrl.GetHostName());
			return error;
		}

		//update 'fConnection' (in a thread-safe way)
		if (!SetConnection(newConnection)) {
			//we've been aborted - give up
			return B_ERROR;
		}
	
		// Some servers (www.census.gov, for one) close the connection
		// immediately upon responding to a request, in spite of their
		// keep-alive claim.  Later, in ReadByteRange(), we try to get a
		// new Connection if our first Read() doesn't work, but if we
		// sent this initial HEAD request again, we'd be right back where
		// we started, with a dead Connection.
		if (sendInitialRequest) {
			error = SendRequest((fFlags & RANDOM_ACCESS) ? kHttpHead :
				(fUrl.GetQueryMethod() == GET_QUERY ? kHttpGet : kHttpPost));
		}
		
		if (error >= B_OK)
			break;
	}

	if (error < 0) {
		// Couldn't send request.
		fKeepAlive = false;
		outErrorInfo->AddString(S_ERROR_TEMPLATE, "Errors/hostrefused.html");
		outErrorInfo->AddString("host", fUrl.GetHostName());
		return error;
	}

	if (sendInitialRequest) {
		do {
			error = ParseResponseHeader(outErrorInfo);
			if (error < 0) {
				outErrorInfo->AddString(S_ERROR_TEMPLATE, "Errors/hostclosed.html");
				outErrorInfo->AddString("host", fUrl.GetHostName());
				fKeepAlive = false;
				return error;
			}
		} while (fResultCode == kHttpContinue); // Skip and read the next header.
		
		if (!fIsChunked && fFullContentLength == kUnknownLength)
			fKeepAlive = false;
	
		if (fResultCode >= kHttpRedirectBase && fResultCode <= kHttpRedirectTop)
			fKeepAlive = false;	// There may still be associated content.  Do this to be safe.
	
		if (fResultCode == kHttpAuthenticationError) {
			fKeepAlive = false;
			outErrorInfo->AddString(S_ERROR_TEMPLATE, "Errors/http.html");
			outErrorInfo->AddString(S_CHALLENGE_STRING, fAuthenticationRealm.String());
			return B_AUTHENTICATION_ERROR;
		}
		
		if ((fFlags & RANDOM_ACCESS) && !fAcceptsRanges)
			return B_NO_RANDOM_ACCESS;
	
		// If an error was returned, check whether we should load the html
		// page that describes the error, or just ignore it (like if it is an
		// inline image).
		if (fResultCode >= kHttpErrorBase && !(fFlags & LOAD_ON_ERROR)) {
			fKeepAlive = false;
			outErrorInfo->AddString(S_ERROR_TEMPLATE, "Errors/http.html");
			return B_ERROR;
		}
	
		if (fResultCode == kHttpNoContent)	// Note: It's ok to keep alive after one of these.
			return B_NO_CONTENT;
	}
	
	return B_OK;
}

status_t Http::SendRequest(HttpRequestType type, int from, int to)
{
	if (fContentRead < fExpectedContentLength)
		return B_ERROR;
	
	fLastRequest = type;
	fExpectedContentLength = fContentRead = 0;
	
	switch(type) {
		case kHttpPost:
			if(strlen(fUrl.GetUploadQuery()) > 0) {
				// we need to post files, so we need to send it using
				// multipart encoding
				return SendRequestMultipartEncoded(type, from, to);
			}
		default:
			return SendRequestUrlEncoded(type, from, to);
	}			
}

status_t Http::BuildRequestHeader(StringBuffer *requestHeader, HttpRequestType type, int from = -1, int to = -1)
{
	switch (type) {
		case kHttpGet:
			*requestHeader << "GET ";
			break;
	
		case kHttpHead:
			*requestHeader << "HEAD ";
			break;
	
		case kHttpPost:
			*requestHeader << "POST ";
			break;
	
		default:
			PRINT(("Bad request type\n"));
			return B_ERROR;
	}		

	// The full url must be send if requesting from a proxy server.
	// Otherwise, just use the path.
	if (fUnencryptedUsingProxy)
		*requestHeader << fUrl;
	else {
		*requestHeader << fUrl.GetPath();
		if (type == kHttpGet && fUrl.GetQuery()[0] != '\0')
			*requestHeader << '?' << fUrl.GetQuery();
	}

	*requestHeader << " " << kHttpVersion << "\r\n";
	*requestHeader << "Host: " << fUrl.GetHostName() << "\r\n";

	if (fUnencryptedUsingProxy) {
		char proxyUser[255];
		char proxyPassword[255];
		if (get_proxy_password(proxyUser, proxyPassword)) {
			// Send proxy credentials
			*requestHeader << "Proxy-Authorization: Basic ";
			EncodeBasicAuthentication(*requestHeader, proxyUser, proxyPassword);
			*requestHeader << "\r\n";
		}
	}
	
	if ((fFlags & RANDOM_ACCESS) && from != -1 && to != -1)
		*requestHeader << "Range: bytes=" << from << "-" << to << "\r\n";

	if (!(fFlags & TERSE_HEADER)) {
		// Only send this info if the TERSE_HEADER flag is not specified.  These
		// headers can confuse shoutcast servers, which is why they can be turned off.
		char buf[128];
		get_navigator_setting("user_agent", buf, sizeof(buf), kDefaultUserAgentString);
		*requestHeader << "User-Agent: " << buf;
		*requestHeader << "\r\nAccept: */*\r\n";
		*requestHeader << "Connection: keep-alive\r\n";
		if (fReferrer.IsValid())
			*requestHeader << "Referer: " /* [sic] */ << fReferrer << "\r\n";
	}

	// Send cookies if needed
	StringBuffer cookieString;
	cookieManager.BuildCookieString(fUrl, cookieString);
	if (cookieString.Length() > 0)
		*requestHeader << "Cookie: " << cookieString.String() << "\r\n";

	// Send username/password if needed
	if (fUrl.GetUserName()[0] != '\0') {
		*requestHeader << "Authorization: Basic ";
		EncodeBasicAuthentication(*requestHeader, fUrl.GetUserName(), fUrl.GetPassword());
		*requestHeader << "\r\n";
	}

	return B_OK;
}

status_t Http::SendRequestMultipartEncoded(HttpRequestType type, int from, int to)
{
	StringBuffer requestHeader;
	BuildRequestHeader(&requestHeader, type, from, to);
	ssize_t totalBytes;
	ssize_t sentBytes;
	ssize_t requestSize;
	status_t err = B_OK;
	
	requestHeader << "Content-Type: multipart/form-data; ";

	StringBuffer boundarySeparator;
	StringBuffer lastBoundarySeparator;
	{
		StringBuffer boundarySeparatorTemp;		
		boundarySeparatorTemp << "---------------------------" << system_time();
		requestHeader << "boundary=" << boundarySeparatorTemp.String() << "\r\n";
	
		boundarySeparator << "\r\n--" << boundarySeparatorTemp.String() << "\r\n";
		lastBoundarySeparator << "\r\n--" << boundarySeparatorTemp.String() << "--\r\n";
	}
	
	// build the headers for each of the query strings and files to
	// upload. We need to do this now so we can calculate the length
	// of the entire thing.
	BArray<requestSection *> sectionArray;

	// scan through each of the queries
	{
		int stringPos = 0;
		int queryLen = strlen(fUrl.GetQuery());
		char *queryString = new char[queryLen+1];
		strncpy(queryString, fUrl.GetQuery(), queryLen);
		
		for(;;) {
			int stop = stringPos;
			while(queryString[stop] != '&' &&
			  queryString[stop] != '=' &&
			  queryString[stop] != '\0') {
				stop++;
			}
			if(queryString[stop] == '&') {
				// start of a new key/value pair. not sure how we got here
				stringPos = stop + 1;
				continue;
			}
			if(queryString[stop] == '\0' || stop == stringPos)
				break;
			queryString[stop] = '\0';
			
			BString queryParam;
			if(fUrl.GetQueryParameter(&queryString[stringPos], &queryParam) != B_OK) {
				stringPos = stop+1;
				continue; // huh?
			}
			
			// build this section of the request
			requestSection *newSection = new requestSection;
			newSection->protocol = 0;

			newSection->sectionText << "Content-Disposition: form-data; ";
			newSection->sectionText << "name=\"" << &queryString[stringPos] << "\"\r\n\r\n";
			newSection->sectionText << queryParam.String() << "\r\n";
			
			sectionArray.AddItem(newSection);
			
			// walk forward until the next query
			stop++;
			while(queryString[stop] != '&' &&
			  queryString[stop] != '\0') {
				stop++;
			}
			if(queryString[stop] == '\0')
				break; // we're done
			
			stringPos = stop+1;
		}
		delete queryString;	
	}	

	// Now we need to look at the files that need to be uploaded
	// This involves parsing another query string that is stuck onto
	// the URL object
	// XXX should merge this code with previous
	{
		int stringPos = 0;
		int queryLen = strlen(fUrl.GetUploadQuery());
		char *queryString = new char[queryLen+1];
		strncpy(queryString, fUrl.GetUploadQuery(), queryLen);
		
		for(;;) {
			int stop = stringPos;
			while(queryString[stop] != '&' &&
			  queryString[stop] != '=' &&
			  queryString[stop] != '\0') {
				stop++;
			}
			if(queryString[stop] == '&') {
				// start of a new key/value pair. not sure how we got here
				stringPos = stop + 1;
				continue;
			}
			if(queryString[stop] == '\0' || stop == stringPos)
				break;
			queryString[stop] = '\0';

			BString uploadURL;
			if(fUrl.GetUploadQueryParameter(&queryString[stringPos], &uploadURL) != B_OK) {
				stringPos = stop+1;
				continue; // huh?
			}

			URL url(uploadURL.String());
						
			requestSection *newSection = new requestSection;

			// build a protocol to cover this file
			newSection->protocol = Protocol::InstantiateProtocol(url.GetScheme());
			if(newSection->protocol == 0) {
				delete newSection;
				delete queryString;
				err = B_ERROR;
				goto out;
			}
			
			BMessage errmessage;
			if(newSection->protocol->Open(url, url, &errmessage, 0) < 0) {
				// error opening protocol
				delete newSection->protocol;
				delete newSection;
				delete queryString;
				err = B_ERROR;
				goto out;
			}

			newSection->streamBytes = newSection->protocol->GetContentLength();
			if(newSection->streamBytes < 0 || newSection->streamBytes == 0x7fffffff) {
				// gotta know the size of the file
				delete newSection->protocol;
				delete newSection;
				delete queryString;
				err = B_ERROR;
				goto out;
			}
			
			newSection->sectionText << "Content-Disposition: form-data; ";
			newSection->sectionText << "name=\"" << &queryString[stringPos] << "\"; ";
			newSection->sectionText << "filename=\"" << url.GetPath() << "\"\r\n";
			newSection->sectionText << "Content-Type: application/octet-stream\r\n\r\n";

			sectionArray.AddItem(newSection);

			// walk forward until the next query
			stop++;
			while(queryString[stop] != '&' &&
			  queryString[stop] != '\0') {
				stop++;
			}
			if(queryString[stop] == '\0')
				break; // we're done
			
			stringPos = stop+1;
		}
		delete queryString;	
	}		

	// Lets figure out how big the request is going to be
	requestSize = 0;
	requestSize += boundarySeparator.Length() - 2; //  boundary sep + \r\n
	for(int i=0; i<sectionArray.CountItems(); i++) {
		requestSection *section = sectionArray.ItemAt(i);
		
		requestSize += section->sectionText.Length();
		
		// if this is a file attachment, add the size of the file
		if(section->protocol)
			requestSize += section->streamBytes;
		
		requestSize += boundarySeparator.Length();
	}
	requestSize += 2; // counts the last '--' added to the last boundary separator

	// finish the header
	requestHeader << "Content-Length: " << requestSize << "\r\n";
	
	// send the initial data back to the calling content instance
	totalBytes = requestSize + requestHeader.Length();
	SendCallback("totalBytes", (char *)&totalBytes);
	sentBytes = 0;

#if ENABLE_HTTP_TRACE
	{
		StringBuffer urlBuf;
		urlBuf << fUrl;
		printf("Request (%s)\n"HTTP_REQUEST_COLOR"%s"HTTP_NORMAL_COLOR"\n", urlBuf.String(),
			requestHeader.String());
	}
#endif

	// start streaming the data
	err = fConnection->Write(requestHeader.String(), requestHeader.Length());
	if(err < 0)
		goto out;
	sentBytes += err;
	SendCallback("sentBytes", (char *)&sentBytes);

	err = fConnection->Write(boundarySeparator.String(), boundarySeparator.Length());
	if(err < 0)
		goto out;
	sentBytes += err;
	SendCallback("sentBytes", (char *)&sentBytes);

	for(int i=0; i<sectionArray.CountItems(); i++) {
		requestSection *section = sectionArray.ItemAt(i);
		
		err = fConnection->Write(section->sectionText.String(), section->sectionText.Length());
		if(err < 0)
			goto out;
		sentBytes += err;
		SendCallback("sentBytes", (char *)&sentBytes);
	
		if(section->protocol) {
			// this is coming from a URL and needs to streamed
			size_t bytesLeft = section->streamBytes;
			while(bytesLeft) {
				char buf[16*1024];
				
				ssize_t amountToRead = MIN(bytesLeft, sizeof(buf));
				ssize_t amountRead = section->protocol->Read(buf, amountToRead);
				if(amountRead <= 0) {
					err = -1;
					goto out;
				}
				
				ssize_t amountWritten = fConnection->Write(buf, amountRead);
				if(amountWritten != amountRead) {
					// error
					section->protocol->Abort();
					err = -1;
					goto out;
				}
				bytesLeft -= amountRead;
				sentBytes += amountRead;
				SendCallback("sentBytes", (char *)&sentBytes);
			}
		}

		// if we're the last section, send the different boundary seperator
		if(i == sectionArray.CountItems() - 1) {
			err = fConnection->Write(lastBoundarySeparator.String(), lastBoundarySeparator.Length());
		} else {
			err = fConnection->Write(boundarySeparator.String(), boundarySeparator.Length());
		}			
		if(err < 0)
			goto out;
		sentBytes += err;
		SendCallback("sentBytes", (char *)&sentBytes);
	}

out:
	for(int i=0; i<sectionArray.CountItems(); i++) {
		requestSection *section = sectionArray.ItemAt(i);
		if(section->protocol) {
			delete section->protocol;
		}
		delete section;
	}	
	sectionArray.MakeEmpty();

	return err;
}

status_t Http::SendRequestUrlEncoded(HttpRequestType type, int from, int to)
{
	StringBuffer requestHeader;
	BuildRequestHeader(&requestHeader, type, from, to);
	
	// Send body of post if needed.
	if (type == kHttpPost) {
		requestHeader << "Content-type: application/x-www-form-urlencoded\r\n";
		requestHeader << "Content-length: " << strlen(fUrl.GetQuery()) << "\r\n\r\n";
		requestHeader << fUrl.GetQuery();	
	}

	requestHeader << "\r\n";

#if ENABLE_HTTP_TRACE
	StringBuffer urlBuf;
	urlBuf << fUrl;
	printf("Request (%s)\n"HTTP_REQUEST_COLOR"%s"HTTP_NORMAL_COLOR"\n", urlBuf.String(),
		requestHeader.String());
#endif

	return fConnection->Write(requestHeader.String(), requestHeader.Length());	
}

status_t Http::ParseResponseHeader(BMessage* outErrorInfo)
{
#if ENABLE_HTTP_TRACE
	StringBuffer _response;
#endif

	fResultCode = 0;
	fSimpleResponseLen = 0;

	enum {
		kScanProtocol,
		kScanResultCode,
		kScanErrorString,
		kScanPreTagWhitespace,
		kScanTag,
		kScanPostTagWhitespace,
		kScanValue
	} state = kScanProtocol;

	StringBuffer errorString(256);	
	StringBuffer currentTag(1024);
	StringBuffer currentValue(4096);
	bool finished = false;
	status_t error = B_OK;
	while (!finished) {
		char c;
		error = fConnection->Read(&c, 1);
		if (error <= 0) {
			finished = true;
			c = '\n';
			if (error == 0) {
				error = B_ERROR;
			}
		}

#if ENABLE_HTTP_TRACE
		_response << c;
#endif
		switch (state) {
			case kScanProtocol: {
				//we'll be very strict in validating the protocol version,
				//and assume an invalid character indicates that we're
				//processing a HTTP/0.9 simple-response (not a full-response)
				bool simple = false;

				//remember the characters we see, so they can be re-Read()
				//later if we determine this is a simple-response
				fSimpleResponse[fSimpleResponseLen] = c;

				switch (fSimpleResponseLen) {
				case 5:
				case 7:
					//these must be digits (the http major/minor version)
					simple = !isdigit(c);
					break;
				
				case 8:
					//this must be the space which trails the version
					if (c == ' ') {
						//this version string is valid - assume full-reponse
						state = kScanResultCode;
						fResultCode = 0;
					} else
						simple = true;
					break;

				case 0:
					//although the RFC specifically forbids it, some
					//HTTP servers begin their responses with leading
					//blank lines - ignore them
					if (c == 10 || c == 13)
						continue;

					//this isn't a leading blank line - fall through

				default:
					simple = (c != kHttpVersion[fSimpleResponseLen]);
					break;
				}

				fSimpleResponseLen++;

				if (simple) {
					//this is a simple-response - there's no (more) header
					//to parse (note that the content-type isn't set - we'll
					//let libwww guess it based on our extension)
#if ENABLE_HTTP_TRACE
					StringBuffer buf;
					buf << fUrl;
					printf("Response (%s)\n"
						HTTP_RESPONSE_COLOR"simple-response"HTTP_NORMAL_COLOR"\n", buf.String());
#endif
					return B_OK;
				}
				break; }
				
			case kScanResultCode:
				if (isdigit(c)) {
					fResultCode = fResultCode * 10 + c - '0';
					currentValue.Append(c);
				} else {
					if (fResultCode >= kHttpErrorBase) {
						// add the result code to the error message
						StringBuffer tmp(4);
						tmp << fResultCode;
						outErrorInfo->AddString("result_code", tmp.String());
					}
					
					state = kScanErrorString;
				}
				
				break;
	
			case kScanErrorString:
				if (c == '\n') {
					state = kScanPreTagWhitespace;
					currentValue.Clear();
					currentTag.Clear();
				} else if (c != '\r')
					errorString << c;
				
				break;
			
			case kScanPreTagWhitespace:
				if (c == '\r')
					break;
					
				if (c == '\n') {
					finished = true;
					break;
				}

				if (isspace(c))
					break;
					
				state = kScanTag;
				// falls through...
				
			case kScanTag:
				if (c == '\n')
					state = kScanPreTagWhitespace;	// Malformed header
				else if (c != ':')
					currentTag.Append(c);
				else
					state = kScanPostTagWhitespace;
				
				break;
				
			case kScanPostTagWhitespace:
				if (c == '\n') {
					state = kScanPreTagWhitespace;	// Malformed header
					break;
				} else if (isspace(c))
					break;
				
				state = kScanValue;
				
				// falls through...
				
			case kScanValue:
				if (c == '\n') {
					ParseHeaderTag(currentTag.String(), currentValue.String());
					currentValue.Clear();
					currentTag.Clear();
					state = kScanPreTagWhitespace;
				} else if (c != '\r')
					currentValue.Append(c);

				// handle LWS here too...
	
				break;
		}
	}

	//we only get here when a full-response has been processed
	fSimpleResponseLen = 0;

	if (state != kScanPreTagWhitespace && state != kScanPostTagWhitespace)
		return error;	// Truncated header

	if (errorString.Length() > 0 && fResultCode != kHttpOk)
		outErrorInfo->AddString("http_message", errorString.String());
	
#if ENABLE_HTTP_TRACE
	StringBuffer buf;
	buf << fUrl;
	printf("Response (%s)\n"
		HTTP_RESPONSE_COLOR"%s"HTTP_NORMAL_COLOR"\n", buf.String(), _response.String());
#endif
	
	return B_OK;
}

void Http::ParseHeaderTag(const char *tag, const char *value)
{
//	printf("ParseHeaderTag '%s: %s'\n",tag,value);
	if (strcasecmp(tag, "location") == 0 && fResultCode >= kHttpRedirectBase &&  
		fResultCode <= kHttpRedirectTop) {
		fRedirectDelay = 0;
		fRedirectUrl.SetTo(fUrl, value);
	} else if (strcasecmp(tag, "content-length") == 0) {
		if (fLastRequest != kHttpHead) {
			fContentRead = 0;
			fExpectedContentLength = atoi(value);
		}
		// The content length specifies the length of the current response.
		//  * In non-random access there is only one response.
		//  * In random access there will be multiple responses, so only update
		//     the first time (which will always be the HEAD request).
		if (!(fFlags & RANDOM_ACCESS) || fFullContentLength == kUnknownLength)
			fFullContentLength = atoi(value);
	} else if (strcasecmp(tag, "date") == 0) {
		fServerDate = parsedate(value,-1);
	} else if (strcasecmp(tag, "content-type") == 0) {
		fContentType = value;
	} else if (strcasecmp(tag, "set-cookie") == 0) {
		cookieManager.ParseCookieString(fUrl, value, fServerDate);
	} else if (strcasecmp(tag, "transfer-encoding") == 0) {
		fIsChunked = strcasecmp(value, "chunked") == 0;
	} else if (strcasecmp(tag, "connection") == 0
		|| strcasecmp(tag, "proxy-connection") == 0) {
		fKeepAlive = strcasecmp(value, "keep-alive") == 0;
	} else if (strcasecmp(tag, "keep-alive") == 0) {
		fKeepAlive = true;
		ParseKeepAlive(value);
	} else if (strcasecmp(tag, "cache-control") == 0 && fCachePolicy != CC_NO_CACHE) {
		if (strcasecmp(value, "no-cache") == 0 || strcasecmp(value, "private") == 0)
			fCachePolicy = CC_NO_CACHE;
		else if (strcasecmp(value, "no-store") == 0)
			fCachePolicy = CC_NO_STORE;
	} else if (strcasecmp(tag, "expires") == 0 && atoi(value) == 0) {
 		fCachePolicy = CC_NO_CACHE;
	} else if (strcasecmp(tag, "www-authenticate") == 0) {
		ParseAuthenticate(value);
	} else if (strcasecmp(tag, "refresh") == 0) {
		ParseRefresh(value);
	} else if (strcasecmp(tag, "pragma") == 0) {
		ParsePragma(value);
	} else if (strcasecmp(tag, "accept-ranges") == 0) {
		fAcceptsRanges = strcasecmp(value, "bytes") == 0;
	} else if (strcasecmp(tag, "x-beia-dms-connect") == 0) {
		BinderNode::property cookie(value);
		cookie = BinderNode::Root()["service"]["mca"]["DMS_Connect"](&cookie,NULL);
		printf("calling Connect function '%s'!\n",cookie.String().String());
	}
	
	SendCallback(tag, value);
}

void Http::ParseKeepAlive(const char *in)
{
	while (*in) {
		while (*in && !isalpha(*in))
			in++;
			
		int len = 0;
		while (isalpha(in[len]))
			len++;

		if (len >= 64) {
			PRINT(("bad name in keep alive response\n"));
			return;
		}
			
		char name[64];
		memcpy(name, in, len);
		name[len] = '\0';
		in += len;
		
		int value = 0;
		while (*in && !isdigit(*in))
			in++;

		while (*in && isdigit(*in))
			value = value * 10 + *in++ - '0';

		if (strcasecmp(name, "timeout") == 0)
			fKeepAliveTimeout = value * 1000000;		// timeout is in seconds
		else if (strcasecmp(name, "max") == 0)
			fMaxKeepAliveRequests = value;
		else
			PRINT(("unknown Keep-alive parameter \"%s\"\n", name));
	}
}

void Http::ParseAuthenticate(const char *in)
{
	enum {
		kScanScheme,
		kScanName,
		kScanStartQuote,
		kScanRealm,
		kScanEndQuote
	} state = kScanScheme;

	StringBuffer tmp;
	while (*in) {
		switch (state) {
			case kScanScheme:
				if (isspace(*in)) {
					if (strcasecmp(tmp.String(), "basic") != 0) {
						printf("Unimplemented authentication scheme\n");
						return;
					}
					
					state = kScanName;
					tmp.Clear();
				} else
					tmp.Append(*in);
	
				break;
			
			case kScanName:
				if (*in == '=') {
					if (strcasecmp(tmp.String(), "realm") == 0)
						state = kScanStartQuote;
					else {
						printf("unknown authentication parameter \"%s\"\n", tmp.String());
						return;
					}
									
					tmp.Clear();	
				} else
					tmp.Append(*in);
				
				break;			
			
			case kScanStartQuote:
				if (*in == '\"')
					state = kScanRealm;
				else {
					printf("Parse error reading authenticate header\n");
					return;
				}
				
				break;
			
			case kScanRealm:
				if (*in == '\"') {
					state = kScanEndQuote;
					fAuthenticationRealm = tmp.String();
				} else
					tmp.Append(*in);
	
				break;
			
			case kScanEndQuote:
				break;
		}
		
		in++;
	}	
}

void Http::ParseRefresh(const char *in)
{
	// of the form: "10; URL=http://foo.bar.org/somefile"
	fRedirectDelay = static_cast<bigtime_t>(atoi(in)) * 1000000;
	char *start = strchr(in, '=');
	if (start)
		fRedirectUrl.SetTo(start + 1);
}

void Http::ParsePragma(const char *value)
{
	if (strcasecmp(value, "no-cache") == 0 || strcasecmp(value, "private") == 0)
		fCachePolicy = CC_NO_CACHE;
	else if (strcasecmp(value, "no-store") == 0)
		fCachePolicy = CC_NO_STORE;
}

void Http::SendCallback(const char *name, const char *value)
{
	if (fMetaCallback)
		(*fMetaCallback)(fMetaCookie, name, value);
}

bool Http::SetConnection(Connection *connection)
{
	//see comment at top of Abort()
	GehnaphoreAutoLock _lock(fConnectionLock);
	
	//put any old connection back into the pool
	if (fConnection != NULL)
		ReturnConnectionToPool();

	//remember our new connection (it must be set if we call
	//ReturnConnectionToPool() below)
	fConnection = connection;

	if (fAborted == true) {
		//we've been aborted - don't bother
		if (fConnection != NULL)
			ReturnConnectionToPool();
		return false;
	}

	return true;
}

void Http::ReturnConnectionToPool(bigtime_t keepAliveTimeout, int maxKeepAliveRequests)
{
	if (unencryptedConnectionPool.ReturnConnection(fConnection, keepAliveTimeout,
		maxKeepAliveRequests) <= B_ERROR) {
		if (encryptedConnectionPool.ReturnConnection(fConnection, keepAliveTimeout,
			maxKeepAliveRequests) <= B_ERROR) {
			encryptedProxiedConnectionPool.ReturnConnection(fConnection, keepAliveTimeout,
				maxKeepAliveRequests);
		}
	}

	fConnection = NULL;
}

void Http::ReturnConnectionToPool()
{
	bigtime_t keepAliveTimeout;
	int maxKeepAliveRequests;
	if (!fKeepAlive || fContentRead < fExpectedContentLength) {

#if ENABLE_HTTP_TRACE
		if (fContentRead < fExpectedContentLength) {
			HTTP_TRACE((HTTP_STATUS_COLOR
				"HTTP Protocol %p returning Connection %p to not be re-used because fContentRead = %d < fExpectedContentLength = %d\n"
				HTTP_NORMAL_COLOR,
				this, fConnection, fContentRead, fExpectedContentLength));
		}
#endif // ENABLE_HTTP_TRACE

		keepAliveTimeout = 0;
		maxKeepAliveRequests = 0;
	} else {
		keepAliveTimeout = fKeepAliveTimeout;
		maxKeepAliveRequests = fMaxKeepAliveRequests;
	}
	
	ReturnConnectionToPool(keepAliveTimeout, maxKeepAliveRequests);
}


status_t EncryptedProxiedHTTPConnection::OpenProxied(const char *hostname, int port,
	const char *proxyHostname, int proxyPort)
{
	status_t error = OpenUnencrypted(proxyHostname, proxyPort);

	if (error >= B_OK) {
		StringBuffer requestHeader;
		requestHeader << "CONNECT " << hostname << ":" << static_cast<unsigned>(port)
			<< ' ' << kHttpVersion << "\r\n";
		requestHeader << "Host: " << hostname << ":" << static_cast<unsigned>(port)
			<< "\r\n";
		
		char proxyUser[255];
		char proxyPassword[255];
		if (get_proxy_password(proxyUser, proxyPassword)) {
			// Authenticate with the proxy server.
			requestHeader << "Proxy-Authorization: Basic ";
			EncodeBasicAuthentication(requestHeader, proxyUser, proxyPassword);
			requestHeader << "\r\n";
		}
		
		requestHeader << "\r\n";

		HTTP_TRACE(("Request (tunnel)\n"HTTP_REQUEST_COLOR"%s\n"HTTP_NORMAL_COLOR,
			requestHeader.String()));

		// Note that this will be unencrypted.
		BufferedConnection::UnbufferedWrite(requestHeader.String(), requestHeader.Length());
		
		error = EatHeader();
		
#if ENABLE_HTTP_TRACE
		if (error <= B_ERROR) {
			HTTP_TRACE(("EatHeader() failed.\n"));
		}
#endif // ENABLE_HTTP_TRACE

		if (error >= B_OK)
			error = EncryptConnection();
	}

	if (error <= B_ERROR)
		Close();

	return error;
}

status_t EncryptedProxiedHTTPConnection::EatHeader()
{
	// I don't really care about anything in this header, so just eat it.
	// Eats characters until it finds two newlines separated only by any
	//  number (including zero) of carriage returns.
	HTTP_TRACE(("Eating header: "));
	status_t error = B_OK;
	int nlCount = 0;
	while (nlCount < 2) {
		char c;
		error = BufferedConnection::UnbufferedRead(&c, 1);
		if (error <= B_ERROR) {
			break;
		}
		
		HTTP_TRACE(("%c", c));
		
		if (c == '\r')
			continue;
		else if (c == '\n')
			nlCount++;
		else
			nlCount = 0;
	}
	
	return error;
}

status_t EncryptedProxiedHTTPConnectionPool::CreateConnection(const char *hostname,
	int port, Connection **new_connection)
{
	EncryptedProxiedHTTPConnection *con = new EncryptedProxiedHTTPConnection;
	
	char proxyHost[255];
	int proxyPort;
	get_proxy_server(proxyHost, &proxyPort);

	status_t error;
	if (proxyHost[0] != '\0')
		error = con->OpenProxied(hostname, port, proxyHost, proxyPort);
	else
		error = con->Open(hostname, port);

	if (error >= B_OK) {
		*new_connection = con;
	}
	else {
		delete con;
	}
	
	return error;
}
