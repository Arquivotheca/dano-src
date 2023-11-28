/*
	Rfc822MessageAdapter.cpp
*/
#include <ctype.h>
#include <Protocol.h>
#include "Rfc822MessageAdapter.h"
#include <String.h>
#include <URL.h>

using namespace Wagner;

Rfc822MessageAdapter::Rfc822MessageAdapter(const char *path, bool skipHeader)
	:	fSource(NULL)
{
	URL url(path);
	Protocol *protocol = Protocol::InstantiateProtocol(url.GetScheme());
	if (protocol != NULL) {
		BMessage errorParams;
		protocol->Open(url, URL(), &errorParams, 0);
		fSource = protocol;
	}

	if (skipHeader) {
		StringBuffer buffer;
		// Ahh, you gotta love Microsoft. When Outlook Express saves an email to disk,
		// it doesn't use CRLF like the RFC 822 says to. Instead, they just use a newline.
		while ((strcmp("\r\n", buffer.String()) != 0) && (strcmp("\n", buffer.String()) != 0)) {
			if (GetLine(buffer) < 1)
				break;
		}
	}
}


Rfc822MessageAdapter::~Rfc822MessageAdapter()
{
	delete fSource;
}

ssize_t Rfc822MessageAdapter::Read(void *buffer, size_t size)
{
	if (fSource != NULL)
		return fSource->Read(buffer, size);
	return 0;
}

ssize_t Rfc822MessageAdapter::Write(const void *, size_t )
{
	return B_ERROR;
}

int32 Rfc822MessageAdapter::ParseHeader(const char *path, atom<BinderContainer> &node)
{
	Rfc822MessageAdapter adapter(path, false);
	bool firstLine = true;
	StringBuffer name;
	StringBuffer value;
	StringBuffer buffer;
	while (strcmp("\r\n", buffer.String()) != 0) {
		if (adapter.GetLine(buffer) < 1)
			break;
		const char *ptr = buffer.String();
		if (isspace(*ptr)) {
			// Skip whitespace
			while (isspace(*ptr))
				ptr++;
			// Append to value
			value << " ";
			while ((*ptr != '\0') && (*ptr != '\r') && (*ptr != '\n'))
				value << *ptr++;
		} else {
			if (!firstLine) {
				// XXX: Hack!
				BString lower(name.String());
				lower.ToLower();
				node->AddProperty(lower.String(), value.String());
				name.Clear();
				value.Clear();
			}
			// Read in name
			while ((*ptr != '\0') && (*ptr != ':'))
				name << *ptr++;
			// Skip colon
			if (*ptr == ':')
				ptr++;
			// Skip leading whitespace
			while (isspace(*ptr))
				ptr++;
			// Read in value
			while ((*ptr != '\0') && (*ptr != '\r') && (*ptr != '\n'))
				value << *ptr++;
			
			firstLine = false;
		}
	}
}

ssize_t Rfc822MessageAdapter::GetLine(StringBuffer &buffer)
{
	if (fSource == NULL)
		return 0;
	
	ssize_t read = 0;
	buffer.Clear();
		
	char c;
	while (fSource->Read(&c, 1) > 0) {
		buffer << c;
		read++;
		if (c == '\n')
			break;
	}
	return read;
}

// End of Rfc822MessageAdapter.cpp
