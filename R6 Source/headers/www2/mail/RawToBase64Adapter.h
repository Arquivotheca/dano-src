/*
	RawToBase64Adapter.h
*/

#ifndef RAW_TO_BASE_64_ADAPTER_H
	#define RAW_TO_BASE_64_ADAPTER_H
	#include <DataIO.h>

class BString;
const int32 kBase64BufferSize = 1024;

class RawToBase64Adapter : public BDataIO
{
	public:
								RawToBase64Adapter(BDataIO *source, bool owning = true);
		virtual					~RawToBase64Adapter();
	
		virtual	ssize_t			Read(void *buffer, size_t size);
		virtual ssize_t			Write(const void *buffer, size_t size);
	
		static int32			Encode(const void *data, int32 dataSize, BString *out);
		void					SetBreakLines(bool state);
		
	private:
		void					FillBuffer();

		char fBuffer[kBase64BufferSize];
		int fBufferSize;
		int fBufferOffset;
		int fColumnCount;
		bool fOwning;
		bool fBreakLines;
		BDataIO *fSource;
		bool fMoreData;
		bool fFinished;
};


#endif
