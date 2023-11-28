/*
	TextToHtmlAdapter.h
*/
#ifndef _TEXT_TO_HTML_ADAPTER_H
	#define _TEXT_TO_HTML_ADAPTER_H
	#include <DataIO.h>

const int kTextBufferSize = 4096;

class TextToHtmlAdapter : public BDataIO {
	public:
								TextToHtmlAdapter(BDataIO *inText, bool owning = true);
		virtual 				~TextToHtmlAdapter();

		virtual	ssize_t 		Read(void *buffer, size_t size);
		virtual	ssize_t 		Write(const void *buffer, size_t size);
	
	private:
		bool                    BadEndChar(char ch);	    
		inline int				GetStateStackSize();
		inline int				ExamineStateHead();
		inline int				PopState();
		inline int				PushState(int state);
		void					DumpStateStack();
		
		BDataIO *fSource;
		char fInBuffer[kTextBufferSize];
		int fInBufferSize;
		int fInBufferPos;
		bool fOwning;
		char fWordBuffer[2048];
		unsigned int fWordBufferPos;
		unsigned int fWordDumpPos;
		unsigned int fStringDumpPos;
		unsigned int fStringToReplace;
		
		int fStateStack[16];
		int fStateStackPos;
};

#endif
