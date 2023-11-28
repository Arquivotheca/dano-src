// AttrIO.cpp
#include "AttrIO.h"

#include <sys/socket.h>
#include <fs_attr.h>
#include <TypeConstants.h>
#include <malloc.h>
#include <string.h>

AttrIO::AttrIO(BNode *_f)
	:	f(_f)
{
}

status_t AttrIO::WriteStringAttr(const char *attr, const char *data)
{
	if (data) {
		int sz = strlen(data)+1;
		return f->WriteAttr(attr,B_STRING_TYPE,0,data,sz);
	}
	return B_NO_ERROR;
}

status_t AttrIO::WriteInt32Attr(const char *attr, int32 v)
{
	v = htonl(v);
	return f->WriteAttr(attr,B_INT32_TYPE,0,&v,sizeof(int32));
}

status_t AttrIO::WriteInt16Attr(const char *attr, int16 v)
{
	v = htons(v);
	return f->WriteAttr(attr,B_INT16_TYPE,0,&v,sizeof(int16));
}

status_t AttrIO::WriteBoolAttr(const char *attr, bool v)
{
	int16 val = v;
	val = htons(val);
	return f->WriteAttr(attr,B_INT16_TYPE,0,&val,sizeof(int16));
}

// ------------------------ Read Calls ------------------------------//

status_t AttrIO::ReadStringAttr(const char *attr, char **data, char *otherwise)
{
	attr_info	aiBuf;
	int sz;
	
	if (f->GetAttrInfo(attr,&aiBuf) == B_NO_ERROR) {
		sz = aiBuf.size;
		if (*data) free(*data);
		*data = (char *)malloc(sz);
		return f->ReadAttr(attr,B_STRING_TYPE,0,*data,sz);
	}
	else {
		*data = otherwise;
	}
	return B_ERROR;
}

status_t AttrIO::ReadStringAttr(const char *attr, char *buf, size_t size)
{
	status_t	err = B_OK;
	attr_info	aiBuf;
	size_t 		sz;
	
	*buf = 0;
	
	if ((err = f->GetAttrInfo(attr,&aiBuf)) >= B_OK) {
		sz = aiBuf.size;
		if (sz > size) {
			sz = size-1;
			buf[sz] = 0;
		}
		err = f->ReadAttr(attr,B_STRING_TYPE,0,buf,sz);
	}
	return err;
}

status_t AttrIO::ReadInt32Attr(const char *attr, int32 *v)
{
	status_t err;
	int32 val;
	err = f->ReadAttr(attr,B_INT32_TYPE,0,&val,sizeof(int32));
	if (err == sizeof(int32))
		*v = ntohl(val);
	return err;
}

status_t AttrIO::ReadInt16Attr(const char *attr, int16 *v)
{
	status_t err;
	int16 val;
	err = f->ReadAttr(attr,B_INT16_TYPE,0,&val,sizeof(int16));
	if (err == sizeof(int16))
		*v = ntohs(val);
	return err;
}

status_t AttrIO::ReadBoolAttr(const char *attr, bool *v)
{
	status_t	err;
	int16	val;
	err = f->ReadAttr(attr,B_INT16_TYPE,0,&val,sizeof(int16));
	if (err == sizeof(int16)) {
		val = ntohs(val);
		*v = val;
	}
	return err;
}
