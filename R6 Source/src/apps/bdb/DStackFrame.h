/*	$Id: DStackFrame.h,v 1.7 1999/05/03 13:09:54 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 05/12/98 13:46:45
*/

#ifndef DSTACKFRAME_H
#define DSTACKFRAME_H

#include "DVariable.h"
#include "DStatement.h"

#include <list>

class DStackCrawl;

const ptr_t kInvalidSpillInfo = ~0L;

	// Mixin class for classes that depend on a current stackframe
class DStackFrameClient
{
  public:
	DStackFrameClient(DStackFrame& frame);
	virtual ~DStackFrameClient();
	
	virtual void FrameOutOfScope();
	const DStackFrame& StackFrame();
	
  private:
	DStackFrame& fFrame;
	bool fOutOfScope;
};

class DStackFrame
{
	friend class DStackFrameClient;
  public:
	DStackFrame(DStackCrawl& sc, ptr_t fp, ptr_t pc);
	virtual ~DStackFrame();
			
	virtual ptr_t GetPC() const						{ return fPC; };
	virtual ptr_t GetFP() const						{ return fFP; };
	DStackCrawl& GetStackCrawl() const			{ return fStackCrawl; };
	DNub& GetNub() const;
	
	virtual void GetRegister(uint32 reg, uint32& value) const; // all of these Get calls should be const
	//virtual void SetRegister(uint32 reg, uint32 value);

	virtual void GetFPURegister(uint32 reg, void *value) const;
	//virtual void SetFPURegister(uint32 reg, const void *value);

	virtual void GetLocals(std::vector<DVariable*>& vars) const;
	void GetFunctionName(string& name)		{ name = fFunctionName; };
	bool GetStatement(DStatement& statement);
	
	DVariable* GetVariable(const char *name) const;

	virtual void CalcFunctionName();
	void ResetStatement() { fStatement.Reset(); }
	
	bool operator== (const DStackFrame& sf);
		
  protected:

	virtual void CalcStatement();

	DStackCrawl& fStackCrawl;
	ptr_t fPC, fFP;
	
	string fFunctionName;
	
	DStatement fStatement;
	mutable bool fStoredInfoValid;
	std::list<DStackFrameClient*> fClients;
};

#endif
