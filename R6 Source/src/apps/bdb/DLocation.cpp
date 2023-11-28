/*	$Id: DLocation.cpp,v 1.8 1999/03/09 09:32:28 maarten Exp $
	
	Copyright Hekkelman Programmatuur
	Maarten L. Hekkelman
*/

#include "bdb.h"
#include <stack>
#include "DLocation.h"
#include "DStackFrame.h"
#include "DDwarf2.h"
#include "DNub.h"
#include <ByteOrder.h>

using namespace dwarf;

locType LocationMachine (const DLocationString& loc, const DStackFrame *frame, DNub& nub, ptr_t& addr, ptr_t base = 0);

locType LocationMachine (const DLocationString& loc, const DStackFrame *frame, DNub& nub, ptr_t& addr, ptr_t base)
{
	DLocationString::const_iterator i = loc.begin();

	if (i == loc.end())
		return ltError;

	stack<int32> locStack;
	uchar opcode = (uchar)*i++;

	if (opcode >= DW_OP_reg0 && opcode <= DW_OP_reg31)
	{
		addr = opcode - DW_OP_reg0;
		return ltRegister;
	}
	
	if (opcode == DW_OP_regx)
	{
		ReadULEB128(i, addr);
		return ltRegister;
	}
	
	locStack.push(base);

	try
	{
		while (true)
		{
			switch (opcode)
			{
				case DW_OP_addr:
				{
					ptr_t r;
					Read(i, r);
					locStack.push(r);
					break;
				}
	
				case DW_OP_deref:
				{
					BAutolock lock(nub);

					long& p = locStack.top();
					nub.Read((ptr_t)p, p);
					break;
				}
	
				case DW_OP_const1u:
				{
					uint8 x;
					Read(i, x);
					locStack.push(x);
					break;
				}
	
				case DW_OP_const1s:
				{
					int8 x;
					Read(i, x);
					locStack.push(x);
					break;
				}
	
				case DW_OP_const2u:
				{
					uint16 x;
					Read(i, x);
					locStack.push(x);
					break;
				}
	
				case DW_OP_const2s:
				{
					int16 x;
					Read(i, x);
					locStack.push(x);
					break;
				}
	
				case DW_OP_const4u:
			    {
			    	uint32 x;
			    	Read(i, x);
			    	locStack.push(x);
					break;
				}
	
				case DW_OP_const4s:
			    {
			    	int32 x;
			    	Read(i, x);
			    	locStack.push(x);
					break;
				}
	
				case DW_OP_const8u:
			    {
			    	uint64 x;
			    	Read(i, x);
			    	locStack.push(x);
					break;
				}
	
				case DW_OP_const8s:
			    {
			    	int64 x;
			    	Read(i, x);
			    	locStack.push(x);
					break;
				}
	
				case DW_OP_constu:
			    {
			    	uint32 x;
			    	ReadULEB128(i, x);
			    	locStack.push(x);
					break;
				}
	
				case DW_OP_consts:
			    {
			    	int32 x;
			    	ReadLEB128(i, x);
			    	locStack.push(x);
					break;
				}
	
				case DW_OP_dup:
					locStack.push(locStack.top());
					break;
	
				case DW_OP_drop:
					locStack.pop();
					break;
	
				case DW_OP_over:
				{
					int32 t1, t2;
					t1 = locStack.top(); locStack.pop();
					t2 = locStack.top();
					locStack.push(t1);
					locStack.push(t2);
					break;
				}
	
				case DW_OP_pick:
				{
					stack<int32> cpy = locStack;
					uint8 index;
					Read(i, index);
					
					while (index-- && cpy.size())
						cpy.pop();
	
					if (cpy.size())
						locStack.push(cpy.top());
					break;
				}
	
				case DW_OP_swap:
				{
					int32 t1, t2;
					t1 = locStack.top(); locStack.pop();
					t2 = locStack.top(); locStack.pop();
					locStack.push(t1);
					locStack.push(t2);
					break;
				}
	
				case DW_OP_rot:
				{
					int32 t1, t2, t3;
					t1 = locStack.top(); locStack.pop();
					t2 = locStack.top(); locStack.pop();
					t3 = locStack.top(); locStack.pop();
					locStack.push(t1);
					locStack.push(t3);
					locStack.push(t2);
					break;
				}
	
				case DW_OP_xderef:
				{
					THROW(("Unsupported location opcode"));
				}
	
				case DW_OP_abs:
					// not aware of any needed operation???
					break;
	
				case DW_OP_and:
				{
					uint32 a, b;
					a = locStack.top(); locStack.pop();
					b = locStack.top(); locStack.pop();
					locStack.push(a & b);
					break;
				}
	
				case DW_OP_div:
				{
					uint32 a, b;
					a = locStack.top(); locStack.pop();
					b = locStack.top(); locStack.pop();
					locStack.push(b / a);
					break;
				}
	
				case DW_OP_minus:
				{
					uint32 a, b;
					a = locStack.top(); locStack.pop();
					b = locStack.top(); locStack.pop();
					locStack.push(b - a);
					break;
				}
	
				case DW_OP_mod:
				{
					uint32 a, b;
					a = locStack.top(); locStack.pop();
					b = locStack.top(); locStack.pop();
					locStack.push(b % a);
					break;
				}
	
				case DW_OP_mul:
				{
					uint32 a, b;
					a = locStack.top(); locStack.pop();
					b = locStack.top(); locStack.pop();
					locStack.push(b * a);
					break;
				}
	
				case DW_OP_neg:
					locStack.top() = -locStack.top();
					break;
	
				case DW_OP_not:
					locStack.top() = ~locStack.top();
					break;
	
				case DW_OP_or:
				{
					uint32 a, b;
					a = locStack.top(); locStack.pop();
					b = locStack.top(); locStack.pop();
					locStack.push(b | a);
					break;
				}
	
				case DW_OP_plus:
				{
					uint32 a, b;
					a = locStack.top(); locStack.pop();
					b = locStack.top(); locStack.pop();
					locStack.push(b + a);
					break;
				}
	
				case DW_OP_plus_uconst:
				{
					uint32 c;
					ReadULEB128(i, c);
					locStack.top() += c;
					break;
				}
	
				case DW_OP_shl:
				{
					uint32 a, b;
					a = locStack.top(); locStack.pop();
					b = locStack.top(); locStack.pop();
					locStack.push(b << a);
					break;
				}
	
				case DW_OP_shr:
				{
					int32 a, b;
					a = locStack.top(); locStack.pop();
					b = locStack.top(); locStack.pop();
					locStack.push(b >> a);
					break;
				}
	
				case DW_OP_shra:
				{
					uint32 a, b;
					a = locStack.top(); locStack.pop();
					b = locStack.top(); locStack.pop();
					locStack.push(b >> a);
					break;
				}
	
				case DW_OP_xor:
				{
					uint32 a, b;
					a = locStack.top(); locStack.pop();
					b = locStack.top(); locStack.pop();
					locStack.push(b ^ a);
					break;
				}
	
				case DW_OP_bra:
				{
					int16 d;
					Read(i, d);
					if (locStack.top() != 0)
						i += d;
					locStack.pop();
					break;
				}
	
				case DW_OP_eq:
				{
					int32 a, b;
					a = locStack.top(); locStack.pop();
					b = locStack.top(); locStack.pop();
					locStack.push(b == a);
					break;
				}
	
				case DW_OP_ge:
				{
					int32 a, b;
					a = locStack.top(); locStack.pop();
					b = locStack.top(); locStack.pop();
					locStack.push(b >= a);
					break;
				}
	
				case DW_OP_gt:
				{
					int32 a, b;
					a = locStack.top(); locStack.pop();
					b = locStack.top(); locStack.pop();
					locStack.push(b > a);
					break;
				}
	
				case DW_OP_le:
				{
					int32 a, b;
					a = locStack.top(); locStack.pop();
					b = locStack.top(); locStack.pop();
					locStack.push(b <= a);
					break;
				}
	
				case DW_OP_lt:
				{
					int32 a, b;
					a = locStack.top(); locStack.pop();
					b = locStack.top(); locStack.pop();
					locStack.push(b < a);
					break;
				}
	
				case DW_OP_ne:
				{
					int32 a, b;
					a = locStack.top(); locStack.pop();
					b = locStack.top(); locStack.pop();
					locStack.push(b != a);
					break;
				}
	
				case DW_OP_skip:
				{
					int16 d;
					Read(i, d);
					i += d;
					break;
				}
	
				case DW_OP_lit0:
				case DW_OP_lit1:
				case DW_OP_lit2:
				case DW_OP_lit3:
				case DW_OP_lit4:
				case DW_OP_lit5:
				case DW_OP_lit6:
				case DW_OP_lit7:
				case DW_OP_lit8:
				case DW_OP_lit9:
				case DW_OP_lit10:
				case DW_OP_lit11:
				case DW_OP_lit12:
				case DW_OP_lit13:
				case DW_OP_lit14:
				case DW_OP_lit15:
				case DW_OP_lit16:
				case DW_OP_lit17:
				case DW_OP_lit18:
				case DW_OP_lit19:
				case DW_OP_lit20:
				case DW_OP_lit21:
				case DW_OP_lit22:
				case DW_OP_lit23:
				case DW_OP_lit24:
				case DW_OP_lit25:
				case DW_OP_lit26:
				case DW_OP_lit27:
				case DW_OP_lit28:
				case DW_OP_lit29:
				case DW_OP_lit30:
				case DW_OP_lit31:
					locStack.push(opcode - DW_OP_lit0);
					break;
	
				case DW_OP_breg0:
				case DW_OP_breg1:
				case DW_OP_breg2:
				case DW_OP_breg3:
				case DW_OP_breg4:
				case DW_OP_breg5:
				case DW_OP_breg6:
				case DW_OP_breg7:
				case DW_OP_breg8:
				case DW_OP_breg9:
				case DW_OP_breg10:
				case DW_OP_breg11:
				case DW_OP_breg12:
				case DW_OP_breg13:
				case DW_OP_breg14:
				case DW_OP_breg15:
				case DW_OP_breg16:
				case DW_OP_breg17:
				case DW_OP_breg18:
				case DW_OP_breg19:
				case DW_OP_breg20:
				case DW_OP_breg21:
				case DW_OP_breg22:
				case DW_OP_breg23:
				case DW_OP_breg24:
				case DW_OP_breg25:
				case DW_OP_breg26:
				case DW_OP_breg27:
				case DW_OP_breg28:
				case DW_OP_breg29:
				case DW_OP_breg30:
				case DW_OP_breg31:
				{
					uint32 reg;
					int32 offset;
					
					FailNilMsg(frame, "This variable needs a stack context");
					
					frame->GetRegister(opcode - DW_OP_breg0, reg);
					ReadLEB128(i, offset);
					locStack.push(reg + offset);
					break;
				}
	
				case DW_OP_fbreg:
				{
					int32 offset;
					ReadLEB128(i, offset);
					
					FailNilMsg(frame, "This variable needs a stack context");

					uint32 addr = base == 0 ? frame->GetFP() : base;

					if (offset < 0)
						addr -= -offset;
					else
						addr += offset;

					locStack.push(addr);
					break;
				}
	
				case DW_OP_bregx:
				{
					uint32 reg;
					int32 offset;
					ReadLEB128(i, offset);
					ReadULEB128(i, reg);
					
					FailNilMsg(frame, "This variable needs a stack context");
					
					frame->GetRegister(reg, reg);
					locStack.push(reg + offset);
					break;
				}
	
				case DW_OP_piece:
					THROW(("Unsupported opcode in location"));
					break;
	
				case DW_OP_deref_size:
				{
					ptr_t p = locStack.top();
					locStack.pop();
					
					uint32 l;
					
					BAutolock lock(nub);
					
					switch (*i++)
					{
	#if __LITTLE_ENDIAN
						case 1:	nub.Read(p, l); l &= 0x000000ff; break;
						case 2:	nub.Read(p, l); l &= 0x0000ffff; break;
						case 3:	nub.Read(p, l); l &= 0x00ffffff; break;
						case 4:	nub.Read(p, l); break;
	#else
	#	error
	#endif
					}
					
					locStack.push(l);
					break;
				}
	
				case DW_OP_xderef_size:
					THROW(("Unsupported opcode in location"));
					break;
	
				case DW_OP_nop:
					break;
	
			}
	
			if (i == loc.end())
				break;

			opcode = (uchar)*i++;
		}
	}
	catch (HErr& e)
	{
		e.DoError();
		return ltError;
	}
	
	addr = locStack.top();
	return ltMemory;
} // LocationMachine

locType LocationMachine (const DLocationString& loc, const DStackFrame& frame, ptr_t& addr, ptr_t base)
{
	return LocationMachine (loc, &frame, frame.GetNub(), addr, base);
} // LocationMachine

ptr_t LocationMachine (const DLocationString& loc, DNub& nub, ptr_t base)
{
	ptr_t addr;
	LocationMachine (loc, NULL, nub, addr, base);
	return addr;
} // LocationMachine
