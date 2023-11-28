#include "MediaItem.h"
#include <MediaRoster.h>

MediaItem::MediaItem()
	: IconItem(),
	  mInfo(),
	  mNode(media_node::null),
	  mView(NULL),
	  mIsLive(false)
{
}

MediaItem::~MediaItem()
{
	if (mNode != media_node::null)
		BMediaRoster::Roster()->ReleaseNode(mNode);
}

void 
MediaItem::SetDormantInfo(dormant_node_info &info)
{
	mInfo = info;
	SetName(info.name);
}

void 
MediaItem::SetNode(const media_node &node)
{
	mNode = node;
	if (mNode == media_node::null)
		mIsLive = false;
	else
		mIsLive = true;
}

