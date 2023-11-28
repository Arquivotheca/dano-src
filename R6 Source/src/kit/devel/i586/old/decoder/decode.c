#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#ifndef _UNICODE
#define _T(x) x
#endif

#include "decver_inc.h"

#undef ISSET 		      		  /*** multiply define in x86.h ***/
#include "ops.h"
#pragma function (memset)

/* #define INT64 */
#include "iel.h"
#include "decp62.h"
#include "dec_priv.h"

#include "x86_inc.h"
#include "x86_type.h"
#include "x86_dec.h"


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

const _TCHAR ia_ver_string[] = VER;

IA_Decoder_Err tr_errors[] =
{
	IA_DECODER_NO_ERROR,
	IA_DECODER_INVALID_PRM_OPCODE,
	IA_DECODER_INVALID_PRM_OPCODE,
	IA_DECODER_INVALID_PRM_OPCODE,
	IA_DECODER_INVALID_PRM_OPCODE,
	IA_DECODER_TOO_LONG_OPCODE,
	IA_DECODER_LOCK_ERR,
	IA_DECODER_OPERAND_ERR,
	IA_DECODER_TOO_SHORT_ERR
};

IA_Decoder_Operand_Type tr_operand_type[] =
{
	IA_DECODER_NO_OPER,	    	   /* X86CT_OPPER_TYPE_NONE=0, */
	IA_DECODER_REGISTER,		   /* X86CT_OPER_TYPE_REG,	   */
	IA_DECODER_IMMEDIATE,		   /* X86CT_OPER_TYPE_IMM,	   */
	IA_DECODER_MEMORY,			   /* 86CT_OPER_TYPE_MEM,	   */
	IA_DECODER_REGISTER,		   /* X86CT_OPER_TYPE_ST,	   */
	IA_DECODER_REGISTER,		   /* X86CT_OPER_TYPE_MMREG,   */
	IA_DECODER_REGISTER,		   /* X86CT_OPER_TYPE_XMMREG,  */
	IA_DECODER_PORT,    		   /* X86CT_OPER_TYPE_PORT=7,  */
	IA_DECODER_IP_RELATIVE, 	   /* X86CT_OPER_TYPE_REL,	   */
	IA_DECODER_SEG_OFFSET		   /* X86CT_OPER_TYPE_SEG_OFF  */
};

IA_Decoder_Reg_Name tr_register[] =
{
	IA_DECODER_NO_REG,				/* NO_REG=0,	*/
	IA_DECODER_REG_EAX,		        /* X86_EAX,		*/
	IA_DECODER_REG_EBX,		        /* X86_EBX,		*/
	IA_DECODER_REG_ECX,		        /* X86_ECX,		*/
	IA_DECODER_REG_EDX,		        /* X86_EDX,		*/
	IA_DECODER_REG_ESI,		        /* X86_ESI,		*/
	IA_DECODER_REG_EDI,		        /* X86_EDI,		*/
	IA_DECODER_REG_EBP,		        /* X86_EBP,		*/
	IA_DECODER_REG_ESP,		        /* X86_ESP,		*/
	
	IA_DECODER_REG_CS,    			/* X86_CS,		*/
	IA_DECODER_REG_SS,		    	/* X86_SS,		*/
	IA_DECODER_REG_DS,			    /* X86_DS,		*/
	IA_DECODER_REG_ES,	    		/* X86_ES,		*/
	IA_DECODER_REG_FS,		    	/* X86_FS,		*/
	IA_DECODER_REG_GS,			    /* X86_GS,		*/
	
	IA_DECODER_REG_EFLAGS,		    /* X86_EFLAGS,	*/

	IA_DECODER_REG_CR0,		        /* X86_CR0,		*/
	IA_DECODER_REG_CR1,		        /* X86_CR1,		*/
	IA_DECODER_REG_CR2,		        /* X86_CR2,		*/
	IA_DECODER_REG_CR3,		        /* X86_CR3,		*/
	IA_DECODER_REG_CR4,		        /* X86_CR4,		*/
	
	IA_DECODER_REG_DR0,	        	/* X86_DR0,		*/
	IA_DECODER_REG_DR1,	        	/* X86_DR1,		*/
	IA_DECODER_REG_DR2,		        /* X86_DR2,		*/
	IA_DECODER_REG_DR3,		        /* X86_DR3,		*/
	IA_DECODER_REG_DR4,		        /* X86_DR4,		*/
	IA_DECODER_REG_DR5,		        /* X86_DR5,		*/
	IA_DECODER_REG_DR6,		        /* X86_DR6,		*/
	IA_DECODER_REG_DR7,		        /* X86_DR7,		*/
	
	IA_DECODER_REG_GDTR_BASE,	    /* X86_GDTR_BASE,  */
	IA_DECODER_REG_GDTR_LIMIT,      /* X86_GDTR_LIMIT, */
	IA_DECODER_REG_IDTR_BASE,	    /* X86_IDTR_BASE,  */
	IA_DECODER_REG_IDTR_LIMIT,      /* X86_IDTR_LIMIT, */
	IA_DECODER_REG_LDTR,		    /* X86_LDTR,	   */
	IA_DECODER_REG_TR,			    /* X86_TR,		   */
	IA_DECODER_REG_TR3,		        /* X86_TR3,		   */
	IA_DECODER_REG_TR4,		        /* X86_TR4,		   */
	IA_DECODER_REG_TR5,		        /* X86_TR5,		   */
	IA_DECODER_REG_TR6,		        /* X86_TR6,		   */
	IA_DECODER_REG_TR7,		        /* X86_TR7,		   */	 
	
	IA_DECODER_NO_REG,				/* Nx86REGS,	   */
	
	IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG,
	IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG,
	IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG,
	IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG,
	IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG,
	IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG,
	IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG,
	IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG,
	IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG,
	IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG,
	IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG,
	IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG,
	IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG,
	IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG,
	IA_DECODER_NO_REG, IA_DECODER_NO_REG, IA_DECODER_NO_REG,

	IA_DECODER_NO_REG,				/* XREGS=100, */
	
	IA_DECODER_REG_AX,			    /* X86_AX,	*/
	IA_DECODER_REG_BX,			    /* X86_BX,	*/
	IA_DECODER_REG_CX,			    /* X86_CX,	*/
	IA_DECODER_REG_DX,			    /* X86_DX,	*/
	IA_DECODER_REG_SI,			    /* X86_SI,	*/
	IA_DECODER_REG_DI,			    /* X86_DI,	*/
	IA_DECODER_REG_BP,			    /* X86_BP,	*/
	IA_DECODER_REG_SP,			    /* X86_SP,	*/

	IA_DECODER_NO_REG,				/* NXREGS,	*/
		
	IA_DECODER_NO_REG,				/* BREGS=110, */
	  
	IA_DECODER_REG_AL,			    /* X86_AL,	*/
	IA_DECODER_REG_BL,			    /* X86_BL,	*/
	IA_DECODER_REG_CL,			    /* X86_CL,	*/
	IA_DECODER_REG_DL,			    /* X86_DL,	*/
	IA_DECODER_REG_AH,			    /* X86_AH,	*/
	IA_DECODER_REG_BH,			    /* X86_BH,	*/
	IA_DECODER_REG_CH,			    /* X86_CH,	*/
	IA_DECODER_REG_DH,			    /* X86_DH,	*/
	IA_DECODER_NO_REG,				/* NBREGS,	*/

	IA_DECODER_NO_REG,				/* MMREGS=120,	*/
	IA_DECODER_REG_MM0,		        /* X86_MM0,		*/
	IA_DECODER_REG_MM1,		        /* X86_MM1,		*/
	IA_DECODER_REG_MM2,		        /* X86_MM2,		*/
	IA_DECODER_REG_MM3,		        /* X86_MM3,		*/
	IA_DECODER_REG_MM4,		        /* X86_MM4,		*/
	IA_DECODER_REG_MM5,		        /* X86_MM5,		*/
	IA_DECODER_REG_MM6,		        /* X86_MM6,		*/
	IA_DECODER_REG_MM7,		        /* X86_MM7,		*/
	IA_DECODER_NO_REG,				/* NMMREGS		*/

	  
};

IA_Decoder_Reg_Name tr_fp_register[] =
{
	IA_DECODER_REG_ST0,			 /* X86_ST0,	 */
	IA_DECODER_REG_ST1,			 /* X86_ST1,	 */
	IA_DECODER_REG_ST2,			 /* X86_ST2,	 */
	IA_DECODER_REG_ST3,			 /* X86_ST3,	 */
	IA_DECODER_REG_ST4,			 /* X86_ST4,	 */
	IA_DECODER_REG_ST5,			 /* X86_ST5,	 */
	IA_DECODER_REG_ST6,			 /* X86_ST6,	 */
	IA_DECODER_REG_ST7				 /* X86_ST7,	 */
};
	
	
static IA_Decoder_Err x86_decoding(const IA_Decoder_Id, const unsigned char *,
								   const int, IA_Decoder_Info *);

/********************************************************************************/
/* ia_decoder_open: opens a new entry in the ia_clients_table and returns the	*/
/*				 index of the entry.										    */
/********************************************************************************/

IA_Decoder_Id ia_decoder_open(void)
{
	int i;
	Client_Entry initiate_entry={1,
								 DEFAULT_MACHINE_TYPE, 
								 DEFAULT_MACHINE_MODE,
								 NULL
								};
	
	for (i=0 ; i < IA_DECODER_MAX_CLIENTS ; i++)
	{
		if ( !(ia_clients_table[i].is_used) )
		{
			ia_clients_table[i] = initiate_entry; 
			return(i);
		}
	}
	return(-1);
}

/*****************************************************************************/
/* legal_id: check whether a given id suits an active entry in the			 */
/*	 clients table.															 */
/*****************************************************************************/

static int legal_id(int id)
{
	if ((id<0)||(id>=IA_DECODER_MAX_CLIENTS))
	{
		return(FALSE);
	}
	if (!ia_clients_table[id].is_used)
	{
		return(FALSE);
	}
	return(TRUE);
}

/*****************************************************************************/
/* ia_decoder_close: closes an entry in the clients table for later use.	 */
/*****************************************************************************/

IA_Decoder_Err ia_decoder_close(const IA_Decoder_Id id)
{
	if (legal_id(id))
	{
		ia_clients_table[id].is_used=0;
		if (ia_clients_table[id].info_ptr != NULL)
		{
			free(ia_clients_table[id].info_ptr);
		}
		return(IA_DECODER_NO_ERROR);
	}
	else
	{
		return(IA_DECODER_INVALID_CLIENT_ID);
	}
}

/*****************************************************************************/
/* legal_type:																 */
/*****************************************************************************/

static int legal_type(IA_Decoder_Machine_Type type)
{
	if (type < IA_DECODER_CPU_LAST)
	{
		return(TRUE);
	}
	return(FALSE);
}

/*****************************************************************************/
/* legal_mode:																 */
/*****************************************************************************/

static int legal_mode(IA_Decoder_Machine_Mode mode)
{
	if (mode == IA_DECODER_MODE_NO_CHANGE)
	{
		return(TRUE);
	}

	if ((mode > IA_DECODER_MODE_NO_CHANGE) && (mode < IA_DECODER_MODE_LAST))
	{
		return(TRUE);
	}
	return(FALSE);
}

/*****************************************************************************/
/* legal_inst:																 */
/*****************************************************************************/

static int legal_inst(IA_Decoder_Inst_Id inst)
{
	if ((inst > X86_FIRST_INST) && (inst < X86_LAST_INST))
	{
		return(TRUE);
	}
	return(FALSE);
}

/****************************************************************************/
/* ia_decoder_setenv: sets the machine type and machine mode variables.		*/
/****************************************************************************/

IA_Decoder_Err ia_decoder_setenv(const IA_Decoder_Id		    id,
								 const IA_Decoder_Machine_Type  type,
								 const IA_Decoder_Machine_Mode  mode)
{
	if (!legal_id(id))
	{
		return(IA_DECODER_INVALID_CLIENT_ID);
	}

	if (!legal_type(type))
	{
		return(IA_DECODER_INVALID_MACHINE_TYPE);
	}

	if (!legal_mode(mode))
	{
		return(IA_DECODER_INVALID_MACHINE_MODE);
	}

	if (type == IA_DECODER_CPU_DEFAULT)
	{
		ia_clients_table[id].machine_type = DEFAULT_MACHINE_TYPE;
	}
	else if (type != IA_DECODER_CPU_NO_CHANGE)
	{
		ia_clients_table[id].machine_type = type;
	}

	if (mode == IA_DECODER_MODE_DEFAULT)
	{
		ia_clients_table[id].machine_mode = DEFAULT_MACHINE_MODE;
	}
	else if (mode != IA_DECODER_MODE_NO_CHANGE)
	{
		ia_clients_table[id].machine_mode = mode;
	}

	return(IA_DECODER_NO_ERROR);
}

/********************************************************************************/
/* ia_decoder_associate_one: adds to the client's entry a pointer to an extra	*/
/* information about a single instruction (inst).							    */
/********************************************************************************/

IA_Decoder_Err ia_decoder_associate_one(const IA_Decoder_Id		 id,
										const IA_Decoder_Inst_Id inst,
										const void *			 client_info)
{
	int		i;
	int		n_insts;
	
	if (!legal_id(id))
	{
		return(IA_DECODER_INVALID_CLIENT_ID);
	}

	if (!legal_inst(inst))
	{
		return(IA_DECODER_INVALID_INST_ID);
	}
	{
		n_insts = IA_DECODER_LAST_INST;
	}
	if (ia_clients_table[id].info_ptr == NULL)
	{
		ia_clients_table[id].info_ptr = calloc((size_t)n_insts, 
						    sizeof(void *));
		for (i=0 ; i < n_insts ; i++)
		{
			ia_clients_table[id].info_ptr[i] = NULL;
		}
	}
	ia_clients_table[id].info_ptr[inst] = (void *)client_info;
	return(IA_DECODER_NO_ERROR);
}


/***************************************************************************** 
 * ia_decoder_associate_check - check the client's array of association		 * 
 *							 valid for P7 cpu only							 * 
 *****************************************************************************/

IA_Decoder_Err ia_decoder_associate_check(const IA_Decoder_Id  id,
										  IA_Decoder_Inst_Id * inst)
{
	IA_Decoder_Inst_Id i;
	
	if(!legal_id(id))
	{
		return(IA_DECODER_INVALID_CLIENT_ID);
	}
	for (i = X86_FIRST_INST + 1 ; i < X86_LAST_INST ; i++)
	{
		if (ia_clients_table[id].info_ptr[i] == NULL)	/*** && (...) ) ***/
		{
			*inst=i;
			return(IA_DECODER_ASSOCIATE_MISS);
		}
	}
	*inst = IA_DECODER_INST_NONE;
	return(IA_DECODER_NO_ERROR);
}

/******************************************************************************
 * ia_decoder_decode                                   						  *
 *																			  *
 * params:																	  *
 *			id - ia_decoder client id										  *
 *			code - pointer to instruction buffer							  * 
 *			max_code_size - instruction buffer size							  *
 *			ia_decoder_info - pointer to ia_decoder_info to fill			  *
 *																			  *
 * returns:																	  *
 *			IA_Decoder_Err													  *
 *																			  *
 ******************************************************************************/

IA_Decoder_Err ia_decoder_decode(const IA_Decoder_Id	 id,
								 const unsigned char *   code,
								 int					 max_code_size,
								 IA_Decoder_Info *		 ia_decoder_info)
{
	IA_Decoder_Err		err;

	if (!legal_id(id))
	{
		return(IA_DECODER_INVALID_CLIENT_ID);
	}

	if (ia_decoder_info == NULL)
	{
		return(IA_DECODER_NULL_PTR);
	}

	if (code == NULL)
	{
		return(IA_DECODER_TOO_SHORT_ERR);
	}

	memset(ia_decoder_info, 0, sizeof(IA_Decoder_Info));

	err = x86_decoding(id, code, max_code_size, ia_decoder_info);
	return(err);
}

/*****************************************************************************/
/* ia_decoder_inst_static_info: return instruction static info (flags and	 */
/*							 client_info pointer)							 */
/*****************************************************************************/

IA_Decoder_Err ia_decoder_inst_static_info(const IA_Decoder_Id			 id,
										   const IA_Decoder_Inst_Id		 inst_id,
										   IA_Decoder_Inst_Static_Info * static_info)
{
	if (!legal_id(id))
	{
		return(IA_DECODER_INVALID_CLIENT_ID);
	}

	if (!legal_inst(inst_id))
	{
		return(IA_DECODER_INVALID_INST_ID);
	}

	if (static_info == NULL)
	{
		return(IA_DECODER_NULL_PTR);
	}

	if (ia_clients_table[id].info_ptr != NULL)
	{
		static_info->client_info = ia_clients_table[id].info_ptr[inst_id];
	}
	else
	{
		static_info->client_info = NULL;
	}
	return(IA_DECODER_NO_ERROR); /*** something fishy here !! ***/
}

/******************************************************************************/

static	X86CT_Decode_errno	  set_x86_env(IA_Decoder_Id id)
{
	unsigned long	  		machine_type = 0;
	X86CT_Active_oper_size	oper_size;

	oper_size = (ia_clients_table[id].machine_mode < IA_DECODER_MODE_PROTECTED_32) ?
				X86CT_ACTIVE_OPER_SIZE_WORD : X86CT_ACTIVE_OPER_SIZE_LONG;

	switch (ia_clients_table[id].machine_mode)
	{
		case IA_DECODER_MODE_86:
		case IA_DECODER_MODE_BIG_REAL:
			machine_type |= X86CT_FLAG_8086;
			break;

		case IA_DECODER_MODE_V86:
			machine_type |= X86CT_FLAG_V86;
			break;

		default:
			/*** the other modes do not impact old ia_decoder machine variable ***/
			break;
	}

	switch (ia_clients_table[id].machine_type)
	{
		case IA_DECODER_CPU_PENTIUM:
			machine_type |= X86CT_FLAG_P5;
			break;

		case IA_DECODER_CPU_P6:
			machine_type |= X86CT_FLAG_P6;
			break;
		case IA_DECODER_CPU_P5MM:
			machine_type |= X86CT_FLAG_P5MM;
			break;
		case IA_DECODER_CPU_P6MM:
			machine_type |= X86CT_FLAG_P6MM;
			break;
		case IA_DECODER_CPU_P7:
			machine_type |= X86CT_FLAG_P7;
			break;
		default:
			break;
	}

	return( X86_decoder_set_decoder(machine_type, oper_size) );
}


/******************************************************************************/

static	void	handle_x86_prefixes(X86CT_Decoder_info *d_info,
									IA_Decoder_Info *ia_decoder_info)
{
	switch (d_info->prefix_info.repeat_type)
	{
	  case PREFIX_REP:		/*** defined in src/include/x86.h ***/
		ia_decoder_info->prefix_info.repeat_type = IA_DECODER_REPE;
		break;
	  case PREFIX_REPNE:
		ia_decoder_info->prefix_info.repeat_type = IA_DECODER_REPNE;
		break;
	  default:
		ia_decoder_info->prefix_info.repeat_type = IA_DECODER_REP_NONE;
		break;
	}

	ia_decoder_info->prefix_info.n_prefixes = d_info->prefix_info.n_prefixes;
	ia_decoder_info->prefix_info.n_rep_pref = d_info->prefix_info.n_rep_pref;
	ia_decoder_info->prefix_info.n_lock_pref= d_info->prefix_info.n_lock_pref;
	ia_decoder_info->prefix_info.n_seg_pref = d_info->prefix_info.n_seg_pref;
	ia_decoder_info->prefix_info.n_oper_size_pref =	d_info->prefix_info.n_oper_size_pref;
	ia_decoder_info->prefix_info.segment_register = tr_register[d_info->seg];
	return;
}
/******************************************************************************/								  
static	void	fill_x86_operand(X86CT_Decoder_info	  *	 d_info,
								 X86CT_Oper_info	  *	 old_oper,
								 IA_Decoder_Operand_Info *	 new_oper)
{
	UL reg;
	
	new_oper->type = tr_operand_type[old_oper->type];
	new_oper->oper_2nd_role = IA_DECODER_OPER_2ND_ROLE_NONE;
	switch( old_oper->type )
	{
	  case X86CT_OPER_TYPE_REG:
		new_oper->reg_info.valid=1;
		reg=old_oper->value;
		new_oper->reg_info.name=tr_register[reg];
		switch(reg)
		{
		  case X86_EAX:
		  case X86_EBX:
		  case X86_ECX:
		  case X86_EDX:
		  case X86_ESI:
		  case X86_EDI:
		  case X86_EBP:
		  case X86_ESP:
			new_oper->reg_info.type = IA_DECODER_INT_REG;
			new_oper->reg_info.value = tr_register[reg] - IA_DECODER_REG_EAX;
			break;
			
		  case X86_DX:
			if (old_oper->flags & X86CT_OPER_FLAG_PORT_IN_DX)
			{
				new_oper->type=IA_DECODER_PORT_IN_DX;
			}
		  case X86_AX:
		  case X86_BX:
		  case X86_CX:
		  case X86_SI:
		  case X86_DI:
		  case X86_BP:
		  case X86_SP:
			new_oper->reg_info.type = IA_DECODER_INT_REG;
			new_oper->reg_info.value = tr_register[reg] - IA_DECODER_REG_AX;
			break;

		  case X86_AL:
		  case X86_BL:
		  case X86_CL:
		  case X86_DL:
		  case X86_AH:
		  case X86_BH:
		  case X86_CH:
		  case X86_DH:
			new_oper->reg_info.type = IA_DECODER_INT_REG;
			new_oper->reg_info.value = tr_register[reg] - IA_DECODER_REG_AL;
			break;

		  
		  case X86_CS:
		  case X86_SS:
		  case X86_DS:
		  case X86_ES:
		  case X86_FS:
		  case X86_GS:
			new_oper->reg_info.type = IA_DECODER_SEG_REG;
			new_oper->reg_info.value = tr_register[reg] - IA_DECODER_REG_ES;
			break;

		  case X86_GDTR_BASE:
		  case X86_GDTR_LIMIT:
		  case X86_IDTR_BASE:
		  case X86_IDTR_LIMIT:
			new_oper->reg_info.type = IA_DECODER_SEG_REG;
			new_oper->reg_info.value =
							tr_register[reg]-IA_DECODER_REG_ESR_BASE+8;
			break;

		  case X86_LDTR:
			new_oper->reg_info.type = IA_DECODER_SEG_REG;
			new_oper->reg_info.value = 7;
			break;

		  case X86_CR0:
		  case X86_CR1:
		  case X86_CR2:
		  case X86_CR3:
		  case X86_CR4:
			new_oper->reg_info.type=IA_DECODER_CTRL_REG;
			new_oper->reg_info.value=tr_register[reg]-IA_DECODER_REG_CR0;
			break;

		  case X86_EFLAGS:
			new_oper->reg_info.type=IA_DECODER_CTRL_REG;
			new_oper->reg_info.value=0;
			break;

		  case X86_DR0:
		  case X86_DR1:
		  case X86_DR2:
		  case X86_DR3:
		  case X86_DR4:
		  case X86_DR5:
		  case X86_DR6:
		  case X86_DR7:
			new_oper->reg_info.type=IA_DECODER_DEBUG_REG;
			new_oper->reg_info.value=tr_register[reg]-IA_DECODER_REG_DR0;
			break;

		  case X86_TR:
			new_oper->reg_info.type=IA_DECODER_TASK_REG;
			new_oper->reg_info.value=0;
			break;

		  case X86_TR3:
		  case X86_TR4:
		  case X86_TR5:
		  case X86_TR6:
		  case X86_TR7:
			new_oper->reg_info.type=IA_DECODER_TASK_REG;
			new_oper->reg_info.value=tr_register[reg]-IA_DECODER_REG_TR3+3;
			break;
		}		 
		break;

	  case X86CT_OPER_TYPE_MM:
		new_oper->reg_info.valid=1;
		reg=old_oper->value;
		new_oper->reg_info.name=tr_register[reg];
		switch(reg)
		{
		  case X86_MM0:
		  case X86_MM1:
		  case X86_MM2:
		  case X86_MM3:
		  case X86_MM4:
		  case X86_MM5:
		  case X86_MM6:
		  case X86_MM7:
			new_oper->reg_info.type = IA_DECODER_MM_REG;
			new_oper->reg_info.value = tr_register[reg] - IA_DECODER_REG_MM0;
			break;
		}
		break;
		
	  case X86CT_OPER_TYPE_IMM:
		if (old_oper->flags & X86CT_OPER_FLAG_IMPLIED)
		{
			new_oper->type = IA_DECODER_CONST;
		}
		new_oper->imm_info.signed_imm=1;
		new_oper->imm_info.value=old_oper->value;
		if ((signed)new_oper->imm_info.value < 0)
		{
			IEL_CONVERT2(new_oper->imm_info.val64,
						 (unsigned int)(new_oper->imm_info.value),(unsigned int)(-1));
		} else
		{
			IEL_CONVERT2(new_oper->imm_info.val64, new_oper->imm_info.value, 0);
		}
		new_oper->imm_info.size = old_oper->size;
		break;
		
	  case X86CT_OPER_TYPE_ST:
		new_oper->reg_info.valid=1;
		reg=old_oper->value;
		new_oper->reg_info.name=tr_fp_register[reg];
		new_oper->reg_info.type=IA_DECODER_FP_REG;
		new_oper->reg_info.value=tr_fp_register[reg]-IA_DECODER_REG_ST0;
		break;
		
	  case X86CT_OPER_TYPE_PORT:
		new_oper->port_number=old_oper->value;
		reg=old_oper->value;
		break;
			
	  case X86CT_OPER_TYPE_MEM:
		if (old_oper->mem_seg)
		{
			new_oper->mem_info.mem_seg.valid = 1;
			new_oper->mem_info.mem_seg.type = IA_DECODER_SEG_REG;
			new_oper->mem_info.mem_seg.name =tr_register[old_oper->mem_seg];
			new_oper->mem_info.mem_seg.value=
			  tr_register[old_oper->mem_seg]-IA_DECODER_REG_ES;
		}
		
		new_oper->mem_info.mem_offset=old_oper->mem_offset;
		new_oper->mem_info.mem_off.value = old_oper->mem_offset;
		IEL_CONVERT1(new_oper->mem_info.mem_off.val64, old_oper->mem_offset);
		new_oper->mem_info.mem_off.signed_imm = 0;
		new_oper->mem_info.mem_off.size = old_oper->dis_size;

		if (old_oper->mem_base)
		{
			new_oper->mem_info.mem_base.valid = 1;
			new_oper->mem_info.mem_base.type  = IA_DECODER_INT_REG;
			new_oper->mem_info.mem_base.name  =
			  tr_register[old_oper->mem_base];

			if (d_info->address_size == X86CT_ACTIVE_ADDR_SIZE_WORD)
			{
				new_oper->mem_info.mem_base.value =
				  tr_register[old_oper->mem_base] - IA_DECODER_REG_AX;
			}
			else
			{
				new_oper->mem_info.mem_base.value =
				  tr_register[old_oper->mem_base] - IA_DECODER_REG_EAX;
			}
		}
		
		if (old_oper->mem_index)
		{
			new_oper->mem_info.mem_index.valid = 1;
			new_oper->mem_info.mem_index.type  = IA_DECODER_INT_REG;
			new_oper->mem_info.mem_index.name  =
			  tr_register[old_oper->mem_index];
			
			if (d_info->address_size == X86CT_ACTIVE_ADDR_SIZE_WORD)
			{
				new_oper->mem_info.mem_index.value =
						tr_register[old_oper->mem_index] - IA_DECODER_REG_AX;
			}
			else
			{
				new_oper->mem_info.mem_index.value =
				  tr_register[old_oper->mem_index] - IA_DECODER_REG_EAX;
			}
			new_oper->mem_info.mem_scale = old_oper->mem_scale;
		}

		switch(old_oper->size)
		{
		  case X86CT_OPER_SIZE_BYTE:
			new_oper->mem_info.size=IA_DECODER_OPER_SIZE_1;
			break;
			
		  case X86CT_OPER_SIZE_WORD:
			new_oper->mem_info.size=IA_DECODER_OPER_SIZE_2;
			break;
			
		  case X86CT_OPER_SIZE_LONG:
			new_oper->mem_info.size=IA_DECODER_OPER_SIZE_4;
			break;
			
		  case X86CT_OPER_SIZE_QUAD:
			new_oper->mem_info.size=IA_DECODER_OPER_SIZE_8;
			break;
			
		  case X86CT_OPER_SIZE_EXT:
			new_oper->mem_info.size=IA_DECODER_OPER_SIZE_10;
			break;

		  case X86CT_OPER_SIZE_DQUAD:
			new_oper->mem_info.size=IA_DECODER_OPER_SIZE_16;
			break;
			
		}
		break;
		
	  case X86CT_OPER_TYPE_REL:
		new_oper->ip_relative_offset=old_oper->value;
		new_oper->imm_info.value = old_oper->value;
		new_oper->imm_info.size = old_oper->size;
		break;
		
	  case X86CT_OPER_TYPE_SEG_OFF:
		new_oper->seg_offset_info.offset=old_oper->mem_offset;
		new_oper->seg_offset_info.segment_number=old_oper->value;
		new_oper->imm_info.size = old_oper->size;
		break;
		
	  default:
		break;
	}
	return;
}

/******************************************************************************
 * x86_decoding - decode x86 instruction									  *
 *																			  *
 * params:																	  *
 *			id - ia_decoder client id										  *
 *			code - pointer to instruction buffer							  * 
 *			max_code_size - instruction buffer size							  *
 *			ia_decoder_info - pointer to ia_decoder_info to fill			  *
 *																			  *
 * returns:																	  *
 *			IA_Decoder_Err													  *
 *																			  *
 ******************************************************************************/

static IA_Decoder_Err x86_decoding(const IA_Decoder_Id	  id,
								   const unsigned char *  code,
								   const int			  max_code_size,
								IA_Decoder_Info *		  ia_decoder_info)
{
	X86CT_Decode_errno	err;                     
	X86CT_Decoder_info	d_info;
	  
	if ((err = set_x86_env(id)) != X86CT_DECODE_NO_ERROR)
	{
		return(IA_DECODER_INTERNAL_ERROR);
	}

	err = X86_decoder_decode((unsigned char *)code, max_code_size, &d_info);

	/***  return error (if any) ***/
	if (err != X86CT_DECODE_NO_ERROR)
	{
		return(tr_errors[err]);
	}

	if (d_info.operand_size == X86CT_ACTIVE_OPER_SIZE_WORD)
	{
		ia_decoder_info->operand_size = IA_DECODER_OPER_SIZE_2;
		ia_decoder_info->imp_info =
			ia_inst_info[d_info.info->x86_enum - X86_FIRST_INST-1].stat_info_p_16;
	}
	else
	{
		ia_decoder_info->operand_size = IA_DECODER_OPER_SIZE_4;
		ia_decoder_info->imp_info =
			ia_inst_info[d_info.info->x86_enum - X86_FIRST_INST-1].stat_info_p_32;
	}

	ia_decoder_info->mrm_info.mrm_type = d_info.mrm_info.mrm_type;
	if (ia_decoder_info->mrm_info.mrm_type != IA_DECODER_MODRM_NONE)
	{
		ia_decoder_info->mrm_info.modrm = d_info.mrm_info.modrm;
		ia_decoder_info->mrm_info.mrm_opr_size = d_info.mrm_info.mrm_opr_size;
		ia_decoder_info->mrm_info.sib = d_info.mrm_info.sib;
	}

	ia_decoder_info->inst = d_info.info->x86_enum;
	ia_decoder_info->flags = d_info.info->flags & X86_USER_FLAGS;
	ia_decoder_info->ext_flags = d_info.info->ext_flags;
	ia_decoder_info->size = (unsigned char)d_info.inst_size;
	ia_decoder_info->opcode_type = d_info.opcode_type;
	handle_x86_prefixes(&d_info, ia_decoder_info);

	ia_decoder_info->implicit_oper_size = d_info.info->iopr_sz / 8;

	if (d_info.prefix_info.n_oper_size_pref & 1) /* odd number */
	{
		if (ia_decoder_info->implicit_oper_size == IA_DECODER_OPER_SIZE_2)
			 ia_decoder_info->implicit_oper_size = IA_DECODER_OPER_SIZE_4;
		else if  (ia_decoder_info->implicit_oper_size == IA_DECODER_OPER_SIZE_4)
			 ia_decoder_info->implicit_oper_size = IA_DECODER_OPER_SIZE_2;
	}

	ia_decoder_info->address_size =
			(d_info.address_size == X86CT_ACTIVE_ADDR_SIZE_WORD) ?
							IA_DECODER_ADDR_SIZE_16 : IA_DECODER_ADDR_SIZE_32;

	if (IA_DECODER_FLOAT(ia_decoder_info))
	{
		ia_decoder_info->fp_opcode = ( (d_info.opcode_1byte-0xd8) << 8 ) +
									d_info.opcode_2byte;
	}
	else
	{
		ia_decoder_info->fp_opcode = 0;
	}

	switch (OPERANDS(d_info.info))
	{
		case OPERANDS_NONE:
			SET_NONE(ia_decoder_info->src1);
			SET_NONE(ia_decoder_info->src2);
			SET_NONE(ia_decoder_info->dst1);
			break;

		case OPERANDS_SRC1:
			fill_x86_operand(&d_info, &d_info.operands[X86CT_OPER_DEST],
							 &ia_decoder_info->src1);
			SET_NONE(ia_decoder_info->src2);
			SET_NONE(ia_decoder_info->dst1);
			break;

		case OPERANDS_DEST:
			fill_x86_operand(&d_info, &d_info.operands[X86CT_OPER_DEST],
								&ia_decoder_info->dst1);
			SET_NONE(ia_decoder_info->src1);
			SET_NONE(ia_decoder_info->src2);
			break;

		case OPERANDS_SRC1nDEST:
			fill_x86_operand(&d_info, &d_info.operands[X86CT_OPER_DEST],
								&ia_decoder_info->dst1);
			ia_decoder_info->dst1.oper_2nd_role = IA_DECODER_OPER_2ND_ROLE_SRC;
			SET_NONE(ia_decoder_info->src1);
			SET_NONE(ia_decoder_info->src2);
			break;

		case OPERANDS_SRC1_SRC2:
			fill_x86_operand(&d_info, &d_info.operands[X86CT_OPER_DEST],
								&ia_decoder_info->src1);
			fill_x86_operand(&d_info, &d_info.operands[X86CT_OPER_SRC1],
								&ia_decoder_info->src2);
			SET_NONE(ia_decoder_info->dst1);
			break;

		case OPERANDS_DEST_SRC1:
			fill_x86_operand(&d_info, &d_info.operands[X86CT_OPER_DEST],
								&ia_decoder_info->dst1);
			fill_x86_operand(&d_info, &d_info.operands[X86CT_OPER_SRC1],
								&ia_decoder_info->src1);
			SET_NONE(ia_decoder_info->src2);
			break;

		case OPERANDS_SRC1nDEST_SRC2:
			fill_x86_operand(&d_info, &d_info.operands[X86CT_OPER_DEST],
								&ia_decoder_info->dst1);
			ia_decoder_info->dst1.oper_2nd_role = IA_DECODER_OPER_2ND_ROLE_SRC;
			fill_x86_operand(&d_info, &d_info.operands[X86CT_OPER_SRC1],
								&ia_decoder_info->src1);
			SET_NONE(ia_decoder_info->src2);
			break;

		case OPERANDS_DEST_SRC1_SRC2:
			fill_x86_operand(&d_info, &d_info.operands[X86CT_OPER_DEST],
								&ia_decoder_info->dst1);
			fill_x86_operand(&d_info, &d_info.operands[X86CT_OPER_SRC1],
								&ia_decoder_info->src1);
			fill_x86_operand(&d_info, &d_info.operands[X86CT_OPER_SRC2],
								&ia_decoder_info->src2);
			break;

		case OPERANDS_SRC1nDEST_SRC2_SRC3:
			fill_x86_operand(&d_info, &d_info.operands[X86CT_OPER_DEST],
								&ia_decoder_info->dst1);
			ia_decoder_info->dst1.oper_2nd_role = IA_DECODER_OPER_2ND_ROLE_SRC;
			fill_x86_operand(&d_info, &d_info.operands[X86CT_OPER_SRC1],
								&ia_decoder_info->src1);
			fill_x86_operand(&d_info, &d_info.operands[X86CT_OPER_SRC2],
								&ia_decoder_info->src2);
			break;

		case OPERANDS_SRC1nDST1_SRC2nDST2:
			fill_x86_operand(&d_info, &d_info.operands[X86CT_OPER_DEST],
								&ia_decoder_info->dst1);
			ia_decoder_info->dst1.oper_2nd_role = IA_DECODER_OPER_2ND_ROLE_SRC;
			fill_x86_operand(&d_info, &d_info.operands[X86CT_OPER_SRC1],
								&ia_decoder_info->src1);
			ia_decoder_info->src1.oper_2nd_role = IA_DECODER_OPER_2ND_ROLE_DST;
			SET_NONE(ia_decoder_info->src2);
			break;
	}

	if (ia_clients_table[id].info_ptr)
	{
		ia_decoder_info->client_info =
								ia_clients_table[id].info_ptr[ia_decoder_info->inst];
	}
	else
	{
		ia_decoder_info->client_info = NULL;
	}
	return(IA_DECODER_NO_ERROR);
}

/******************************************************************************/

/* Auxliary functions */

const _TCHAR* ia_decoder_ver_str()
{
	return(ia_ver_string);
}

IA_Decoder_Err ia_decoder_ver(long *major, long *minor)
{

	if ((major==NULL) ||
		(minor==NULL))
	{
		return(IA_DECODER_NULL_PTR);
	}
		  
	*major = API_MAJOR;
	*minor = API_MINOR;
	return(IA_DECODER_NO_ERROR);
}




