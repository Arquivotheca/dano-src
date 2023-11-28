/*
	MimePartHeaderAdapter.cpp
*/
#include <stdio.h>
#include "MimePartHeaderAdapter.h"
#include "StringBuffer.h"

MimePartHeaderAdapter::MimePartHeaderAdapter(mime_part_type type, bool needToClose, StringBuffer &outerBoundary,
												StringBuffer &innerBoundary, SendMessageContainer *container,
												const char *contentType, const char *fileName)
	:	fNeedToClose(needToClose),
		fIndex(0),
		fAddedFooter(false),
		fFooter(NULL)
{
	fHeader = new BMallocIO;
	fHeader->SetBlockSize(1024);
	// Create a boundary if we're starting a new section
	if (type == kTopLevelPart)
		CreateBoundary(&outerBoundary);
	if (type == kMultipartAlternativePart)
		CreateBoundary(&innerBoundary);
		
	// Prefix the header with the outerBoundary, but only if we're not the top part
	if (type != kTopLevelPart)
		InsertBoundary(outerBoundary.String());

	switch (type) {
		case kTopLevelPart:
			BuildRfc822Header(outerBoundary.String(), container);
			break;
		case kMultipartAlternativePart:
			BuildMultipartAlternativeHeader(innerBoundary.String());
			break;
		case kTextPart:
			BuildTextHeader();
			break;
		case kHtmlPart:
			BuildHtmlHeader();
			break;
		case kAttachmentPart:
			BuildAttachmentHeader(contentType, fileName);
			break;
		default:
			// Should not be here!
			break;
	}
	
	if (needToClose) {
		fFooter = new BMallocIO;
		if (type == kMultipartAlternativePart)
			InsertBoundary(innerBoundary.String(), true);
		else
			InsertBoundary(outerBoundary.String(), true);
		fFooter->Seek(0, SEEK_SET);
	}
		
	fHeader->Seek(0, SEEK_SET);
	AddStream(fHeader);
}

MimePartHeaderAdapter::~MimePartHeaderAdapter()
{
	int count = fSources.CountItems() - 1;
	for (int i = count; i >= 0; i--)
		delete reinterpret_cast<BDataIO*>(fSources.RemoveItem(i));
	// If the footer was not yet added to the streams, then delete it manually
	if (!fAddedFooter)
		delete fFooter;
}

void MimePartHeaderAdapter::BuildRfc822Header(const char *boundary, SendMessageContainer *container)
{
	StringBuffer buffer;
	buffer << "To: ";
	const recipient_container *recipient;
	int32 count = container->CountRecipients();
	for (int32 i = 0; i < count; i++) {
		recipient = container->RecipientAt(i);
		if (i > 0)
			buffer << "\t";
		if (recipient->fName.String() != "")
			buffer << "\"" << recipient->fName.String() << "\" <";
		buffer << recipient->fAddress.String();
		if (recipient->fName.String() != "")
			buffer << ">";
		if ((i + 1) < count)
			buffer << ",";
		buffer << "\r\n";
	}
	buffer << "Subject: " << container->GetSubject() << "\r\n";
	buffer << "From: " << container->GetFrom() << "\r\n";
	buffer << "Reply-To: " << container->GetReplyTo() << "\r\n";
	// Date
	time_t time = real_time_clock();
	struct tm *lcltm = localtime(&time);
	char date[512];
	strftime(date,512,"%a, %d %b %Y %H:%M:%S",lcltm);
	sprintf(date, "%s %0+5d", date, (lcltm->tm_gmtoff / 3600) * 100);
	buffer << "Date: " << date << "\r\n";
	buffer << "MIME-Version: 1.0\r\n";
	buffer << "Content-type: ";
	buffer << (container->CountAttachments() > 0 ? "multipart/mixed;" : "multipart/alternative;");
	buffer << "\r\n\tboundary=\"" << boundary << "\"\r\n";
	// ** Should really add a Message-ID field **
	buffer << "X-Mailer: " << kMailerIdentification << "\r\n";
	buffer << "\r\n"; // Required blank line seperating header from body
	buffer << "This messsage contains a multipart message in MIME format.\r\n\r\n";
	fHeader->Write((void *)buffer.String(), buffer.Length());
}

void MimePartHeaderAdapter::BuildMultipartAlternativeHeader(const char *boundary)
{
	StringBuffer buffer;
	buffer << "Content-Type: multipart/alternative;";
	buffer << "\r\n\tboundary=\"" << boundary << "\"\r\n";
	buffer << "\r\n";
	fHeader->Write((void *)buffer.String(), buffer.Length());
}

void MimePartHeaderAdapter::BuildTextHeader()
{
	StringBuffer buffer;
	buffer << "Content-Type: text/plain;\r\n";
//	buffer << "\tcharset=\"iso-8859-1\"\r\n";	// ** LIAR! Make sure this gets changed when proper support is added
	buffer << "Content-Transfer-Encoding: 7bit\r\n";
	buffer << "\r\n";
	fHeader->Write((void *)buffer.String(), buffer.Length());
}

void MimePartHeaderAdapter::BuildHtmlHeader()
{
	StringBuffer buffer;
	buffer << "Content-Type: text/html;\r\n";
	buffer << "\tcharset=\"iso-8859-1\"\r\n";	// ** LIAR! Make sure this gets changed when proper support is added
	buffer << "Content-Transfer-Encoding: quoted-printable\r\n";
	buffer << "\r\n";
	fHeader->Write((void *)buffer.String(), buffer.Length());
}

void MimePartHeaderAdapter::BuildAttachmentHeader(const char *contentType, const char *fileName)
{
	StringBuffer buffer;
	buffer << "Content-Type: " << contentType << ";\r\n";
	buffer << "\tname=\"" << fileName << "\"\r\n";
	buffer << "Content-Transfer-Encoding: base64\r\n";
	buffer << "Content-Disposition: attachment;\r\n";
	buffer << "\tfilename=\"" << fileName << "\"\r\n";
	buffer << "\r\n";
	fHeader->Write((void *)buffer.String(), buffer.Length());
}

void MimePartHeaderAdapter::CreateBoundary(StringBuffer *buffer)
{
	buffer->Clear();
	StringBuffer boundary;
	for (int8 i = 0; i < 10; i++)
		boundary << rand() % 10;
	
	*buffer << "_boundary_kcc_" << boundary.String() << "_";
}

void MimePartHeaderAdapter::InsertBoundary(const char *boundary, bool close)
{
	StringBuffer buffer;
	buffer << "--";
	buffer << boundary;
	buffer << (close ? "--\r\n" : "\r\n");
	if (close)
		fFooter->Write((void *)buffer.String(), buffer.Length());
	else
		fHeader->Write((void *)buffer.String(), buffer.Length());
}

ssize_t MimePartHeaderAdapter::Read(void *buffer, size_t size)
{
	ssize_t totalRead = 0;

	// We've sent all our stuff, now send the data for our children	
	while (totalRead < size) {
		BDataIO *stream = static_cast<BDataIO *>(fSources.ItemAt(fIndex));
		if (stream == 0) {
			// Little hack, tuck in the footer. This ensures that the footer
			// is always the last stream.
			if ((!fAddedFooter) && (fFooter != NULL)){
				AddStream(fFooter);
				fAddedFooter = true;
				continue;
			}
			break;
		}

		ssize_t got = stream->Read(static_cast<char *>(buffer) + totalRead, size - totalRead);
		if (got <= 0)
			fIndex++;
		else
			totalRead += got;
		
	}
	return totalRead;
}

void MimePartHeaderAdapter::AddStream(BDataIO *stream)
{
	fSources.AddItem(stream);
}

ssize_t MimePartHeaderAdapter::Write(const void *, size_t)
{
	return B_ERROR;
}



