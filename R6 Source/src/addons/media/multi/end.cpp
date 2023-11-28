#include "EndPoint.cpp"
#include <multi_audio.h>

class MultiEndPoint : public EndPoint
{
public:
	MultiEndPoint(int32 id, EndPoint::endpoint_type type, multi_channel_info &info, const char *name = NULL)
		: EndPoint(id, type, name){ fInfo = info; };
	~MultiEndPoint(){ };
	
	void SetChannelInfo(const multi_channel_info &info) { fInfo = info; };
	void ChannelInfo(multi_channel_info *info) { *info = fInfo; };
	int32 ChannelID() { return fInfo.channel_id; };

private:
	multi_channel_info fInfo;
};