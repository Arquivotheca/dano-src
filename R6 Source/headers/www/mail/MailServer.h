#ifndef _MAIL_SERVER_H_
#define _MAIL_SERVER_H_

#include <support/DataIO.h>
#include <www/StringBuffer.h>
#include <www/mail/MimeMessage.h>


const int kLargeReadSize = 0x8000;		// 32 k
const int kLargeReadThreshold = 0x0400;	// 1024

enum AuthenticationType {
	kBestType = 0,
	kAuthPlainText,
	kAuthLogin,
	kAuthPlain,
	kAuthCramMd5,
	kAuthAPOP
};

// I can't find any RFC that accurately describes what
// the different capabilities are that can be reported.
// So, I've just scrafed these values from the Mozilla
// stuff...
enum CapabilityFlags {
	kNoCapability = 0x00000000, 
	kHasAuthPlainCapability = 0x00000001,
	kHasAuthLoginCapability = 0x00000002, 
	kHasCRAMCapability = 0x00000004,
	kIMAP4Capability = 0x00000008,
	kIMAP4rev1Capability = 0x00000010,
	kNamespaceCapability = 0x00000020,
	kQuotaCapability = 0x00000040
};

extern const char *	IMAP_SERVER;
extern const char *	POP3_SERVER;

class MailServer : public BDataIO {
	public:
		static MailServer *		MakeServer(const char *serverType);
								
		virtual					~MailServer();
		
		virtual	status_t		Connect(const char *server, int port);
		virtual	void 			Disconnect();
		virtual	status_t		Login(const char *name, const char *password, uint32 authType = kBestType) = 0;
		virtual	status_t		Logout() = 0;
		virtual status_t		Capability();

		virtual	status_t		FetchSection(const char *uid, const char *section) = 0;
		virtual status_t		GetMessageInfo(const char *uid, MimeMessage *letter) = 0;
		virtual	status_t		GetMessageUidList(BList *list) = 0;
		virtual status_t		StatMailbox() = 0;

		virtual	ssize_t 		Read(void *buffer, size_t size) = 0;
		virtual	ssize_t 		Write(const void *buffer, size_t size) = 0;

		virtual	int32			GetTotalMessageCount() const;
		virtual	int32			GetRecentMessageCount() const;
		virtual	int32			GetUnseenMessageCount() const;
		virtual	uint32			GetCapability() const;
		virtual	int32			GetMailboxSize() const;
		virtual	int32			GetMailboxQuota() const;
		virtual	bool			IsLoggedIn() const;
		
		// virtual functions with empty implementations
		virtual	status_t		CreateMailbox(const char *mailbox);
		virtual	status_t		SelectMailbox(const char *mailbox, bool writable = false);
		virtual	status_t		QuotaRoot(const char *mailbox);
		virtual	status_t		Append(const char *mailbox, size_t bufferSize);
		virtual	status_t		SetFlags(const char *uid, uint32 flags);
		virtual	status_t		Expunge();
		virtual	status_t		Close();
		virtual	status_t		GetStructure(const char *uid, MimeMessage *letter);
		virtual	status_t		GetEnvelope(const char *uid, MimeMessage *letter);
		virtual	status_t		GetRecentMessagesList(BList *list);
		virtual	const char *	GetFolderUid() const;
		virtual	const char *	GetNextUid() const;
		virtual	const char *	GetCurrentFolder() const;
		

	protected:
								MailServer();
		status_t 				SendCommand(StringBuffer&);
		ssize_t 				ReadSocketBuffered(void *buffer, size_t count);
		ssize_t 				ReadSocketUnbuffered(void *buffer, size_t size);
		ssize_t 				WriteSocket(const void *buffer, size_t size);

		int 	fSocket;
		char	fBuffer[kLargeReadSize];
		int		fAmountBuffered;
		int		fBufferReadPos;
		int32	fTotalMessageCount;
		int32	fRecentMessageCount;
		int32	fUnseenMessageCount;
		uint32	fCapability;
		int32	fMailboxSize;
		int32	fMailboxQuota;
		bool	fLoggedIn;
};

#endif
