/*
	MimePartHeaderAdapter.h
*/
#ifndef _MIME_PART_HEADER_ADAPTER_H
#define _MIME_PART_HEADER_ADAPTER_H
#include <DataIO.h>
#include <Protocol.h>
#include <String.h>
#include "SendMessageContainer.h"
#include "StringBuffer.h"
	
static const char *kMailerIdentification = "Merlin for BeIA, Powered by Cereal Debugging";
	
enum mime_part_type {
	kTopLevelPart = 0,
	kMultipartAlternativePart,
	kTextPart,
	kHtmlPart,
	kAttachmentPart
};
	
class MimePartHeaderAdapter : public BDataIO {
	public:
								MimePartHeaderAdapter(mime_part_type type, bool needToClose, StringBuffer &outerBoundary,
														StringBuffer &innerBoundary, SendMessageContainer *container = NULL,
														const char *contentType = "", const char *fileName = "");
		virtual 				~MimePartHeaderAdapter();

		virtual	ssize_t 		Read(void *buffer, size_t size);
		virtual	ssize_t 		Write(const void*, size_t);

		void					BuildRfc822Header(const char *boundary, SendMessageContainer *container);
		void					BuildMultipartAlternativeHeader(const char *boundary);
		void					BuildTextHeader();
		void					BuildHtmlHeader();
		void					BuildAttachmentHeader(const char *contentType, const char *fileName);
		void					InsertBoundary(const char *boundary, bool close = false);
		void 					CreateBoundary(StringBuffer *buffer);
		void 					AddStream(BDataIO *stream);


	private:

		bool fNeedToClose;
		int fIndex;
		bool fAddedFooter;
		BList fSources;
		BMallocIO *fHeader;
		BMallocIO *fFooter;
};
#endif
