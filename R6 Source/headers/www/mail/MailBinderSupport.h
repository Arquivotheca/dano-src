/*
	MailBinderSupport.h
*/
#ifndef MAIL_BINDER_SUPPORT_H
#define MAIL_BINDER_SUPPORT_H
#include <Binder.h>

namespace Wagner {

class MailboxNodeContainer : public BinderContainer {
	public:
								MailboxNodeContainer();
		virtual					~MailboxNodeContainer();
								// Binder virtuals
		virtual get_status_t	ReadProperty(const char *name, property &outProperty, const property_list &inArgs = empty_arg_list);
};

class AccountStatusNode : public BinderContainer {
	public:
								AccountStatusNode();
		virtual					~AccountStatusNode();
								// Binder virtuals
		virtual put_status_t	WriteProperty(const char *name, const property &inProperty);
		virtual get_status_t	ReadProperty(const char *name, property &outProperty, const property_list &inArgs = empty_arg_list);
};

class SendingStatusNode : public BinderContainer {
	public:
								SendingStatusNode();
		virtual					~SendingStatusNode();
								// Binder virtuals
		virtual get_status_t	ReadProperty(const char *name, property &outProperty, const property_list &inArgs = empty_arg_list);
};

class SendingNodeProxy {
	public:
								SendingNodeProxy(const char *username, uint32 status = 0);
								~SendingNodeProxy();
		
		void					SetStatus(uint32 status);
		void					SetTotalBytes(int32 totalBytes);
		void					UpdateBytesSent(int32 bytes);
		void					Reset();
			
	private:
								// Do not implement
								SendingNodeProxy();
		SendingNodeProxy &		operator=(const SendingNodeProxy &rhs);
		
		BinderNode::property fNode;
};

}
#endif
