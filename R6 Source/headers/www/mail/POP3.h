
#ifndef _POP3__H_
#define _POP3__H_

#include <www/mail/MailServer.h>
#include <www/mail/PopHash.h>

class POP3Parse;
struct uid_map{
	BString	uid;
	uint32	mailNo;
};

class POP3 : public MailServer {
	public:
								POP3();
		virtual					~POP3();

		virtual	status_t		Connect(const char *server, int port);
		virtual void			Disconnect();
		virtual	status_t		Login(const char *name, const char *password, uint32 authType = kBestType);
		virtual	status_t		Logout();

		virtual	status_t		FetchSection(const char *uid, const char *section);
		virtual status_t		GetMessageInfo(const char *uid, MimeMessage *letter);
		virtual	status_t		GetMessageUidList(BList *list);
		virtual	int32			GetTotalMessageCount() const;

		virtual	ssize_t 		Read(void *buffer, size_t size);
		virtual	ssize_t 		Write(const void *buffer, size_t size);

		status_t				StatMailbox();

	private:
		status_t				AuthenticateAPOP(const char * name, const char * password);
		status_t				AuthenticatePlain(const char * name, const char * password);
		
		enum ResponseType {
			kSimple,
			kOptionalArg,
			kSingleArg,
			kDoubleArg,
			kMessage,
			kSection
		};

		status_t				ParseNextResponseLine(ResponseType type, bool firstLine);
		status_t				GetMultilineMessage(POP3Parse *parser);
		
		enum {
			kUnconnected	= -1,
			kAuthorization = 0,
			kTransaction = 1,
			kUpdate = 2
		}				fState;

		StringBuffer	fArgs[2];
		PopHash			fUIDHash;
#define ResetArgs() fArgs[0].Clear(); fArgs[1].Clear();
};

#endif


