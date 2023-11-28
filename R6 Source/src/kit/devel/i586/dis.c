#include <assert.h>
#include <stdio.h>

#include <support/SupportDefs.h>

#include <disasm.h>
#include <disasm_p.h>

#include "dis_p.h"
#include "x86tables.h"

#ifdef DEBUG_X86_DIS
	#include <support/Debug.h>
	#undef zprintf
	#define zprintf _sPrintf
#elif defined(KOUTPUT)
	#define zprintf kprintf
#else
	int zprintf(const char *s, ...) {
		return (*s == 0);
	}
#endif

#define TOUCH(x) ((void)(x))

#define IS16OP		(state->flags & B_DISASM_FLAG_OP_SIZE_16)
#define IS16ADDR	(state->flags & B_DISASM_FLAG_ADDR_SIZE_16)
#define ISINTEL		(state->flags & B_DISASM_FLAG_INTEL_STYLE)

typedef struct {
	char *s;
	uint32 cur_size;
	uint32 max_size;
} outbuffer_t;

typedef struct {
	/* information about the current instruction */
	int			instr_size;
	instr_t		instr;
	operand_t	ops[3];
	uchar		mod_rm, sib;
	uint32		displacement;
	uchar		*immediate;

	char		symbol[32]; /* found by lookup */
	int32		symbol_offset;
	uint32		symbol_address; /* if went through GOT */

	/* information about the output buffer */
	outbuffer_t	out;

	uint32		insize;
	uint32		flags;
	uint32		eip;
	status_t	(*lookup)(void *cookie, uint32 eip, uint32 *sym_addr,
						char *sym_name, int max_name_len, int is_lower);
	void		*cookie;
} state_t;

static void
out_char(state_t *state, char c)
{
	if (state->out.cur_size + 1 >= state->out.max_size)
		return;

	state->out.s[state->out.cur_size++] = c;
}

static void
out_string(state_t *state, const char *s)
{
	int remaining = state->out.max_size - state->out.cur_size - 1;
	
	while ((remaining > 0) && (*s))
		state->out.s[state->out.cur_size++] = *(s++);
}

static void
out_register(state_t *state, const char *reg)
{
	if (!ISINTEL)
		out_char(state, '%');

	out_string(state, reg);
}

static const char hex[] = "0123456789abcdef";

static void
out_uint8(state_t *state, uint8 i)
{
	out_char(state, '0');
	if (!ISINTEL) out_char(state, 'x');

	out_char(state, hex[(i >>  4) & 15]);
	out_char(state, hex[(i      ) & 15]);

	if (ISINTEL)
		out_char(state, 'h');
}

static void
out_uint16(state_t *state, uint16 i)
{
	out_char(state, '0');
	if (!ISINTEL) out_char(state, 'x');

	out_char(state, hex[(i >> 12) & 15]);
	out_char(state, hex[(i >>  8) & 15]);
	out_char(state, hex[(i >>  4) & 15]);
	out_char(state, hex[(i      ) & 15]);

	if (ISINTEL)
		out_char(state, 'h');
}

static void
out_uint32(state_t *state, uint32 i)
{
	out_char(state, '0');
	if (!ISINTEL) out_char(state, 'x');

	out_char(state, hex[(i >> 28)     ]);
	out_char(state, hex[(i >> 24) & 15]);
	out_char(state, hex[(i >> 20) & 15]);
	out_char(state, hex[(i >> 16) & 15]);
	out_char(state, hex[(i >> 12) & 15]);
	out_char(state, hex[(i >>  8) & 15]);
	out_char(state, hex[(i >>  4) & 15]);
	out_char(state, hex[(i      ) & 15]);

	if (ISINTEL)
		out_char(state, 'h');
}

#define LEADING_PLUS 1

static void
out_int8(state_t *state, int8 i, uint32 flags)
{
	if (i < 0) {
		out_char(state, '-');
		i = -i;
	} else if (flags & LEADING_PLUS)
		out_char(state, '+');
	out_uint8(state,i);
}

static void
out_int16(state_t *state, int16 i, uint32 flags)
{
	if (i < 0) {
		out_char(state, '-');
		i = -i;
	} else if (flags & LEADING_PLUS)
		out_char(state, '+');
	out_uint16(state,i);
}

static void
out_int32(state_t *state, int32 i, uint32 flags)
{
	if (i < 0) {
		out_char(state, '-');
		i = -i;
	} else if (flags & LEADING_PLUS)
		out_char(state, '+');
	out_uint32(state,i);
}

static void
dump_op_addr(uchar addr)
{
	const char op_addrs[] = "xACDEFGIJMOPQRSTXY";

	if (addr == op_addr_reg)
		zprintf("reg");
	else if ((addr == 0) || (addr >= sizeof(op_addrs)))
		zprintf("unknown");
	else
		zprintf("%c", op_addrs[addr]);
}

static void
dump_op_type(uchar type)
{
	const char op_types[] = "abcdpoqsvw";
	
	if (type >= sizeof(op_types))
		zprintf("unknown");
	zprintf("%c", op_types[type]);
}

static status_t
output_register(state_t *state, operand_t op, uint32 reg)
{
	const char **string_table;

/*	assert(reg < 8);*/
	if (reg > 7) return EINVAL;

	if (op.addr == op_addr_S) {
		string_table = regseg;
	} else if (op.addr == op_addr_C) {
		string_table = regc;
	} else if (op.addr == op_addr_D) {
		string_table = regd;
	} else if ((op.addr == op_addr_P) || (op.addr == op_addr_Q) || (op.addr == op_addr_Qmmx)) {
		if (op.type == op_type_o)
			string_table = regxmm;
		else
			string_table = regmmx;
	} else if (op.addr == op_addr_D) {
		string_table = regt;
	} else {
		switch (op.type) {
			case op_type_b : string_table = reg8; break;
			case op_type_c : string_table = (IS16OP) ? reg8 : reg16; break;
			case op_type_d : string_table = reg32; break;
			case op_type_v : string_table = (IS16OP) ? reg16 : reg32; break;
			case op_type_w : string_table = reg16; break;
			case op_type_sreg : string_table = regseg; break;
			case op_type_fpreg : string_table = regfp; break;
			default :
				zprintf("Invalid operand type for register: %x\n", op.type);
				return EINVAL;
		}
	}

	if (string_table == regseg) {
/*		assert(reg < 6);*/
		if (reg > 5) return EINVAL;
	}

	out_register(state, string_table[reg]);

	return B_OK;
}

static void
lookup_symbol(state_t *state, uint32 addr)
{
	uint32 symaddr, val;
	void *cookie = state->cookie;
	struct disasm_cookie *d = (struct disasm_cookie *)state->cookie;

	state->symbol_address = 0;

	if (state->flags & B_DISASM_FLAG_DEBUG_SERVER_PRIVATE) {
		cookie = d->tmr;
		if (	(state->instr == instr_call) &&
				(d->got_address != NULL) &&
				(d->read_word(d->thr, (void *)addr, &val) == B_OK) &&
				((val & 0xffff) == 0xa3ff) &&
				(d->read_word(d->thr, (void *)(addr+2), &val) == B_OK) &&
				(d->read_word(d->thr, (void *)((uint32)d->got_address + val), 
						&val) == B_OK)) {
			state->symbol_address = addr = val;
		}
	}

	state->symbol[0] = 0;
	if ((*state->lookup)(cookie, addr, &symaddr, state->symbol,
			sizeof(state->symbol), 1) == B_OK) {
		state->symbol_offset = addr - symaddr;
	}
}

static status_t
output_address(state_t *state, int32 reg1, int32 reg2,
		uint32 factor, int32 disp, const char **regtbl)
{
	if ((reg1 < 0) && (reg2 < 0) &&
			!(state->flags & B_DISASM_FLAG_RELATIVE_ADDRESSES) &&
			state->lookup) {
		lookup_symbol(state, *(uint32 *)&disp);
	}
	
	if (state->flags & B_DISASM_FLAG_INTEL_STYLE) {
		out_char(state, '[');
		if ((reg1 < 0) && (reg2 < 0)) {
			out_uint32(state, *(uint32 *)&disp);
			out_char(state, ']');
			return B_OK;
		}
		if (reg1 >= 0)
			out_register(state, regtbl[reg1]);
		if (reg2 >= 0) {
			if (reg1 >= 0) out_char(state, '+');
			if (factor != 1) {
				out_char(state, factor + '0');
				out_char(state, '*');
			}
			out_register(state, regtbl[reg2]);
		}
		if (disp)
			out_int32(state, disp, LEADING_PLUS);
		out_char(state, ']');
	} else {
		if ((reg1 < 0) && (reg2 < 0)) {
			out_uint32(state, *(uint32 *)&disp);
			return B_OK;
		}
		if (disp)
			out_int32(state, disp, 0);
		out_char(state, '(');
		if (reg1 >= 0)
			out_register(state, regtbl[reg1]);
		if ((reg1 >= 0) && (reg2 >= 0))
			out_char(state, ',');
		if (reg2 >= 0) {
			out_register(state, regtbl[reg2]);
			out_char(state, ',');
			out_char(state, factor + '0');
		}
		out_char(state, ')');
	}
	
	return B_OK;
}

static status_t
decode_mod_rm(state_t *state, operand_t op)
{
	uchar mod_rm = state->mod_rm, sib = state->sib;
	uchar mod = mod_rm >> 6, rm = mod_rm & 7;

	if (mod == 3) /* reg */
		return output_register(state, op, rm);

	if (IS16ADDR) {
		int32 reg1tab[] = {
				reg_bx, reg_bx, reg_bp, reg_bp,
				reg_si, reg_di, reg_bp, reg_bx
		}, reg2tab[] = {
				reg_si, reg_di, reg_si, reg_di,
				-1, -1, -1, -1
		};
		if ((mod == 0) && (rm == 6)) {
			return output_address(state,-1,-1,1,state->displacement,reg16);
		}
		return output_address(state,reg1tab[rm],reg2tab[rm],1,
				state->displacement,reg16);
	} else {
		uchar has_sib = (rm == 4) ? 1 : 0;

		if ((mod == 0) && (rm == 5))
			return output_address(state,-1,-1,1,state->displacement,reg32);

		if (has_sib) {
			int32 base = sib & 7, index = (sib >> 3) & 7;
			if (index == 4) index = -1;
			if ((base == 5) && (mod == 0)) base = -1;
			return output_address(state, base, index,
					1 << (sib >> 6), state->displacement,reg32);
		}

		return output_address(state,rm,-1,1,state->displacement,reg32);
	}
}

static status_t
output_immediate(state_t *state, operand_t op, uchar *immediate)
{
	int size;

/*	assert(immediate);*/
	if (!immediate) return EINVAL;
	
	switch (op.type) {
		case op_type_a :
			zprintf("vyt: todo\n");
			return EINVAL;
		case op_type_b : size = 1; break;
		case op_type_c : size = (IS16OP) ? 1 : 2; break;
		case op_type_d : size = 4; break;
		case op_type_q : size = 8; break;
		case op_type_v : size = (IS16OP) ? 2 : 4; break;
		case op_type_w : size = 2; break;
		default :
			zprintf("Invalid immediate type (%x)\n", op.type);
			return EINVAL;
	}

	#define WORD(x) ((x)[0] + 0x100*(x)[1])
	#define DWORD(x) (WORD(x) + 0x10000*WORD((x)+2))
	
	if (!ISINTEL) out_char(state, '$');

	switch (size) {
		case 1 : out_uint8(state, *immediate); break;
		case 2 : out_uint16(state, WORD(immediate)); break;
		case 4 : out_uint32(state, DWORD(immediate)); break;
		case 8 : out_uint32(state, DWORD(immediate+4));
				 out_uint32(state, DWORD(immediate));
				 break;
		default: zprintf("Unknown size for immediate type (%x)\n", size); return EINVAL;
	}

	return B_OK;
}

static status_t
output_absolute(state_t *state, operand_t op)
{
	switch (op.type) {
		case op_type_p :
			if (IS16OP) {
				if (!ISINTEL) out_char(state, '$');
				out_uint16(state, WORD(state->immediate+2));
				out_char(state, ':');
				if (!ISINTEL) out_char(state, '$');
				out_uint16(state, DWORD(state->immediate));
			} else {
				if (!ISINTEL) out_char(state, '$');
				out_uint16(state, WORD(state->immediate+4));
				out_char(state, ':');
				if (!ISINTEL) out_char(state, '$');
				out_uint32(state, DWORD(state->immediate));
			}
			break;
		default : zprintf("Unsupported absolute type (%x)\n", op.type); return EINVAL;
	}
	return B_OK;
}

static status_t
output_relative(state_t *state, operand_t op)
{
	if (state->flags & B_DISASM_FLAG_RELATIVE_ADDRESSES) {
		out_char(state, (state->flags & B_DISASM_FLAG_INTEL_STYLE) ? '$' : '.');
	
		switch (op.type) {
			case op_type_b :
				out_int8(state, *(state->immediate) + state->instr_size, LEADING_PLUS);
				break;
			case op_type_v :
				if (IS16OP) {
					out_int16(state, WORD(state->immediate) + state->instr_size, LEADING_PLUS);
				} else {
					out_int32(state, DWORD(state->immediate) + state->instr_size, LEADING_PLUS);
				}
				break;
			default : zprintf("Unknown relative type (%x)\n", op.type); return EINVAL;
		}
	} else {
		uint32 addr = state->eip + state->instr_size;
		
		switch (op.type) {
			case op_type_b :
				addr += *(int8 *)(state->immediate);
				break;
			case op_type_v :
				if (IS16OP) {
					uint16 temp = WORD(state->immediate);
					addr += *(int16 *)&temp;
				} else {
					uint32 temp = DWORD(state->immediate);
					addr += *(int32 *)&temp;
				}
				break;
			default : zprintf("Unknown relative type (%x)\n", op.type); return EINVAL;
		}
		
		out_uint32(state, addr);
		if (state->lookup) {
			lookup_symbol(state, addr);
		}
	}
	
	return B_OK;
}

static status_t
output_operand(state_t *state, int i)
{
	operand_t op = state->ops[i];

	switch (op.addr) {
		case op_addr_none : zprintf("Invalid operand type: none\n"); return B_ERROR;

		case op_addr_A :
			return output_absolute(state, op);
		
		case op_addr_C : /* reg = control register */
		case op_addr_D : /* reg = debug register */
		case op_addr_G : /* reg = general register */
		case op_addr_P : /* reg = packed qword MMX register */
		case op_addr_S : /* reg = segment register */
		case op_addr_T : /* reg = test register */
			return output_register(state, op, (state->mod_rm >> 3) & 7);

		case op_addr_M : /* must be memory */
			if ((state->mod_rm >> 6) == 3) return EINVAL;
		case op_addr_R : /* must be register */
		case op_addr_Qmmx : /* must be mmx */
			if (((op.addr == op_addr_R) || (op.addr == op_addr_Qmmx)) && ((state->mod_rm >> 6) != 3)) return EINVAL;
		case op_addr_Q :
		case op_addr_E : /* mod R/M */
			return decode_mod_rm(state, op);

		case op_addr_F :
			return B_OK;

		case op_addr_I :
			return output_immediate(state, op, state->immediate);
			
		case op_addr_J :
			return output_relative(state, op);

		case op_addr_O :
			if (IS16ADDR) {
				return output_address(state,-1,-1,1,WORD(state->immediate),reg16);
			} else {
				return output_address(state,-1,-1,1,DWORD(state->immediate),reg16);
			}

		case op_addr_X :
			return output_address(state,reg_esi,-1,1,0,IS16ADDR?reg16:reg32);

		case op_addr_Y :
			return output_address(state,reg_edi,-1,1,0,IS16ADDR?reg16:reg32);

		case op_addr_reg :
			return output_register(state, op, op.data);

		case op_addr_immed :
			out_char(state, op.data + '0');
			return B_OK;

		default :
			zprintf("Unknown operand addressing mode (%x)\n", op.addr);
			return B_ERROR;
	}
}

static bool
has_mod_rm(operand_t op)
{
	switch (op.addr) {
		case op_addr_C:
		case op_addr_D:
		case op_addr_E:
		case op_addr_G:
		case op_addr_M:
		case op_addr_P:
		case op_addr_Q:
		case op_addr_Qmmx:
		case op_addr_R:
		case op_addr_S:
		case op_addr_T:
			return true;
		default:
			return false;
	}
}

static bool
has_immediate(operand_t op)
{
	switch (op.addr) {
		case op_addr_A:
		case op_addr_I:
		case op_addr_J:
		case op_addr_O: /* it's really an address */
			return true;
		default:
			return false;
	}
}

static int32
operand_type_op_size(state_t *state, operand_t op)
{
	switch (op.type) {
		case op_type_a : return (IS16OP) ? 2 : 4;
		case op_type_b : return 1;
		case op_type_c : return (IS16OP) ? 1 : 2;
		case op_type_d : return 4;
		case op_type_o : return 16;
		case op_type_p : return (IS16OP) ? 4 : 6;
		case op_type_q : return 8;
		case op_type_s : return 6;
		case op_type_v : return (IS16OP) ? 2 : 4;
		case op_type_w : return 2;
		case op_type_sreg : return 2;
		default :
			zprintf("Unknown operand type (%x)\n", op.type);
			return 0;
	}
}

static int32
operand_type_addr_size(state_t *state, operand_t op)
{
	switch (op.type) {
		case op_type_a : return (IS16ADDR) ? 2 : 4;
		case op_type_b : return 1;
		case op_type_c : return (IS16ADDR) ? 1 : 2;
		case op_type_d : return 4;
		case op_type_o : return 16;
		case op_type_p : return (IS16ADDR) ? 4 : 6;
		case op_type_q : return 8;
		case op_type_s : return 6;
		case op_type_v : return (IS16ADDR) ? 2 : 4;
		case op_type_w : return 2;
		case op_type_sreg : return 2;
		default :
			zprintf("Unknown operand type (%x)\n", op.type);
			return 0;
	}
}

static status_t
output_instruction(state_t *state, int opcode_size, instr_t instr,
		const operand_t op1, const operand_t op2, const operand_t op3,
		uchar *in)
{
	status_t error;
	const char **string_table;
	uchar *p;
	const operand_t *immediate_op;
	int i;

	if (instr >= instr_last) {
		zprintf("Unknown instruction: %x\n", instr);
		return EINVAL;
	}

	/* parse the instruction */
	p = in;

	state->instr = instr;
	state->ops[0] = op1;
	state->ops[1] = op2;
	state->ops[2] = op3;
	state->mod_rm = 0;
	state->sib = 0;
	state->displacement = 0;
	state->immediate = NULL;
	
	if (has_mod_rm(op1) || has_mod_rm(op2) || has_mod_rm(op3)) {
		uchar mod, rm;
		state->instr_size++;
		if (state->instr_size > state->insize) return E2BIG;
		state->mod_rm = *(p++);
		mod = state->mod_rm >> 6;
		rm = state->mod_rm & 7;
		if (!IS16ADDR && (rm == 4) && (mod != 3)) {
				state->instr_size++;
				if (state->instr_size > state->insize) return E2BIG;
				state->sib = *(p++);
		}
		if (mod == 1) {
			/* 8 bit displacement */
			state->instr_size++;
			if (state->instr_size > state->insize) return E2BIG;
			state->displacement = *(int8 *)(p++);
		} else if ((mod == 2) ||
					(IS16ADDR && (mod == 0) && (rm == 6)) ||
					(!IS16ADDR && (mod == 0) && (rm == 5))) {
			if (IS16ADDR) {
				/* 16 bit displacement */
				state->instr_size += 2;
				if (state->instr_size > state->insize) return E2BIG;
				state->displacement = WORD(p);
				p += 2;
				if (state->displacement & 0x8000)
					state->displacement |= 0xffff0000;
			} else {
				/* 32 bit displacement */
				state->instr_size += 4;
				if (state->instr_size > state->insize) return E2BIG;
				state->displacement = DWORD(p);
				p += 4;
			}
		}
	}
	
	immediate_op = NULL;
	if (has_immediate(op1))
		immediate_op = &op1;
	else if (has_immediate(op2))
		immediate_op = &op2;
	else if (has_immediate(op3))
		immediate_op = &op3;

	if (immediate_op) {
		int32 immediate_size = operand_type_op_size(state, *immediate_op);
		state->instr_size += immediate_size;
		if (state->instr_size > state->insize) return E2BIG;
		state->immediate = p;
	}

	//zprintf("mod_rm = %x, sib = %x\n", state->mod_rm, state->sib);

	string_table = intel_instrs;

	if ((*(string_table[instr]) >= 'A') && (*(string_table[instr]) <= 'Z')) {
		out_char(state, *(string_table[instr]) - 'A' + 'a');
		out_string(state, string_table[instr] + 1);
		/* add suffix to at&t style */
		if (!ISINTEL) {
			if (op1.addr != op_addr_none) {
				switch (operand_type_addr_size(state, op1)) {
					case 1 : out_char(state, 'b'); break;
					case 2 : out_char(state, 'w'); break;
					case 4 : out_char(state, 'l'); break;
					default: break;
				}
			} else {
				out_char(state, IS16OP ? 'w' : 'l');
			}
		}
	} else {
		out_string(state, string_table[instr]);
	}

	if (state->out.cur_size > 7)
		out_char(state, ' ');
	else
		for (i=state->out.cur_size;i<8;i++)
			out_char(state, ' ');

	if (state->ops[0].addr == op_addr_none) {
		;
	} else if (state->ops[1].addr == op_addr_none) {
		if ((error = output_operand(state, 0)) < B_OK) return error;
	} else {
		if (state->flags & B_DISASM_FLAG_INTEL_STYLE) {
			if ((error = output_operand(state, 0)) < B_OK) return error;
			out_string(state, ", ");
			if ((error = output_operand(state, 1)) < B_OK) return error;
		} else {
			if ((error = output_operand(state, 1)) < B_OK) return error;
			out_string(state, ", ");
			if ((error = output_operand(state, 0)) < B_OK) return error;
		}

		if (state->ops[2].addr != op_addr_none) {
			out_string(state, ", ");
			if ((error = output_operand(state, 2)) < B_OK) return error;
		}
	}
	
	return state->instr_size;
}

static status_t
process_fp_instr(state_t *state, uchar *in)
{
	static const operand_t op = { op_addr_I, op_type_b, 0 };
	static const operand_t mem32 = { op_addr_M, op_type_d, 0 };
	static const operand_t st0 = { op_addr_reg, op_type_fpreg, reg_st0 };

	operand_t st = { op_addr_reg, op_type_fpreg, 0 };
	uchar nnn;

	if (state->instr_size >= state->insize) return E2BIG;

	nnn = (in[1] >> 3) & 7;
	st.data = in[1] & 7;
	
	if ((in[0] == 0xdf) && (in[1] == 0xe0)) {
		static const operand_t operand_ax = { op_addr_reg, op_type_w, reg_ax };
		state->instr_size++;
		return output_instruction(state, 2, instr_fstsw,
				operand_ax, no_operand, no_operand, in + 2);
	}

	if ((in[0] == 0xd8) && (in[1] >= 0xc0)) {
		state->instr_size++;
		return output_instruction(state, 2, (instr_t)(instr_fadd + nnn),
				st0, st, no_operand, in + 2);
	}
	
	if ((in[0] == 0xd9) && (in[1] >= 0xe8)) {
		if (in[1] != 0xef) {
			state->instr_size++;
			return output_instruction(state, 2, (instr_t)(instr_fld1 + in[1] - 0xe8),
					no_operand, no_operand, no_operand, in + 2);
		}
	}

	if ((in[0] == 0xdc) && (in[1] >= 0xc0) && ((in[1] & 0xf0) != 0xd0)) {
		operand_t st = { op_addr_reg, op_type_fpreg, 0 };
		st.data = in[1] & 7;
		state->instr_size++;
		return output_instruction(state, 2, (instr_t)(instr_fadd + nnn),
				st, st0, no_operand, in + 2);
	}
	
	if ((in[0] == 0xde) && (in[1] >= 0xc0) && ((in[1] & 0xf0) != 0xd0)) {
		static const instr_t instrs[] = {
			instr_faddp, instr_fmulp, instr_last, instr_last,
			instr_fsubrp, instr_fsubp, instr_fdivrp, instr_fdivp
		};
		state->instr_size++;
		return output_instruction(state, 2, instrs[nnn],
				st, st0, no_operand, in + 2);
	}
	
	if ((in[0] == 0xdd) && (in[1] >= 0xc0)) {
		static const instr_t instrs[] = {
			instr_ffree, instr_last, instr_fst, instr_fstp,
			instr_fucom, instr_fucomp, instr_last, instr_last
		};
		state->instr_size++;
		if (instrs[nnn] != instr_last) {
			return output_instruction(state, 2, instrs[nnn],
					st, (instrs[nnn] == instr_fucom) ? st0 : no_operand,
					no_operand, in + 2);
		}
	}

	if (in[1] >= 0xc0) {
		const no_arg_fp_instr_t *p0;
		const two_args_fp_instr_t *p2;
		for (p0=no_arg_fp_instr;p0->opcode1;p0++) {
			if ((p0->opcode1 == in[0]) && (p0->opcode2 == in[1])) {
				state->instr_size++;
				return output_instruction(state, 2, p0->instruction,
						no_operand, no_operand, no_operand, in + 2);
			}
		}
		
		for (p2=two_args_fp_instr;p2->opcode1;p2++) {
			if ((p2->opcode1 == in[0]) && (p2->opcode2_base == (in[1] & 0xf8))) {
				state->instr_size++;
				return output_instruction(state, 2, p2->instruction,
						st0, st, no_operand, in + 2);
			}
		}
	}

	if (in[1] < 0xc0) {
		if ((in[0] != 0xdb) && (in[0] != 0xdd) && (in[0] != 0xdf)) {
			static const instr_t bases[] = {
				instr_fadd, instr_fld, instr_fiadd, instr_last,
				instr_fadd, instr_last, instr_fiadd, instr_last
			};
			/* look for blanks in opcode map */
			#define EQ(a,b) ((in[0] == (a)) && (nnn == (b)))
			if (EQ(0xd9,1)) goto unknown;
			/* vyt: sizes */
			return output_instruction(state, 1, (instr_t)(bases[*in-0xd8] + nnn),
					mem32, no_operand, no_operand, in + 1);
		}
	
		if (*in == 0xdb) {
			static const instr_t instrs[8] = {
				instr_fild, instr_last, instr_fist, instr_fistp,
				instr_last, instr_fld, instr_last, instr_fstp
			};
			if (instrs[nnn] == instr_last) goto unknown;
			/* vyt: sizes */
			return output_instruction(state, 1, instrs[nnn],
					mem32, no_operand, no_operand, in + 1);
		}

		if (*in == 0xdd) {
			static const instr_t instrs[8] = {
				instr_fld, instr_last, instr_fst, instr_fstp,
				instr_frstor, instr_last, instr_fsave, instr_fstsw
			};
			if (instrs[nnn] == instr_last) goto unknown;
			/* vyt: sizes */
			return output_instruction(state, 1, instrs[nnn],
					mem32, no_operand, no_operand, in + 1);
		}

		if (*in == 0xdf) {
			static const instr_t instrs[8] = {
				instr_fild, instr_last, instr_fist, instr_fistp,
				instr_fbld, instr_fild, instr_fbstp, instr_fistp
			};
			if (instrs[nnn] == instr_last) goto unknown;
			/* vyt: sizes */
			return output_instruction(state, 1, instrs[nnn],
					mem32, no_operand, no_operand, in + 1);
		}
	}

unknown:
	return output_instruction(state, 1, (instr_t)(instr_esc0 + *in - 0xd8),
			op, no_operand, no_operand, in + 1);
}

static status_t
hardcoded_one_byte_opcodes(state_t *state, uchar *in)
{
	if ((*in < 0x40) && ((*in & 7) < 6)) {
		static const operand_t op1[] = {
				{ op_addr_E, op_type_b, 0 }, { op_addr_E, op_type_v, 0 },
				{ op_addr_G, op_type_b, 0 }, { op_addr_G, op_type_v, 0 },
				{ op_addr_reg, op_type_b, reg_al }, { op_addr_reg, op_type_v, reg_eax }
		};
		static const operand_t op2[] = {
				{ op_addr_G, op_type_b, 0 }, { op_addr_G, op_type_v, 0 },
				{ op_addr_E, op_type_b, 0 }, { op_addr_E, op_type_v, 0 },
				{ op_addr_I, op_type_b, 0 }, { op_addr_I, op_type_v, 0 }
		};
		static const instr_t instr[] =
				{ instr_add, instr_or, instr_adc, instr_sbb, 
				  instr_and, instr_sub, instr_xor, instr_cmp };

		return output_instruction(state, 1, instr[*in >> 3],
				op1[*in & 7], op2[*in & 7], no_operand, in + 1);
	}

	if ((*in >= 0x40) && (*in < 0x60)) {
		static const instr_t instr[] = {
			instr_inc, instr_dec, instr_push, instr_pop
		};
		operand_t op = { op_addr_reg, op_type_v, 0 };
		op.data = *in & 7;
		return output_instruction(state, 1, instr[(*in-0x40)>>3],
				op, no_operand, no_operand, in + 1);
	}

	if ((*in & 0xf0) == 0x70) {
		static const operand_t op = { op_addr_J, op_type_b, 0 };
		return output_instruction(state, 1, (instr_t)(instr_jo + *in - 0x70),
				op, no_operand, no_operand, in + 1);
	}

	if ((*in > 0x90) && (*in < 0x98)) {
		operand_t op1 = { op_addr_reg, op_type_v, 0 },
					op2 = { op_addr_reg, op_type_v, 0 };
		op2.data = *in - 0x90;
		return output_instruction(state, 1, instr_xchg,
				op1, op2, no_operand, in + 1);
	}

	if ((*in & 0xf8) == 0xd8) {
		return process_fp_instr(state, in);
	}

	if ((*in & 0xf0) == 0xb0) {
		operand_t op1 = { op_addr_reg, op_type_v, 0 },
					op2 = { op_addr_I, op_type_v, 0 };
		op1.data = *in & 7;
		if (*in < 0xb8)
			op1.type = op2.type = op_type_b;
		return output_instruction(state, 1, instr_mov,
				op1, op2, no_operand, in + 1);
	}

	return ENOSYS;
}

static status_t
hardcoded_0f_opcodes(state_t *state, uchar *in)
{
	in++;
	
	if ((*in & 0xf0) == 0x40) {
		static const operand_t op1 = { op_addr_G, op_type_v, 0 };
		static const operand_t op2 = { op_addr_E, op_type_v, 0 };
		return output_instruction(state, 2, (instr_t)(instr_cmovo + *in - 0x40),
				op1, op2, no_operand, in + 1);
	}

	if (((*in & 0xf0) == 0x60) && (*in < 0x6c)) {
		static const operand_t op1 = { op_addr_P, op_type_q, 0 };
		static const operand_t op2 = { op_addr_Q, op_type_d, 0 };
		return output_instruction(state, 2, (instr_t)(instr_punpcklbw + *in - 0x60),
				op1, op2, no_operand, in + 1);
	}

	if ((*in & 0xf0) == 0x80) {
		static const operand_t op = { op_addr_J, op_type_v, 0 };
		return output_instruction(state, 2, (instr_t)(instr_jo + *in - 0x80),
				op, no_operand, no_operand, in + 1);
	}

	if ((*in & 0xf0) == 0x90) {
		static const operand_t op = { op_addr_E, op_type_b, 0 };
		return output_instruction(state, 2, (instr_t)(instr_seto + *in - 0x90),
				op, no_operand, no_operand, in + 1);
	}

	if ((*in & 0xf8) == 0xc8) {
		operand_t op = { op_addr_reg, op_type_d, 0 };
		op.data = *in - 0xc8;
		return output_instruction(state, 2, instr_bswap,
				op, no_operand, no_operand, in + 1);
	}

	return ENOSYS;
}

static status_t
hardcoded_f30f_opcodes(state_t *state, uchar *in)
{
	TOUCH(state);
	TOUCH(in);

	return ENOSYS;
}

static status_t
disasm_real(state_t *state, uchar *in,
		const instr0_desc_t *i0, const instr1_desc_t *i1,
		const instr2_desc_t *i2, const instr3_desc_t *i3,
		const opcode_extension_t *oext,
		status_t (*extra)(state_t *, uchar *),
		int opcode_size)
{
	status_t error;

	state->instr_size += opcode_size;
	if (state->instr_size > state->insize) return E2BIG;

	error = (*extra)(state, in);
	if (error != ENOSYS) return error;

	{
		const instr0_desc_t *ptable0 = i0;
		while (ptable0->opcode) {
			if (ptable0->opcode == in[opcode_size-1]) {
				return output_instruction(state, opcode_size, ptable0->instruction,
						no_operand, no_operand, no_operand, in + opcode_size);
			}
			ptable0++;
		}
	}

	{
		const instr1_desc_t *ptable1 = i1;
		while (ptable1->opcode) {
			if (ptable1->opcode == in[opcode_size-1]) {
				return output_instruction(state, opcode_size, ptable1->instruction,
						ptable1->op1, no_operand, no_operand, in + opcode_size);
			}
			ptable1++;
		}
	}

	{
		const instr2_desc_t *ptable2 = i2;
		while (ptable2->opcode) {
			if (ptable2->opcode == in[opcode_size-1]) {
				return output_instruction(state, opcode_size, ptable2->instruction,
						ptable2->op1, ptable2->op2, no_operand, in + opcode_size);
			}
			ptable2++;
		}
	}

	{
		const instr3_desc_t *ptable3 = i3;
		while (ptable3->opcode) {
			if (ptable3->opcode == in[opcode_size-1]) {
				return output_instruction(state, opcode_size, ptable3->instruction,
						ptable3->op1, ptable3->op2, ptable3->op3, in + opcode_size);
			}
			ptable3++;
		}
	}
	
	{
		const opcode_extension_t *ext = oext;
		while (ext->opcode != END_EXTENSIONS) {
			if (ext->opcode == in[opcode_size-1]) {
				operand_t op2 = ext->op2;

				if (state->instr_size >= state->insize) return E2BIG;

				if (*in != 0x0f)
					if (((in[opcode_size-1] == 0xf6) ||
						  (in[opcode_size-1] == 0xf7)) &&
						 (((in[opcode_size]>>3)&7) == 0)) {
						/* hack for test weirdness */
						op2 = ext->op1;
						op2.addr = op_addr_I;
					}

				return output_instruction(state, opcode_size,
						ext->instrs[(in[opcode_size]>>3)&7],
						(ext->op1s ? ext->op1s[(in[opcode_size]>>3)&7] : ext->op1),
						op2, no_operand, in + opcode_size);
			}
			ext++;
		}
	}

	return EINVAL;
}

status_t disasm(uchar *in, uint32 insize, char *out, uint32 outsize,
		uint32 eip, uint32 flags,
		status_t (*lookup)(void *cookie, uint32 eip, uint32 *sym_addr,
							char *sym_name, int max_name_len, int is_lower),
		void *cookie)
{
	uint32 prefixes = 0;
	state_t state;
	status_t error;
	uchar *p;

/*	vyt: bring it back
	assert(sizeof(intel_instrs) / sizeof(intel_instrs[0]) == instr_last);
*/

	if (flags & ~DISASM_USER_FLAGS)
		return EINVAL;

	state.symbol[0] = 0;
	state.eip = eip;
	state.lookup = lookup;
	state.cookie = cookie;

	p = in;

	state.instr_size = 0;
	state.insize = insize;

	error = E2BIG; /* prepare for the worst */
	while (1) {
		if (*p == 0x66) {
			state.instr_size++;
			if (state.instr_size > state.insize) goto err;
			flags ^= B_DISASM_FLAG_OP_SIZE_16;
			p++;
		} else if (*p == 0x67) {
			state.instr_size++;
			if (state.instr_size > state.insize) goto err;
			flags ^= B_DISASM_FLAG_ADDR_SIZE_16;
			p++;
		} else
			break;
	}

	state.out.s = out;
	state.out.cur_size = 0;
	state.out.max_size = outsize;
	state.flags = flags;

	if ((*p == 0xf3) && (state.instr_size + 1 <= state.insize) && (*(p+1) == 0x0f)) {
		error = disasm_real(&state, p, instr0_f30f, instr1_f30f, instr2_f30f, instr3_f30f,
				opcode_extensions_f30f, hardcoded_f30f_opcodes, 3);
	} else if (*p == 0x0f) {
		error = disasm_real(&state, p, instr0_0f, instr1_0f, instr2_0f, instr3_0f,
				opcode_extensions_0f, hardcoded_0f_opcodes, 2);
	} else {
		error = disasm_real(&state, p, instr0, instr1, instr2, instr3,
				opcode_extensions, hardcoded_one_byte_opcodes, 1);
	}
err:
	if (error < 1) {
		state.out.cur_size = 0;
		if (flags & B_DISASM_FLAG_INTEL_STYLE)
			out_string(&state, "db      ");
		else
			out_string(&state, ".byte   ");
		out_uint8(&state, *in);
		state.out.s[state.out.cur_size] = 0;

		return 1;	
	} else if (*(state.symbol)) {
		if ((state.symbol_offset < 16384) && (state.symbol_offset > -16384)) {
			out_string(&state, " <");
			out_string(&state, state.symbol);
			if (state.symbol_address != 0) {
				out_string(&state, "(");
				out_uint32(&state, state.symbol_address);
				out_string(&state, ")");
			}
			if (state.symbol_offset != 0)
				out_int32(&state, state.symbol_offset, LEADING_PLUS);
			out_char(&state, '>');
		}
	}
	
	state.out.s[state.out.cur_size] = 0;

	return error + prefixes;
}
