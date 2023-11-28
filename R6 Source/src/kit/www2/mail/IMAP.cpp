/*
	IMAP.cpp
*/

#define IMAP_TRACE 0

#include <ctype.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <String.h>
#include <Debug.h>
#include "RawToBase64Adapter.h"
#include "Base64ToRawAdapter.h"
#include "DNSCache.h"
#include "IMAP.h"
#include "KeyedMD5.h"
#include "MailDebug.h"


#define REPORT_ERROR(x) printf("%d: at token \"%s\": %s\n", __LINE__, GetTokenString(), #x);

const int kMaxTokenLength = 512;
const char *kTagPrefix = "Tag";
bool IMAP::fFoldersCreated = false;

IMAP::IMAP()
	:	fSocket(-1),
		fToken(kMaxTokenLength),
		fAmountBuffered(0),
		fBufferReadPos(0),
		fNextTag(1000),
		fPushBack(false),
		fTotalMessageCount(0),
		fRecentMessageCount(0),
		fUnseenMessageCount(0),
		fParseMessage(0),
		fTokenType(kEndOfStreamToken),
		fCurrentMessageLength(0),
		fCurrentMessageOffset(0),
		fInGreeting(true),
		fInHeader(true),
		fLoggedIn(false),
		fCurrentFolder(""),
		fCurrentFolderUid(""),
		fNextUid(""),
		fSearchResultList(NULL),
		fCapability(kNoCapability),
		fMailboxSize(0),
		fMailboxQuota(0)
{

}

IMAP::~IMAP()
{
	Disconnect();
}

status_t IMAP::Connect(const char *server, int port)
{
	fSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fSocket < 0)
		return errno;

	// Set a 60 second timeout
	struct timeval tv ={60, 0};
	//set the receive timeout
	if (setsockopt(fSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0) {
		//that didn't work, should we do something here?
	}
	// set the send timeout (usually not necessary, since send() returns before the data is
	// actually sent)
	setsockopt(fSocket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));

	status_t result = dnsCache.GetHostByName(server, &addr.sin_addr.s_addr);
	if (result < 0)
		return result;
	
	addr.sin_len = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (connect(fSocket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
		return errno;

	result = ParseResponse();
	fInGreeting = false;
	if (result != B_OK)
		return result;
	return Capability();
}

void IMAP::Disconnect()
{
	if (fLoggedIn) 
		Logout();
	if (fSocket >= 0) {
		close(fSocket);
		fSocket = -1;
	}
}

status_t IMAP::Capability()
{
	MDB(MailDebug md);
	
	StringBuffer request;
	request << NewTag() << " capability\r\n";
	status_t err = SendCommand(request);
	if (err < 0)
		return err;

	return ParseResponse();
}

status_t IMAP::Login(const char *name, const char *password, uint32 authType)
{
	if ((name == NULL) || (password == NULL))
		return B_ERROR;
	
	if (strcmp(name, B_EMPTY_STRING) == 0)
		return B_ERROR;
		
	if (fLoggedIn)
		Logout();

	status_t result;
	if (authType == kBestType) {
		if (fCapability & kHasCRAMCapability)
			result = AuthenticateCramMd5(name, password);
		else if (fCapability & kHasAuthLoginCapability)
			result = AuthenticateLogin(name, password);
		else if (fCapability & kHasAuthPlainCapability)
			result = AuthenticatePlain(name, password);
		else
			result = AuthenticatePlainText(name, password);
	} else {
		if (authType == kAuthCramMd5)
			result = AuthenticateCramMd5(name, password);
		else if (authType == kAuthLogin)
			result = AuthenticateLogin(name, password);
		else if (authType == kAuthPlain)
			result = AuthenticatePlain(name, password);
		else
			result = AuthenticatePlainText(name, password);
	}
	if (result != B_OK)
		return result;
	// Otherwise, check if authentication passed.
	result = ParseResponse();
	if (result == B_OK) {
		fLoggedIn = true;
		if (!IMAP::fFoldersCreated) {
			CreateMailbox("Drafts");
			CreateMailbox("Sent Items");
			IMAP::fFoldersCreated = true;
		}
	}
	return result;
}

status_t IMAP::Logout()
{
	MDB(MailDebug md);
	
	StringBuffer request;
	request << NewTag() << " logout\r\n";
	status_t err = SendCommand(request);
	if (err < 0)
		return err;

	fLoggedIn = false;
	return ParseResponse();
}

status_t IMAP::CreateMailbox(const char *mailbox)
{
	// INBOX should always be there, create the other two
	StringBuffer request;
	request << NewTag() << " create \"" << mailbox << "\"\r\n";
	status_t err = SendCommand(request);
	if (err < 0)
		return err;
		
	return ParseResponse();
}

status_t IMAP::SelectMailbox(const char *mailbox, bool writable)
{
	StringBuffer request;
	request << NewTag();
	if (writable)
		request << " select \"";
	else
		request << " examine \"";
	 
	request << mailbox << "\"\r\n";
	status_t err = SendCommand(request);
	if (err < 0)
		return err;

	err = ParseResponse();
	if (err == B_OK) {
		fCurrentFolder = mailbox;
		if (fCapability & kQuotaCapability)
			err = QuotaRoot(mailbox);
	}

	return err;
}

status_t IMAP::QuotaRoot(const char *mailbox)
{
	StringBuffer request;
	request << NewTag() << " getquotaroot \"" << mailbox << "\"\r\n";

	status_t err = SendCommand(request);
	if (err < 0)
		return err;
		
	return ParseResponse();
}

status_t IMAP::StatMailbox()
{
	StringBuffer request;
	request << NewTag() << " status \"" << fCurrentFolder.String() << "\" (messages recent unseen)\r\n";

	status_t err = SendCommand(request);
	if (err < 0)
		return err;
		
	return ParseResponse();
}

status_t IMAP::Append(const char *mailbox, size_t bufferSize)
{
	StringBuffer request;
	request << NewTag();
	request << " append \"" << mailbox << "\" (\\Seen) {" << bufferSize << "}\r\n";
	status_t err = SendCommand(request);
	if (err < 0)
		return err;
	
	return ParseResponse();
}

status_t IMAP::FetchSection(const char *uid, const char *section)
{
	StringBuffer request;
	request << NewTag() << " uid fetch " << uid << " body[" << section << "]\r\n";
	status_t err = SendCommand(request);
	if (err < 0)
		return err;

	return ParseResponse();
}

status_t IMAP::SetFlags(const char *uid, uint32 flags)
{
	StringBuffer request;
	request << NewTag() << " uid store " << uid << " flags.silent (";
	
	if (flags & kMessageAnswered)
		request << "\\answered ";
		
	if (flags & kMessageFlagged)
		request << "\\flagged ";
	
	if (flags & kMessageDeleted)
		request << "\\deleted ";
		
	if (flags & kMessageSeen)
		request << "\\seen ";
		
	if (flags & kMessageDraft)
		request << "\\draft ";
		
	if (flags & kMessageRecent)
		request << "\\recent";

	request << ")\r\n";
	status_t err = SendCommand(request);
	if (err < 0)
		return err;
		
	return ParseResponse();			
}

status_t IMAP::Expunge()
{
	StringBuffer request;
	request << NewTag() << " expunge\r\n";
	
	status_t err = SendCommand(request);
	if (err < 0)
		return err;
		
	return ParseResponse();
}

status_t IMAP::Close()
{
	StringBuffer request;
	request << NewTag() << " close\r\n";
	status_t err = SendCommand(request);
	if (err < 0)
		return err;
		
	return ParseResponse();
}

status_t IMAP::GetStructure(const char *uid, MimeMessage *letter)
{
	StringBuffer request;
	request << NewTag() << " uid fetch " << uid << " (bodystructure)\r\n";

	status_t err = SendCommand(request);
	if (err < 0)
		return err;

	letter->SetMailbox(fCurrentFolder.String());
	fParseMessage = letter;
	err = ParseResponse();
	fParseMessage = 0;
	return err;
}

status_t IMAP::GetEnvelope(const char *msgid, MimeMessage *letter)
{
	MDB(MailDebug md);

	StringBuffer request;
	request << NewTag() << " uid fetch " << msgid << " (envelope flags rfc822.size uid body.peek[header.fields (content-type)])\r\n";
	status_t err = SendCommand(request);
	if (err < 0)
		return err;

	letter->SetMailbox(fCurrentFolder.String());
	fParseMessage = letter;
	err = ParseResponse();
	
	// The section data we will now receive from Read contains part of the header
	// of the message.  Scan through it and find the content type.  This is a little
	// clunky, but the normal parser doesn't handle it because it's not really an
	// IMAP header.
	char header[1024];
	int got = 0;
	
	while(got < fCurrentMessageLength) {
		// grab enough buffer to eat up the rest of the body section of
		// the header. Sometimes a single call of Read does not return
		// all of the header
		int retval = Read(&header[got], 1024);
		if(retval <= 0) {
			// XXX uh, what to do
			break;
		}
		got += retval;
	}	
	header[got] = '\0';

	char *typeStart = strchr(header, ':');
	if (typeStart) {
		typeStart++; // Skip past ':'
		while (*typeStart && isspace(*typeStart))
			typeStart++;
			
		char *end = typeStart;
		while (*end && *end != '\0' && *end != '\r' && *end != ';' && !isspace(*end))
			end++;
	
		*end = '\0';
		fParseMessage->SetContentType(typeStart);
	} else {
		fParseMessage->SetContentType("text/plain");
	}

	fParseMessage = 0;
	return err;
}

status_t IMAP::GetRecentMessagesList(BList *list)
{
	StringBuffer request;
	request << NewTag() << " search (recent)\r\n";

	status_t err = SendCommand(request);
	if (err < 0)
		return err;
		
	fSearchResultList = list;
	return ParseResponse();
}

status_t IMAP::GetMessageUidList(BList *list)
{
	// This, and the function above, really should be merged into
	// a more generic 'fetch' call... Kenny
	StringBuffer request;
	request << NewTag() << " uid search all\r\n";
	
	status_t err = SendCommand(request);
	if (err < 0)
		return err;
	
	fSearchResultList = list;
	return ParseResponse();
}

int32 IMAP::GetTotalMessageCount() const
{
	return fTotalMessageCount;
}

int32 IMAP::GetRecentMessageCount() const
{
	return fRecentMessageCount;
}

int32 IMAP::GetUnseenMessageCount() const
{
	return fUnseenMessageCount;
}

uint32 IMAP::GetCapability() const
{
	return fCapability;
}

int32 IMAP::GetMailboxSize() const
{
	return fMailboxSize;
}

int32 IMAP::GetMailboxQuota() const
{
	return fMailboxQuota;
}

const char *IMAP::GetFolderUid() const
{
	return fCurrentFolderUid.String();
}

const char *IMAP::GetNextUid() const
{
	return fNextUid.String();
}

const char *IMAP::GetCurrentFolder() const
{
	return fCurrentFolder.String();
}

bool IMAP::IsLoggedIn() const
{
	return fLoggedIn;
}

//	Read is a little weird, because it allows "out-of-band" streaming
// 	of some part of the server response (usually a message body part).
//	This always occurs inside a 'BODY' tag.
ssize_t IMAP::Read(void *buf, size_t size)
{
	if (fCurrentMessageOffset >= fCurrentMessageLength)
		return 0;
		
	ssize_t got = ReadSocketBuffered(buf, MIN(size, fCurrentMessageLength - fCurrentMessageOffset));
	if (got > 0)
		fCurrentMessageOffset += got;
	
	if (fCurrentMessageOffset == fCurrentMessageLength) {
		fInHeader = true;

		// Seems the new CommuniGate Pro server that Earthlink just
		// installed returns ' UID #)' instead of just the closing
		// paren if the fetch was done using a UID. Not sure if this
		// is legal since I can't find any mention of this in the RFC...
		GetNextToken();
		if (strcmp(GetTokenString(), "UID") == 0) {
			// Next token should be the UID number
			GetNextToken();
			if (!isdigit(GetTokenString()[0])) {
				REPORT_ERROR("Expected UID number, number was not there.");
				return B_ERROR;
			}
			// Next token should be the closing paren.
			GetNextToken();
		}
		// Close out the body tag and continue parsing the rest of the response
		if (strcmp(GetTokenString(), ")") != 0) {
			REPORT_ERROR("Expected ')'");
			return B_ERROR;
		}


		ParseResponse();
	}
	return got;
}

ssize_t IMAP::Write(const void *buffer, size_t size)
{
	ssize_t sent = 0;
	if (size > 0) {
		sent = WriteSocket(buffer, size);
	} else {
		ParseResponse();
	}
	return sent;
}

status_t IMAP::AuthenticateCramMd5(const char *name, const char *password)
{
	MDB(MailDebug md);
	
	if (fPushBack && (fTokenType == kEndOfLineToken))
		fPushBack = false;
		
	StringBuffer command;
	command << NewTag() << " authenticate cram-md5\r\n";
	status_t result = SendCommand(command);
	if (result != B_OK)
		return result;
	GetNextToken();
	if (strcasecmp(GetTokenString(), "+") != 0)
		return B_ERROR;
	GetNextToken();
	char response[512];
	hmac_challenge_response(GetTokenString(), name, password, response, 512);
	command.Clear();
	command << response << "\r\n";
	result = SendCommand(command);
	if (result != B_OK)
		return result;
	return B_OK;
}

status_t IMAP::AuthenticateLogin(const char *name, const char *password)
{
	MDB(MailDebug md);
	
	// ** Warning the auth=login is not formally described in any
	// RFC. Everyone says it's in RFC 2554, but it's not. Netscape
	// has a blurp on their web site at the following address:
	// http://help.netscape.com/kb/corporate/19980217-9.html
	StringBuffer command;
	command << NewTag() << " authenticate login\r\n";
	status_t result = SendCommand(command);
	if (result != B_OK)
		return result;
	GetNextToken();
	if (strcasecmp(GetTokenString(), "+") != 0)
		return B_ERROR;
	// Probably get something like "User" or "Username" here, we don't care
	GetNextToken();
	// Encode the username in base64
	BString encoded;
	RawToBase64Adapter::Encode(name, strlen(name), &encoded);
	command.Clear();
	command << encoded.String() << "\r\n";
	result = SendCommand(command);
	if (result != B_OK)
		return result;
	// Eat EOL token and then get server continuation token
	if (GetNextToken() == kEndOfLineToken)
		GetNextToken();
	if (strcasecmp(GetTokenString(), "+") != 0)
		return B_ERROR;
	// Get the returned base64 encoded string.
	// Probably get something like "Pass" or "Password", we don't care
	GetNextToken();
	// Encode the password in base64
	RawToBase64Adapter::Encode(password, strlen(password), &encoded);
	command.Clear();
	command << encoded.String() << "\r\n";
	result = SendCommand(command);
	if (result != B_OK)
		return result;
	return B_OK;
}

status_t IMAP::AuthenticatePlain(const char *, const char *)
{
	MDB(MailDebug md);

	// How do I implement this?
	// Should we implement this?
	
	return B_ERROR;
}

status_t IMAP::AuthenticatePlainText(const char *name, const char *password)
{
	MDB(MailDebug md);
	
	StringBuffer command;
	command << NewTag() << " login " << name << ' ' << password << "\r\n";
	status_t result = SendCommand(command);
	return result;
}

status_t IMAP::ParseResponse()
{
	MDB(MailDebug md);
	
	while (fInHeader) {
		TokenType type = GetNextToken();
		if (type == kEndOfStreamToken) {
			return B_ERROR;
		} else if (type == kEndOfLineToken) {
			continue;
		} else if (strcmp(GetTokenString(), "+") == 0) {
			// Server continuation line, it's waiting
			// for us to send it data... Just eat the
			// rest of the response
			EatRestOfLine();
			return B_OK;
		} else if (strcmp(GetTokenString(), "*") == 0) {
			// Data
			GetNextToken();
			if (strcasecmp(GetTokenString(), "ok") == 0) {
				if (ParseOk() < 0)
					return B_ERROR;
				// We need to handle the server greeting slightly
				// differently, since it could start with an untagged
				// * OK. We need to explicitly exit from this greeting
				// It'd call this a bug, wanna fix it?
				if (fInGreeting) {
					// Don't push back
					fPushBack = false;
					break;
				}
			} else if (strcasecmp(GetTokenString(), "no") == 0) {
				// Warnings, basically.
				EatRestOfLine();
			} else if (isdigit(GetTokenString()[0])) {
				PushBack();
				if (ParseNumeric() < 0)
					return B_ERROR;
			} else if (strcasecmp(GetTokenString(), "bye") == 0) {
				EatRestOfLine();
				Disconnect();
				return B_OK;
			} else if (strcasecmp(GetTokenString(), "flags") == 0) {
				uint32 flags;
				if (ParseFlags(flags) < 0)
					return B_ERROR;
			} else if (strcasecmp(GetTokenString(), "status") == 0) {
				if (ParseStatus() < 0)
					return B_ERROR;
			} else if (strcasecmp(GetTokenString(), "search") == 0) {
				if (ParseSearch() < 0)
					return B_ERROR;
			} else if (strcasecmp(GetTokenString(), "capability") == 0) {
				if (ParseCapability() < 0)
					return B_ERROR;
			} else if (strcasecmp(GetTokenString(), "quota") == 0) {
				if (ParseQuota() < 0)
					return B_ERROR;
			} else {
				DB(md.Print("IMAP is ignoring server response '%s'\n", GetTokenString()));
				EatRestOfLine();
			}
		} else {
			// Status
			GetNextToken();
			if (strcasecmp(GetTokenString(), "no") == 0 || strcasecmp(GetTokenString(), "bad") == 0) {
				EatRestOfLine();
				return B_ERROR;
			}

			EatRestOfLine();
			break;
		}
	}

	return B_OK;
}

status_t IMAP::ParseFetchList()
{
	MDB(MailDebug md);
	GetNextToken();
	if (strcmp(GetTokenString(), "(") != 0) {
		REPORT_ERROR("Expected '('");
		return B_ERROR;
	}

	while (fInHeader) {
		TokenType token = GetNextToken();
		if (token == kEndOfLineToken) {
			PushBack();
			break;
		} else if (token == kEndOfStreamToken) {
			return -1;
		} else if (strcasecmp(GetTokenString(), "bodystructure") == 0) {
			if (!fParseMessage) {
				printf("Got structure information I wasn't looking for\n");
				EatRestOfLine();
				return B_OK;
			}

			GetNextToken();
			if (strcmp(GetTokenString(), "(") != 0) {
				REPORT_ERROR("Expected '('");
				return B_ERROR;
			}
		
			MessagePart *rootPart = new MessagePart;
			StringBuffer rootPath;
			if (ParseSectionOrContainer(rootPath, rootPart) < 0)
				return B_ERROR;
			
			fParseMessage->SetRoot(rootPart);
		} else if (strcasecmp(GetTokenString(), "envelope") == 0) {
			if (ParseEnvelope() < 0)
				return B_ERROR;
		} else if (strcasecmp(GetTokenString(), "body") == 0) {
			return ParseBody();
		} else if (strcasecmp(GetTokenString(), "flags") == 0) {
			uint32 flags;
			if (ParseFlags(flags) < 0)
				return B_ERROR;
				
			if (fParseMessage)
				fParseMessage->SetFlags(flags);
		} else if (strcasecmp(GetTokenString(), "rfc822.size") == 0) {
			GetNextToken();
			if (fParseMessage)
				fParseMessage->SetSize(atoi(GetTokenString()));
		} else if (strcasecmp(GetTokenString(), "uid") == 0) {
			GetNextToken();
			if (fParseMessage)
				fParseMessage->SetUid(GetTokenString());
		} else if (strcmp(GetTokenString(), ")") == 0)
			break;
		else
			printf("Unknown fetch tag %s\n", GetTokenString());
	}

	return B_OK;
}

status_t IMAP::ParseNumeric()
{
	MDB(MailDebug md);

	GetNextToken();
	int value = atoi(GetTokenString());
	GetNextToken();
	if (strcasecmp(GetTokenString(), "fetch") == 0) {
		if (ParseFetchList() < 0)
			return B_ERROR;
	} else if (strcasecmp(GetTokenString(), "exists") == 0) {
		fTotalMessageCount = value;
	} else if (strcasecmp(GetTokenString(), "recent") == 0) {
		fRecentMessageCount = value;
	} else if (strcasecmp(GetTokenString(), "expunge") == 0) {
		printf("expunge = %d\n", value);
	} else {
		printf("Unknown numeric tag %s\n", GetTokenString());
		EatRestOfLine();
	}

	return B_OK;
}

status_t IMAP::ParseOk()
{
	MDB(MailDebug md);
	for (;;) {
		TokenType tokenType = GetNextToken();
		if (tokenType == kEndOfLineToken) {
			PushBack();
			break;
		}
		
		if (strcasecmp(GetTokenString(), "capability") == 0) {
			ParseCapability();
		} else if (strcasecmp(GetTokenString(), "uidvalidity") == 0) {
			GetNextToken();
			fCurrentFolderUid = GetTokenString();
		}
	}
			
	EatRestOfLine();
	return B_OK;
}

status_t IMAP::ParseStatus()
{
	MDB(MailDebug md);
	// This is the mailbox name that we did a status on..
	// We can just ignore it...
	GetNextToken();

	// This should be investigated. Do all servers return the list
	// inside paranthesis?
	GetNextToken();
	if (strcmp(GetTokenString(), "(") != 0)
		return B_ERROR;

	for (;;) {
		GetNextToken();
		if (strcmp(GetTokenString(), ")") == 0)
			break;
	
		if (strcasecmp(GetTokenString(), "messages") == 0) {
			GetNextToken();
			fTotalMessageCount = atoi(GetTokenString());
		} else if (strcasecmp(GetTokenString(), "recent") == 0) {
			GetNextToken();
			fRecentMessageCount = atoi(GetTokenString());
		} else if (strcasecmp(GetTokenString(), "unseen") == 0) {
			GetNextToken();
			fUnseenMessageCount = atoi(GetTokenString());
		} else
			printf("Unknown flag %s\n", GetTokenString());
	}

	return B_OK;
}

status_t IMAP::ParseSearch()
{
	// The results of a search come back in the following format:
	// S: * SEARCH n1 n2 n3 n4 n5\r\n
	// Where n1, n2... are the message numbers of the messages
	// that passed the search. (I don't think they are the uid's
	// though, which would be nice).
	
	// Parse till CRLF
	for(;;) {
		TokenType tokenType = GetNextToken();
		if ((tokenType == kEndOfLineToken) || (tokenType == kEndOfStreamToken))
			break;
		int value = atoi(GetTokenString());
		BString *messageNumber = new BString;
		*messageNumber << value;
		fSearchResultList->AddItem(messageNumber);
	}
	return B_OK;
}

// Note: The opening '(' gets eaten before this is called.
status_t IMAP::ParseSectionOrContainer(StringBuffer &path, MessagePart *part)
{
	MDB(MailDebug md);

	GetNextToken();
	if (strcmp(GetTokenString(), "(") == 0) {
		PushBack();
		return ParseContainer(path, part);
	} else {
		// Special case: a top level single part document
		StringBuffer subPath;
		if (path.Length() == 0)
			subPath << "1";
		else
			subPath << path.String();

		PushBack();
		return ParseSection(subPath, part);
	}

	return B_OK;
}

status_t IMAP::ParseContainer(StringBuffer &path, MessagePart *part)
{
	MDB(MailDebug md);

	part->isContainer = true;

	GetNextToken();
	if (strcmp(GetTokenString(), "(") != 0) {
		REPORT_ERROR("Expected '('\n");
		return B_ERROR;
	}

	// Subparts within this container.
	long subIndex = 1;
	for (;;) {
		StringBuffer subPath;
		if (path.Length())
			subPath << path.String() << '.';
			
		subPath << subIndex++;
		MessagePart *newPart = new MessagePart;
		part->subParts.AddItem(newPart);
		ParseSectionOrContainer(subPath, newPart);

		// See if another message or container follows this one.
		GetNextToken();
		if (strcmp(GetTokenString(), "(") != 0) {
			PushBack();
			break;
		}
	}

	// Get container type.
	GetNextToken();
	part->type << "multipart/";
	if (strcasecmp(GetTokenString(), "mixed") == 0) {
		part->containerType = MessagePart::kMixed;
		part->type << "mixed";
	} else if (strcasecmp(GetTokenString(), "alternative") == 0) {
		part->containerType = MessagePart::kAlternative;
		part->type << "alternative";
	} else if (strcasecmp(GetTokenString(), "related") == 0) {
		part->containerType = MessagePart::kRelated;
		part->type << "related";
	} else {
		part->containerType = MessagePart::kMixed;
		part->type << "mixed";
	}
	// Get extension data.
	for (;;) {
		GetNextToken();
		if (strcmp(GetTokenString(), ")") == 0)
			break;
		else if (strcmp(GetTokenString(), "(") == 0) {
			PushBack();
			EatParenthesizedList();
		}
	}

	return B_OK;
}

status_t IMAP::ParseSection(StringBuffer &path, MessagePart *part)
{
	MDB(MailDebug md);

	part->isContainer = false;
	part->disposition = MessagePart::kNoDisposition;
	part->id = path.String();
	// Mime type
	GetNextToken();
	if (strcmp(GetTokenString(), "(") == 0)
		GetNextToken();
		
	StringBuffer superType;
	superType << GetTokenString();
	part->type << GetTokenString();
	GetNextToken();
	part->type << "/" << GetTokenString();
	
	if (GetNextToken() != kNilToken) {
		if (strcmp(GetTokenString(), "(") != 0) {
			REPORT_ERROR("Expected '('\n");
			return B_ERROR;
		}
	
		// Name value pairs.  Pull out stuff we know.	
		for (;;) {
			GetNextToken();
			if (strcmp(GetTokenString(), ")") == 0)
				break;

			StringBuffer tmp;
			tmp << GetTokenString();
			GetNextToken();
			if (strcasecmp(tmp.String(), "charset") == 0)
				part->characterSet = GetTokenString();
			else if (strcasecmp(tmp.String(), "name") == 0)
				part->name = GetTokenString();
		}
	}

	// Basic Headers
	GetNextToken();	// Content ID
	part->contentID = GetTokenString();
	GetNextToken();	// Body description
	GetNextToken(); // Body encoding
	part->encoding = GetTokenString();
	GetNextToken(); // body size
	part->size = atoi(GetTokenString());

	MessagePart *newPart = NULL;
	
	if (part->type.ICompare("message/rfc822") == 0) {
		if (ParseEnvelope() < 0)
			return B_ERROR;

		GetNextToken();		
		if (strcmp(GetTokenString(), "(") != 0) {
			REPORT_ERROR("Expected '('\n");
			return B_ERROR;
		}
		
		StringBuffer rootPath;
		newPart = new MessagePart;
		if (ParseSectionOrContainer(rootPath, newPart) < 0) {
			delete newPart;
			return B_ERROR;
		}
	}

	for (int fieldid = 0; ; fieldid++) {
		if (fieldid == 0 &&
			strcasecmp(superType.String(), "text") != 0 &&
			part->type.ICompare("message/rfc822") != 0)
			continue;	// Skip line count for non-text.
	
		if (fieldid == 2) {
			if (ParseDisposition((newPart == NULL) ? part : newPart) < 0) {
				delete newPart;
				return B_ERROR;
			}
						
			continue;
		}

		GetNextToken();

		// Closing paren
		if (strcmp(GetTokenString(), ")") == 0)
			break;
	}
	delete newPart;

	return B_OK;
}

status_t IMAP::ParseDisposition(MessagePart *part)
{
	MDB(MailDebug md);

	if (GetNextToken() == kNilToken)
		return B_OK;
	
	if (strcmp(GetTokenString(), "(") != 0) {
		REPORT_ERROR("Expected '('");
		return B_ERROR;
	}

	GetNextToken();
	if (strcasecmp(GetTokenString(), "inline") == 0)
		part->disposition = MessagePart::kInline;
	else if (strcasecmp(GetTokenString(), "attachment") == 0)
		part->disposition = MessagePart::kAttached;
		
	for (;;) {
		GetNextToken();
		if (strcmp(GetTokenString(), "(") == 0) {
			PushBack();
			EatParenthesizedList();
		} else if (strcmp(GetTokenString(), ")") == 0)
			break;
	}

	return B_OK;
}

status_t IMAP::ParseEnvelope()
{
	MDB(MailDebug md);
	if (!fParseMessage) {
		printf("Got envelope information I wasn't looking for\n");
		EatRestOfLine();
		return B_OK;
	}

	GetNextToken();
	if (strcasecmp(GetTokenString(), "(") != 0) {
		REPORT_ERROR("Expected '('");
		return B_ERROR;
	}

	GetNextToken();
	fParseMessage->SetDate(GetTokenString());
	GetNextToken();
	fParseMessage->SetSubject(GetTokenString());	

	BString fullAddress;
	BString name;

	// From
	if (ParseAddressList(fullAddress, name) < 0)
		return B_ERROR;

	fParseMessage->AddFrom(fullAddress.String());
	fParseMessage->AddFromName(name.String());
	
	// Sender
	name = fullAddress = "";	
	if (ParseAddressList(fullAddress, name) < 0)
		return B_ERROR;

	fParseMessage->AddSender(fullAddress.String());
	fParseMessage->AddSenderName(name.String());

	// ReplyTo
	name = fullAddress = "";	
	if (ParseAddressList(fullAddress, name) < 0)
		return B_ERROR;

	fParseMessage->AddReplyTo(fullAddress.String());
	fParseMessage->AddReplyToName(name.String());

	// To
	name = fullAddress = "";	
	if (ParseAddressList(fullAddress, name) < 0)
		return B_ERROR;

	fParseMessage->AddRecipient(fullAddress.String());
	fParseMessage->AddRecipientName(name.String());

	// Cc
	name = fullAddress = "";
	if (ParseAddressList(fullAddress, name) < 0)
		return B_ERROR;

	fParseMessage->AddCc(fullAddress.String());
	fParseMessage->AddCcName(name.String());

	// Bcc, we don't care
	name = fullAddress = "";
	if (ParseAddressList(fullAddress, name) < 0)
		return B_ERROR;
	
	GetNextToken();
	fParseMessage->SetInReplyTo(GetTokenString());
	GetNextToken();
	fParseMessage->SetMessageID(GetTokenString());
	GetNextToken();	
	if (strcmp(GetTokenString(), ")") != 0) {
		REPORT_ERROR("Expected ')'");
		return B_ERROR;
	}

	return B_OK;
}

status_t IMAP::ParseAddressList(BString &fullAddress, BString &name)
{
	MDB(MailDebug md);
	
	if (GetNextToken() == kNilToken)
		return B_OK;
	
	if (strcmp(GetTokenString(), "(") != 0) {
		REPORT_ERROR("Expected '('");
		return B_ERROR;
	}
	
	for (;;) {
		GetNextToken();
		if (strcmp(GetTokenString(), ")") == 0)
			break;

		if (fullAddress.Length() > 0)
			fullAddress << "; ";

		PushBack();
		if (ParseAddress(fullAddress, name) < 0)
			return B_ERROR;
	}
		
	return B_OK;
}

status_t IMAP::ParseAddress(BString &fullAddress, BString &name)
{
	MDB(MailDebug md);

	GetNextToken();
	if (strcmp(GetTokenString(), "(") != 0) {
		REPORT_ERROR("Expected '('");
		return B_ERROR;
	}

	// If there is a name, then use it. Otherwise
	// the name will simply be the address
	if (GetNextToken() != kNilToken) {
		name << GetTokenString();
		fullAddress << "\"" << GetTokenString() << "\" <";
	}
	bool noName = (name == "");
	
	GetNextToken();								// Route, skip it
	GetNextToken();
	fullAddress << GetTokenString();			// Mailbox
	fullAddress << "@";
	if (noName)
		name << GetTokenString() << "@";
	GetNextToken();
	fullAddress << GetTokenString();			// Host
	if (!noName)
		fullAddress << ">";	
	if (noName)
		name << GetTokenString();
	

	GetNextToken();
	if (strcmp(GetTokenString(), ")") != 0) {
		REPORT_ERROR("Expected ')'");
		return B_ERROR;
	}
		
	return B_OK;
}

status_t IMAP::ParseBody()
{
	MDB(MailDebug md);

	GetNextToken();	
	if (strcmp(GetTokenString(), "[") != 0) {
		REPORT_ERROR("Expected '['");
		return B_ERROR;
	}
	
	// Skip through the list of sections that are included.
	for (;;) {
		GetNextToken();
		if (strcmp(GetTokenString(), "]") == 0)
			break;
	}

	fCurrentMessageLength = 0;
	fCurrentMessageOffset = 0;

	// Do this manually so GetNextToken doesn't interpret literal.
	int32 quoteCount = 0;
	for (;;) {
		char c;
		if (ReadSocketBuffered(&c, 1) < 0) {
			printf("Error reading\n");
			return B_ERROR;
		}
			
		if (isdigit(c)) {
			fCurrentMessageLength  = fCurrentMessageLength * 10 + c - '0';
		} else if (c == '"') {
			quoteCount++;
			if (quoteCount > 1)
				break;
		} else if (c == '\n')
			break;
	}

	if (fCurrentMessageLength > 0)
		fInHeader = false;

	return B_OK;
}

status_t IMAP::ParseFlags(uint32 &outFlags)
{
	MDB(MailDebug md);

	GetNextToken();
	if (strcmp(GetTokenString(), "(") != 0)
		return B_ERROR;

	outFlags = 0;
	for (;;) {
		GetNextToken();
		if (strcmp(GetTokenString(), ")") == 0)
			break;
	
		if (strcasecmp(GetTokenString(), "\\answered") == 0)
			outFlags |= kMessageAnswered;
		else if (strcasecmp(GetTokenString(), "\\flagged") == 0)
			outFlags |= kMessageFlagged;
		else if (strcasecmp(GetTokenString(), "\\deleted") == 0)
			outFlags |= kMessageDeleted;
		else if (strcasecmp(GetTokenString(), "\\seen") == 0)
			outFlags |= kMessageSeen;
		else if (strcasecmp(GetTokenString(), "\\draft") == 0)
			outFlags |= kMessageDraft;
		else if (strcasecmp(GetTokenString(), "\\recent") == 0)
			outFlags |= kMessageRecent;
		else
			printf("Unknown flag %s\n", GetTokenString());
	}

	return B_OK;
}

status_t IMAP::ParseCapability()
{
	MDB(MailDebug md);
	
	// Parse till CRLF or closing brace ']'
	for (;;) {
		TokenType type = GetNextToken();
		DB(md.Print("Capability token: '%s'\n", GetTokenString()));
		if ((strcmp(GetTokenString(), "]") == 0) || (type == kEndOfLineToken) || (type == kEndOfStreamToken))
			break;
		if (strcasecmp(GetTokenString(), "AUTH=PLAIN") == 0)
			fCapability |= kHasAuthPlainCapability;
		else if (strcasecmp(GetTokenString(), "AUTH=LOGIN") == 0)
			fCapability |= kHasAuthLoginCapability;
		else if (strcasecmp(GetTokenString(), "AUTH=CRAM-MD5") == 0)
			fCapability |= kHasCRAMCapability;
		else if (strcasecmp(GetTokenString(), "IMAP4") == 0)
			fCapability |= kIMAP4Capability;
		else if (strcasecmp(GetTokenString(), "IMAP4REV1") == 0)
			fCapability |= kIMAP4rev1Capability;
		else if (strcasecmp(GetTokenString(), "NAMESPACE") == 0)
			fCapability |= kNamespaceCapability;
		else if (strcasecmp(GetTokenString(), "QUOTA") == 0)
			fCapability |= kQuotaCapability;
	}
	return B_OK;
}

status_t IMAP::ParseQuota()
{
	MDB(MailDebug md);

	// Parse till CRLF
	for (;;) {
		TokenType type = GetNextToken();
		if (type == kEndOfLineToken) {
			PushBack();
			break;
		}
		// Storage tag is in the format: 'STORAGE' bytesUsed bytesAllowed
		if (strcasecmp(GetTokenString(), "STORAGE") == 0) {
			// Extract bytes used
			type = GetNextToken();
			if (type == kTextToken)
				fMailboxSize = max_c(atol(GetTokenString()), 0);
			// Extract quota allowed
			type = GetNextToken();
			if (type == kTextToken)
				fMailboxQuota = max_c(atol(GetTokenString()), 0);			
		}
	}
	return B_OK;
}

inline const char *IMAP::NewTag()
{
	StringBuffer tag;
	tag << kTagPrefix << fNextTag++; 
	return tag.String();
}

inline status_t IMAP::SendCommand(StringBuffer &request)
{
#if IMAP_TRACE
	printf("C: %s\n", request.String());
#endif

	if (WriteSocket(request.String(), request.Length()) < request.Length())
		return B_ERROR;

	return B_OK;
}

inline const char *IMAP::GetTokenString()
{
	return fToken.String();
}

IMAP::TokenType IMAP::GetNextToken()
{
	if (fPushBack) {
#if IMAP_TRACE
		printf("Got pushed back token %s\n", GetTokenString());
#endif
		fPushBack = false;
		return fTokenType;
	}

	const char *kSymbolicChars = "()[]*";
	int literalLength = 0;
	enum State {
		kScanSpace,
		kScanToken,
		kScanQuotedString,
		kScanLiteralStringLen,
		kScanLiteralString,
		kScanDone
	} state = kScanSpace;

	fToken.Clear();
	while (state != kScanDone) {
		char c;
		if (ReadSocketBuffered(&c, 1) < 0) {
			printf("read error\n");
			fTokenType = kEndOfStreamToken;
			break;
		}
		
		switch (state) {
			case kScanSpace:
				if (strchr(kSymbolicChars, c) != 0) {
					fToken << c;
					state = kScanDone;
					fTokenType = kTextToken;
				} else if (c == '"') {
 					state = kScanQuotedString;
				} else if (c == '\n') {
					fToken.Clear();
					fTokenType = kEndOfLineToken;
					state = kScanDone;
				} else if (c == '{') {
					literalLength = 0;
					state = kScanLiteralStringLen;
				} else if (!isspace(c) && c != '\r') {
					state = kScanToken;
					PushBackChar();
				}
				
				break;
			
			case kScanQuotedString:
				if (c == '"') {
					state = kScanDone;
					fTokenType = kTextToken;
				} else
					fToken << c;

				break;
			
			case kScanToken:
				if (isspace(c) || strchr(kSymbolicChars, c) != 0 || c == '"') {
					PushBackChar();
					state = kScanDone;
					if (strcasecmp(GetTokenString(), "nil") == 0) {
						fTokenType = kNilToken;
						fToken.Clear();
					} else
						fTokenType = kTextToken;
				} else
					fToken << c;
				
				break;
				
			case kScanLiteralStringLen:
				if (isdigit(c))
					literalLength = literalLength * 10 + c - '0';
				else if (c == '\n') {
					state = kScanLiteralString;
					fToken.Clear();
				}
			
				break;
				
			case kScanLiteralString:
				fToken << c;
				if (--literalLength == 0)
					state = kScanDone;
		
				break;
			
			default:
				;
		}
	}

#if IMAP_TRACE
	printf("<%s> %d\n", GetTokenString(), fTokenType);
#endif
	return fTokenType;
}

void IMAP::EatRestOfLine()
{
	MDB(MailDebug md);

	for (;;) {
		TokenType type = GetNextToken();
		if (type == kEndOfLineToken) {
			PushBack();
			break;
		} else if (type == kEndOfStreamToken)
			break;
	}
}

void IMAP::EatParenthesizedList()
{
	MDB(MailDebug md);

	int level = 0;
	for (;;) {
		if (GetNextToken() == kEndOfStreamToken)
			break;
		else if (strcmp(GetTokenString(), ")") == 0 && --level == 0)
			break;		
		else if (strcmp(GetTokenString(), "(") == 0)
			level++;
	}
}

void IMAP::PushBack()
{
	fPushBack = true;
}

void IMAP::PushBackChar()
{
	fBufferReadPos--;
}

ssize_t IMAP::ReadSocketBuffered(void *buffer, size_t count)
{
	if (fSocket < 0)
		return ENOTCONN;

	if (count == 0)
		return 0;
		
	//	If there is buffered data, copy that first
	if (fBufferReadPos < fAmountBuffered) {
		size_t sizeToCopy = MIN(count, fAmountBuffered - fBufferReadPos);
		memcpy(buffer, fBuffer + fBufferReadPos, sizeToCopy);
		fBufferReadPos += sizeToCopy;
		return sizeToCopy;
	}

	if (count < static_cast<size_t>(kLargeReadThreshold)) {
		//	This is a small read.  It would be inefficient to do a lot
		// 	of these, so read into a larger buffer and copy smaller chunks
		// 	out of that.
		ssize_t sizeReceived = ReadSocketUnbuffered(fBuffer, kLargeReadSize);
		if (sizeReceived <= 0)
			return sizeReceived;
		
		fAmountBuffered = sizeReceived;
		size_t sizeToCopy = MIN(count, fAmountBuffered);
		memcpy(buffer, fBuffer, sizeToCopy);
		fBufferReadPos = sizeToCopy;
		return sizeToCopy;
	}

	//	This is a fairly large read, so it just wastes time to
	//	copy into a temporary buffer.  Read from the socket directly into
	//	the user buffer
	return ReadSocketUnbuffered(buffer, count);
}

ssize_t IMAP::ReadSocketUnbuffered(void *buffer, size_t count)
{
	ssize_t retval = read(fSocket, buffer, count);
	if (retval < 0)
		retval = errno;
	else if (retval == 0)
		retval = -1;	// a zero byte read signifies the end of the stream

	return retval;
}

ssize_t IMAP::WriteSocket(const void *buffer, size_t count)
{
	if (fSocket < 0)
		return ENOTCONN;

	size_t sent = 0;
	while (sent < count) {
		ssize_t result = -1;
		result = write(fSocket, (char*) buffer + sent, count - sent);
		if (result <= 0)
			break;
		
		sent += result;
	}
	return sent;
}
