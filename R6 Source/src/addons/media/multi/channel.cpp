#include "channel.h"

MultiChannel::MultiChannel(int32 id, EndPoint::endpoint_type type, multi_channel_info &info, const char *name)
	: EndPoint(id, type, name),
	fInfo(info),
	fObscured(0),
	fConnected(false)
{
}


MultiChannel::~MultiChannel()
{
}

