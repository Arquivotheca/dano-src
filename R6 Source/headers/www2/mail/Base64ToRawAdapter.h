/*
	Base64ToRawAdapter.h
*/
#ifndef BASE_64_TO_RAW_ADAPTER_H
	#define BASE_64_TO_RAW_ADAPTER_H
	#include <DataIO.h>
	
class BString;
const int kRawBufferSize = 4096;

class Base64ToRawAdapter : public BDataIO {
	public:
								Base64ToRawAdapter(BDataIO *source, bool owning = true);
		virtual 				~Base64ToRawAdapter();

		virtual					ssize_t	Read(void *buffer, size_t size);
		virtual	ssize_t			Write(const void *buffer, size_t size);

		static int32			Decode(const void *data, int32 dataSize, BString *out);
		static int32			Decode(const void *data, int32 dataSize, BDataIO *out);

	private:
		inline ssize_t 			FillBuffer();

		BDataIO *fSource;
		bool fOwning;
		char fBuffer[kRawBufferSize];
		uchar fLeftOver[3];
		int fLeftCount;
		int fDecodedSize;
		int fReadOffset;
		bool fMoreData;
};

#endif

