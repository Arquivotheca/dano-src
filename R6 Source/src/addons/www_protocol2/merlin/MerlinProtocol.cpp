/*
	MerlinProtocol.cpp
*/
#include "Base64ToRawAdapter.h"
#include <Binder.h>
#include "ConcatenateAdapter.h"
#include "HTMLMailAdapter.h"
#include "MailDebug.h"
#include "MerlinProtocol.h"
#include "MimeMessage.h"
#include "PartContainer.h"
#include "PostOffice.h"
#include "QPToPlainAdapter.h"
#include "Rfc822MessageAdapter.h"
#include <String.h>
#include <StringBuffer.h>
#include "TextToHtmlAdapter.h"

// HTML Used when formatting the message body
static const char * const kHeaderHtml = \
"<html><head></head><body text='#000000'>\n"
"<table border='0' cellpadding='15'><tr><td valign='top'><tt>";
static const char * const kFooterHtml = \
"\n</tt></td></tr></table>\n</body></html>";

MerlinProtocol::MerlinProtocol(void *inHandle)
	:	Protocol(inHandle),
		fContentType("text/html"),
		fContentLength(0x7fffffff),
		fMessage(NULL),
		fMessageStream(NULL),
		fCachePolicy(CC_CACHE),
		fRedirect(false)
{

}

MerlinProtocol::~MerlinProtocol()
{
	delete fMessageStream;
}

status_t MerlinProtocol::Open(const URL &url, const URL &, BMessage *errorParams, uint32 )
{
	BString mailbox;
	url.GetQueryParameter("mailbox", &mailbox);
	BString username = BinderNode::Root()["user"]["~"]["name"];

	status_t result = B_OK;
	// Select this mailbox in the PostOffice.
	if (mailbox != "*be:eml") {
		result = PostOffice::MailMan()->SelectMailbox(username.String(), mailbox.String());
		if (result != B_OK) {
			StringBuffer errormsg;
			errormsg << "Could not select mailbox '" << mailbox.String() << "' for user '" << username.String() << "'.";
			errorParams->AddString(S_ERROR_TEMPLATE, "Errors/mail.html");
			errorParams->AddString(S_ERROR_MESSAGE, errormsg.String());
			return result;
		}
	}
	// Extract the UID for the message in question
	BString uid;
	url.GetQueryParameter("uid", &uid);
	// Handle this case specially...
	if (uid == "*be:eml") {
		BString emlurl;
		if (url.GetQueryParameter("emlurl", &emlurl) != B_OK) {
			errorParams->AddString(S_ERROR_TEMPLATE, "Errors/mail.html");
			errorParams->AddString(S_ERROR_MESSAGE, "Tried to load saved message, but did not have a valid url.");
			return B_ERROR;
		}
		// Otherwise, create an adapter for this entry.
		// Note: We don't need to mark this message as read, nor do we want
		// to add this entry into the mail cache. Just keep things simple
		// for the time being...
		ConcatenateAdapter *merged = new ConcatenateAdapter;
		BMallocIO *header = new BMallocIO();
		header->Write(kHeaderHtml, strlen(kHeaderHtml));
		header->Seek(0, SEEK_SET);
		merged->AddStream(header);
		// Find out the content-type
		atom<BinderContainer> node = new BinderContainer;
		Rfc822MessageAdapter::ParseHeader(emlurl.String(), node);
		BinderNode::property ctype;
		node->ReadProperty("content-type", ctype);
		if (ctype.IsString() && (ctype == "text/html"))
			merged->AddStream(new Rfc822MessageAdapter(emlurl.String()));
		else
			merged->AddStream(new TextToHtmlAdapter(new Rfc822MessageAdapter(emlurl.String()), true));
		BMallocIO *footer = new BMallocIO();
		footer->Write(kFooterHtml, strlen(kFooterHtml));
		footer->Seek(0, SEEK_SET);
		merged->AddStream(footer);
		fMessageStream = merged;
		return B_OK;	
	}
	
	fMessage = PostOffice::MailMan()->FindMessage(uid.String());
	if (fMessage == NULL) {
		errorParams->AddString(S_ERROR_TEMPLATE, "Errors/mail.html");
		errorParams->AddString(S_ERROR_MESSAGE, "Could not find message. Possible invalid message uid.");
		return B_ERROR;
	}
	fMessage->AcquireReference();
	// See if we're downloading this to media
	BString download;
	bool downloadingBody = (url.GetQueryParameter("download", &download) == B_OK);
	 
	MessagePart *part = 0;
	BString partID;
	url.GetQueryParameter("msgpart", &partID);
	// No part ID? Get the main doc, otherwise fetch the specified part.
	bool isMainPart = (partID == "");
	// If we're downloading the body, try and get the text/plain section
	if (downloadingBody)
		part = fMessage->GetMainDoc("text/plain");
	if (!part || !downloadingBody)
		part = isMainPart ? fMessage->GetMainDoc("text/html") : fMessage->FindPart(partID.String());

	if (part && !part->isContainer) {
		fContentType = part->type;
		// FetchSection returns us an allocated DataIO that we take ownership of
		PartContainer container(mailbox.String(), uid.String(), part->id.String());
		container.SetSize(part->size);
		// Set these pointers so that we can see if they get updated
		container.SetContentTypePtr(&fContentType);
		container.SetRedirectUrlPtr(&fRedirectUrl);
		container.SetCachePolicyPtr(&fCachePolicy);
		// Check to see if we should bypass the redirect server. We pretty much only do this
		// if we are saving an attachment to disk.
		BString redirectParam;
		if ((url.GetQueryParameter("bypassRedirect", &redirectParam) == B_OK) && (redirectParam == "true"))
			container.SetBypassRedirectServer(true);
		// If this is the main part, mark it as read and try and cache it.
		container.SetIsMainPart(isMainPart);
		container.SetFlagRead(isMainPart);
		container.SetCache(isMainPart);
				
		BDataIO *source = PostOffice::MailMan()->FetchSection(container);
		if (source == NULL) {
			if (fRedirectUrl.IsValid()) {
				// Not downloading the main body, and the redirect url is valid. This must
				// mean that we're downloading an attachment, through the decoding server.
				// We can simply redirect this because we wouldn't be cacheing it anyways...
				// We're redirecting in the first place so that things like the PDF content
				// viewer, can gain access to features that might only be implemented in the
				// respective protocol. (Like with PDF, the need for byte range requests)
				// In this case, do thing here, and simply handle it in the GetRedirectURL()
				// Not that if this condition is true, then the FetchSection above will have
				// returned NULL.
				fRedirect = true;
				fMessage->ReleaseReference();
				return B_OK;
			} else {
				errorParams->AddString(S_ERROR_TEMPLATE, "Errors/mail.html");
				StringBuffer errormsg;
				errormsg << "Tried to use the redirect url but could not establish a connection. '";
				fRedirectUrl.AppendTo(errormsg);
				errormsg << "'";
				errorParams->AddString(S_ERROR_MESSAGE, errormsg.String());
				fMessage->ReleaseReference();
				return B_ERROR;
			}
		} else {
			fMessageStream = source;
			// Either we are not using the redirect server, or we are fetching the main body part (or both). 
			// First, convert from the source encoding to a raw format.
			if (part->encoding.ICompare("base64") == 0)
				fMessageStream = new Base64ToRawAdapter(fMessageStream);
			else if (part->encoding.ICompare("quoted-printable") == 0)
				fMessageStream = new QPToPlainAdapter(fMessageStream, MessagePart::CharsetConversion(part->characterSet));
	
			// Perform higher level conversions
			if (fContentType.ICompare("text/", 5) == 0) {
				ConcatenateAdapter *merged = new ConcatenateAdapter;
				if (fContentType.ICompare("text/plain") == 0) {
					// ** Kind of a hack, but there's not really a good place
					// to splice in some HTML text anywhere...
					if (!downloadingBody) {
						BMallocIO *header = new BMallocIO();
						header->Write(kHeaderHtml, strlen(kHeaderHtml));
						header->Seek(0, SEEK_SET);
						merged->AddStream(header);
						merged->AddStream(new TextToHtmlAdapter(fMessageStream));
					} else {
						BMallocIO *rfc822Header = new BMallocIO();
						CreateRfc822Header(fMessage, "text/plain", rfc822Header);
						rfc822Header->Seek(0, SEEK_SET);
						merged->AddStream(rfc822Header);
						merged->AddStream(fMessageStream);
					}
					
					if (!downloadingBody) {
						BMallocIO *footer = new BMallocIO();
						footer->Write(kFooterHtml, strlen(kFooterHtml));
						footer->Seek(0, SEEK_SET);
						merged->AddStream(footer);
					} 
					// Override the content type to text/html, since that's
					// what we're really going to be sending back out...
					fContentType = "text/html";
				} else if (fContentType.ICompare("text/html") == 0) {
					if (!downloadingBody) {
						merged->AddStream(new HTMLMailAdapter(fMessage, fMessageStream));
					} else {
						BMallocIO *rfc822Header = new BMallocIO();
						CreateRfc822Header(fMessage, "text/html", rfc822Header);
						rfc822Header->Seek(0, SEEK_SET);
						merged->AddStream(rfc822Header);
						merged->AddStream(new HTMLMailAdapter(fMessage, fMessageStream));
					}
				}
				fMessageStream  = merged;
			}
		}
	} else {
		// This specific part is not here, fail.
		const char *kCastigation = "<html><head><body><p><b>eVilla:</b> This message only contained an attachment, there was no body.</body></head></html>";
		BMallocIO *data = new BMallocIO;
		data->Write(kCastigation, strlen(kCastigation));
		data->Seek(0, SEEK_SET);
		fMessageStream  = data;
	}
	fMessage->ReleaseReference();
	return B_OK;
}

ssize_t MerlinProtocol::GetContentLength()
{
	return fContentLength;
}

void MerlinProtocol::GetContentType(char *type, int size)
{
	MDB(MailDebug md);
	DB(md.Print("Reporting our content-type as: '%s'\n", fContentType.String()));
	
	strncpy(type, fContentType.String(), size);
}

CachePolicy MerlinProtocol::GetCachePolicy()
{
	return fCachePolicy;
}

bool MerlinProtocol::GetRedirectURL(URL &redirectUrl, bigtime_t *outDelay)
{
	MDB(MailDebug md);
	if (fRedirect) {
		redirectUrl = fRedirectUrl;
		*outDelay = 0;
		StringBuffer buffer;
		redirectUrl.AppendTo(buffer);
		DB(md.Print("Doing a top-level redirect to '%s'\n", buffer.String()));
	}
	return fRedirect;
}

ssize_t MerlinProtocol::Read(void *buffer, size_t size)
{
	if (!fMessageStream)
		return B_ERROR;

	return fMessageStream->Read(buffer, size);
}

ssize_t MerlinProtocol::ReadAt(off_t , void *, size_t )
{
	return B_NO_RANDOM_ACCESS;
}

off_t MerlinProtocol::Seek(off_t , uint32 )
{
	return B_NO_RANDOM_ACCESS;
}

void MerlinProtocol::CreateRfc822Header(const MimeMessage *message, const char *contentType, BDataIO *stream)
{
	// Create a basic rfc822 formatted header so that we can save this message
	// to a media device.
	WriteHeader(stream, "From", message->GetFrom());
	WriteHeader(stream, "To", message->GetRecipient());
	WriteHeader(stream, "Subject", message->GetSubject());
	WriteHeader(stream, "Date", message->GetDate());
	if (strcmp(message->GetReplyTo(), "") != 0)
		WriteHeader(stream, "ReplyTo", message->GetReplyTo());
	if (strcmp(message->GetMessageID(), "") != 0)
		WriteHeader(stream, "Message-ID", message->GetMessageID());
	WriteHeader(stream, "Content-Type", contentType);
	WriteHeader(stream, "MIME-Version", "1.0");
	stream->Write("\r\n", 2);
}

inline void MerlinProtocol::WriteHeader(BDataIO *stream, const char *header, const char *data)
{
	stream->Write(header, strlen(header));
	stream->Write(": ", 2);
	stream->Write(data, strlen(data));	// XXX: This should really wrap at 80 characters.
	stream->Write("\r\n", 2);
}

class MerlinProtocolFactory : public ProtocolFactory {
	public:
		virtual void GetIdentifiers(BMessage* into) {
			 /*
			 ** BE AWARE: Any changes you make to these identifiers should
			 ** also be made in the 'addattr' command in the makefile.
			 */
			into->AddString(S_PROTOCOL_SCHEMES, "merlin");
		}
	
		virtual Protocol* CreateProtocol(void* handle, const char*) {
			return new MerlinProtocol(handle);
		}
		
		virtual bool KeepLoaded() const {
			return true;
		}
};

extern "C" _EXPORT ProtocolFactory* make_nth_protocol(int32 n, image_id /* you */, uint32 /* flags */, ...)
{
	if (n == 0)
		return new MerlinProtocolFactory;

	return 0;
}

