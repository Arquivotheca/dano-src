/*	$Id: Dx86StackFrame.h,v 1.3 1999/01/05 22:09:24 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 05/14/98 14:45:06
*/

#ifndef DX86STACKFRAME_H
#define DX86STACKFRAME_H

#include "DStackFrame.h"

class Dx86StackFrame : public DStackFrame
{
  public:
	Dx86StackFrame(DStackCrawl& sc, ptr_t fp, ptr_t pc);
	~Dx86StackFrame();
			
	virtual void GetRegister(uint32 reg, uint32& value) const;
	virtual void GetFPURegister(uint32 reg, void *value) const;

  private:
  	
  	void CalcSpillInfo();
  	
	mutable ptr_t fSpillInfo[8];		// store maximum of 8 registers.
};

#endif
