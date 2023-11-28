/*	$Id: DStackCrawl.h,v 1.3 1999/01/11 22:38:17 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 05/04/98 19:35:13
*/

#ifndef DSTACKCRAWL_H
#define DSTACKCRAWL_H

class DThread;
class DSymWorld;
class DCpuState;

#include "DStackFrame.h"
typedef std::vector<DStackFrame*> frame_list;

class DStackCrawl {
public:
	DStackCrawl(DThread& thread);
	DStackCrawl(const std::vector<ptr_t>& pcs, const DSymWorld& symWorld);
	
	DStackFrame& GetNthStackFrame(int indx);
	DStackFrame& GetFrameFromFramePtr(ptr_t framePtr);
	DStackFrame& GetCurrentFrame()						{ return *fFrames.back(); };
	uint32 CountFrames() const							{ return fFrames.size(); };
	uint32 DeepestUserCode();
	
	void Update(DCpuState& cpu);
	
	DStackFrame& operator[](int indx)					{ return GetNthStackFrame(indx); };
	
	frame_list& Frames()						{ return fFrames; };
	DThread& GetThread()								{ return *fThread; };
	const DThread& GetThread() const					{ return *fThread; };
	const DSymWorld& GetSymWorld()				{ return fSymWorld; }
	
private:
	DThread *fThread;
	const DSymWorld& fSymWorld;
	frame_list fFrames;
};

inline DStackFrame& DStackCrawl::GetNthStackFrame(int indx)
{
	if (indx < 0 || indx >= (int) fFrames.size())
		THROW(("Stackframe index out of range"));
	return *fFrames[indx];
} /* DStackCrawl::GetNthStackFrame */

#endif
