
#ifndef _GROUPNAMESPACE_H_
#define _GROUPNAMESPACE_H_

#include <Binder.h>
#include <URL.h>
#include <SecurityManager.h>

class GroupNamespace : public BinderContainer
{
	public:

		virtual	status_t				OpenProperties(void **cookie, void *copyCookie);
		virtual	status_t				NextProperty(void *cookie, char *nameBuf, int32 *len);
		virtual	status_t				CloseProperties(void *cookie);

		virtual	put_status_t			WriteProperty(const char *name, const property &prop);
		virtual	get_status_t			ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);

		virtual	status_t				WillRewritten(binder_node disgruntledAncestor, uint32 event, const char *name);

	private:

		Gehnaphore						m_lock;
		BString							m_relURL;
		Wagner::URL						m_base;
		Wagner::GroupID					m_securityGroup;
};

#endif
