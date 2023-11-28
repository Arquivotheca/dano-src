
#include <www/mail/POP3.h>

#include <www/mail/md5.h>
#include <www/mail/MailDebug.h>
#include <www/mail/POP3Parse.h>

#include <stdlib.h>
#include <OS.h>

const char * POP3_OK = "+OK";
const char * POP3_ERR = "-ERR";

const char * CRLF = "\r\n";

POP3::POP3() :
	fState(kUnconnected)
{
}


POP3::~POP3()
{
	Disconnect();
}

status_t 
POP3::Connect(const char *server, int port)
{
	MDB(MailDebug md);
	if (fState != kUnconnected || fSocket != -1 || !server) {
		printf("Connect: already connected? fState: \n");
		return EALREADY;
	}
	// be paranoid - clear the argument buffers
	ResetArgs();
	DB(md.Print("connect: %s %d\n", server, port));
	status_t status = MailServer::Connect(server, port);
	if (status == B_OK) {
		// the network connection was successful
		// check the reply from the server
		// and get the timestamp if APOP is supported
		status = ParseNextResponseLine(kOptionalArg, true);
		if (status == 1) {
			// see if the argument matches what we want
//			printf("fArgs[0]: %s\n", fArgs[0].String());
			if ((fArgs[0].String())[0] != '<')
				fArgs[0].Clear();
			status = B_OK;
		}
		
		if (status == B_OK) {
			fState = kAuthorization;
		}
		else
			status = B_ERROR;
	}
	return status;
}

void 
POP3::Disconnect()
{
	MailServer::Disconnect();
}

status_t 
POP3::Login(const char *name, const char *password, uint32 authType)
{
	MDB(MailDebug md);
	if (fState != kAuthorization)
		return B_NOT_ALLOWED;
	
	if (!name || !password)
		return B_BAD_VALUE;
	
	status_t status = B_OK;
	if (authType == kAuthPlainText)
		status = AuthenticatePlain(name, password);
	else
		status = AuthenticateAPOP(name, password); 
	// clear out any arguments
	ResetArgs();
	DB(md.Print("status: %ld\n", status));
	return status;
}

status_t 
POP3::AuthenticateAPOP(const char *name, const char *password)
{
	MDB(MailDebug md);
	if (fArgs[0].Length() == 0 || (fArgs[0].String())[0] != '<') {
		printf("invalid apop message!: %s\n", fArgs[0].String());
		fArgs[0].Clear();
		return B_BAD_VALUE;
	}
	
	StringBuffer toDigest;
	toDigest << fArgs[0].String() << password;
	DB(md.Print("toDigest: %s\n", toDigest.String()));
	uint8 digest[16];
	md5_buffer(toDigest.String(), toDigest.Length(), digest);

	toDigest.Clear();
	for (int ix = 0; ix < 16; ix++) {
		char tmp[8];
		sprintf(tmp, "%02x", digest[ix]);
		toDigest << tmp;
	}

	StringBuffer apop;
	apop << "APOP " << name << " " << toDigest.String() << CRLF;
	DB(md.Print("apop: %s\n", apop.String()));

	ResetArgs();
	status_t status = SendCommand(apop);
	if (status == B_OK) {
		status = ParseNextResponseLine(kSimple, true);
		if (status == B_OK) {
			fLoggedIn = true;
			fState = kTransaction;
		}
	}
	printf("AuthenticateAPOP: %ssuccessful\n", (status == B_OK) ? "" : "un");
	return status;
}

status_t 
POP3::AuthenticatePlain(const char *name, const char *password)
{
	MDB(MailDebug md);
	StringBuffer request;
	request << "USER " << name << CRLF;
	DB(md.Print("sending: %s\n", request.String()));
	ResetArgs();
	status_t status = SendCommand(request);
	if (status == B_OK) {
		// check the response
		status = ParseNextResponseLine(kSimple, true);
		if (status == B_OK) {
			request.Clear();
			request << "PASS " << password << CRLF;
			DB(md.Print("sending: %s\n", request.String()));
			ResetArgs();
			status = SendCommand(request);
			if (status == B_OK) {
				status = ParseNextResponseLine(kSimple, true);
				if (status == B_OK) {
					fLoggedIn = true;
					fState = kTransaction;
				}
			}
		}
	}
	DB(md.Print("AuthenticatePlain: %ssucessful\n", (status == B_OK) ? "" : "un"));
	return status;
}

status_t 
POP3::Logout()
{
	MDB(MailDebug md);
	if (fState == kUnconnected)
		return B_ERROR;
		
	// issue a QUIT command
	StringBuffer quit;
	quit << "QUIT\r\n";
	ResetArgs();
	status_t status = SendCommand(quit);
	status = ParseNextResponseLine(kSimple, true);
	fState = kUnconnected;
	fTotalMessageCount = 0;
	return status;
}

status_t 
POP3::FetchSection(const char *uid, const char */*section*/)
{
	MDB(MailDebug md);
	if (!fUIDHash.IsValid()) {
		if (GetMessageUidList(NULL) != B_OK)
			return B_ERROR;
		
	}
	int32 idx = fUIDHash.Lookup(uid);
	if (idx == -1)
		return B_BAD_VALUE;
		
	StringBuffer request;
	request << "RETR " << idx << CRLF;
	status_t status = SendCommand(request);
	if (status == B_OK) {
		status = ParseNextResponseLine(kSimple, true);
		if (status == B_OK) {
			
		}
	}
	return status;
}

status_t 
POP3::GetMessageInfo(const char *uid, MimeMessage *letter)
{
	MDB(MailDebug md);
	DB(md.Print("uid: %s\n", uid));
	// only retrieve and printup listnum info
	if (!fUIDHash.IsValid()) {
		if (GetMessageUidList(NULL) != B_OK)
			return B_ERROR;
	}
	int32 idx = fUIDHash.Lookup(uid);
	if (idx == -1) {
		DB(md.Print("couldn't find: %s in list!\n", uid));
		return B_ERROR;
	}
	else
		DB(md.Print("found %s at %ld\n", uid, idx));

	StringBuffer request;

	request << "LIST " << idx << CRLF;
	DB(md.Print("sending: %s\n", request.String()));
	status_t status = SendCommand(request);
	if (status == B_OK) {
		status = ParseNextResponseLine(kDoubleArg, true);
		if (status == B_OK) {
			int32 size = atol(fArgs[1].String());
			DB(md.Print("message %ld size: %ld\n", idx, size));
			request.Clear();
			if (size < 0x8000) {
				// get the whole stinking message
				request << "RETR " << idx << CRLF;
			}
			else {
				request << "TOP " << idx << " 0" << CRLF;
			}
			
			DB(md.Print("sending: %s\n", request.String()));
			status = SendCommand(request);
			if (status == B_OK) {
				status = ParseNextResponseLine(kSimple, true);
				if (status == B_OK) {
					POP3Parse parser(letter);
					status = GetMultilineMessage(&parser);
					if (status == B_OK) {
						letter->SetMailbox(GetCurrentFolder());
						letter->SetSize(size);
						letter->SetUid(uid);
						parser.Finish();
					}
						
				}
			}
		}	
	}
	return status;
}

status_t 
POP3::GetMessageUidList(BList *list)
{
	MDB(MailDebug md);
	if (fState != kTransaction)
		return B_ERROR;

	// resize fUIDHash to the correct size
	status_t status = B_OK;
	status = fUIDHash.ResizeTable(fTotalMessageCount);
	DB(md.Print("resize table returns: %ld\n", status));
	if (status != B_OK)
		return B_ERROR;
		
	StringBuffer uidl;
	uidl << "UIDL\r\n";
	ResetArgs();
	status = SendCommand(uidl);
	if (status == B_OK) {
		// parse the response line
		status = ParseNextResponseLine(kSimple, true);
		if (status == B_OK) {
			while (ParseNextResponseLine(kDoubleArg, false) == B_OK) {
				BString * string = fUIDHash.Insert(fArgs[1].String(),atol(fArgs[0].String()));
				if (string && list)
					list->AddItem(string);
			}
		}
	}
	ResetArgs();

//	if (status == B_OK)
//		fUIDHash.PrintToStream(stdout);

	return status;
}

int32 
POP3::GetTotalMessageCount() const
{
	return fTotalMessageCount;	
}

ssize_t 
POP3::Read(void *buffer, size_t size)
{
	(void) buffer; (void) size;
	return -1;
}

ssize_t 
POP3::Write(const void *, size_t)
{
	return B_NOT_ALLOWED;
}

status_t 
POP3::StatMailbox()
{
	MDB(MailDebug md);

	if (fState != kTransaction) {
		DB(md.Print("Attempting to stat mailbox when not in transaction state! state: %d\n", fState));
		return B_ERROR;
	}
	StringBuffer statMail;
	statMail << "STAT\n";
	ResetArgs();
	status_t status = SendCommand(statMail);
	if (status == B_OK) {
		status = ParseNextResponseLine(kDoubleArg, true);
		if (status == B_OK) {
			int32 count = atol(fArgs[0].String());
			int32 size = atol(fArgs[1].String());
			fTotalMessageCount = count;
			fMailboxSize = size;
		}
		else {
			DB(md.Print("ParseNextResponseLine returns: %ld\n", status));
		}
	}
	return status;
}

status_t 
POP3::ParseNextResponseLine(ResponseType type, bool firstLine)
{
	status_t status = B_OK;
	bool crlfFound = false;
	int32 argsFound = 0;
	ResetArgs();
	
	if (firstLine) {
		fBufferReadPos = fAmountBuffered = 0;
	}
	
	while (!crlfFound) {
		// get more data if necessary
		if (fBufferReadPos >= fAmountBuffered) {
			status = ReadSocketUnbuffered(fBuffer, kLargeReadSize);
			fBufferReadPos = 0;
			if (status <= 0) {
				fAmountBuffered = 0;
				return B_ERROR;
			}

			fAmountBuffered = status;
//			printf("read: %ld\n %.*s\n", fAmountBuffered, fAmountBuffered, fBuffer);
		}

		// check for first line info
		if (firstLine) {
//			printf("parse first line\n");
			if (fBuffer[0] == '+') {
				status = B_OK; fBufferReadPos = 4;
//				printf("status good! pos: %ld/%ld\n", fBufferReadPos, fAmountBuffered);
			}
			else if (fBuffer[0] == '-') {
				status = B_ERROR; fBufferReadPos = 5;
//				printf("status bad! pos: %ld/%ld\n", fBufferReadPos, fAmountBuffered);
			}
			else {
				// something exceedingly bad happened and we got garbage
				// throw everything away
				status = B_ERROR; fBufferReadPos = fAmountBuffered = 0;
//				printf("status very bad! pos: %ld/%ld\n", fBufferReadPos, fAmountBuffered);
			}

			if (status == B_ERROR)
				return status;
				
			firstLine = false;
		}
		
		// walk through the amount we have buffered
		for (; fBufferReadPos < fAmountBuffered && !crlfFound; fBufferReadPos++) {
			bool addToArg = true;
			bool incArgs = false;
			switch(fBuffer[fBufferReadPos]) {
				case ' ':
					// if we are reading message info we want the spaces
					if (type != kMessage) {
						addToArg = false;
						incArgs = true;
					}
					break;
					
				case '\r':
					if (type != kMessage) {
						fBufferReadPos++;
						addToArg = false;
						crlfFound = true;
						incArgs = true;
					}
					break;

				case '\n':
					if (type != kMessage)
						addToArg = false;
					crlfFound = true;
					incArgs = true;;
					break;
					
				default:
					addToArg = true;
			}
			if (addToArg && argsFound < 2)
				fArgs[argsFound] << fBuffer[fBufferReadPos];
			if (incArgs)
				argsFound++;
		}
	}
	
	switch(type) {
		case kOptionalArg:
			status = (argsFound >= 1) ? 1 : 0;
			break;
		case kSingleArg:
			status = (argsFound >= 1) ? B_OK : B_ERROR;
			break;
		case kDoubleArg:
			status = (argsFound >= 2) ? B_OK : B_ERROR;
			break;
			
		case kMessage:
		case kSection:
			status = (argsFound == 1) ? B_OK : B_ERROR;
			break;
		
		case kSimple:
			break;
			
		default:
			printf("hit default in switch!!!!\n");
			status = B_ERROR;
	}
	
//	printf("ParseNextResponseLine returning: %d  %d/%d argsFound: %d\n", status, fBufferReadPos, fAmountBuffered, argsFound);
//	for (int ix = 0; ix < argsFound && ix < 2; ix++)
//		printf("fArgs[%d]: %s\n", ix, fArgs[ix].String());
	return status;

}

status_t 
POP3::GetMultilineMessage(POP3Parse *parser)
{
	MDB(MailDebug md);
	DB(md.Print("fAmountBuffered: %ld fBufferReadPos: %ld\n", fAmountBuffered, fBufferReadPos));
	bool termFound = false;
	int32 lineCount = 0;
	status_t status = B_OK;
	ResetArgs();
	while (!termFound && fAmountBuffered < kLargeReadSize) {
		status = ParseNextResponseLine(kMessage, false);
		if (status < B_OK)
			break;
		int32 len = fArgs[0].Length();
		const char *arg = fArgs[0].String();
		if (len == 3 && arg[0] == '.' && arg[1] == '\r' && arg[2] == '\n') {
			termFound = true;
		}
		else if (len == 4 && arg[0] == '.' && arg[1] == '.' && arg[2] == '\r' && arg[3] == '\n') {
			// byte stuffed!
			len--;
			arg = &arg[1];
		}
		if (!termFound) {
			// write the line!
			parser->Write(arg, len);
		}
		lineCount++;
	}
	return status;
}


