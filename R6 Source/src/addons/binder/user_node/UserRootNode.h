
#ifndef _USERROOTNODE_H_
#define _USERROOTNODE_H_

#include <Binder.h>
#include "AssociativeArray.h"

#define BINDER_USER_ROOT "/boot/binder/user"

class UserRootNode : public BinderNode
{
		binder_node								m_defaultUser;
		binder_node								m_currentUserLink;
		AssociativeArray<BString,binder_node>	m_activeUserNodes;

	public:

		UserRootNode();

		virtual	status_t						OpenProperties(void **cookie, void *copyCookie);
		virtual	status_t						NextProperty(void *cookie, char *nameBuf, int32 *len);
		virtual	status_t						CloseProperties(void *cookie);
	
		virtual	put_status_t					WriteProperty(const char *name, const property &prop);
		virtual	get_status_t					ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);
};

#endif
