/*
	SendMessageContainer.h
*/
#ifndef _SEND_MESSAGE_CONTAINER_H
#define _SEND_MESSAGE_CONTAINER_H
#include <DataIO.h>
#include <List.h>
#include <String.h>
#include <TextView.h> // For text_run_array

namespace Wagner {

struct recipient_container {
	recipient_container(const char *address, const char *name) 
		: fAddress(address), fName(name) { }
	BString fAddress;
	BString fName;
};

struct attachment_container {
	attachment_container(const char *path, const char *name)
		:	fPath(path), fName(name) {}

	BString fPath;
	BString fName;
};
	
class SendMessageContainer {
	public:
								SendMessageContainer();
								SendMessageContainer(bool copyToImap);
								~SendMessageContainer();
		void					Init();
		
		void					SetUser(const char *userName);
		void					SetSubject(const char *subject);
		void					AddRecipient(const char *address, const char *name);
		void					AddAttachment(attachment_container *attachment);
		void					SetBodyText(const char *plainText, text_run_array *textRun);
	
		const char * 			GetUserName() const;
		const char *			GetSubject() const;
		const char *			GetFrom() const;
		const char *			GetFromAddress() const;
		const char *			GetReplyTo() const;
		const char *			GetLogin() const;
		const char *			GetPassword() const;
		const char *			GetImapServer() const;
		int						GetImapPort() const;
		const char *			GetSmtpServer() const;
		int						GetSmtpPort() const;
		
		bool					CopyToImapServer() const;
		
		int32					CountRecipients() const;
		const recipient_container * RecipientAt(int32 idx);
		
		int32					CountAttachments() const;
		const attachment_container * AttachmentAt(int32 idx);
		
		BDataIO	*				TextSource();
		text_run_array *		TextRun();
		
	private:
		BString fUser;
		BString fSubject;
		BList fRecipients;
		BList fAttachments;
		BString fFrom;
		BString fFromAddress;
		BString fReplyTo;
		BString fLogin;
		BString fPassword;
		BString fImapServer;
		int fImapPort;
		BString fSmtpServer;
		int fSmtpPort;
		
		bool fCopyToImapServer;
		BMallocIO *fSource;
		text_run_array *fTextRun;

};
}
#endif
