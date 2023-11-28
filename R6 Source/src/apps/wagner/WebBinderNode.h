
#ifndef WEB_NODE_H
#define WEB_NODE_H

#include <Binder.h>

class WebBinderNode : public BinderNode
{
	public:
											WebBinderNode();
		virtual								~WebBinderNode();

	protected:

		virtual	status_t					OpenProperties(void **cookie, void *copyCookie);
		virtual	status_t					NextProperty(void *cookie, char *nameBuf, int32 *len);
		virtual	status_t					CloseProperties(void *cookie);
		virtual	put_status_t				WriteProperty(const char *name, const property &prop);
		virtual	get_status_t				ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);
		virtual	status_t					HandleMessage(BMessage *msg);
};

#endif
