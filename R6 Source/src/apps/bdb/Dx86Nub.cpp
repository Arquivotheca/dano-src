/*	$Id: Dx86Nub.cpp,v 1.5 1999/02/03 08:30:06 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 05/14/98 14:12:07

	Revised:  now a mixin class for handling stack crawls on x86
*/

#include "bdb.h"
#include "Dx86Nub.h"
#include "DStackFrame.h"
#include "DStackCrawl.h"
#include "Dx86StackFrame.h"
#include "DCpuState.h"

#include <algorithm>

Dx86Nub::Dx86Nub()
{
} /* Dx86Nub::Dx86Nub() */

void Dx86Nub::GetStackCrawl(DCpuState& frame, DStackCrawl& sc)
{
	enum InstructionState
	{
		kNormal,
		kPrologBegin,
		kPrologMiddle,
		kReturn
	};

	if (! IsLocked())
		THROW(("Nub not locked!"));
	
	ptr_t pc = frame.GetPC();
	ptr_t sp = frame.GetSP();
	ptr_t fp = frame.GetFP();

	frame_list& sf = sc.Frames(), nsf;
	
	try
	{
		ptr_t ebp;
		ptr_t nebp;
		ptr_t ret;
		
		ebp = fp;
		ret = pc;
		InstructionState state = kNormal;
			
		// If we are sitting at (or in) a function prolog
		// ...pretend we are past it
		// This will give us a valid stack frame and variable view
		// The function prolog =
		//	pushl   %ebp 		(55)
		//	movl    %esp, %ebp	(e589)
		// We recognize the function prolog even if we are at the movl instruction
		// (Of course this doesn't help us in optimized functions that
		// don't set up ebp at all.)
		// Notice that we have to know the difference of being at the 
		// beginning or in the middle of the prolog because the stack is different
		// It might be tempting to set up ebp here as we decide where we are,
		// but we need ebp for the second time through the loop

		if (pc)
		{
			unsigned char code[3];
			this->ReadData(pc, code, 3);
			
			if ((code[0] | 0x0009) == 0x00cb)
			{
				// c2, c3, ca and cb are all return instructions...
				state = kReturn;
			}
			else if (code[0] == 0x55 && code[1] == 0x89 && code[2] == 0xe5)
			{
				// we are sitting at start of function prolog
				state = kPrologBegin;
			}
			else if (code[0] == 0x89 && code[1] == 0xe5)
			{
				// check if we are in the middle of the function prolog
				this->ReadData(pc-1, code, 1);
				if (code[0] == 0x55) 
				{
					state = kPrologMiddle;
				}
			}
		}
		else
		{
			// this case is when the PC is null, i.e. we just called through a NULL
			// function pointer.  we can't read the code at address 0x0, so we just
			// pretend that we're pointing at the beginning of a function prologue,
			// since the stack state is equivalent.
			state = kPrologBegin;
		}

		do
		{
			ptr_t current_ebp = ebp;
			if (state == kPrologBegin)
			{
				// simulate the stack being pushed and then new location
				// being saved in ebp
				current_ebp = sp - 4;
			}
			else if (state == kPrologMiddle)
			{
				// simulate the stack has already been pushed
				// so ebp will be sp
				current_ebp = sp;
			}
			nsf.push_back(new Dx86StackFrame(sc, current_ebp, ret));
			
			if (state == kNormal)
			{
				Read(ebp, nebp);
				Read(ebp + 4, ret);
			}
			else
			{
				// At the prolog beginning or at the return, sp
				// contains our return address.  In the middle
				// of the prolog, sp+4 contains return address
				Read((state == kPrologMiddle) ? sp+4 : sp, ret);
				nebp = ebp;
				state = kNormal;
			}

			if (ret < 0x80000000 || ret >= 0xfc000000)
				break;
			
			ebp = nebp;
		}
		while (ebp != 0);
	}
	catch (HErr& e)
	{
		e.DoError();
	}
	
	reverse(nsf.begin(), nsf.end());
	
	frame_list::iterator oi, ni, i;
	
	oi = sf.begin();
	ni = nsf.begin();
	
	while (oi != sf.end() && ni != nsf.end() && *(*oi) == *(*ni))
	{
		oi++;
		ni++;
	}
	
	for (i = oi; i != sf.end(); i++)
		delete *i;
	
	sf.erase(oi, sf.end());
	sf.insert(sf.end(), ni, nsf.end());
} /* Dx86Nub::GetStackCrawl */
