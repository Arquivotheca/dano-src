/*
	Rfc822MessageAdapter.h
*/
#ifndef _RFC822_MESSAGE_ADAPTER_H
#define _RFC822_MESSAGE_ADAPTER_H
#include <Binder.h>
#include <DataIO.h>
#include <StringBuffer.h>

class Rfc822MessageAdapter : public BDataIO {
	public:
								Rfc822MessageAdapter(const char *path, bool skipHeader = true);
								~Rfc822MessageAdapter();
		
		virtual ssize_t			Read(void *buffer, size_t size);
		virtual ssize_t			Write(const void *buffer, size_t size);						

		static int32			ParseHeader(const char *path, atom<BinderContainer> &node);
		ssize_t					GetLine(StringBuffer &buffer);
		
	private:
		BDataIO *fSource;
};

#endif
// End of Rfc822MessageAdapter.h
