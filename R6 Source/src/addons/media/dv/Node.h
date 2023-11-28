#ifndef _DV_MEDIA_EVENT_LOOPER_H
#define _DV_MEDIA_EVENT_LOOPER_H

#include <media/MediaEventLooper.h>

class DVMediaEventLooper :
	public virtual BMediaEventLooper
{
public:
					DVMediaEventLooper() : BMediaNode(""), BMediaEventLooper() { }
virtual				~DVMediaEventLooper() { }

virtual	status_t	InitCheck() const = 0;
virtual	status_t	SetTo(int32 bus, uint64 guid, bool PAL) = 0;
virtual	void		NotifyPresence(bool presence) = 0;
};

#endif
