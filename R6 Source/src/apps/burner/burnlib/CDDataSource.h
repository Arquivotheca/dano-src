
#ifndef CDDATASOURCE_H
#define CDDATASOURCE_H

#include <OS.h>
#include <Archivable.h>
#include <storage/Path.h>

class BFile;

// A source of raw data -- should be seekable
// 
class CDDataSource : public BArchivable
{
public:
	virtual ~CDDataSource() {}
	virtual status_t InitCheck(void) = 0;
	virtual status_t Read(void *data, size_t len, off_t posn) = 0;
	virtual size_t Length(void) = 0;
	virtual bool IsAudio() = 0;
	virtual char *Description() = 0;
	
	virtual status_t Archive(BMessage *archive, bool deep = true) const = 0;
};

class CDFillDataSource : public CDDataSource
{
public:
	CDFillDataSource(uchar _fill, size_t _len);
	CDFillDataSource(BMessage *archive);

	virtual status_t InitCheck(void);
	virtual status_t Read(void *data, size_t len, off_t posn);
	virtual size_t Length(void);
	virtual char *Description();
	virtual bool IsAudio();

	static BArchivable *Instantiate(BMessage *archive);
	virtual status_t Archive(BMessage *archive, bool deep = true) const;

private:
	uchar fill;
	size_t len;
};

class BEntry;
class BSoundFile;

// CDAudioFileDataSource is obsoleted by MediaFileDataSource
class CDAudioFileDataSource : public CDDataSource
{
public:
	CDAudioFileDataSource(BEntry *entry);
	CDAudioFileDataSource(BMessage *archive);
	
	virtual status_t InitCheck(void);
	virtual ~CDAudioFileDataSource();
	virtual status_t Read(void *data, size_t len, off_t posn);	
	virtual size_t Length(void);
	virtual char *Description();
	virtual bool IsAudio();

	static BArchivable *Instantiate(BMessage *archive);
	virtual status_t Archive(BMessage *archive, bool deep = true) const;
	
private:
	void Init(BEntry *entry);
	
	BSoundFile *sf;
	BPath path;
	int byteswap;
};

class CDDataFileDataSource : public CDDataSource
{
public:
	CDDataFileDataSource(BEntry *entry);
	CDDataFileDataSource(BMessage *archive);
	
	virtual status_t InitCheck(void);
	virtual ~CDDataFileDataSource();
	virtual status_t Read(void *data, size_t len, off_t posn);	
	virtual size_t Length(void);	
	virtual char *Description();
	virtual bool IsAudio();
	
	static BArchivable *Instantiate(BMessage *archive);
	virtual status_t Archive(BMessage *archive, bool deep = true) const;
	
private:
	void Init(BEntry *entry);
	
	BFile *f;
	BPath path;
};

class CDDiskDeviceDataSource : public CDDataSource
{
public:
	CDDiskDeviceDataSource(char *_path);
	CDDiskDeviceDataSource(BMessage *archive);
	
	virtual status_t InitCheck(void);
	virtual ~CDDiskDeviceDataSource();
	virtual status_t Read(void *data, size_t len, off_t posn);	
	virtual size_t Length(void);	
	virtual char *Description();
	virtual bool IsAudio();
	
	static BArchivable *Instantiate(BMessage *archive);
	virtual status_t Archive(BMessage *archive, bool deep = true) const;
	
private:
	void Init(char *_path);

	int fd;
	off_t blocks;
	off_t blocksize;
	BPath path;
};

#endif

