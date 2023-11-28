#include "TeePusher.h"


TeePusher::TeePusher(Pusher *sink, FILE *othersink)
	: Pusher(sink), fOtherSink(othersink), fCloseFile(false)
{
}

TeePusher::TeePusher(Pusher *sink, char const *name)
	: Pusher(sink), fOtherSink(0), fCloseFile(false)
{
	SetFile(name);
}

TeePusher::~TeePusher()
{
	if (fCloseFile) fclose(fOtherSink);
}

ssize_t 
TeePusher::Write(const uint8 *buffer, ssize_t length, bool finish)
{
	fwrite(buffer, length, 1, fOtherSink); fflush(fOtherSink);
	return Pusher::Write(buffer, length, finish);
}

void 
TeePusher::SetFile(FILE *newsink)
{
	fOtherSink = newsink;
}

void 
TeePusher::SetFile(char const *name)
{
	if (fCloseFile)
	{
		fclose(fOtherSink);
		fCloseFile = false;
	}
	fOtherSink = fopen(name, "w");
	if (fOtherSink) fCloseFile = true;
}

