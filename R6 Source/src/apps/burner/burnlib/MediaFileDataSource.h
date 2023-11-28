
#ifndef _MEDIA_FILE_DATA_SOURCE_H_
#define _MEDIA_FILE_DATA_SOURCE_H_

#include <MediaFile.h>
#include <Path.h>
#include "CDDataSource.h"
/*
class BEntry;
class _resampler_base;


class MediaFileDataSource : public CDDataSource
{
public:
//						MediaFileDataSource(entry_ref *ref);
						MediaFileDataSource(BEntry *entry);
						MediaFileDataSource(BMessage *archive);
						
	virtual				~MediaFileDataSource();
	
	virtual status_t	InitCheck();
	virtual status_t	Read(void *data, size_t len, off_t posn);
	virtual size_t		Length(void);
	virtual bool		IsAudio();
	virtual char 		*Description();

	static BArchivable *Instantiate(BMessage *archive);
	virtual status_t Archive(BMessage *archive, bool deep = true) const;
	
	
};
*/
#endif // _MEDIA_FILE_DATA_SOURCE_H_
