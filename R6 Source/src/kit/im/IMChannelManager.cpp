/*
	IMChannelManager.cpp
*/
#include "IMChannelManager.h"

IMChannelManager::IMChannelManager()
	:	BinderNode()
{

}


IMChannelManager::~IMChannelManager()
{

}

put_status_t IMChannelManager::WriteProperty(const char *name, const property &prop)
{
	return BinderNode::WriteProperty(name, prop);
}

get_status_t IMChannelManager::ReadProperty(const char *name, property &prop, const property_list &args)
{
	return BinderNode::ReadProperty(name, prop);
}


// End of IMChannelManager.cpp

