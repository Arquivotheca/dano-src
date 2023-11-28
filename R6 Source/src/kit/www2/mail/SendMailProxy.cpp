/*
	SendMailProxy.cpp
*/
#include <UTF8.h>
#include "MailBinderSupport.h"
#include "MailDebug.h"
#include "MailStatusCodes.h"
#include "MimePartHeaderAdapter.h"
#include "PlainToQPAdapter.h"
#include "PostOffice.h"
#include "Protocol.h"
#include "RawToBase64Adapter.h"
#include "SendMailProxy.h"
#include "SendMessageContainer.h"
#include "StyledTextToHtmlAdapter.h"
#include "TextToRfc822BodyAdapter.h"
#include "URL.h"

using namespace Wagner;

SendMailProxy &SendMailProxy::Newman()
{
	static SendMailProxy newman;
	return newman;
}

SendMailProxy::SendMailProxy()
	: 	fKeepDelivering(true)
{
	fQueueSem = create_sem(1,"fQueueSem");
	fSignalSem = create_sem(0, "WaitingForNewMailToSend");
	fQueueThread = spawn_thread(DeliveryManEntry, "Newman", B_LOW_PRIORITY, this);
	resume_thread(fQueueThread);
}

SendMailProxy::~SendMailProxy()
{
	MDB(MailDebug md);
	fKeepDelivering = false;
	release_sem(fSignalSem);
	status_t result;
	wait_for_thread(fQueueThread, &result);
	
	delete_sem(fQueueSem);
	delete_sem(fSignalSem);
}

status_t SendMailProxy::SendMessage(SendMessageContainer *container)
{
	if(acquire_sem(fQueueSem) == B_NO_ERROR) {
		// We assume ownership of container
		fQueuedMessages.AddItem(container);
		release_sem(fQueueSem);
		release_sem(fSignalSem);
	} else {
		return B_ERROR;
	}

	return B_OK;
}

status_t SendMailProxy::DeliverTheMessage(SendMessageContainer *container)
{
	MDB(MailDebug md);
	status_t result = B_OK;
	// Initialize the mail status proxy
	MailStatusProxy mailStatus(kStatusSendingMessage);
	// Inititalize the newman node proxy
	SendingNodeProxy sendingNodeProxy(container->GetUserName(), kStatusSendingMessage);
	sendingNodeProxy.SetTotalBytes(0);	// XXX: To do, figure out how large the message is!
	// Verify that all the attachments are valid
	fInvalidAttachment = B_EMPTY_STRING;
	result = VerifyAttachmentsExist(container, fInvalidAttachment);
	if (result != B_OK)
		return result;
	// Open a connection to the Smtp and Imap servers
	IMAP imap;
	SmtpDaemon smtp;
	if ((result = OpenImapSmtpConnections(imap, smtp, container)) != B_OK) {
		return result;
	}
	// Create all the adapter chains, we have to free what's returned
	BDataIO *topAdapter = CreateAdapterChain(container);	
	// Start the data session
	if ((result = smtp.StartData()) != B_OK) {
		delete topAdapter;
		return result;
	}
	// Bombs away!
	char buffer[4096];
	ssize_t read = 0;
	ssize_t messageSize = 0;
	while ((read = topAdapter->Read(buffer, 4096)) > 0) {
		messageSize += read;
		ssize_t sent = smtp.Write(buffer, read);
		sendingNodeProxy.UpdateBytesSent(sent);
	}
	// Close the data sessoion
	if ((result = smtp.EndData()) != B_OK) {
		delete topAdapter;
		return result;
	}
	// Quit smtp connection and close imap connection
	if ((result = smtp.Disconnect()) != B_OK)
		DB(md.Print("Error on disconnect\n"));
	// Delete the adapter.. Ideally we'd like to reuse it below
	// but there's no way to 'rewind' it.
	delete topAdapter;
	// This is totally stupid but I don't think there is another way around this.
	// There's no way that we can know the size of this message ahead of time without
	// completely building it. However, the IMAP command 'append' requires that the
	// size of the message be specified before you upload it. (Dumb!) Pretty much
	// the only way to do this is to send to the smtp server, count how many bytes
	// we sent, then do it all over to the IMAP server. I can't just reuse the
	// adapter chain from the smtp because it would have to be reset, which is
	// a bit tricky. Arg! Kenny
	if (container->CopyToImapServer()) {
		mailStatus.SetStatus(kStatusUploadingMessage);
		// Reset the newman status node
		sendingNodeProxy.Reset();
		sendingNodeProxy.SetStatus(kStatusUploadingMessage);
		sendingNodeProxy.SetTotalBytes(messageSize);
		// Make sure there's room on the server
		if (imap.GetMailboxSize() + messageSize < imap.GetMailboxQuota()) {
			topAdapter = CreateAdapterChain(container);
			// Now send through the imap command
			if ((result = imap.Append("Sent Items", messageSize)) != B_OK) {
				delete topAdapter;
				return result;
			}
			while ((read = topAdapter->Read(buffer, 4096)) > 0) {
				imap.Write(buffer, read);
				sendingNodeProxy.UpdateBytesSent(read);
			}
			// Tell imap server to parse response and disconnect
			imap.Write("\r\n", 2);
			imap.Write("", 0);
			imap.Disconnect();
			// Free all the adapters
			delete topAdapter;
		} else {
			return B_DEVICE_FULL;
		}
	}
	return B_OK;
}

status_t SendMailProxy::OpenImapSmtpConnections(IMAP &imap, SmtpDaemon &smtp, SendMessageContainer *container)
{
	MDB(MailDebug md);
	status_t result;
	// Start with the smtp server
	if ((result = smtp.Connect(container)) != B_OK) {
		DB(md.Print("Connection to smtp server '%s' at port '%d' failed.\n", container->GetSmtpServer(), container->GetSmtpPort()));
		return result;
	}
	// Log into the smtp and imap server
	if ((result = smtp.Login(container)) != B_OK) {
		DB(md.Print("Loging to smtp server failed.\n"));
		return result;
	}
	// Pre-flight the smtp
	if ((result = smtp.PreFlightSmtp(container)) != B_OK) {
		DB(md.Print("Preflight of smtp failed.\n"));
		return result;
	}
	// Open the imap connection
	if ((result = imap.Connect(container->GetImapServer(), container->GetImapPort())) != B_OK) {
		DB(md.Print("Connection to server '%s' at port '%d' failed.\n", container->GetImapServer(), container->GetImapPort()));
		return result;
	}
	if ((result = imap.Login(container->GetLogin(), container->GetPassword())) != B_OK) {
		DB(md.Print("Login of user '%s' with password '%s' failed.\n", container->GetLogin(), container->GetPassword()));
		return result;
	}
	if ((result = imap.SelectMailbox("Sent Items", true)) != B_OK) {
		DB(md.Print("Selecting folder '%s' failed.\n", "Sent Items"));
		return result;
	}
	return B_OK;
}

BDataIO *SendMailProxy::CreateAdapterChain(SendMessageContainer *container)
{
	// Create the top level boundary. (There is always at least
	// one boundary to seperate the plain text and html versions)
	StringBuffer outerBoundary;
	StringBuffer innerBoundary;
	// Create the top level mime container
	MimePartHeaderAdapter *topPart = new MimePartHeaderAdapter(kTopLevelPart, true, outerBoundary, innerBoundary, container);
	// If there are no attachments, then the top part will be multipart/alternative
	if (container->CountAttachments() == 0) {
		// Text adapter
		MimePartHeaderAdapter *textPart = new MimePartHeaderAdapter(kTextPart, false, outerBoundary, innerBoundary);
		textPart->AddStream(new TextToRfc822BodyAdapter(container->TextSource(), 76, false));
		topPart->AddStream(textPart);
		// Html adapter
		MimePartHeaderAdapter *htmlPart = new MimePartHeaderAdapter(kHtmlPart, false, outerBoundary, innerBoundary);
		StyledTextToHtmlAdapter *textToHtml = new StyledTextToHtmlAdapter(container->TextSource(), container->TextRun(), false);
		htmlPart->AddStream(new PlainToQPAdapter(textToHtml, B_ISO1_CONVERSION, true));
		topPart->AddStream(htmlPart);
	} else {
		MimePartHeaderAdapter *alternativePart = new MimePartHeaderAdapter(kMultipartAlternativePart, true, outerBoundary, innerBoundary);
		
		MimePartHeaderAdapter *textPart = new MimePartHeaderAdapter(kTextPart, false, innerBoundary, innerBoundary);
		textPart->AddStream(new TextToRfc822BodyAdapter(container->TextSource(), 76, false));
		alternativePart->AddStream(textPart);
		
		MimePartHeaderAdapter *htmlPart = new MimePartHeaderAdapter(kHtmlPart, false, innerBoundary, innerBoundary);
		StyledTextToHtmlAdapter *textToHtml = new StyledTextToHtmlAdapter(container->TextSource(), container->TextRun(), false);
		htmlPart->AddStream(new PlainToQPAdapter(textToHtml, B_ISO1_CONVERSION, true));
		alternativePart->AddStream(htmlPart);
		
		topPart->AddStream(alternativePart);
		
		for (int32 i = 0; i < container->CountAttachments(); i++) {
			const attachment_container *attachment = reinterpret_cast<const attachment_container *>(container->AttachmentAt(i));
			if (attachment == NULL)
				continue;
			URL url(attachment->fPath.String());	
			// We take ownership of this procotol.
			Protocol *protocol = Protocol::InstantiateProtocol(url.GetScheme());
			if (protocol == 0)
				continue;

			URL requestor;
			BMessage errorParams;
			if (protocol->Open(url, requestor, &errorParams, 0) != B_OK) {
				delete protocol;
				continue;
			}
			char contentType[512];
			protocol->GetContentType(contentType, 512);
			MimePartHeaderAdapter *attachmentPart = new MimePartHeaderAdapter(kAttachmentPart, false, outerBoundary,
																				innerBoundary, NULL, contentType, attachment->fName.String());
			// RawToBase64Adapter assumes ownership of the protocol!
			attachmentPart->AddStream(new RawToBase64Adapter(protocol, true));
			topPart->AddStream(attachmentPart);
		}
	}
	return topPart;
}

status_t SendMailProxy::DeliveryManEntry(void *arg)
{
	SendMailProxy *proxy = static_cast<SendMailProxy *>(arg);
	return proxy->DeliveryMan();
}

status_t SendMailProxy::DeliveryMan()
{
	while (fKeepDelivering) {
		if (acquire_sem(fSignalSem) == B_NO_ERROR) {
			if (!fKeepDelivering)
				break;

			SendMessageContainer *container = NULL;
			if (acquire_sem(fQueueSem) == B_NO_ERROR) {
				container = static_cast<SendMessageContainer *>(fQueuedMessages.RemoveItem(0L));
				release_sem(fQueueSem);
			}
			if (container != NULL) {
				status_t result = DeliverTheMessage(container);
				if (result == B_OK) {
					BMessage message(kSyncUserMsg);
					message.AddString("user", container->GetUserName());
					PostOffice::MailMan()->PostMessage(message);
				}
				delete container;

				switch (result) {
					case B_ENTRY_NOT_FOUND:
						DoJavascriptCallback(kInvalidAttachment);
						break;
					case B_DEVICE_FULL:
						DoJavascriptCallback(kNoSpace);
						break;
					case B_OK:
						DoJavascriptCallback(kConfirmSend);
						break;
					default:
						DoJavascriptCallback(kUnableToSend);
						break;
				}
			}
		}
	}
	return B_OK;
}

void SendMailProxy::DoJavascriptCallback(callback_action action)
{
	MDB(MailDebug md);

	switch (action) {
		case kConfirmSend: {
			BinderNode::Root()["service"]["web"]["topContent"]["window"]["mail_confirm_mail_send"]();
			break;
		}
		case kUnableToSend: {
			BinderNode::Root()["service"]["web"]["topContent"]["window"]["mail_alert_unable_to_send"]();
			break;
		}
		case kInvalidAttachment: {
			BinderNode::property arg = fInvalidAttachment.String();
			BinderNode::Root()["service"]["web"]["topContent"]["window"]["mail_alert_invalid_attachment"](&arg, NULL);
			break;
		}
		case kNoSpace: {
			BinderNode::Root()["service"]["web"]["topContent"]["window"]["mail_alert_no_space"]();
			break;
		}
		default:
			break;
	}
}

status_t SendMailProxy::VerifyAttachmentsExist(SendMessageContainer *container, BString &attachmentName)
{
	MDB(MailDebug md);

	status_t result = B_OK;
	
	for (int32 i = 0; i < container->CountAttachments(); i++) {
		const attachment_container *attachment = reinterpret_cast<const attachment_container *>(container->AttachmentAt(i));
		if (attachment == NULL)
			continue;
		// Ensure that the path in question actually exists (only applies of it's a file:/// type, which is the norm);
		URL url(attachment->fPath.String());
		// If it's a file, then check that it exists
		if (strncasecmp(url.GetScheme(), "file", 4) == 0) {
			BEntry entry(url.GetPath());
			if ((entry.InitCheck() != B_OK) || (!entry.Exists()))
				result = B_ENTRY_NOT_FOUND;			
		} else {
			// Otherwise make sure we have a valid protocol
			URL url(attachment->fPath.String());	
			Protocol *protocol = Protocol::InstantiateProtocol(url.GetScheme());
			if (protocol == 0)
				result = B_ENTRY_NOT_FOUND;
			delete protocol;
		}
		
		if (result != B_OK) {
			attachmentName = url.GetPath();
			break;
		}
	}
	return result;
}

/* End of SendMailProxy.cpp */
