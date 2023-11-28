/*
	SendMessageContainer.cpp
*/
#include <Binder.h>
#include "MallocIOAdapter.h"
#include "SendMessageContainer.h"
#include "StringBuffer.h"
#include "www/util.h"

using namespace Wagner;

SendMessageContainer::SendMessageContainer()
	:	fCopyToImapServer(true)
{
	Init();
}

SendMessageContainer::SendMessageContainer(bool copyToImap)
	:	fCopyToImapServer(copyToImap)
{
	Init();
}

SendMessageContainer::~SendMessageContainer()
{
	delete fSource;
	if (fTextRun != NULL)
		free(fTextRun);
	int32 count = fRecipients.CountItems() - 1;
	for (int32 i = count; i >= 0; i--)
		delete static_cast<BString *>(fRecipients.RemoveItem(i));
	count = fAttachments.CountItems() - 1;
	for (int32 j = count; j >= 0; j--)
		delete static_cast<attachment_container *>(fAttachments.RemoveItem(j));
}

void SendMessageContainer::Init()
{
	fSource = NULL;
	fTextRun = NULL;
}

void SendMessageContainer::SetUser(const char *userName)
{
	fUser = userName;
	BinderNode::property userNode = BinderNode::Root() / "user" / userName / "email" / "account";
	fFromAddress = userNode["Email"].String();
	fFrom << "\"" << userNode["DisplayName"].String() << "\" <" << userNode["Email"].String() << ">";
	fReplyTo = userNode["ReplyTo"].String();
	fLogin = userNode["Login"].String();
	fPassword = userNode["Password"].String();
	fImapServer = userNode["ImapServer"].String();
	fImapPort = atoi(userNode["ImapPort"].String().String());
	fSmtpServer = userNode["SmtpServer"].String();
	fSmtpPort = atoi(userNode["SmtpPort"].String().String());
}

void SendMessageContainer::SetSubject(const char *subject)
{
	fSubject = subject;
}

void SendMessageContainer::AddRecipient(const char *address, const char *name)
{
	fRecipients.AddItem(new recipient_container(address, name));
}

void SendMessageContainer::AddAttachment(attachment_container *attachment)
{
	fAttachments.AddItem(new attachment_container(attachment->fPath.String(), attachment->fName.String()));
}

void SendMessageContainer::SetBodyText(const char *plainText, text_run_array *textRun)
{
	fSource = new MallocIOAdapter;
	fSource->SetBlockSize(strlen(plainText));
	fSource->Write((void *)plainText, strlen(plainText));
	fSource->Seek(0, SEEK_SET);
	// We assume ownership of textRun. It was *malloc'd* in the text view..
	fTextRun = textRun;
}

const char *SendMessageContainer::GetUserName() const
{
	return fUser.String();
}

const char *SendMessageContainer::GetSubject() const
{
	return fSubject.String();
}

const char *SendMessageContainer::GetFrom() const
{
	return fFrom.String();
}

const char *SendMessageContainer::GetFromAddress() const
{
	return fFromAddress.String();
}

const char *SendMessageContainer::GetReplyTo() const
{
	return fReplyTo.String();
}

const char *SendMessageContainer::GetLogin() const
{
	return fLogin.String();
}

const char *SendMessageContainer::GetPassword() const
{
	return fPassword.String();
}

const char *SendMessageContainer::GetImapServer() const
{
	return fImapServer.String();
}

int SendMessageContainer::GetImapPort() const
{
	return fImapPort;
}

const char *SendMessageContainer::GetSmtpServer() const
{
	return fSmtpServer.String();
}

int SendMessageContainer::GetSmtpPort() const
{
	return fSmtpPort;
}

bool SendMessageContainer::CopyToImapServer() const
{
	return fCopyToImapServer;
}

int32 SendMessageContainer::CountRecipients() const
{
	return fRecipients.CountItems();
}

const recipient_container *SendMessageContainer::RecipientAt(int32 idx)
{
	return (reinterpret_cast<const recipient_container *>(fRecipients.ItemAt(idx)));
}

int32 SendMessageContainer::CountAttachments() const
{
	return fAttachments.CountItems();
}

const attachment_container *SendMessageContainer::AttachmentAt(int32 idx)
{
	return (reinterpret_cast<const attachment_container *>(fAttachments.ItemAt(idx)));
}

BDataIO *SendMessageContainer::TextSource()
{
	return fSource;
}

text_run_array *SendMessageContainer::TextRun()
{
	return fTextRun;
}
