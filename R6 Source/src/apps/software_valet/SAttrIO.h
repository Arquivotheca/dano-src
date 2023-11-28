#ifndef _SATTRIO_H_
#define _SATTRIO_H_

#include <DataIO.h>
#include <Node.h>


class SAttrIO : public BPositionIO
{
public:
				SAttrIO(BNode *node, const char *attrname, attr_info *ai);
				SAttrIO(BNode *node, const char *attrname, type_code atype = -1);
	virtual		~SAttrIO();
	
	virtual	ssize_t		Read(void *buffer, size_t size);
	virtual	ssize_t		Write(const void *buffer, size_t size);
	
	virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t size);
	virtual	ssize_t		WriteAt(off_t pos, const void *buffer, size_t size);
	
	virtual off_t		Seek(off_t position, uint32 seek_mode);
	virtual	off_t		Position() const;
private:
	BNode		*fNode;
	off_t		fPos;
	off_t		fEnd;

	char		*fName;	
	type_code	fType;
};

#endif
