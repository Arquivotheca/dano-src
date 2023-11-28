#ifndef _STREAM_HANDLER_H
#define _STREAM_HANDLER_H

#include <DataIO.h>
#include <ObjectList.h>
#include <String.h>

class URL;

class StreamHandler;

typedef StreamHandler* (*StreamHandlerInstantiateFunc)(const char *scheme);

struct StreamHandlerInfo {
	BString fScheme;			
	StreamHandlerInstantiateFunc Instantiate;
};

class StreamHandler : public BPositionIO {
public:
	static void RegisterStreamHandler(const char *scheme,
		StreamHandlerInstantiateFunc instantiate);
	static StreamHandler* InstantiateStreamHandler(const char *scheme);

	StreamHandler();
	virtual ~StreamHandler();
	virtual status_t SetTo(const URL&, BString &outError);
	virtual status_t Unset();
	virtual	ssize_t ReadAt(off_t pos, void *buffer, size_t size);
	virtual bool IsDataAvailable(off_t pos, size_t size);
	virtual void SetCookies(const char*);
	virtual off_t Seek(off_t position, uint32 seek_mode);
	virtual	off_t Position() const;
	virtual off_t GetDownloadedSize() const;
	virtual size_t GetLength() const;
	virtual bool IsContinuous() const;
	virtual bool IsBuffered() const;
	virtual const char* GetConnectionName();
	virtual void DescribeConnection(BString &outString);
	virtual float BufferUtilization() const;
	
	virtual void GetStats(struct player_stats *stats);

private:
	virtual	ssize_t	WriteAt(off_t pos, const void *buffer, size_t size);
	static BObjectList<StreamHandlerInfo> *fStreamHandlers;
};

extern "C" int query_handler(int index, char *out_scheme_name,
	StreamHandlerInstantiateFunc *out_factory);

#endif

