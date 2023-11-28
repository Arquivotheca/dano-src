#include "OldScreenSaver.h"
#include <Window.h>
#include <View.h>
#include <StringView.h>

BScreenSaver::BScreenSaver(BMessage *, image_id)
 : ticksize(50000), looponcount(0), loopoffcount(0)
{
}

BScreenSaver::~BScreenSaver()
{
}

status_t BScreenSaver::InitCheck()
{
	return B_OK;
}

status_t BScreenSaver::StartSaver(BView *, bool )
{
	return B_OK;
}

void BScreenSaver::StopSaver()
{
}

void BScreenSaver::Draw(BView *, int32 )
{
}

void BScreenSaver::DirectConnected(direct_buffer_info *)
{
}

void BScreenSaver::DirectDraw(int32 )
{
}


void BScreenSaver::StartConfig(BView *)
{
}

void BScreenSaver::StopConfig()
{
}

void BScreenSaver::SupplyInfo(BMessage *) const
{
}

void BScreenSaver::ModulesChanged(const BMessage *)
{
}

status_t BScreenSaver::SaveState(BMessage *) const
{
	return B_ERROR;	// no settings
}

void BScreenSaver::SetTickSize(bigtime_t ts)
{
	ticksize = ts;
}

bigtime_t BScreenSaver::TickSize() const
{
	return ticksize;
}

void BScreenSaver::SetLoop(int32 on_count, int32 off_count)
{
	looponcount = on_count;
	loopoffcount = off_count;
}

int32 BScreenSaver::LoopOnCount() const
{
	return looponcount;
}

int32 BScreenSaver::LoopOffCount() const
{
	return loopoffcount;
}

void BScreenSaver::ReservedScreenSaver1()
{
}

void BScreenSaver::ReservedScreenSaver2()
{
}

void BScreenSaver::ReservedScreenSaver3()
{
}

void BScreenSaver::ReservedScreenSaver4()
{
}

void BScreenSaver::ReservedScreenSaver5()
{
}

void BScreenSaver::ReservedScreenSaver6()
{
}

void BScreenSaver::ReservedScreenSaver7()
{
}

void BScreenSaver::ReservedScreenSaver8()
{
}
