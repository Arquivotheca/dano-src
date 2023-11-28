//=======================================================================
//	Protocols.h
//  Copyright 1998 by Be Incorporated.
//=======================================================================

#ifndef __PROTOCOLS__
#define __PROTOCOLS__

#include "FileSystemHTML.h"
#include "UResource.h"

#include <Looper.h>
#include <String.h>

class UResourceImp;
class ConnectionManager;
class ConnectionInfo;
class FileSystemHTML;
class BeSSL;

#define MAX_URL_LENGTH 1024
#define SHOW_NO_ERROR "sner"

//=======================================================================
//	Ring buffer for streaming data

class Buffer {
public:
					Buffer(uint32 bufferSize = 0x1000);
	virtual			~Buffer();
	
			void	Reset();
			
	virtual	long	ReadLine(char *line, int maxSize);			// Buffer methods	
	
	virtual	void*	GetReadBuffer(uint32* maxSize);
	virtual	void	ReadBuffer(uint32 size);
	
	virtual	void*	GetWriteBuffer(uint32* maxSize);
	virtual	void	WriteBuffer(uint32 size);
		
protected:
			char*	mBuffer;
			uint32	mBufferSize;
			long	mBufferMask;
			uint32	mHead;
			uint32	mTail;
};


//=======================================================================

enum NetResult {
	NET_NO_ERROR			=  0,
	NET_ERROR				= -1,
	NET_NO_SOCKETS			= -2,
	NET_UNKNOWN_HOST		= -3,
	NET_CANT_CONNECT		= -4,
	NET_CANT_SEND			= -5,
	NET_CANT_RECV			= -6,
	NET_TIMEOUT				= -7,
	NET_IS_CLOSED			= -8,
	NET_ALREADY_CLOSED		= -9,
	NET_DONT_DOWNLOAD		= -10,
	NET_REFUSED				= -11,
	NET_UNAUTHORIZED		= -12,
	NET_FILE_NOT_FOUND		= -13,

	NET_GET_URL				= 'nget',
	NET_CONNECTION_AVAIL	= 'ncav',
	NET_AUTH_CANCELED		= 'ncan',
		
//	HTTP Header Fields

	HTTP_ERROR				= -101

//	FTP  Results

};

class Proxy {
public:
					Proxy();

	void			SetActive(bool active);
	bool			Active() const;

	void			SetHostPort(int port);
	void			SetSecurePort(int port);
	int				HostPort(URLParser &url, bool secure) const;

	void			SetHostName(const BString &name);
	const char*		HostName(URLParser &url) const;

	void			GetNameAndPort(BString &name, int *port, bool secure) const;

private:
	bool			mActive; 
	int				mPort;
	int				mSecurePort;
	BString			mName;
};

//=======================================================================
//	Create the correct kind of protocol based on the url

BMessenger *CreateProtocol(char *url);

const char* NETRESULTSTR(NetResult result);

//=======================================================================
//	Protocol Base Class

class Protocol : public BLooper {
friend class ConnectionInfo;
friend class SSLConnectionInfo;
public:
					Protocol(ConnectionManager *mgr, UResourceImp *resImp);
virtual				~Protocol();
		void		Abort();
		void		SetConnectionManager(ConnectionManager *mgr);
		URLParser *	GetParser() {return &mURL;}
		UResourceImp*	GetResource() {return mResImp;}

static void			Init();
static void			Cleanup();
static BLooper*		mDNSLooper;

protected:
		void		CloseSocket();
		void		SetStatus(StoreStatus status);
		void		RequestConnection();
//virtual bool		QuitRequested();
		
//		Main phases of all protocols
virtual	NetResult	Connect(bool *reused, const char *proxyName = NULL, int proxyPort = 0, bool secure = false);
//virtual	NetResult	Request();
virtual	NetResult	Response() = 0;

//			Reading and writing data
		long		FillBuffer(int socket,Buffer& buffer);
		NetResult	ReadLine(char *str, int maxSize);
virtual	bool		SkipBody();
virtual	NetResult	ReadBody() = 0;

virtual	const Proxy&	ProxyObject();

		void		CreateErrorMessage(NetResult result, BString &message);
		
//			Every protocol has a socket and a buffer...
		ConnectionInfo	*mConnection;
		UResourceImp*	mResImp;
		URLParser		mURL;
		
		long		mResponseCode;
		Buffer		mBuffer;
		long		mDataCount;
		bool		mDownload;
		bool		mSocketLocked;
		ConnectionManager *mConnectionMgr;
		bool		mAbortRequested;
		bool		mSecure;
		int32		mRangeBegin;	// This is the beginning of the byte range request we send to the server.
		int32		mFirstByte;		// This is the beginning of the byte range we get back from the server.

static	Proxy		sProxy;	// dummy
};

//=======================================================================
//	HTTP

class HTTP : public Protocol {
public:
					HTTP(ConnectionManager *mgr, UResourceImp *res, bool neverReuse = false);

static void			SetProxyActive(bool active);
static void			SetProxyNameAndPort(const BString &name, int port, int securePort);
static bool			ProxyActive();
static void			GetProxyNameAndPort(BString &name, int *port, int *securePort);

protected:
virtual	void		MessageReceived(BMessage *msg);
			
//		Custom request and response handling for HTTP

virtual	NetResult	Request(const char *formData, const char *authorization, const char *realm, const char *referrer, const char *proxyName, int proxyPort);
virtual	NetResult	Response();
		NetResult	ProcessRequest(const char *formData, const char *authorization, const char *realm, const char *referrer, const char *proxyName, int proxyPort);

//		HTTP requires Redirection and authentication

		NetResult	Redirect(char *url);
		NetResult	Authenticate(char *field);
virtual	bool		SkipBody();

		NetResult	SetLastModified(long modified);
		NetResult	ReadBody();

virtual const Proxy&	ProxyObject();
//		
		bool		mRedirecting;
		bool		mAuthenticating;
		bool		mSentAuthentication;
		bool		mUseCache;
		bool		mHeadersOnly;
		bool		mResourceIsCached;
		BString		mRealm;
		BString		mFormData;
		BString		mAuthorization;
		BString		mReferrer;
		bool		mChunked;
		BString		mProxyName;
		int			mProxyPort;
		bool		mNeverReuse;
		
//		UResourceImp*	mRes;

static	Proxy		sProxy;
};

//=======================================================================
//	FTP

class FTP : public Protocol {
public:
					FTP(ConnectionManager *mgr, UResourceImp *resImp);

static void			SetProxyActive(bool active);
static void			SetProxyNameAndPort(const BString &name, int port);
static bool			ProxyActive();
static void			GetProxyNameAndPort(BString &name, int *port);

protected:

virtual	void		MessageReceived(BMessage *msg);
virtual	NetResult	CloseSocket();
			
//		Custom request and response handling for FTP

		NetResult	LogOn();
		NetResult 	Command(char *str);
		NetResult	OpenDataSocket();
		
		NetResult	ReadDir();
		void		AddLine(FileSystemHTML& dir, const char* line);
		
virtual	NetResult	Request();
virtual	NetResult	Response();
virtual	NetResult	ReadBody();

virtual const Proxy&	ProxyObject();

//
		
protected:
		int			mResponseCode;
		long		mPASVAddr;
		long		mPASVPort;
		
		BString		mWorkingDir;
		
		int			mDataSocket;
		long		mDataSize;

static	Proxy		sProxy;
};


class ConnectionManager : public Counted {
public:		
					ConnectionManager();
		void		SetMaxConnections(uint32 max);
		void		RequestConnection(Protocol *protocol, bool secure);
		void		ConnectionFinished(Protocol *protocol);
		void		KillAllConnections();
		void		SetConnectionReuse(ConnectionInfo *connection, bool reuse);
		void		GetFTP(UResourceImp *resource, const BString& url, const char *downloadPath, bool forceDownload, int32 rangeBegin = 0);
		void		GetHTTP(UResourceImp *resource, const BString& url, BString* formData, BString* referrer, const char *downloadPath, bool secure = false, const char *proxyName = NULL, int proxyPort = 0, bool headersOnly = false, bool forceDownload = false, int32 rangeBegin = 0, bool neverReuse = false);
		long		RequestSocket();
		long		GetTotalNumConnections();
		bool		AdoptResource(UResourceImp* resource, ConnectionManager *fromMgr);
		
		void		SetReferrer(const char *referrer) {mReferrer = referrer;}
		const char *GetReferrer() {return mReferrer.String();}
		
protected:
		bool		ActivateQueuedConnection();
		
		uint32		mMaxConnections;
		BList		mPendingConnections;
		BList		mQueuedConnections;
		TLocker		mLocker;
		BString		mReferrer;

//private:
					~ConnectionManager();
};

class ConnectionInfo : public Counted {
public:
				ConnectionInfo();
				
	bool		LockSocket();
	void		UnlockSocket();
	
	virtual NetResult	Connect(const BString& host, int port, Protocol *protocol);
	virtual bool		IsOpen();
	virtual	long		FillBuffer(Buffer& buffer);
	virtual	long		Send(const void *data, long length);
		
	BList		mProtocols;
	BString		mHostName;
	int			mHostPort;
	bool		mReuse;
	bool		mSecure;

protected:
	long		mSocket;
	bool		mDontUse;
//private:
	virtual		~ConnectionInfo();
	TLocker		mSocketLocker;
};

#ifndef NOSSL
class SSLConnectionInfo : public ConnectionInfo {
public:
				SSLConnectionInfo();
	virtual NetResult	Connect(const BString& host, int port, Protocol *protocol);
	virtual bool		IsOpen();
	virtual	long		FillBuffer(Buffer& buffer);
	virtual	long		Send(const void *data, long length);

protected:
	BeSSL		*mBeSSL;
	bool		mIsOpen;
	virtual		~SSLConnectionInfo();
};
#endif

class DNSLooper : public BLooper {
public:
				DNSLooper();
virtual void	MessageReceived(BMessage *msg);
static ulong	GetHostByName(const char *host);

protected:
	enum {	msg_GetHostByName = 'ghbn',
			msg_Failure = 'fail',
			msg_Success = 'good'
	};
};
#endif
