#include <Be.h>

#include "SAttrIO.h"

#define DEBUG 0
#include <Debug.h>

SAttrIO::SAttrIO(BNode *node, const char *attrname, attr_info *info)
	:	fNode(node),
		fPos(0),
		fType(info->type),
		fEnd(info->size)
{
	// fNode->Lock();
	fName = strdup(attrname);
}

SAttrIO::SAttrIO(BNode *node, const char *attrname, type_code type)
	:	fNode(node),
		fPos(0),
		fType(type),
		fEnd(0)
{
	// fNode->Lock();
	fName = strdup(attrname);
}

SAttrIO::~SAttrIO()
{
	// fNode->Unlock();
	free(fName);
}

ssize_t		SAttrIO::Read(void *buffer, size_t	size)
{
	ssize_t	result;
	
	if (fPos >= fEnd)
		return 0;
			
	result = fNode->ReadAttr(fName,fType,fPos,buffer,size);
	
	PRINT(("SAttrI0 read at pos %Ld, result %d\n",fPos,result));

	if (result > 0)
		fPos += result;
		
	return result;
}

ssize_t		SAttrIO::Write(const void *buffer, size_t size)
{
	ssize_t	result;
	result = fNode->WriteAttr(fName,fType,fPos,buffer,size);
	if (result > 0) {
		fPos += result;
		if (fPos > fEnd) fEnd = fPos;	
	}
	PRINT(("write attr result %d\n",result));
		
	return result;
}

ssize_t		SAttrIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	ssize_t	result;
	result = fNode->ReadAttr(fName,fType,pos,buffer,size);		
	return result;
}

ssize_t		SAttrIO::WriteAt(off_t pos, const void *buffer, size_t size)
{
	ssize_t	result;
	result = fNode->WriteAttr(fName,fType,pos,buffer,size);
	if (result > 0) {
		off_t newPos = pos + result;
		if (newPos > fEnd) fEnd = newPos;	
	}		
	return result;
}

off_t		SAttrIO::Seek(off_t pos, uint32 seek_mode)
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

off_t		SAttrIO::Position() const
{
	return fPos;
}
