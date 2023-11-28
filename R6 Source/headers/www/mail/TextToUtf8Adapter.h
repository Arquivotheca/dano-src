/*
	TextToUtf8Adapter.h
*/
#ifndef _TEXT_TO_UTF8_ADAPTER_H
	#define _TEXT_TO_UTF8_ADAPTER_H
	#include <DataIO.h>

class TextToUtf8Adapter : public BDataIO {
	public:
								TextToUtf8Adapter(BDataIO *source, bool owning = true);
		virtual 				~TextToUtf8Adapter();

		virtual	ssize_t 		Read(void *buffer, size_t size);
		virtual	ssize_t			Write(const void *buffer, size_t size);

	private:

		BDataIO *fSource;
		bool fOwning;
};

#endif
