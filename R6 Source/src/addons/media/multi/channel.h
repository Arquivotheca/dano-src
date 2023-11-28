#ifndef MULTICHANNEL_H
#define MULTICHANNEL_H

#include "EndPoint.h"
#include <multi_audio.h>

class MultiChannel : public EndPoint
{
public:
	MultiChannel(int32 id, EndPoint::endpoint_type type, multi_channel_info &info, const char *name);
	~MultiChannel();
	
	void SetChannelInfo(const multi_channel_info &info) { fInfo = info; };
	void ChannelInfo(multi_channel_info *info) { *info = fInfo; };
	int32 ChannelID() { return fInfo.channel_id; };
	
	void SetConnected(bool connected = true) { fConnected = connected; };
	bool Connected() { return fConnected; };
	
	void SetObscured(int32 channel = 0) { fObscured = channel; };
	int32 Obscured() { return fObscured; };
	
private:
	multi_channel_info fInfo;
	int32 fObscured;
	bool fConnected;
};

#endif //MULTICHANNEL_H
