/*
	MerlinProtocol.h
*/
#ifndef MAILBOX_PROTOCOL_H
#define MAILBOX_PROTOCOL_H
#include "MimeMessage.h"
#include <Protocol.h>
	
using namespace Wagner;
class MimeMessage;

class MerlinProtocol : public Protocol {
	public:
								MerlinProtocol(void *inHandle);
		virtual 				~MerlinProtocol();

		virtual status_t		Open(const URL &url, const URL &requestor, BMessage *errorParams, uint32 flags);
		virtual ssize_t			GetContentLength();
		virtual void 			GetContentType(char *type, int size);
		virtual CachePolicy		GetCachePolicy();
		virtual	bool			GetRedirectURL(URL &redirectUrl, bigtime_t *outDelay);

		virtual ssize_t 		Read(void *buffer, size_t size);
		virtual ssize_t 		ReadAt(off_t pos, void *buffer, size_t size);
		virtual off_t 			Seek(off_t position, uint32 seek_mode);
		
	private:
		void					CreateRfc822Header(const MimeMessage *message, const char *contentType, BDataIO *stream);
		inline void				WriteHeader(BDataIO *stream, const char *header, const char *data);
		
		BString	fContentType;
		ssize_t	fContentLength;
		MimeMessage *fMessage;
		BDataIO	*fMessageStream;
		CachePolicy fCachePolicy;
		bool fRedirect;
		URL fRedirectUrl;
};

#endif
