#ifndef _FILE_HANDLER_H
#define _FILE_HANDLER_H

#include <File.h>
#include "StreamHandler.h"
#include "MediaController.h"

class URL;

class FileHandler : public StreamHandler {
public:

	static StreamHandler* InstantiateFileStream(const char*);

	FileHandler();
	virtual status_t SetTo(const URL&, BString &outError);
	virtual status_t Unset();
	virtual	ssize_t ReadAt(off_t pos, void *buffer, size_t size);
	virtual bool IsDataAvailable(off_t pos, size_t size);
	virtual off_t Seek(off_t position, uint32 seek_mode);
	virtual	off_t Position() const;
	virtual off_t GetDownloadedSize() const;
	virtual size_t GetLength() const;
	virtual void GetStats(player_stats*);

private:
	BFile fFile;
	off_t fLength;
	
	bigtime_t fLastRead;
	double fRawDataRate;
};


#endif

