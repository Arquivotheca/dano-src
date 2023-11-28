#ifndef _HTTP_HEADER_H
#define _HTTP_HEADER_H

#include <DataIO.h>
#include <String.h>
#include <ObjectList.h>

struct Attr;

class HTTPHeader {
public:
	HTTPHeader();
	virtual ~HTTPHeader();
	status_t ParseStream(BDataIO *stream, int *out_result);
	const char* FindAttr(const char *name) const;
	void PrintToStream() const;

private:
	void AddAttr(const char *name, const char *value);
	BObjectList<Attr> fAttrList;
};




#endif

