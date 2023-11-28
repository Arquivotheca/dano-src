/*
	IMAP.h
*/
#ifndef _IMAP_H
	#define _IMAP_H
	#include <DataIO.h>
	#include <OS.h>
	#include "MimeMessage.h"
	#include "StringBuffer.h" 

const int kLargeReadSize = 0x8000;
const int kLargeReadThreshold = 1024;

// Flags that you can use when doing a Status()
// on the mailbox...
enum StatusFlags {
	kMessagesStatus,
	kRecentStatus,
	kUidNextStatus,
	kUidValidityStatus,
	kUnseenStatus
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

enum AuthenticationType {
	kBestType = 0,
	kAuthPlainText,
	kAuthLogin,
	kAuthPlain,
	kAuthCramMd5,
};

class IMAP : public BDataIO {
	public:
								IMAP();
		virtual 				~IMAP();
		
		status_t 				Connect(const char *server, int port);
		void 					Disconnect();
		status_t				Capability();
		status_t 				Login(const char *name, const char *password, uint32 authType = kBestType);
		status_t 				Logout();
		status_t 				CreateMailbox(const char *mailbox);
		status_t 				SelectMailbox(const char *mailbox, bool writable = false);
		status_t 				StatMailbox();
		status_t				QuotaRoot(const char *mailbox);
		status_t 				Append(const char *mailbox, size_t bufferSize);
		status_t 				FetchSection(const char *uid, const char *section);
		status_t 				SetFlags(const char *uid, uint32 flags);
		status_t 				Expunge();
		status_t				Close();
		
		status_t 				GetStructure(const char *uid, MimeMessage *letter);
		status_t 				GetEnvelope(const char *uid, MimeMessage *letter);
		status_t 				GetRecentMessagesList(BList *list);
		status_t				GetMessageUidList(BList *list);
		int32 					GetTotalMessageCount() const;
		int32 					GetRecentMessageCount() const;
		int32 					GetUnseenMessageCount() const;
		uint32 					GetCapability() const;
		int32					GetMailboxSize() const;
		int32					GetMailboxQuota() const;
		
		const char 				*GetFolderUid() const;
		const char 				*GetNextUid() const;
		const char				*GetCurrentFolder() const;
		bool 					IsLoggedIn() const;
		
		virtual	ssize_t 		Read(void *buffer, size_t size);
		virtual	ssize_t 		Write(const void *buffer, size_t size);

	private:
		enum TokenType {
			kNilToken,
			kEndOfLineToken,
			kEndOfStreamToken,
			kTextToken
		};
	
		status_t				AuthenticateCramMd5(const char *name, const char *password);
		status_t				AuthenticateLogin(const char *name, const char *password);
		status_t				AuthenticatePlain(const char *name, const char *password);
		status_t				AuthenticatePlainText(const char *name, const char *password);

		status_t 				ParseResponse();
		status_t 				ParseFetchList();
		status_t 				ParseNumeric();
		status_t 				ParseOk();
		status_t 				ParseStatus();
		status_t 				ParseSearch();
		status_t 				ParseSectionOrContainer(StringBuffer &path, MessagePart *part);
		status_t 				ParseContainer(StringBuffer &path, MessagePart *part);
		status_t 				ParseSection(StringBuffer &path, MessagePart *part);
		status_t 				ParseDisposition(MessagePart*);
		status_t 				ParseEnvelope();
		status_t 				ParseAddressList(BString &fullAddress, BString &name);
		status_t 				ParseAddress(BString &fullAddress, BString &name);
		status_t 				ParseBody();
		status_t 				ParseFlags(uint32 &outFlags);
		status_t 				ParseCapability();
		status_t				ParseQuota();

		inline const char 		*NewTag();
		inline status_t 		SendCommand(StringBuffer&);
		inline const char		*GetTokenString();
		TokenType 				GetNextToken();
		void 					EatRestOfLine();
		void 					EatParenthesizedList();
		void 					PushBack();
		void 					PushBackChar();

		ssize_t 				ReadSocketBuffered(void *buffer, size_t count);
		ssize_t 				ReadSocketUnbuffered(void *buffer, size_t size);
		ssize_t 				WriteSocket(const void *buffer, size_t size);

		int fSocket;
		char fBuffer[kLargeReadSize];
		StringBuffer fToken;
		int fAmountBuffered;
		int fBufferReadPos;
		long fNextTag;
		bool fPushBack;
		int32 fTotalMessageCount;
		int32 fRecentMessageCount;
		int32 fUnseenMessageCount;
		MimeMessage *fParseMessage;
		TokenType fTokenType;
		int fCurrentMessageLength;
		int fCurrentMessageOffset;
		bool fInGreeting;
		bool fInHeader;
		bool fLoggedIn;
		BString fCurrentFolder;
		BString	fCurrentFolderUid;
		BString	fNextUid;
		BList *fSearchResultList;
		static bool fFoldersCreated;
		uint32 fCapability;
		int32 fMailboxSize;
		int32 fMailboxQuota;
};

#endif
