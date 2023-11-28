/*
	HTMLMailAdapter.h
*/
#ifndef _HTML_MAIL_ADAPATER_H
	#define _HTML_MAIL_ADAPATER_H
	#include <MimeMessage.h>
	#include <DataIO.h>
	#include <StringBuffer.h>

const int kMaxBufferSize = 4096;

class HTMLMailAdapter : public BDataIO {
	public:
								HTMLMailAdapter(MimeMessage *msg, BDataIO *source, bool owning = true);
		virtual 				~HTMLMailAdapter();

		virtual	ssize_t 		Read(void *buffer, size_t size);
		virtual	ssize_t 		Write(const void *buffer, size_t size);

	private:
		void 					ReplaceURL();

		MimeMessage *fMessage;
		BDataIO *fSource;
		bool fOwning;
		StringBuffer fReplaceString;
		StringBuffer fCurrentName;
		int fURLIndex;
		enum State {
			kScanText,
			kScanTag,
			kScanQuotedString,
			kScanName,
			kScanValueStart,
			kScanEqual,
			kScanValue,
			kReplace
		} fState;
		bool fQuoted;
		int fInBufferPos;
		int fReadBufferSize;
		char fInBuffer[kMaxBufferSize];
		
};

#endif
