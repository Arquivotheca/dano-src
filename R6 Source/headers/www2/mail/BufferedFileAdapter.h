/*
	BufferedFileAdapter.h
*/
#ifndef BUFFERED_FILE_ADAPTER_H
#define BUFFERED_FILE_ADAPTER_H
#include <DataIO.h>
	
class BufferedFileAdapter : public BDataIO {
	public:
								BufferedFileAdapter(BDataIO *source, int bufferSize, bool owning = true);
		virtual 				~BufferedFileAdapter();

		virtual	ssize_t			Read(void *buffer, size_t size);
		char					Peek();
		virtual	ssize_t			Write(const void *buffer, size_t size);
		
	private:
		inline ssize_t 			FillBuffer();

		BDataIO *fSource;
		bool fOwning;
		int fReadOffset;
		int fBufferPosition;
		int fBytesInBuffer;
		int fBufferSize;
		char *fBuffer;
};

#endif

