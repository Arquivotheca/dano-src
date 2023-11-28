#ifndef _ATTRIO_H_
#define _ATTRIO_H_

// AttrIO.h

#include <Node.h>

class AttrIO
{
public:
	AttrIO(BNode *f);
	
	//-------- Write calls ----------
	status_t	WriteStringAttr(const char *attr,
								const char *data);
	status_t	WriteInt32Attr(const char *attr,
								int32 v);
	status_t	WriteInt16Attr(const char *attr,
								int16 v);
	status_t	WriteBoolAttr(const char *attr, bool v);
	
	//--------- Read calls ----------
	
	status_t	ReadStringAttr(const char *attr,
								char **data, char *other = NULL);
	status_t	ReadStringAttr(const char *attr,
								char *buf,
								size_t size);
	status_t	ReadInt32Attr(const char *,int32 *);
	status_t	ReadInt16Attr(const char *,int16 *);
	status_t	ReadBoolAttr(const char *attr, bool *v);
private:
	BNode	*f;
};

#endif

