/*
	IMChannel.cpp
*/
#include "IMChannel.h"

IMChannel::IMChannel()
	:	BinderNode()
{

}


IMChannel::~IMChannel()
{

}

put_status_t IMChannel::WriteProperty(const char *name, const property &prop)
{
	return BinderNode::WriteProperty(name, prop);
}

get_status_t IMChannel::ReadProperty(const char *name, property &prop, const property_list &args)
{
	return BinderNode::ReadProperty(name, prop);
}


// End of IMChannel.cpp

