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

class MailBinderNode : public BinderContainer {
	public:
								MailBinderNode();
		virtual					~MailBinderNode();
								// Binder virtuals
		virtual put_status_t	WriteProperty(const char *name, const property &inProperty);
		virtual get_status_t	ReadProperty(const char *name, property &outProperty, const property_list &inArgs = empty_arg_list);
};

}
#endif
