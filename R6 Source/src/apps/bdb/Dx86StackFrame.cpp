/*	$Id: Dx86StackFrame.cpp,v 1.4 1999/01/05 22:09:23 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
	
	Created: 05/14/98 14:49:52
*/

#include "bdb.h"
#include "Dx86StackFrame.h"
#include "DCpuState.h"
#include "DSymWorld.h"
#include "DThread.h"
#include "DTeam.h"
#include "DFunction.h"
#include "DNub.h"

Dx86StackFrame::Dx86StackFrame(DStackCrawl& sc, ptr_t fp, ptr_t pc)
	: DStackFrame(sc, fp, pc)
{
	CalcFunctionName();
} /* Dx86StackFrame::Dx86StackFrame */

Dx86StackFrame::~Dx86StackFrame()
{
} /* Dx86StackFrame::~Dx86StackFrame */

void Dx86StackFrame::GetRegister(uint32 reg, uint32& value) const
{
	if (reg > 7)
		THROW(("ERROR: attempt to get invalid register %lu\n", reg));

	if (!fStoredInfoValid)
		const_cast<Dx86StackFrame *>(this)->CalcSpillInfo();
			// OK to cast away constness here, modified members are mutable
			// (probably would be cleaner to have CalcSpillInfo be const and
			// called CalcSpillInfoIfNeeded)
	
	if (fSpillInfo[reg] == 0)
	{
		Dx86CpuState* x86state = dynamic_cast<Dx86CpuState*>(&(fStackCrawl.GetThread().GetCPU()));
		if (!x86state) THROW(("Attempt to get x86 registers from non-x86 target thread!"));
		x86_cpu_state cpu = x86state->GetNativeState();

		switch (reg)
		{
			case 0:	value = cpu.eax; break;
			case 1:	value = cpu.ecx; break;
			case 2:	value = cpu.edx; break;
			case 3:	value = cpu.ebx; break;
			case 4:	value = cpu.uesp; break;
			case 5:	value = cpu.ebp; break;
			case 6:	value = cpu.esi; break;
			case 7:	value = cpu.edi; break;
		}
	}
	else if (fSpillInfo[reg] == kInvalidSpillInfo)
	{
		static const char* const sRegNames[] = { "eax", "ecx", "edx", "ebx", "uesp", "ebp", "esi", "edi" };
		THROW(("Stack frame failure getting register %s", sRegNames[reg]));
	}
	else
	{
		DNub& nub = fStackCrawl.GetThread().GetTeam().GetNub();
		
		BAutolock lock(nub);
		nub.Read(fSpillInfo[reg], value);
	}
} /* Dx86StackFrame::GetRegister */

void Dx86StackFrame::GetFPURegister(uint32 /*reg*/, void* /*value*/) const
{
} /* Dx86StackFrame::GetFPURegister */

void Dx86StackFrame::CalcSpillInfo()
{
	if (fStoredInfoValid)
		return;

	static uint8 DEBUGGER_SYSCALL_IMPLEMENTATION[] = {
		0xb8, 0x37, 0x00, 0x00, 0x00,		// movl	$0x00000037, %eax
		0xcd, 0x25,									// int		$0x25
		0xc3												// retn
	};

	if (this == fStackCrawl.Frames().back())
	{
		for (int i = 0; i < 8; i++)
			fSpillInfo[i] = 0;
	}
	else
	{
		try
		{
			frame_list::iterator fi = find(fStackCrawl.Frames().begin(), fStackCrawl.Frames().end(), this);
			Dx86StackFrame *next = dynamic_cast<Dx86StackFrame*>(*(fi + 1));
			
				// copy the spill info from the next stack frame
			next->CalcSpillInfo();
			memcpy(fSpillInfo, next->fSpillInfo, 8 * sizeof(ptr_t));
		
				// and now we're going to dissassemble the next procedure
				// to find out where it stored our registers
				
			ptr_t pc = fStackCrawl.GetThread().GetTeam().GetSymWorld().GetFunctionLowPC(next->fPC);
			ptr_t sp = next->fFP;
			
			DNub& nub = fStackCrawl.GetThread().GetTeam().GetNub();
			BAutolock lock(nub);
			
			int state = 1;
			unsigned char code[10];
			
			while (state)
			{
				if (pc >= next->fPC)
				{
					if (state < 10)
						state = 99;
					else
						break;
				}

				switch (state)
				{
						// For now we assume all the procedures have this layout:
						//
						//		55			push	%ebp
						//		89 e5		mov	%esp,%ebp
						//		83 ec ??	sub	$0x..,%esp		optional
						//		??				push	%any			this is what we're looking for
						//
						
						// If prologues can have a different structure this state
						// machine has to be revised

						// EXCEPTION:  the _debugger syscall is the following:
						//
						//		b8 37 00 00 00	movl    $0x00000038, %eax
						//		cd 25					int     $0x25
						//		c3						retn
						//					<-- this is where "pc" points
						//
						// In this case, there is no local frame, so we have to drop back up one
						// level to the debugger() call implementation that called _debugger in
						// order to find an ebp prologue.

						// Construct below is a state machine walking through the 
						// prologue of the procedure

						// start with reading the first byte of the procedure.
					case 1:
						nub.Read(pc, code[0]);
						if (code[0] == 0x055)
						{
							pc++;
							state = 2;
						}
						else
						{
							nub.ReadData(pc - 8, &code, 8);		// read enough to verify that this is, in fact, _debugger
							if (!memcmp(&code, DEBUGGER_SYSCALL_IMPLEMENTATION, 8))
							{
								// aha!  we're in _debugger, so we know how to back up to debugger():
								// (sp-12) is the return address, points to (debugger() + 0x19)
								uint32 newpc;
								nub.Read(sp-12, newpc);
								// now read backwards from newpc until we find 0x55, i.e. the start of the debugger() prologue
								do
								{
									newpc--;
									nub.Read(newpc, code[0]);
								} while (code[0] != 0x55);
								// okay, we're there - now adjust pc & pick up the normal state machine operation here
								pc = newpc;
								// sp is actually the parent frame's sp already, so we don't need to fix it up
							}
							else state = 99;
						}
						break;
					
					case 2:
						nub.ReadData(pc, code, 2);
						pc += 2;
						if (code[0] == 0x089 && code[1] == 0x0e5)
							state = 3;
						else
							state = 99;
						break;
					
					case 3:
						nub.ReadData(pc, code, 2);
						if (code[0] == 0x081 && code[1] == 0x0ec)
						{
							state = 10;
							pc += 2;
						}
						else if (code[0] == 0x083 && code[1] == 0x0ec)
						{
							state = 15;
							pc += 2;
						}
						else
							state = 20;
						break;
					
					case 10:
					{
						int32 offset;
						nub.Read(pc, offset);
						pc += sizeof(offset);
						sp += offset;
						state = 20;
						break;
					}
					
					case 15:
					{
						int8 offset;
						nub.Read(pc, offset);
						pc += sizeof(offset);
						sp -= offset;
						state = 20;
						break;
					}
					
					case 20:
					{
						nub.Read(pc++, code[0]);
						if (code[0] >= 0x050 && code[0] <= 0x057)
						{
							sp -= 4;
							fSpillInfo[code[0] - 0x050] = sp;
						}
						else
							state = 0;
						break;
					}

						// error state
					case 99:
						for (int i = 0; i < 8; i++)
							fSpillInfo[i] = kInvalidSpillInfo;
						state = 0;
						break;
				}
			}
			
			fStoredInfoValid = true;
		}
		catch (HErr& e)
		{
			e.DoError();
		}
	}
} // Dx86StackFrame::CalcSpillInfo
