
#include "SResIO.h"

#define DEBUG 1
#include <Debug.h>


SResIO::SResIO()
	:	fPos(0),
		fType(0),
		fID(0),
		fEnd(0),
		fInited(false)
{
	TRESPASS();
}

SResIO::~SResIO()
{
}

SResIO::SResIO(BFile *file, type_code type, int32 id)
	:	fPos(0),
		fType(type),
		fID(id),
		fEnd(0),
		fInited(false)
{
	TRESPASS();
}

status_t SResIO::SetTo(BFile *file, type_code type, int32 id)
{
	TRESPASS();
		
	return B_OK;
}

ssize_t		SResIO::Write(const void *buffer, size_t size)
{
	TRESPASS();		
	return 0;
}

ssize_t		SResIO::Read(void *buffer, size_t size)
{
	TRESPASS();		
	return 0;
}

ssize_t		SResIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	TRESPASS();
	return 0;
}

ssize_t		SResIO::WriteAt(off_t pos, const void *buffer, size_t size)
{
	TRESPASS();
	return 0;
}

off_t		SResIO::Seek(off_t pos, uint32 seek_mode)
{
	TRESPASS();
	return 0;
}

off_t		SResIO::Position() const
{
	TRESPASS();
	return 0;
}

int			SResIO::RemoveResource()
{
	TRESPASS();
	return 0;
}
