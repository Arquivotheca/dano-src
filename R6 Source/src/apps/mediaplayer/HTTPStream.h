#ifndef _HTTP_STREAM_H
#define _HTTP_STREAM_H

#include <OS.h>
#include <Locker.h>
#include "TCPSocket.h"
#include "FileCache.h"
#include "MemoryCache.h"
#include "StreamHandler.h"
#include "MediaController.h"

class HTTPAttrInfo;
class MemoryCache;

class HTTPStream : public StreamHandler {
public:

	static StreamHandler* InstantiateHTTPStream(const char *scheme);
	
	HTTPStream();
	virtual ~HTTPStream();
	
	status_t InitCheck() const;
	virtual status_t SetTo(const URL&, BString &outError);
	status_t Unset();

	virtual	ssize_t ReadAt(off_t pos, void *buffer, size_t size);
	virtual bool IsDataAvailable(off_t pos, size_t size);
	void SetCookies(const char*);
	virtual off_t Seek(off_t position, uint32 seek_mode);
	virtual	off_t Position() const;
	virtual off_t GetDownloadedSize() const;
	virtual size_t GetLength() const;
	virtual bool IsContinuous() const;
	virtual bool IsBuffered() const;
	virtual const char* GetConnectionName();
	virtual void DescribeConnection(BString &outString);
	virtual float BufferUtilization() const;
	virtual void GetStats(player_stats*);

private:

	static int32 StartDownloadThread(void *stream);
	void DownloadThread();
	bool HandleSocketError();
	static void InterruptSocketRead(int);
	status_t SendRequest(off_t start = 0, off_t size = 0x7fffffffffffffffLL);
	void EscapeString(BString *outEscaped, const char *in);

	BLocker fFileCacheLock;
	FileCache *fFileCache;
	MemoryCache *fMemoryCache;
	BString fConnectionName;

	off_t fFileLength;
	status_t fError;
	off_t fReadOffset;
	off_t fRequestedPosition;
	size_t fRequestedSize;
	TCPSocket fSocket;
	bool fUseByteRanges;
	size_t fChunkBytesToRead;
	
	int fNumHangUps;
	sem_id fDataAvailable;

	thread_id fDownloadThread;
	off_t fDownloadPos;
	BString fFilePath;
	BString fHostName;
	
	off_t fDownloadedSize;
	
	bool fClosing;
	bool fUnknownFileSize;

	BString fCookieString;
	
	MemoryCache *fMemBuffer;
	BString fDescription;
	
	// Stats
	double fDownloadRate;
	double fRawDataRate;
	bigtime_t fLastRead;
};

#endif
