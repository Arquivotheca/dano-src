#include <stdio.h>
#include "TextToUtf8Adapter.h"

TextToUtf8Adapter::TextToUtf8Adapter(BDataIO *source, bool owning)
	:	fSource(source),
		fOwning(owning)
{

}

TextToUtf8Adapter::~TextToUtf8Adapter()
{
	if (fOwning)
		delete fSource;
}

ssize_t TextToUtf8Adapter::Read(void *buffer, size_t size)
{
	// This should know the charset of the text from it's
	// source and do the appropriate convert_to_utf8 call.
	
	ssize_t totalRead = 0;
	char *out = static_cast<char*>(buffer);

	while (totalRead < size) {
		char c;
		if (fSource->Read(&c, 1) < 1)
			break;
		
		// Skip carriage returns.
		if(c == '\r')
			continue;
			
		out[totalRead++] = c;
	}
	return totalRead;
}

ssize_t TextToUtf8Adapter::Write(const void *, size_t )
{
	return B_ERROR;
}


