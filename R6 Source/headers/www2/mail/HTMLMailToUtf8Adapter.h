/*
	HTMLMailToUtf8Adapter.h
*/
#ifndef _HTML_TO_UTF8_ADAPTER_H
	#define _HTML_TO_UTF8_ADAPTER_H
	#include <DataIO.h>

class HTMLMailToUtf8Adapter : public BDataIO {
	public:
								HTMLMailToUtf8Adapter(BDataIO *source, bool owning = true);
		virtual 				~HTMLMailToUtf8Adapter();

		virtual	ssize_t 		Read(void *buffer, size_t size);
		virtual	ssize_t			Write(const void *buffer, size_t size);

	private:
	
		BDataIO *fSource;
		bool fOwning;
	
		enum State {
			kScanText,
			kScanTag
		} fState;
};

#endif
