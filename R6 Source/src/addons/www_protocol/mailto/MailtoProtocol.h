/*
	MailtoProtocol.h
*/
#ifndef _MailtoProtocol_h
	#define _MailtoProtocol_h
	#include <Protocol.h>
	
using namespace Wagner;

class MailtoProtocol : public Protocol {
	public:
								MailtoProtocol(void*);
		virtual 				~MailtoProtocol();

		virtual status_t		Open(const URL &inUrl, const URL &requestor, BMessage *errorParams, uint32 flags);
		virtual ssize_t			GetContentLength();
		virtual void 			GetContentType(char *type, int size);
		virtual CachePolicy		GetCachePolicy();

		virtual	void 			Abort();
		virtual ssize_t 		Read(void *buffer, size_t size);
		virtual ssize_t 		ReadAt(off_t pos, void *buffer, size_t size);
		virtual off_t 			Seek(off_t position, uint32 seek_mode);
		virtual	off_t 			Position() const;

	private:
};

#endif
