#include <stdio.h>
#include "HTMLMailToUtf8Adapter.h"

HTMLMailToUtf8Adapter::HTMLMailToUtf8Adapter(BDataIO *source, bool owning)
	:	fSource(source),
		fOwning(owning),
		fState(kScanText)
{

}

HTMLMailToUtf8Adapter::~HTMLMailToUtf8Adapter()
{
	if (fOwning)
		delete fSource;
}

ssize_t HTMLMailToUtf8Adapter::Read(void *buffer, size_t size)
{
	ssize_t totalRead = 0;
	char *out = static_cast<char*>(buffer);
	while (totalRead < size) {
		char c;
		if (fSource->Read(&c, 1) < 1)
			break;
		
		// For now, this just strips out the HTML tags. In the future
		// it would be nice if it could try and keep the formatting.
		switch (fState) {
			case kScanText:
				if (c == '<') {
					fState = kScanTag;
					continue;
				}
				if (c == '\r')
					continue;
				out[totalRead++] = c;
				break;
			
			case kScanTag:
				if (c == '>')
					fState = kScanText;
				break;
			default:
				break;
		}
	}
	return totalRead;
}

ssize_t HTMLMailToUtf8Adapter::Write(const void *, size_t )
{
	return B_ERROR;
}


