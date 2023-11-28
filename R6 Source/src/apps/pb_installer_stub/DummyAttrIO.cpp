// AttrIO.cpp
#include "AttrIO.h"

#include <sys/socket.h>
#include <fs_attr.h>
#include <TypeConstants.h>
#include <malloc.h>
#include <string.h>

#define DEBUG 1
#include <Debug.h>

AttrIO::AttrIO(BNode *_f)
	:	f(_f)
{
	TRESPASS();
}

status_t AttrIO::WriteStringAttr(const char *attr, const char *data)
{
	TRESPASS();
	return B_NO_ERROR;
}

status_t AttrIO::WriteInt32Attr(const char *attr, int32 v)
{
	TRESPASS();
	return B_NO_ERROR;
}

status_t AttrIO::WriteInt16Attr(const char *attr, int16 v)
{
	TRESPASS();
	return B_NO_ERROR;
}

status_t AttrIO::WriteBoolAttr(const char *attr, bool v)
{
	TRESPASS();
	return B_NO_ERROR;
}

// ------------------------ Read Calls ------------------------------//

status_t AttrIO::ReadStringAttr(const char *attr, char **data, char *otherwise)
{
	TRESPASS();
	return B_NO_ERROR;
}

status_t AttrIO::ReadStringAttr(const char *attr, char *buf, size_t size)
{
	TRESPASS();
	return B_NO_ERROR;
}

status_t AttrIO::ReadInt32Attr(const char *attr, int32 *v)
{
	TRESPASS();
	return B_NO_ERROR;
}

status_t AttrIO::ReadInt16Attr(const char *attr, int16 *v)
{
	TRESPASS();
	return B_NO_ERROR;
}

status_t AttrIO::ReadBoolAttr(const char *attr, bool *v)
{
	TRESPASS();
	return B_NO_ERROR;
}
