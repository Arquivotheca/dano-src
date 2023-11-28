/*
	TextToRfc822BodyAdapter.h
*/
#ifndef TEXT_TO_RFC822_BODY_ADAPTER_H
	#define TEXT_TO_RFC822_BODY_ADAPTER_H
	#include <DataIO.h>
	
class TextToRfc822BodyAdapter : public BDataIO {
	public:
								TextToRfc822BodyAdapter(BDataIO *source, int columnBreak = 76, bool owning = true);
		virtual					~TextToRfc822BodyAdapter();
	
		virtual	ssize_t			Read(void *buffer, size_t size);
		virtual ssize_t			Write(const void *buffer, size_t size);
	
	private:

		BDataIO	*fSource;
		bool fOwning;

		char fInBuffer[2048];
		ssize_t fInBufferSize;
		ssize_t fInBufferPos;
		int fWordBuffer[76];
		ssize_t fWordBufferPos;
		ssize_t fWordDumpCount;
		ssize_t fLineLen;
		int fState;
		int fCRLFNextState;
		int fWordDumpNextState;
		int fEatingSpacesNextState;
		int fReadBufferNextState;
		int fColumnBreak;
};

#endif
