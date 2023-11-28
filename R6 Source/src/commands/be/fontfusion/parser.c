#include "parser.h"

byte_code instructions[] = {
	{	0x00,	0x01,	"SVTCA",		0,	0,	0,	0,	""	},
	{	0x02,	0x03,	"SPVTCA",		0,	0,	0,	0,	""	},
	{	0x04,	0x05,	"SFVTCA",		0,	0,	0,	0,	""	},
	{	0x06,	0x07,	"SPVTL",		2,	0,	0,	0,	""	},
	{	0x08,	0x09,	"SFVTL",		2,	0,	0,	0,	""	},
	{	0x0A,	0,		"SPVFS",		2,	0,	0,	0,	""	},
	{	0x0B,	0,		"SFVFS",		2,	0,	0,	0,	""	},
	{	0x0C,	0,		"GPV",			0,	2,	0,	0,	""	},
	{	0x0D,	0,		"GFV",			0,	2,	0,	0,	""	},
	{	0x0E,	0,		"SFVTPV",		-1,	-1,	31,	0,	"## WARNING ##"	},
	{	0x0F,	0,		"ISECT",		5,	0,	0,	0,	""	},
	{	0x10,	0,		"SRP0",			1,	0,	0,	0,	""	},
	{	0x11,	0,		"SRP1",			1,	0,	0,	0,	""	},
	{	0x12,	0,		"SRP2",			1,	0,	0,	0,	""	},
	{	0x13,	0,		"SZP0",			1,	0,	0,	0,	""	},
	{	0x14,	0,		"SZP1",			1,	0,	0,	0,	""	},
	{	0x15,	0,		"SZP2",			1,	0,	0,	0,	""	},
	{	0x16,	0,		"SZPS",			1,	0,	0,	0,	""	},
	{	0x17,	0,		"SLOOP",		-1,	-1,	15,	0,	"LOOPCOUNT-1;"	},
	{	0x18,	0,		"RTG",			0,	0,	0,	0,	""	},
	{	0x19,	0,		"RTHG",			0,	0,	0,	0,	""	},
	{	0x1A,	0,		"SMD",			1,	0,	0,	0,	""	},
	{	0x1B,	0,		"ELSE",			-1,	-1,	21,	0,	""	},
	{	0x1C,	0,		"JMPR",			-1,	-1,	28,	0,	"OFFSET-1 (+1);"	},
	{	0x1D,	0,		"SCVTCI",		1,	0,	0,	0,	""	},
	{	0x1E,	0,		"SSWCI",		1,	0,	0,	0,	""	},
	{	0x1F,	0,		"SSW",			1,	0,	0,	0,	""	},
	{	0x20,	0,		"DUP",			1,	2,	84,	0,	""	},
	{	0x21,	0,		"POP",			1,	0,	0,	0,	""	},
	{	0x22,	0,		"CLEAR",		-1,	-1,	0,	0,	"POP EVERYTHING;"	},
	{	0x23,	0,		"SWAP",			2,	2,	80,	0,	""	},
	{	0x24,	0,		"DEPTH",		0,	1,	0,	0,	""	},
	{	0x25,	0,		"CINDEX",		1,	1,	81,	0,	""	},
	{	0x26,	0,		"MINDEX",		2,	1,	82,	0,	""	},
	{	0x27,	0,		"ALIGNPTS",		2,	0,	0,	0,	""	},
	{	0x28,	0,		"RAW",			-1,	-1,	31,	0,	"## WARNING ##"	},
	{	0x29,	0,		"UTP",			1,	0,	0,	0,	""	},
	{	0x2A,	0,		"LOOPCALL",		-1,	-1,	26,	0,	"INDEX-1; LOOPCOUNT-2;"	},
	{	0x2B,	0,		"CALL",			-1,	-1,	27,	0,	"INDEX-1;"	},
	{	0x2C,	0,		"FDEF",			-1,	-1,	24,	0,	"INDEX-1;"	},
	{	0x2D,	0,		"ENDF",			-1,	-1,	25,	0,	""	},
	{	0x2E,	0x2F,	"MDAP",			1,	0,	0,	0,	""	},
	{	0x30,	0x31,	"IUP",			0,	0,	0,	0,	""	},
	{	0x32,	0x33,	"SHP",			1,	0,	23,	0,	""	},
	{	0x34,	0x35,	"SHC",			1,	0,	0,	0,	""	},
	{	0x36,	0x37,	"SHZ",			1,	0,	0,	0,	""	},
	{	0x38,	0,		"SHPIX",		-1,	-1,	16,	0,	"POP 1+n(LOOP);"	},
	{	0x39,	0,		"IP",			1,	0,	23,	0,	""	},
	{	0x3A,	0x3B,	"MSIRP",		2,	0,	0,	0,	""	},
	{	0x3C,	0,		"ALIGNRP",		1,	0,	23,	0,	""	},
	{	0x3D,	0,		"RTDG",			0,	0,	0,	0,	""	},
	{	0x3E,	0x3F,	"MIAP",			2,	0,	0,	-1,	"CVT-1;"	},
	{	0x40,	0,		"NPUSHB",		-1,	-1,	11,	0,	""	},
	{	0x41,	0,		"NPUSHW",		-1,	-1,	12,	0,	""	},
	{	0x42,	0,		"WS",			2,	0,	0,	0,	"WRITE STORAGE;"	},
	{	0x43,	0,		"RS",			1,	1,	0,	0,	"READ STORAGE;"	},
	{	0x44,	0,		"WCVTP",		2,	0,	0,	-2,	"CVT-2;"	},
	{	0x45,	0,		"RCVT",			1,	1,	0,	-1,	"CVT-1;"	},
	{	0x46,	0x47,	"GC",			1,	1,	0,	0,	""	},
	{	0x48,	0,		"SCFS",			2,	0,	0,	0,	""	},
	{	0x49,	0x4A,	"MD",			2,	1,	0,	0,	""	},
	{	0x4B,	0,		"MPPEM",		0,	1,	0,	0,	""	},
	{	0x4C,	0,		"MPS",			0,	1,	0,	0,	""	},
	{	0x4D,	0,		"FLIPON",		0,	0,	0,	0,	""	},
	{	0x4E,	0,		"FLIPOFF",		0,	0,	0,	0,	""	},
	{	0x4F,	0,		"DEBUG",		-1,	-1,	31,	0,	"## WARNING ##; POP 1;"	},
	{	0x50,	0,		"LT",			2,	1,	0,	0,	""	},
	{	0x51,	0,		"LTEQ",			2,	1,	0,	0,	""	},
	{	0x52,	0,		"GT",			2,	1,	0,	0,	""	},
	{	0x53,	0,		"GTEQ",			2,	1,	0,	0,	""	},
	{	0x54,	0,		"EQ",			2,	1,	0,	0,	""	},
	{	0x55,	0,		"NEQ",			2,	1,	0,	0,	""	},
	{	0x56,	0,		"ODD",			1,	1,	0,	0,	""	},
	{	0x57,	0,		"EVEN",			1,	1,	0,	0,	""	},
	{	0x58,	0,		"IF",			1,	0,	20,	0,	""	},
	{	0x59,	0,		"ENDIF",		-1,	-1,	22,	0,	""	},
	{	0x5A,	0,		"AND",			2,	1,	0,	0,	""	},
	{	0x5B,	0,		"OR",			2,	1,	0,	0,	""	},
	{	0x5C,	0,		"NOT",			1,	1,	0,	0,	""	},
	{	0x5D,	0,		"DELTAP1",		-1,	-1,	17,	0,	"POP 1(n) + 2*n;"	},
	{	0x5E,	0,		"SDB",			1,	0,	0,	0,	""	},
	{	0x5F,	0,		"SDS",			1,	0,	0,	0,	""	},
	{	0x60,	0,		"ADD",			2,	1,	0,	0,	""	},
	{	0x61,	0,		"SUB",			2,	1,	0,	0,	""	},
	{	0x62,	0,		"DIV",			2,	1,	0,	0,	""	},
	{	0x63,	0,		"MUL",			2,	1,	0,	0,	""	},
	{	0x64,	0,		"ABS",			1,	1,	0,	0,	""	},
	{	0x65,	0,		"NEG",			1,	1,	0,	0,	""	},
	{	0x66,	0,		"FLOOR",		1,	1,	0,	0,	""	},
	{	0x67,	0,		"CEILING",		1,	1,	0,	0,	""	},
	{	0x68,	0x6B,	"ROUND",		1,	1,	0,	0,	""	},
	{	0x6C,	0x6F,	"NROUND",		1,	1,	0,	0,	""	},
	{	0x70,	0,		"WCVTFOD",		2,	0,	0,	-2,	"CVT-2;"	},
	{	0x71,	0,		"DELTAP2",		-1,	-1,	17,	0,	"POP 1(n) + 2*n;"	},
	{	0x72,	0,		"DELTAP3",		-1,	-1,	17,	0,	"POP 1(n) + 2*n;"	},
	{	0x73,	0,		"DELTAC1",		-1,	-1,	18,	0,	"POP 1(n) + 2*n; CVT -2*k, k=1-n};"	},
	{	0x74,	0,		"DELTAC2",		-1,	-1,	18,	0,	"POP 1(n) + 2*n; CVT -2*k, k=1-n};"	},
	{	0x75,	0,		"DELTAC3",		-1,	-1,	18,	0,	"POP 1(n) + 2*n; CVT -2*k, k=1-n};"	},
	{	0x76,	0,		"SROUND",		1,	0,	0,	0,	""	},
	{	0x77,	0,		"S45ROUND",		1,	0,	0,	0,	""	},
	{	0x78,	0,		"JROT",			2,	0,	19,	0,	"OFFSET-1;"	},
	{	0x79,	0,		"JROF",			2,	0,	19,	0,	"OFFSET-1;"	},
	{	0x7A,	0,		"ROFF",			0,	0,	0,	0,	""	},
	{	0x7B,	0,		"ILLEGAL",		-1,	-1,	31,	0,	"## WARNING ##"	},
	{	0x7C,	0,		"RUTG",			0,	0,	0,	0,	""	},
	{	0x7D,	0,		"RDTG",			0,	0,	0,	0,	""	},
	{	0x7E,	0,		"SANGW",		1,	0,	0,	0,	""	},
	{	0x7F,	0,		"AA",			-1,	-1,	31,	0,	"## WARNING ##"	},
	{	0x80,	0,		"FLIPPT",		1,	0,	23,	0,	""	},
	{	0x81,	0,		"FLIPRGON",		2,	0,	0,	0,	""	},
	{	0x82,	0,		"FLIPRGOFF",	2,	0,	0,	0,	""	},
	{	0x83,	0,		"RMVT",			-1,	-1,	31,	0,	"## WARNING ##"	},
	{	0x84,	0,		"WMVT",			-1,	-1,	31,	0,	"## WARNING ##"	},
	{	0x85,	0,		"SCANCTRL",		1,	0,	29,	0,	""	},
	{	0x86,	0x87,	"SDPVTL",		2,	0,	0,	0,	""	},
	{	0x88,	0,		"GETINFO",		1,	1,	0,	0,	""	},
	{	0x89,	0,		"IDEF",			-1,	-1,	31,	0,	"## WARNING ##; OPCODE-1;"	},
	{	0x8A,	0,		"ROLL",			3,	3,	83,	0,	""	},
	{	0x8B,	0,		"MAX",			2,	1,	0,	0,	""	},
	{	0x8C,	0,		"MIN",			2,	1,	0,	0,	""	},
	{	0x8D,	0,		"SCANTYPE",		1,	0,	29,	0,	""	},
	{	0x8E,	0,		"INSTCTRL",		2,	0,	0,	0,	"## WARNING ##"	},
	{	0x8F,	0xAF,	"ILLEGAL",		-1,	-1,	31,	0,	"## WARNING ##"	},
	{	0xB0,	0xB7,	"PUSHB",		-1,	-1,	13,	0,	""	},
	{	0xB8,	0xBF,	"PUSHW",		-1,	-1,	14,	0,	""	},
	{	0xC0,	0xDF,	"MDRP",			1,	0,	0,	0,	""	},
	{	0xE0,	0xFF,	"MIRP",			2,	0,	0,	-1,	"CVT-1;"	},
	{	0xAF,	0xAF,	"ENDCALL",		0,	0, 	30,	0,	"" 	}
};

static bool parsing_error;

int32 FindCst(	int32		cur_ass,
				uint32		*cst,
				ass_item	*ass_list,
				int32		*cur_label,
				int32		stack_offset,
				uint8		*type) {
	int32		i, ass_index, level;
	int32		stack_delta;
	ass_item	*ass1, *ass2;
	
	level = 0;
	stack_delta = 0;
	for (ass_index = cur_ass-1; ass_index >= 0; ass_index--) {
		ass1 = ass_list+ass_index;
		/* check for FDEF and ENDF */
		if (ass1->ref->special == 24) {
			if ((*type&15) != FCT_INDEX) {
				if (level == 0) {
					*type = CALL_INDEX;
					parsing_error = true;
					return stack_delta-stack_offset;
				}
				else {
					fprintf(stderr, "## Tracking back failure on FDEF\n");
					parsing_error = true;
					return -1;
				}
			}
			level--;			
			stack_delta++;
		}
		else if (ass1->ref->special == 25) {
			if ((*type&15) != FCT_INDEX) {
				fprintf(stderr, "## Tracking back failure on ENDF\n"); 
				parsing_error = true;
				return -1;
			}
			level++;
		}
		else if (level == 0) {
			/* encounter am instruction with conditional execution */
			if (ass1->ref->special == 21) { /* ELSE */
				if ((ass1->types[0]&15) == UNKNOWN) {
					ass1->types[0] = UNSOLVABLE;
					for (i=ass_index-1; i>=0; i--)
						if (ass_list[i].ref->special == 20) {
							ass1->types[0] = ASS_INDEX;
							ass1->params[0] = i;
							break;
						}
				}
				if ((ass1->types[0]&15) == ASS_INDEX)
					ass_index = ass1->params[0]+1;
				else
					goto unsolved_instruction;
			}
			else if (ass1->ref->special == 22) { /* ENDIF */
				if ((ass1->types[0]&15) == UNKNOWN) {
					if (ass_list[ass_index-1].ref->special == 28) {	/* JMPR */
						for (i=ass_index-1; i>=0; i--)
							if (ass_list[i].ref->special == 20) {
								ass1->types[0] = ASS_INDEX;
								ass1->params[0] = i;
								break;
							}
					}
					else {
						ass1->types[0] = UNSOLVABLE;
						for (i=ass_index-1; i>=0; i--)
							if (ass_list[i].level == ass1->level) {						
								if (ass_list[i].ref->special == 20) {	/* IF */
									ass1->types[0] = ASS_INDEX;
									ass1->params[0] = ass_index-1;
									break;
								}
								else if (ass_list[i].ref->special == 21) {	/* ELSE */
									ass1->types[0] = ASS_INDEX;
									ass1->params[0] = i-1;
									break;
								}
							}
					}
				}
				if ((ass1->types[0]&15) == ASS_INDEX)
					ass_index = ass1->params[0]+1;
				else
					goto unsolved_instruction;
			}
			else if (ass1->ref->special == 30) { /* ENDCALL */
				if ((ass1->types[0]&15) == UNKNOWN) {
					ass2 = ass1;
					for (i=ass_index-1; i>=0; i--) {
						ass2--;
						if (ass2->call_level == ass1->call_level-1)
							break;
					}
					ProcessStack(ass1-ass2-1, ass2+1, ass1);
					if ((ass1->pp_type&15) == UNKNOWN)
						ass1->types[0] = UNSOLVABLE;
					else {
						ass1->types[0] = ASS_INDEX | (ass1->pp_type&UNCERTAIN);
						ass1->params[0] = ass2-ass_list;
					}
				}
				if (((ass1->types[0]&15) == ASS_INDEX) &&
					((ass1->pp_type&15) == POP_PUSH)) {
					if (stack_offset < (stack_delta-ass1->push)) {
						stack_delta += (ass1->pop - ass1->push);
						ass_index = ass1->params[0]+1;
					}
				}
			}
			else if (ass1->ref->special == 28) { /* JMPR */
				if ((ass1->types[2]&15) == UNKNOWN) {
					for (i=ass_index-1; i>=0; i--)
						if (ass_list[i].ref->special == 19) {	/* JROx */
							if (((ass_list[i].types[0]&15) == JUMP_OFFSET) &&
								(ass_list[i].params[0]+ass_list[i].offset == ass1[1].offset)) {
								ass1->types[2] = ASS_INDEX;
								ass1->params[2] = i;
								break;
							}
							ass1->types[2] = UNSOLVABLE;
							break;
						}
				}
				if ((ass1->types[2]&15) == ASS_INDEX)
					ass_index = ass1->params[2]+1;
				else
					goto unsolved_instruction;
			}
			/* encounter an instruction whose stack behavior is unknown. Failure */
			else if ((ass1->pp_type&15) == UNKNOWN) {
unsolved_instruction:
				*type = LABEL_STACK;
				if (ass_list[ass_index+1].label_id == 0) {
					ass_list[ass_index+1].label_id = *cur_label;
					(*cur_label)++;
				}
				parsing_error = true;
				return (ass_list[ass_index+1].label_id<<16) | (stack_delta-stack_offset);
			}
			/* stack move instruction */
			else if (ass1->ref->special >= 80)
				switch (ass1->ref->special) {
				case 80:	/* SWAP */
					if (stack_offset == (stack_delta-1))
						stack_offset = stack_delta-2;
					else if (stack_offset == (stack_delta-2))
						stack_offset = stack_delta-1;
					break;
				case 81:	/* CINDEX */
					if (stack_offset == (stack_delta-1)) {
						if ((ass1->types[0]&15) == STACK_INDEX) {
							stack_offset = stack_delta-(1+ass1->params[0]);
							*type |= (ass1->types[0]&UNCERTAIN);
						}
						else {
							fprintf(stderr, "## Tracked back to a CINDEX(?)\n"); 
							parsing_error = true;
							return -1;
						}
					}
					break;
				case 82:	/* MINDEX */
					if ((ass1->types[0]&15) == STACK_INDEX) {
						if (stack_offset == (stack_delta-1))
							stack_offset = stack_delta-(1+ass1->params[0]);
						else if (stack_offset < (stack_delta-ass1->params[0]))
							stack_offset--;
						*type |= (ass1->types[0]&UNCERTAIN);
					}
					else {
						fprintf(stderr, "## Tracking back through a MINDEX(?)\n"); 
						parsing_error = true;
						return -1;
					}
					break;
				case 83:	/* ROLL */
					if (stack_offset == (stack_delta-1))
						stack_offset = stack_delta-3;
					else if (stack_offset == (stack_delta-2))
						stack_offset = stack_delta-1;
					else if (stack_offset == (stack_delta-3))
						stack_offset = stack_delta-2;
					break;
				case 84:	/* DUP */
					stack_delta--;
					if (stack_delta == stack_offset)
						stack_offset--;
					break;
				}
			/* standard instruction */
			else {
				/* Did this instruction create the target stack level ? */
				stack_delta -= ass1->push;
				if (stack_delta <= stack_offset) {
					/* we found the constant ! */
					if (ass1->cst_count	> 0)
						return ass1->cst_offset + (stack_offset-stack_delta);
					/* The value has been processed :-( */
					else {
						fprintf(stderr, "## Tacking back a processed value (%s/%d)\n",
								ass1->ref->name, ass1-ass_list); 
						parsing_error = true;
						return -1;
					}
				}
				stack_delta += ass1->pop;
				/* cumulate the uncertainty */
				*type |= (ass1->pp_type&UNCERTAIN);
			}
		}
	}
fprintf(stderr, "############ Failure 6\n"); 
	parsing_error = true;
	return -1;
}

void GetParams(	int32		cur_ass,
				uint32		*cst,
				ass_item	*ass_list,
				int32		*cur_label,
				int32		stack_offset,
				ass_item	*ass,
				int32		p_index,
				uint8		type) {
	int32		index;
	
	index = FindCst(cur_ass, cst, ass_list, cur_label, stack_offset, &type);
	if (index >= 0) {
		ass->types[p_index] = type;
		if ((type == CALL_INDEX) || (type == LABEL_STACK))
			ass->params[p_index] = index;
		else
			ass->params[p_index] = cst[index];
	}
	else {
		ass->types[p_index] = UNKNOWN;
		ass->params[p_index] = 0;
	}
}

void MarkCst(	int32		cur_ass,
				uint8		*cst_status,
				uint32		*cst,
				ass_item	*ass_list,
				int32		*cur_label,
				int32		stack_offset,
				ass_item	*ass,
				int32		p_index,
				uint8		type,
				uint8		type_cst) {
	int32		index;
	
	index = FindCst(cur_ass, cst, ass_list, cur_label, stack_offset, &type);
	if (index >= 0) {
		ass->types[p_index] = type;
		if ((type == CALL_INDEX) || (type == LABEL_STACK))
			ass->params[p_index] = index;
		else {
			ass->params[p_index] = cst[index];
			cst_status[index] = type_cst | (type&UNCERTAIN);
		}
	}
	else {
		ass->types[p_index] = UNKNOWN;
		ass->params[p_index] = 0;
	}
}

void SetCst(int32		cur_ass,
			uint32		*cst,
			ass_item	*ass_list,
			int32		*cur_label,
			int32		stack_offset,
			uint32		value) {
	uint8		type;
	int32		index;
	
	index = FindCst(cur_ass, cst, ass_list, cur_label, stack_offset, &type);
	if (index >= 0)
		cst[index] = value;
	else {
		fprintf(stderr, "The impossible happened. Call Bob !\n");
		exit(1);
	}
}

void ProcessStack(int32 cur_ass, ass_item *ass_list, ass_item *ass) {
	uint8		type;
	int32		ass_index;
	int32		stack_delta, stack_delta_max;
	ass_item	*ass1;
	
	type = POP_PUSH;
	stack_delta = 0;
	stack_delta_max = 0;
	for (ass_index = cur_ass-1; ass_index >= 0; ass_index--) {
		ass1 = ass_list+ass_index;
		/* look for the FDEF */
		if (ass1->ref->first == 0x2c)
			break;
		/* undefined pop/push instruction */
		if ((ass1->pp_type&15) == UNKNOWN) {
			ass->pp_type = UNKNOWN;
			return;
		}	
		/* simulate stack moves */
		stack_delta += ass1->push;
		if (stack_delta > stack_delta_max)
			stack_delta_max = stack_delta;
		stack_delta -= ass1->pop;	
		type |= (ass1->pp_type&UNCERTAIN);
	}
	ass->pp_type = type;
	ass->pop = stack_delta_max-stack_delta; 
	ass->push = stack_delta_max;
}

void print_param(char *str, uint8 type, int32 param) {
	if ((type&15) == UNKNOWN)
		sprintf(str, "?");
	else if ((type&15) == CALL_INDEX)
		sprintf(str, "[%d]", param);
	else if ((type&15) == LABEL_STACK) {
		sprintf(str, "[L%03d/%d]", (param>>16), param&0xffff);
	}
	else {
		sprintf(str, "%d", param);
		if (type & UNCERTAIN)
			sprintf(str, "?");
	}
}

void print_param_in_par(char *str, uint8 type, int32 param) {
	sprintf(str, "(");
	print_param(str+1, type, param);
	str += strlen(str);
	sprintf(str, ")");
}

void print_label(char *str, uint8 type, int32 param) {
	if ((type&15) == UNKNOWN)
		sprintf(str, "L?");
	else {
		sprintf(str, "L%03d", param);
		if (type & UNCERTAIN)
			sprintf(str+4, "?");
	}
}

bool dump_code(	uint8		*instr,
				uint8		*instr_end,
				ass_item	*ass_list,
				uint32		*cst,
				uint8		*cst_status,
				func		*func_table,
				int32		*cur_ass2,
				int32		*cur_cst2,
				int32		*cur_label2,
				int32		*cur_level2,
				int32		*cur_offset2,
				int32		call_level) {
	uint8			code;
	int32			i, j, k, tmp;
	uint8			*cur_instr;
	int32			cur_ass, cur_cst, cur_level;	
	int32			cur_label, cur_func, cur_offset;
	ass_item		*ass1, *ass2;

	parsing_error = false;

	cur_func = -1;
	cur_instr = instr;
	cur_ass = *cur_ass2;
	cur_cst = *cur_cst2;
	cur_level = *cur_level2;
	cur_label = *cur_label2;
	cur_offset = *cur_offset2;

	/* reserve its offset space */
	cur_offset += instr_end - instr;

	/* first parsing pass */
	while (cur_instr < instr_end) {
		code = *cur_instr;
		for (i=0;;i++) if ((instructions[i].first == code) ||
			((instructions[i].first < code) && (instructions[i].last >= code))) {
			/* create the new instruction descriptor */			
			ass1 = ass_list+cur_ass;
			ass1->level = cur_level;
			ass1->label_id = 0;
			ass1->offset = (cur_instr-instr)+*cur_offset2;
			ass1->ref = instructions+i;
			ass1->cst_count = 0;
			ass1->cst_offset = cur_cst;
			ass1->instr = cur_instr;
			ass1->types[0] = UNKNOWN;
			ass1->types[1] = UNKNOWN;
			ass1->types[2] = UNKNOWN;
			ass1->types[3] = UNKNOWN;
			ass1->pop = ass1->ref->pop;
			ass1->push = ass1->ref->push;
			ass1->call_level = call_level;
			if (ass1->ref->pop < 0)
				ass1->pp_type = UNKNOWN;
			else
				ass1->pp_type = POP_PUSH;
			/* special program */
			switch (ass1->ref->special) {
			case 11:	/* NPUSHB */
				ass1->cst_count = cur_instr[1];
				for (j=0; j<ass1->cst_count; j++) {
					cst[j+ass1->cst_offset] = (uint32)cur_instr[j+2];
					cst_status[j+ass1->cst_offset] = UNKNOWN;
				}
				cur_cst += ass1->cst_count;
				cur_instr += ass1->cst_count+1;
				ass1->pop = 0;
				ass1->push = ass1->cst_count;
				ass1->pp_type = POP_PUSH;
				break;
			case 12:	/* NPUSHW */
				ass1->cst_count = cur_instr[1];
				for (j=0; j<ass1->cst_count; j++) {
					cst[j+ass1->cst_offset] =
						(((int32)((int8)cur_instr[2*j+2]))<<8)+cur_instr[2*j+3];
					cst_status[j+ass1->cst_offset] = UNKNOWN;
				}
				cur_cst += ass1->cst_count;
				cur_instr += ass1->cst_count*2+1;
				ass1->pop = 0;
				ass1->push = ass1->cst_count;
				ass1->pp_type = POP_PUSH;
				break;
			case 13:	/* PUSHB */
				ass1->cst_count = code-0xAF;
				for (j=0; j<ass1->cst_count; j++) {
					cst[j+ass1->cst_offset] = (uint32)cur_instr[j+1];
					cst_status[j+ass1->cst_offset] = UNKNOWN;
				}
				cur_cst += ass1->cst_count;
				cur_instr += ass1->cst_count;
				ass1->pop = 0;
				ass1->push = ass1->cst_count;
				ass1->pp_type = POP_PUSH;
				break;
			case 14:	/* PUSHW */
				ass1->cst_count = code-0xB7;
				for (j=0; j<ass1->cst_count; j++) {
					cst[j+ass1->cst_offset] =
						(((int32)((int8)cur_instr[2*j+1]))<<8)+cur_instr[2*j+2];
					cst_status[j+ass1->cst_offset] = UNKNOWN;
				}
				cur_cst += ass1->cst_count;
				cur_instr += ass1->cst_count*2;
				ass1->pop = 0;
				ass1->push = ass1->cst_count;
				ass1->pp_type = POP_PUSH;
				break;
			case 15:	/* SLOOP */
				GetParams(	cur_ass, cst, ass_list, &cur_label,
							-1, ass1, 0, LOOP_COUNT);
				ass1->pop = 1;
				ass1->push = 0;
				ass1->pp_type = POP_PUSH;
				break;
			case 16:	/* SHPIX */
				if (cur_ass > 0) {
					ass2 = ass_list+(cur_ass-1);
					if (ass2->ref->first == 0x17) {
						if ((ass2->types[0]&15) == LOOP_COUNT) {
							ass1->pop = 1+ass2->params[0];
							ass1->push = 0;
							ass1->pp_type = (ass2->types[0]&UNCERTAIN) | POP_PUSH;
						}
						else
							ass1->pp_type = UNKNOWN;
					}
					else {
						ass1->pop = 2;
						ass1->push = 0;
						ass1->pp_type = POP_PUSH;
					}
				}
				else {
					ass1->pop = 2;
					ass1->push = 0;
					ass1->pp_type = POP_PUSH;
				}
				break;
			case 17:	/* DELTAPx */
				GetParams(	cur_ass, cst, ass_list, &cur_label,
							-1, ass1, 0, LOOP_COUNT);
				if ((ass1->types[0]&15) == LOOP_COUNT) {
					ass1->pop = 1+2*ass1->params[0];
					ass1->push = 0;
					ass1->pp_type = (ass1->types[0]&UNCERTAIN) | POP_PUSH;
				}
				else
					ass1->pp_type = UNKNOWN;
				break;
			case 18:	/* DELTACx */
				GetParams(	cur_ass, cst, ass_list, &cur_label,
							-1, ass1, 0, LOOP_COUNT);
				if ((ass1->types[0]&15) == LOOP_COUNT) {
					ass1->pop = 1+2*ass1->params[0];
					ass1->push = 0;
					ass1->pp_type = (ass1->types[0]&UNCERTAIN) | POP_PUSH;
					for (j=0; j<ass1->params[0]; j++)
						MarkCst(cur_ass, cst_status, cst, ass_list, &cur_label, 
								-2*(j+1), ass1, 0, UNKNOWN, CVT_INDEX);
				}
				else
					ass1->pp_type = UNKNOWN;
				break;
			case 19:	/* JROx */
				GetParams(	cur_ass, cst, ass_list, &cur_label,
							-1, ass1, 0, JUMP_OFFSET);
				if ((ass1->types[0]&15) == JUMP_OFFSET)
					ass1->params[0]++;
				break;
			case 20:	/* IF */
				cur_level++;
				break;
			case 21:	/* ELSE */
				ass1->level--;
				break;
			case 22:	/* ENDIF */
				cur_level--;
				ass1->level--;
				break;
			case 23:	/* ALIGNRP, FLIPPT, IP or SHP */
				if (cur_ass > 0) {
					ass2 = ass_list+(cur_ass-1);
					if (ass2->ref->first == 0x17) {
						if ((ass2->types[0]&15) == LOOP_COUNT) {
							ass1->pop = ass2->params[0];
							ass1->push = 0;
							ass1->pp_type = (ass2->types[0]&UNCERTAIN) | POP_PUSH;
						}
						else
							ass1->pp_type = UNKNOWN;
					}
					else {
						ass1->pop = 1;
						ass1->push = 0;
						ass1->pp_type = POP_PUSH;
					}
				}
				else {
					ass1->pop = 1;
					ass1->push = 0;
					ass1->pp_type = POP_PUSH;
				}
				break;
			case 24:	/* FDEF */
				cur_level++;
				MarkCst(cur_ass, cst_status, cst, ass_list, &cur_label,
						-1, ass1, 0, FCT_INDEX, FCT_INDEX);
				if ((ass1->types[0]&15) == FCT_INDEX) {
					if (func_table[ass1->params[0]].first == NULL) {
						func_table[ass1->params[0]].first = cur_instr+1;
						cur_func = ass1->params[0]; 
					}
				}
				break;
			case 25:	/* ENDF */
				if (cur_func >= 0) {
					func_table[cur_func].last = cur_instr;
					cur_func = -1;
				}
				ProcessStack(cur_ass, ass_list, ass1);
				cur_level--;
				ass1->level--;
				break;
			case 26:	/* LOOPCALL */
				MarkCst(cur_ass, cst_status, cst, ass_list, &cur_label,
						-1, ass1, 0, FCT_INDEX, FCT_INDEX);
				GetParams(	cur_ass, cst, ass_list, &cur_label,
							-2, ass1, 1, LOOP_COUNT);
				if (((ass1->types[0]&15) == FCT_INDEX) &&
					((ass1->types[1]&15) == LOOP_COUNT) &&
					(call_level < 32))
					if (func_table[ass1->params[0]].first != NULL) {
						ass1->pp_type = POP_PUSH | (ass1->types[0]&UNCERTAIN)
												 | (ass1->types[1]&UNCERTAIN);
						ass1->pop = 2;
						ass1->push = 0;
						cur_level++;
						cur_ass++;
						for (j=0; j<ass1->params[1]; j++)
							dump_code(	func_table[ass1->params[0]].first,
									  	func_table[ass1->params[0]].last,
										ass_list,
										cst,
										cst_status,
										func_table,
										&cur_ass,
										&cur_cst,
										&cur_label,
										&cur_level,
										&cur_offset,
										call_level+1);
						cur_level--;
						ass2 = ass_list+cur_ass;
						ass2->level = cur_level;
						ass2->label_id = 0;
						ass2->offset = 0x7fffffff;
						ass2->ref = instructions+END_CALL_INDEX;
						ass2->cst_count = 0;
						ass2->cst_offset = cur_cst;
						ass2->types[0] = UNKNOWN;
						ass2->types[1] = UNKNOWN;
						ass2->types[2] = UNKNOWN;
						ass2->types[3] = UNKNOWN;
						ass2->call_level = call_level+1;
						ass2->pp_type = UNKNOWN;
					}
				break;
			case 27:	/* CALL */
				MarkCst(cur_ass, cst_status, cst, ass_list, &cur_label,
						-1, ass1, 0, FCT_INDEX, FCT_INDEX);
				if (((ass1->types[0]&15) == FCT_INDEX) && (call_level < 32))
					if (func_table[ass1->params[0]].first != NULL) {
						ass1->pp_type = POP_PUSH | (ass1->types[0]&UNCERTAIN);
						ass1->pop = 1;
						ass1->push = 0;
						cur_level++;
						cur_ass++;
						dump_code(	func_table[ass1->params[0]].first,
								  	func_table[ass1->params[0]].last,
									ass_list,
									cst,
									cst_status,
									func_table,
									&cur_ass,
									&cur_cst,
									&cur_label,
									&cur_level,
									&cur_offset,
									call_level+1);
						cur_level--;
						ass2 = ass_list+cur_ass;
						ass2->level = cur_level;
						ass2->label_id = 0;
						ass2->offset = 0x7fffffff;
						ass2->ref = instructions+END_CALL_INDEX;
						ass2->cst_count = 0;
						ass2->cst_offset = cur_cst;
						ass2->types[0] = UNKNOWN;
						ass2->types[1] = UNKNOWN;
						ass2->types[2] = UNKNOWN;
						ass2->types[3] = UNKNOWN;
						ass2->call_level = call_level+1;
						ass2->pp_type = UNKNOWN;
					}
				break;
			case 28:	/* JMPR */
				GetParams(	cur_ass, cst, ass_list, &cur_label,
							-1, ass1, 0, JUMP_OFFSET);
				break;
			case 29:	/* SCANCTRL or SCANTYPE */
				GetParams(	cur_ass, cst, ass_list, &cur_label,
							-1, ass1, 0, PARAMETER);
				break;
			case 31:
				parsing_error = true;
				break;
			case 81:	/* CINDEX */
				GetParams(	cur_ass, cst, ass_list, &cur_label,
							-1, ass1, 0, STACK_INDEX);
				break;
			case 82:	/* MINDEX */
				GetParams(	cur_ass, cst, ass_list, &cur_label,
							-1, ass1, 0, STACK_INDEX);
				if ((ass1->types[0]&15) == STACK_INDEX) {
					ass1->pop = 1+ass1->params[0];
					ass1->push = ass1->params[0];
					ass1->pp_type |= (ass1->types[0]&UNCERTAIN);
				}
				else
					ass1->pp_type = POP_PUSH | UNCERTAIN;
				break;
			}
			/* Generic CVT feedback ? */
			if (ass1->ref->cvt_offset != 0)
				MarkCst(cur_ass, cst_status, cst, ass_list, &cur_label,
						ass1->ref->cvt_offset, ass1, 3, CVT_INDEX, CVT_INDEX);
			break;
		}
		/* next instruction... */
		cur_ass++;
		cur_instr++;
	}	

	/* second pass : tag labels */
	for (i=0; i<cur_ass; i++) switch (ass_list[i].ref->special) {
	case 19 :	/* JROx */
	case 28 :	/* JMPR */
		ass1 = ass_list+i;
		if ((ass1->types[0]&15) == JUMP_OFFSET) {
			tmp = ass1->offset+ass1->params[0];
			for (j=0; j<cur_ass; j++)
				if (ass_list[j].offset == tmp) {
					if (ass_list[j].label_id == 0)
						ass_list[j].label_id = cur_label++;
					ass1->params[1] = ass_list[j].label_id;
					ass1->types[1] = LABEL_ID | (ass1->types[0]&UNCERTAIN);
					break;
				}
		}
		break;
	}

	*cur_ass2 = cur_ass;
	*cur_cst2 = cur_cst;
	*cur_level2 = cur_level;
	*cur_label2 = cur_label;
	*cur_offset2 = cur_offset;
	return parsing_error;
}

void offset_cst(uint32		*cst,
				uint8		*cst_status,
				int32		cur_cst,
				int32		cvt_offset,
				int32		func_offset) {
	int32		i;
	
	for (i=0; i<cur_cst; i++) {
		if ((cst_status[i]&15) == CVT_INDEX)
			cst[i] += cvt_offset;
		else if ((cst_status[i]&15) == FCT_INDEX)
			cst[i] += func_offset;
	}
}

int32 evaluate_size(uint32 *cst, int32 count) {
	int32		i;
	
	for (i=0; i<count; i++) {
		if (cst[i] < 0)
			return 2;
		if (cst[i] > 255)
			return 2;
	}
	return 1;
}

void assemble_code(	ass_item	*ass,
					ass_item	*ass_end,
					uint32		*cst,
					uint8		**output2) {
	int32		i, j, k, offset, size, cur_label;
	int32		*label_refs;
	uint8		*output;
	ass_item	*ass1;

	output = *output2;

	label_refs = (int32*)malloc(1000*sizeof(int32));
	for (i=0; i<1000; i++)
		label_refs[i] = NULL;
		
	/* simulate code generation size and record label */
	offset = 0;
	ass1 = ass;
	while (ass1 < ass_end) {
		/* process only real instructions */
		if (ass1->call_level == 0) {
			/* record label reference */
			if (ass1->label_id > 0)
				label_refs[ass1->label_id] = offset;
			ass1->output_offset = offset;
			/* count the basic instruction */
			if (ass1->offset != 0x7fffffff)
				offset++;	
			/* do special processing if needed */
			switch (ass1->ref->special) {
			case 11:	/* NPUSHB */
				offset += 1+evaluate_size(cst+ass1->cst_offset, ass1->cst_count)*ass1->cst_count;
				break;
			case 12:	/* NPUSHW */
				offset += 1+2*ass1->cst_count;
				break;
			case 13:	/* PUSHB */
				offset += evaluate_size(cst+ass1->cst_offset, ass1->cst_count)*ass1->cst_count;
				break;
			case 14:	/* PUSHW */
				offset += 2*ass1->cst_count;
				break;
			}
		}
		ass1++;
	}
	
	/* check the JUMP_OFFSET constants */
	ass1 = ass;
	while (ass1 < ass_end) {
		/* process only real instructions */
		if (ass1->call_level == 0)
			switch (ass1->ref->special) {
			case 19:	/* JROx */
				offset = label_refs[ass1->params[1]]-ass1->output_offset-1;
				SetCst(ass1-ass, cst, ass, &cur_label, -1, offset); 
				break;
			case 28:	/* JMPR */
				offset = label_refs[ass1->params[1]]-ass1->output_offset;
				SetCst(ass1-ass, cst, ass, &cur_label, -1, offset); 
				break;
			}
		ass1++;
	}
	
	/* generate the code */
	ass1 = ass;
	while (ass1 < ass_end) {
		/* process only real instructions */
		if (ass1->call_level == 0) {
			/* count the basic instruction */
			if (ass1->offset != 0x7fffffff)
				*output++ = *ass1->instr;
			/* do special processing if needed */
			switch (ass1->ref->special) {
			case 11:	/* NPUSHB */
				if (evaluate_size(cst+ass1->cst_offset, ass1->cst_count) == 1) {
					*output++ = ass1->cst_count;
					for (j=0; j<ass1->cst_count; j++)
						*output++ = cst[ass1->cst_offset+j];
					break;
				}
				output[-1] = 0x41;
			case 12:	/* NPUSHW */
				*output++ = ass1->cst_count;
				for (j=0; j<ass1->cst_count; j++) {
					*output++ = cst[ass1->cst_offset+j]>>8;
					*output++ = cst[ass1->cst_offset+j]&0xff;
				}
				break;
			case 13:	/* PUSHB */
				if (evaluate_size(cst+ass1->cst_offset, ass1->cst_count) == 1) {
					for (j=0; j<ass1->cst_count; j++)
						*output++ = cst[ass1->cst_offset+j];
					break;
				}
				output[-1] += 8;
			case 14:	/* PUSHW */
				for (j=0; j<ass1->cst_count; j++) {
					*output++ = cst[ass1->cst_offset+j]>>8;
					*output++ = cst[ass1->cst_offset+j]&0xff;
				}
				break;
			}
		}
		ass1++;
	}		
	
	free(label_refs);
	
	*output2 = output;
}

void print_code(FILE		*fp,
				ass_item	*ass_list,
				int32		cur_ass,
				uint32		*cst,
				uint8		*cst_status) {
	char			buffer[4096];
	char			*str;
	uint8			code;
	int32			i, j, k, tmp;
	ass_item		*ass1, *ass2;

	/* generate the output file */
	ass1 = ass_list;
	fprintf(fp, "\n");
	for (i=0; i<cur_ass; i++) {
		/* init the char buffer */
		buffer[0] = 0;
		str = buffer;
		/* print the label if any */
		if (ass1->label_id > 0)
			fprintf(fp, "L%03d:\n", ass1->label_id);
		/* print main instruction label */
		for (j=0; j<ass1->level; j++) {			
			sprintf(str, "  ");
			str += strlen(str);
		}
		for (j=0; j<ass1->call_level; j++)
			buffer[j] = '|';
		sprintf(str, "%s", ass1->ref->name);
		str += strlen(str);
		/* print extra information */
		switch (ass1->ref->special) {
		case 11:	/* NPUSHB */
		case 12:	/* NPUSHW */
		case 13:	/* PUSHB */
		case 14:	/* PUSHW */
			sprintf(str, "(%d) :", ass1->cst_count);
			str += strlen(str);
			for (j=0; j<ass1->cst_count; j++) {
				sprintf(str, " %d", cst[ass1->cst_offset+j]);
				str += strlen(str);
				switch (cst_status[ass1->cst_offset+j]&15) {
				case CVT_INDEX :
					str[0] = 'c';
					str[1] = 0;
					str++;
					break;
				case FCT_INDEX :
					str[0] = 'f';
					str[1] = 0;
					str++;
					break;
				}
				if (cst_status[ass1->cst_offset+j]&UNCERTAIN) {
					str[0] = '?';
					str[1] = 0;
					str++;
				}
			}
			break;
		case 15:	/* SLOOP */
		case 17:	/* DELTAPx */
		case 18:	/* DELTACx */
		case 24:	/* FDEF */
		case 27:	/* CALL */
		case 29:	/* CALL */
		case 81:	/* CINDEX */
		case 82:	/* MINDEX */
			print_param_in_par(str, ass1->types[0], ass1->params[0]); 
			str += strlen(str);
			break;
		case 19:	/* JROx */
		case 28:	/* JMPR */
			print_param_in_par(str, ass1->types[0], ass1->params[0]); 
			str += strlen(str);
			sprintf(str, " ");
			print_label(str+1, ass1->types[1], ass1->params[1]);
			str += strlen(str);
			break;
		case 26:	/* LOOPCALL */
			sprintf(str, "(");
			print_param(str+1, ass1->types[1], ass1->params[1]); 
			str += strlen(str);
			sprintf(str, "x ");
			print_param(str+1, ass1->types[0], ass1->params[0]); 
			str += strlen(str);
			sprintf(str, ")");
			str++;
			break;
		}
		/* Generic CVT feedback ? */
		if (ass1->ref->cvt_offset != 0) {
			str[0] = '{';
			str[1] = 0;
			print_param(str+1, ass1->types[3], ass1->params[3]);
			str += strlen(str);
			str[0] = '}';
			str[1] = 0;
			str++;
		}
		/* print extra info in comments */
		tmp = strlen(buffer);
		if (tmp < 30) {
			for (j=tmp; j<30; j++)
				buffer[j] = ' ';
			buffer[30] = 0;
			str = buffer+30;
		} 
		if (ass1->offset != 0x7fffffff)
			sprintf(str, "  ## {%02x} ", *ass1->instr);
		else
			sprintf(str, "  ## {--} ");
		str += strlen(str);
		if ((ass1->pp_type&15) != UNKNOWN) {
			if (ass1->pop != 0) {
				sprintf(str, "POP %d", ass1->pop);
				str += strlen(str);
				if (ass1->pp_type&UNCERTAIN) {
					sprintf(str, "?");
					str += strlen(str);
				}
				sprintf(str, "; ");
				str += strlen(str);
			}
			if (ass1->push != 0) {
				sprintf(str, "PUSH %d", (uint32)ass1->push);
				str += strlen(str);
				if (ass1->pp_type&UNCERTAIN) {
					sprintf(str, "?");
					str += strlen(str);
				}
				sprintf(str, "; ");
				str += strlen(str);
			}
		}
		fprintf(fp, "%s%s\n", buffer, ass1->ref->comment);
		if (ass1->ref->special == 25)
			fprintf(fp, "\n");
		str += strlen(str);
		ass1++;	
	}
}
