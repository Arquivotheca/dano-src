#ifndef _SRESIO_H_
#define _SRESIO_H_

#include <DataIO.h>
#include <Resources.h>

class SResIO : public BPositionIO
{
public:
				SResIO();
				SResIO(BFile *file, type_code type, int32 id);
	virtual		~SResIO();
	
			status_t	SetTo(BFile *file, type_code type, int32 id);
	virtual	ssize_t		Read(void *buffer, size_t size);
	virtual	ssize_t		Write(const void *buffer, size_t size);
	
	virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t size);
	virtual	ssize_t		WriteAt(off_t pos, const void *buffer, size_t size);
	
	virtual off_t		Seek(off_t position, uint32 seek_mode);
	virtual	off_t		Position() const;
	
			int			RemoveResource();
private:
	BResources	fRes;
	off_t		fPos;
	off_t		fEnd;

	char		*fName;	
	type_code	fType;
	int32		fID;
	bool		fInited;
};


#endif
