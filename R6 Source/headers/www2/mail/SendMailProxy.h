/*
	SendMailProxy.h
*/
#ifndef _SENDMAIL_PROXY_H
#define _SENDMAIL_PROXY_H
#include "IMAP.h"
#include "SMTP.h"

namespace Wagner {
// Forward Declarations
class SendMessageContainer;

class SendMailProxy {
	public:
		static SendMailProxy	&Newman();
								~SendMailProxy();

		status_t				SendMessage(SendMessageContainer *container);
		status_t				OpenImapSmtpConnections(IMAP &imap, SmtpDaemon &smtp, SendMessageContainer *container);
		BDataIO	*				CreateAdapterChain(SendMessageContainer *container);

	private:
								// There is only one send mail proxy
								SendMailProxy();
								
		static	status_t		DeliveryManEntry(void *arg);
		status_t				DeliveryMan();
		status_t				DeliverTheMessage(SendMessageContainer *container);

		enum callback_action {
			kNoAction = 0,
			kConfirmSend,
			kUnableToSend,
			kInvalidAttachment,
			kNoSpace
		};
		void					DoJavascriptCallback(callback_action action);
		status_t				VerifyAttachmentsExist(SendMessageContainer *container, BString &attachmentName);
		
		bool fKeepDelivering;
		BString fInvalidAttachment;
		sem_id fSignalSem;
		sem_id fQueueSem;
		thread_id fQueueThread;
		BList fQueuedMessages;
};
}
#endif
