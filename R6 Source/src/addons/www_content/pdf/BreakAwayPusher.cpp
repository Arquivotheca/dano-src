#include "BreakAwayPusher.h"


BreakAwayPusher::BreakAwayPusher(Pusher *sink)
	: Pusher(sink)
{
}

BreakAwayPusher::~BreakAwayPusher()
{
	// disconnect our child so it doesn't get deleted
	SetSink(0);
}

