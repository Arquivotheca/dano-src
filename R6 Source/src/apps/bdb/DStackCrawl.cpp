/*	$Id: DStackCrawl.cpp,v 1.2 1998/11/17 12:16:41 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 05/14/98 10:37:32
*/

#include "bdb.h"
#include "DStackCrawl.h"
#include "DTeam.h"
#include "DThread.h"
#include "DNub.h"
#include "DCpuState.h"

DStackCrawl::DStackCrawl(DThread& thread)
	: fThread(&thread), fSymWorld(thread.GetTeam().GetSymWorld())
{
} /* DStackCrawl::DStackCrawl */

DStackCrawl::DStackCrawl(const std::vector<ptr_t>& pcs, const DSymWorld& symWorld)
	: fSymWorld(symWorld)
{
	fThread = NULL;
	for (std::vector<ptr_t>::const_iterator i = pcs.begin(); i != pcs.end(); i++)
	{
		DStackFrame *frame;
		fFrames.push_back(frame = new DStackFrame(*this, (ptr_t)0, *i));
		frame->CalcFunctionName();
	}
} /* DStackCrawl::DStackCrawl */

DStackFrame&  DStackCrawl::GetFrameFromFramePtr(ptr_t /*framePtr*/)
{
	THROW(("Unimplemented"));
} /* DStackCrawl::GetFrameFromFramePtr */

void  DStackCrawl::Update(DCpuState& cpu)
{
	for (frame_list::iterator sfi = fFrames.begin(); sfi != fFrames.end(); sfi++)
		delete (*sfi);
	fFrames.clear();

	FailNilMsg(fThread, "Not connected to a running thread");
	DNub& nub = fThread->GetTeam().GetNub();

	BAutolock lock(nub);

	if (lock.IsLocked())
		nub.GetStackCrawl(cpu, *this);
} /* DStackCrawl::Update */

uint32 DStackCrawl::DeepestUserCode()
{
	// since we now do assembly, the deepest user code is the last frame
	// (previously it was the last frame that had a DStatement)
	
	uint32 size = fFrames.size();
	return size > 0 ? size - 1 : 0;
	
} /* DStackCrawl::DeepestUserCode */
