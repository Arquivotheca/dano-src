/*	$Id: DStackFrame.cpp,v 1.6 1999/05/03 13:09:53 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 05/14/98 14:38:21
*/

#include "bdb.h"
#include "DStackFrame.h"
#include "DStackCrawl.h"
#include "DThread.h"
#include "DTeam.h"
#include "DSymWorld.h"
#include "DFunction.h"

DStackFrameClient::DStackFrameClient(DStackFrame& frame)
	: fFrame(frame)
{
	fFrame.fClients.push_back(this);
	fOutOfScope = false;
} // DStackFrameClient::DStackFrameClient

DStackFrameClient::~DStackFrameClient()
{
	if (! fOutOfScope)
	{
		list<DStackFrameClient*>::iterator ci = find(fFrame.fClients.begin(), fFrame.fClients.end(), this);
		if (ci != fFrame.fClients.end())
			fFrame.fClients.erase(ci);
	}
} // DStackFrameClient::~DStackFrameClient

void DStackFrameClient::FrameOutOfScope()
{
	fOutOfScope = true;
} // DStackFrameClient::FrameOutOfScope

const DStackFrame& DStackFrameClient::StackFrame()
{
	if (fOutOfScope)
		THROW(("Frame is out of scope"));
	
	return fFrame;
} // DStackFrameClient::StackFrame

//#pragma mark -

DStackFrame::DStackFrame(DStackCrawl& sc, ptr_t fp, ptr_t pc)
	: fStackCrawl(sc)
{
	fPC = pc;
	fFP = fp;
	fStoredInfoValid = false;
} /* DStackFrame::DStackFrame */

DStackFrame::~DStackFrame()
{
	for (list<DStackFrameClient*>::iterator ci = fClients.begin(); ci != fClients.end(); ci++)
		(*ci)->FrameOutOfScope();
} /* DStackFrame::DStackFrame */

bool DStackFrame::GetStatement(DStatement& statement)
{
	if (fStatement.fLine <= 0)
		CalcStatement();
	
	if (fStatement.fLine > 0)
	{
		statement = fStatement;
		return true;
	}
	else
		return false;
} /* DStackFrame::GetStatement */

void DStackFrame::GetLocals(vector<DVariable*>& vars) const
{
	fStackCrawl.GetSymWorld().GetLocals(vars, fPC);
} /* DStackFrame::GetLocals */

DNub& DStackFrame::GetNub() const
{
	return fStackCrawl.GetThread().GetTeam().GetNub();
} // DStackFrame::GetNub

bool DStackFrame::operator== (const DStackFrame& sf)
{
	return fPC == sf.fPC && fFP == sf.fFP;
} // DStackFrame::operator==

void DStackFrame::CalcStatement()
{
	if (!fStackCrawl.GetSymWorld().GetStatement(fPC, fStatement))
		fStatement.Reset();
} // DStackFrame::CalcStatement

DVariable* DStackFrame::GetVariable(const char *name) const
{
	return fStackCrawl.GetSymWorld().GetVariable(name, fPC);
} // DStackFrame::GetVariable

void DStackFrame::CalcFunctionName()
{
	const DSymWorld& sw = fStackCrawl.GetSymWorld();
	
	sw.GetFunctionName(fPC, fFunctionName);
} /* DStackFrame::CalcFunctionName */

void DStackFrame::GetRegister(uint32 /*reg*/, uint32& /*value*/) const
{
	ASSERT(false);
} // DStackFrame::GetRegister

void DStackFrame::GetFPURegister(uint32 /*reg*/, void* /*value*/) const
{
	ASSERT(false);
} // DStackFrame::GetFPURegister
