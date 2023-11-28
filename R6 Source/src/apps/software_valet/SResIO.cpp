
#include "SResIO.h"

#define DEBUG 0
#include <Debug.h>


SResIO::SResIO()
	:	fPos(0),
		fType(0),
		fID(0),
		fEnd(0),
		fInited(false)
{
}

SResIO::SResIO(BFile *file, type_code type, int32 id)
	:	fPos(0),
		fType(type),
		fID(id),
		fEnd(0),
		fInited(false)
{
	SetTo(file,type,id);
}

status_t SResIO::SetTo(BFile *file, type_code type, int32 id)
{
	fPos = 0;
	fType = type;
	fID = id;
	fEnd = 0;
	fInited = false;
	
	status_t	err = B_OK;
	if ((err = fRes.SetTo(file)) >= B_OK ||
		(err = fRes.SetTo(file, true)) >= B_OK)
	{
		const char *name;
		size_t	size;
		if (fRes.GetResourceInfo(fType, fID, &name,&size)) {
			fEnd = size;
			fInited = true;
		}
		else if ((err = fRes.AddResource(fType,fID,&fType,sizeof(fType),"")) >= B_OK) {
			fInited = true;
		}
	}
	return err;
}

SResIO::~SResIO()
{
}

ssize_t		SResIO::Read(void *buffer, size_t	size)
{
	ssize_t	result;

	if (!fInited)
		return B_NO_INIT;	
	if (fPos >= fEnd)
		return 0;
			
	result = fRes.ReadResource(fType,fID,buffer,fPos,size);
	
	PRINT(("SResIO read at pos %Ld, result %d\n",fPos,result));

	if (result >= B_OK) {
		// we had to assume that we got it all
		// since BResource doesn't tell us how much data was
		// actually read

		result = size;
		fPos += result;
	}
		
	return result;
}

ssize_t		SResIO::Write(const void *buffer, size_t size)
{
	if (!fInited)
		return B_NO_INIT;
		
	ssize_t	result;
	result = fRes.WriteResource(fType,fID,buffer,fPos,size);
	if (result >= B_OK) {
		result = size;
		fPos += result;
		if (fPos > fEnd) fEnd = fPos;
	}
	PRINT(("SResIO write now at pos %Ld, result %d\n",fPos,result));
		
	return result;
}

ssize_t		SResIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	if (!fInited)
		return B_NO_INIT;

	ssize_t	result;
	result = fRes.ReadResource(fType,fID,buffer,pos,size);
	if (result >= B_OK) {
		result = size;
		fPos = pos + result;
	}
	PRINT(("SResIO ReadAt now at pos %Ld, result %d\n",fPos,result));

	return result;
}

ssize_t		SResIO::WriteAt(off_t pos, const void *buffer, size_t size)
{
	if (!fInited)
		return B_NO_INIT;

	ssize_t	result;
	result = fRes.WriteResource(fType,fID,buffer,pos,size);
	if (result >= B_OK) {
		result = size;
		off_t newPos = pos + result;
		if (newPos > fEnd) fEnd = newPos;	
	}
	PRINT(("SResIO ReadAt now at pos %Ld, result %d\n",fPos,result));
	
	return result;
}

off_t		SResIO::Seek(off_t pos, uint32 seek_mode)
{
	switch (seek_mode) {
		case SEEK_SET:
			fPos = pos;
			break;	
		case SEEK_CUR:
			fPos += pos;
			break;
		case SEEK_END:
			fPos = fEnd + pos;
			break;
	}
	if (fPos < 0) fPos = 0;
	if (fPos > fEnd) fEnd = fPos;
	
	return fPos;
}

off_t		SResIO::Position() const
{
	return fPos;
}

int			SResIO::RemoveResource()
{
	return	fRes.RemoveResource(fType,fID);
}
