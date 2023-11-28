#ifndef _PARSER_H
#define _PARSER_H

#include "ttf.h"

typedef struct {
	uint8		first;
	uint8		last;
	char		name[14];
	int8		pop;
	int8		push;
	uint8		special;
	int8		cvt_offset;
	char		comment[44];
} byte_code;

typedef struct {
	int32		level;
	int32		label_id;		// label number (if > 0)
	int32		offset;			// offset in the instruction flow
	byte_code	*ref;			// pointer to the byte_code info
	int32		cst_count;		// count of constants pushed on the stack
	int32		cst_offset;		// offset of the contants in the contants table
	uint8		pop;			// equivalent pop count
	uint8		push;			// equivalent push count
	uint8		pp_type;		// certainty on pop/push 
	uint8		call_level;		// recursive function resolution level
	int32		params[4];		// params
	uint8		types[4];		// params types (when solved).
	uint8		*instr;			// pointer to the real instruction
	int32		output_offset;	// offset to the last output adress
} ass_item;

typedef struct {
	uint8		*first;
	uint8		*last;
} func;

#define	END_CALL_INDEX		127

#define	UNKNOWN		0
#define	LOOP_COUNT	1
#define CVT_INDEX	2
#define JUMP_OFFSET	3
#define	POP_PUSH	4
#define	FCT_INDEX	5
#define	LABEL_ID	6
#define	STACK_INDEX	7
#define	CALL_INDEX	8
#define	LABEL_STACK	9
#define	PARAMETER	10
#define	ASS_INDEX	11
#define	UNSOLVABLE	12

#define UNCERTAIN	16

int32 FindCst(	int32		cur_ass,
				uint32		*cst,
				ass_item	*ass_list,
				int32		*cur_label,
				int32		stack_offset,
				uint8		*type);
void GetParams(	int32		cur_ass,
				uint32		*cst,
				ass_item	*ass_list,
				int32		*cur_label,
				int32		stack_offset,
				ass_item	*ass,
				int32		p_index,
				uint8		type);
void MarkCst(	int32		cur_ass,
				uint8		*cst_status,
				uint32		*cst,
				ass_item	*ass_list,
				int32		*cur_label,
				int32		stack_offset,
				ass_item	*ass,
				int32		p_index,
				uint8		type,
				uint8		type_cst);
void SetCst(int32		cur_ass,
			uint32		*cst,
			ass_item	*ass_list,
			int32		*cur_label,
			int32		stack_offset,
			uint32		value);
void ProcessStack(int32 cur_ass, ass_item *ass_list, ass_item *ass);
void print_param(char *str, uint8 type, int32 param);
void print_param_in_par(char *str, uint8 type, int32 param);
void print_label(char *str, uint8 type, int32 param);
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
				int32		*cur_offset,
				int32		call_level);
void offset_cst(uint32		*cst,
				uint8		*cst_status,
				int32		cur_cst,
				int32		cvt_offset,
				int32		func_offset);
int32 evaluate_size(uint32 *cst, int32 count);
void assemble_code(	ass_item	*ass,
					ass_item	*ass_end,
					uint32		*cst,
					uint8		**output2);
void print_code(FILE		*fp,
				ass_item	*ass_list,
				int32		cur_ass,
				uint32		*cst,
				uint8		*cst_status);

#endif
