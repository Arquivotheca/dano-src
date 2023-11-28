#include "NetSwitchboardNode.h"

#include <String.h>

#include <Binder.h>

#define streq(s1,s2) (strcmp(s1,s2) == 0)
#define strcaseeq(s1,s2) (strcasecmp(s1,s2) == 0)

NetSwitchboardNode::NetSwitchboardNode()
	: BinderNode()
{
}


NetSwitchboardNode::~NetSwitchboardNode()
{
}

put_status_t 
NetSwitchboardNode::WriteProperty(const char * /*name*/, const property &/*prop*/)
{
	return EPERM;
}

get_status_t 
NetSwitchboardNode::ReadProperty(const char *name, property &prop, const property_list &args)
{
	get_status_t read_error = B_OK;
	
	if (streq(name, "Connect")) {
		prop = b_Open(args[0].String(), args[1].Number());
	} else if (streq(name, "Bind")) {
		prop = b_Listen(args[0].Number());
	} else {
		read_error = ENOENT;
	}

	return read_error;
}

BinderNode::property 
NetSwitchboardNode::b_Open(const BString &host, const int32 port)
{
	NetEndpointNode *node = new NetEndpointNode();
	
	property_list args;
	
	property host_prop(host.String());
	property port_prop(port);
	args.AddItem(&host_prop);
	args.AddItem(&port_prop);

	property open_error;
	get_status_t discard = node->GetProperty("Open", open_error, args);

	(void)discard;
	
	if (streq(open_error.String().String(), "ok")) {
		return property(node);
	} else {
		delete node;
		return open_error;
	}
}

BinderNode::property 
NetSwitchboardNode::b_Listen(const int32 port)
{
	NetServerNode *node = new NetServerNode();
	
	property_list args;
	
	property port_prop(port);
	args.AddItem(&port_prop);

	property open_error;
	get_status_t discard = node->GetProperty("Listen", open_error, args);
	(void)discard;
	
	if (streq(open_error.String().String(), "ok")) {
		return property(node);
	} else {
		delete node;
		return open_error;
	}
}



//-----------------------------------------------------------------------

extern "C" _EXPORT BinderNode *return_binder_node()
{
	return new NetSwitchboardNode();
}
