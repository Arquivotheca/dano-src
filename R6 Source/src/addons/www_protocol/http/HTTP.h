#ifndef _HTTP_H
#define _HTTP_H

#include "Protocol.h"
#include "ConnectionPool.h"
#include "SSLConnection.h"

using namespace Wagner;

class Connection;

class Http : public Protocol {
public:
	Http(void *handle);
	virtual ~Http();
	virtual status_t Open(const URL &url, const URL &referrer, BMessage *outErrorInfo,
		uint32 flags);
	virtual ssize_t GetContentLength();
	virtual void GetContentType(char*, int);
	virtual CachePolicy GetCachePolicy();
	virtual ssize_t Read(void*, size_t);
	virtual ssize_t ReadAt(off_t, void*, size_t);
	virtual off_t Seek(off_t, uint32);
	virtual	off_t Position() const;
	virtual bool GetRedirectURL(URL&, bigtime_t *outDelay);
	virtual void Abort();
	virtual status_t SetMetaCallback(void*, MetaCallback);

private:
	enum HttpRequestType {
		kHttpOptions,
		kHttpGet,
		kHttpHead,
		kHttpPost,
		kHttpPut,
		kHttpDelete,
		kHttpTrace
	};

	ssize_t ReadCommon(int pos, void*, int count);
	ssize_t ReadByteRange(int pos, void*, int count);
	ssize_t ReadChunked(void*, int);
	ssize_t ReadRaw(void*, int);
	status_t OpenInternal(BMessage *outErrorParams, bool sendInitialRequest);
	status_t SendRequest(HttpRequestType, int from = -1, int to = -1);
	status_t SendRequestMultipartEncoded(HttpRequestType, int from = -1, int to = -1);
	status_t SendRequestUrlEncoded(HttpRequestType, int from = -1, int to = -1);
	status_t BuildRequestHeader(StringBuffer *requestHeader, HttpRequestType type, int from = -1, int to = -1);
	status_t ParseResponseHeader(BMessage *outErrorParams);
	void ParseHeaderTag(const char*, const char*);
	void ParseKeepAlive(const char*);
	void ParseAuthenticate(const char*);
	void ParseRefresh(const char*);
	void ParsePragma(const char*);
	void SendCallback(const char *name, const char *val);
	bool SetConnection(Connection *connection);
	void ReturnConnectionToPool(bigtime_t keepAliveTimeout, int maxKeepAliveRequests);
	void ReturnConnectionToPool(/* determines reasonable values */);
	
	Connection *fConnection;
	bool fAborted;
	Gehnaphore fConnectionLock;
	bool fUnencryptedUsingProxy;
	int fFullContentLength;
	size_t fExpectedContentLength;
	size_t fContentRead;
	HttpRequestType fLastRequest;
	uint fFlags;
	bool fIsChunked;
	URL fUrl;
	int fCurrentPosition;
	BString fContentType;
	URL fRedirectUrl;
	bigtime_t fRedirectDelay;
	int fResultCode;
	CachePolicy fCachePolicy;
	BString fAuthenticationRealm;
	URL fReferrer;
	bool fAcceptsRanges;
	bool fKeepAlive;
	bigtime_t fKeepAliveTimeout;
	int fMaxKeepAliveRequests;
	int fCurrentChunkSize;
	time_t fServerDate;
	enum {
		kScanChunkSize,
		kScanChunkExtension,
		kScanChunkData,
		kScanChunkDelimiter,
	} fChunkReadState;
	MetaCallback fMetaCallback;
	void *fMetaCookie;
	int8 fSimpleResponseLen;
	char fSimpleResponse[9];
};

class EncryptedProxiedHTTPConnection: public SSLConnection {
public:
	
	// Additional interface
	virtual status_t OpenProxied(const char *hostname, int port,
		const char *proxyHostname, int proxyPort);

private:

	status_t EatHeader();
};

class EncryptedProxiedHTTPConnectionPool : public ConnectionPool {
public:
	
	virtual status_t CreateConnection(const char *hostname, int port,
		Connection **new_connection);
};

#endif
