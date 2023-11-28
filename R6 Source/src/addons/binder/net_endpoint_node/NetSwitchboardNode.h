#ifndef NETSWITCHBOARDNODE_H
#define NETSWITCHBOARDNODE_H

#include "NetEndpointNode.h"
#include "NetServerNode.h"

class NetSwitchboardNode : public BinderNode {
public:
							NetSwitchboardNode();
							~NetSwitchboardNode();

	virtual	put_status_t	WriteProperty(const char *name, const property &prop);
	virtual	get_status_t	ReadProperty(const char *name, property &prop, const property_list &args = empty_arg_list);

protected:
	virtual property		b_Open(const BString &host, const int32 port);
	virtual property		b_Listen(const int32 port);

};

#endif

