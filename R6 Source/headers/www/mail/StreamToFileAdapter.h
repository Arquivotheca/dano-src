/*
	StreamToFileAdapter.h
*/
#ifndef _STREAM_TO_FILE_ADAPTER_H
#define _STREAM_TO_FILE_ADAPTER_H
#include <DataIO.h>
#include <File.h>

class StreamToFileAdapter : public BDataIO {
	public:
								StreamToFileAdapter(BDataIO *source, BFile &file, bool owning = true);
		virtual 				~StreamToFileAdapter();

		virtual	ssize_t 		Read(void *buffer, size_t size);
		virtual	ssize_t			Write(const void *buffer, size_t size);
		
		inline void				Flush();
		
	private:

		BDataIO *fSource;
		BFile fFile;
		bool fOwning;
		BMallocIO fBuffer;
};

#endif
