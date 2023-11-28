#include <ctype.h>
#include "HTTPHeader.h"
#include "debug.h"

struct Attr {
	Attr(const char *name, const char *value)
		:	fName(name), fValue(value)
	{}
	
	BString fName;
	BString fValue;
};

HTTPHeader::HTTPHeader()
	:	fAttrList(10, true)	// fAttrList is owning, deleting its elements
{
}

HTTPHeader::~HTTPHeader()
{
}

status_t HTTPHeader::ParseStream(BDataIO *stream, int *out_result)
{
	*out_result = -1;
	enum State {
		SCAN_PROTOCOL,
		SCAN_RESULTCODE,
		SCAN_END_OF_RESPONSE,
		SCAN_TAG,
		SCAN_TAG_WS,
		SCAN_VALUE
	} state = SCAN_PROTOCOL;
	
	int resultCode = 0;
	BString currentTag;
	BString currentValue;

	fAttrList.MakeEmpty();
	
	bool finished = false;
	while (!finished) {
		char c;
		status_t error = stream->Read(&c, 1);	// (The socket class does buffering)
		if (error < 0)
			return error;

		if (c == 0)
			continue;

		switch (state) {
			case SCAN_PROTOCOL:
				if (c == ' ') {
					AddAttr("meta:protocol", currentValue.String());
					currentValue = "";
					state = SCAN_RESULTCODE;
					resultCode = 0;
				} else
					currentValue += c;
				
				break;
				
			case SCAN_RESULTCODE:
				if (isdigit(c))
					resultCode = resultCode * 10 + c - '0';
				else {
					*out_result = resultCode;
					state = SCAN_END_OF_RESPONSE;
					currentValue = "";
					currentValue << (int32) resultCode << " ";
				}
				
				break;
	
			case SCAN_END_OF_RESPONSE:
				if (c == '\n') {
					AddAttr("meta:error_string", currentValue.String());
					state = SCAN_TAG;
					currentValue = "";
					currentTag = "";
				} else if (c != '\r')
					currentValue << c;
				
				break;
				
			case SCAN_TAG:
				if (c == '\r')
					break;
					
				if (c == '\n') {
					finished = true;
					break;
				}
	
				if (c != ':')
					currentTag += c;
				else
					state = SCAN_TAG_WS;
				
				break;
				
			case SCAN_TAG_WS:
				state = SCAN_VALUE;
				if (isspace(c))
					break;
				
				// falls through...
				
			case SCAN_VALUE:
				if (c == '\n') {
					AddAttr(currentTag.String(), currentValue.String());
					currentValue = "";
					currentTag = "";
					state = SCAN_TAG;
				} else if (c != '\r')
					currentValue += c;
	
				break;
		}
	}

	return B_OK;	
}

void HTTPHeader::AddAttr(const char *name, const char *value)
{
	fAttrList.AddItem(new Attr(name, value));
}

const char* HTTPHeader::FindAttr(const char *name) const
{
	for (int i = 0; i < fAttrList.CountItems(); i++) {
		Attr *attr = fAttrList.ItemAt(i);
		if (attr->fName.ICompare(name) == 0)
			return attr->fValue.String();
	}
			
	return 0;
}

void HTTPHeader::PrintToStream() const
{
	for (int i = 0; i < fAttrList.CountItems(); i++) {
		Attr *attr = fAttrList.ItemAt(i);
		printf("%s %s\n", attr->fName.String(), attr->fValue.String());
	}
}
