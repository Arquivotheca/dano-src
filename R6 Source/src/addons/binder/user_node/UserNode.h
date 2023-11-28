
#ifndef _USERNODE_H_
#define _USERNODE_H_

#include <Binder.h>
#include <xml/BXMLBinderNode.h>

using namespace B::XML;

class UserNode : public XMLBinderNode
{
		BString m_name;

	public:

								UserNode(const BString &name, const BDirectory &dir);
	
				void			SetName(const char *name);
				
		virtual	put_status_t	WriteProperty(const char *name, const property &prop);
		virtual	get_status_t	ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);
};

#endif

