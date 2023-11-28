#include "stdlib.h"
#include "stdio.h"
#include "memory.h"
#include "string.h"

#ifndef OPTIMIZE
#undef  IEL_USE_FUNCTIONS
#define IEL_USE_FUNCTIONS
#endif

#ifndef _UNICODE
#define _tcscpy strcpy
#define _tcscmp strcmp
#define _tcscspn strcspn
#define _T(x) x
#else
#include <tchar.h>
#endif

#include "disver_inc.h"
#include "iel.h"
/* #define INT64 */
#include "disp62.h"
#include "dis_priv.h"
#include "stat.c"
#include "tool_x86.h"

#define PUT_FIELD(Type)                                  \
{                                                        \
    ia_dis_fields[ia_dis_last_field++].type = (Type);    \
}

U32  IEL_tempc;
U64  IEL_et1, IEL_et2;
U128 IEL_ext1, IEL_ext2, IEL_ext3, IEL_ext4, IEL_ext5;
S128 IEL_ts1, IEL_ts2;
/*****************************************************************************/
/***   Default values for Disassembler variables                           ***/
/*****************************************************************************/

static  IA_Decoder_Id              ia_dis_id           = -1;


static  IA_Decoder_Machine_Type    ia_dis_machine_type = IA_DECODER_CPU_DEFAULT;
static  IA_Decoder_Machine_Mode    ia_dis_machine_mode = IA_DECODER_MODE_PROTECTED_32;

static  long                       ia_dis_aliases      = IA_DIS_ALIAS_ALL_REGS;
static  IA_Dis_Radix               ia_dis_radix        = IA_DIS_RADIX_HEX;
static  IA_Dis_Style               ia_dis_style        = IA_DIS_STYLE_USL;
static  int                        ia_dis_last_field   = 0;
static  IA_Dis_Fields              ia_dis_fields;
static  int                        ia_dis_spaces = 1;
static  int                        prefix;
static  int                        siz, tabstop;
static  IA_Decoder_Inst_Id         instid;
static  IA_Dis_Err                 (*ia_dis_client_gen_sym)(U64 ,
															unsigned int,
															char *,
															int *,
															U64 *) = default_function;

/*****************************************************************************/
/***   ia_dis_setup function                                               ***/
/*****************************************************************************/

IA_Dis_Err     ia_dis_setup(const IA_Decoder_Machine_Type  type,
							const IA_Decoder_Machine_Mode  mode,
							const long                     aliases,
							const IA_Dis_Radix             radix,
							const IA_Dis_Style             style,
					        IA_Dis_Err               (*client_gen_sym)(U64,
																	   unsigned int,	
																	   char *,
																	   int *,
																	   U64 *))
{
    int i, do_assoc = 0;
    IA_Decoder_Err err;
    
    if (!legal_aliases(aliases)) 
    {
        return(IA_DIS_INVALID_ALIASES);
    }

	if (aliases == IA_DIS_ALIAS_TOGGLE_SPACE)
	{
		ia_dis_spaces = !ia_dis_spaces;
	} else
    if (aliases != IA_DIS_ALIAS_NO_CHANGE )
    {
        ia_dis_aliases = aliases;
    }

    if (!(legal_radix(radix)))
    {
        return(IA_DIS_INVALID_RADIX);
    }
    if (radix != IA_DIS_RADIX_NO_CHANGE )
    {
        ia_dis_radix = radix;
    }

    if ( !(legal_style(style)))
    {
        return(IA_DIS_INVALID_STYLE);
    }

    if (style != IA_DIS_STYLE_NO_CHANGE )
    {
        ia_dis_style = style;
    }

    /***   check on the function pointer   ***/
    if (client_gen_sym != IA_DIS_FUNC_NO_CHANGE )
    {
        ia_dis_client_gen_sym = client_gen_sym;
    }

    if ( ia_dis_id == -1 )      /*** ia_decoder never opened ***/
    {
        if ( (ia_dis_id = ia_decoder_open()) == -1)
        {
            return(IA_DIS_INTERNAL_ERROR);
        }
		do_assoc = 1;
    }

    if ((ia_dis_machine_type != type)      || (ia_dis_machine_mode != mode)      ||
        (type != IA_DECODER_CPU_NO_CHANGE) || (mode != IA_DECODER_MODE_NO_CHANGE)   )
    {
        int ia_dis_setenv_err;

        if (type != IA_DECODER_CPU_NO_CHANGE)
        {
            ia_dis_machine_type = type;
        }
        if (mode != IA_DECODER_MODE_NO_CHANGE)
        {
            ia_dis_machine_mode = mode;
        }
        ia_dis_setenv_err = ia_decoder_setenv(ia_dis_id, ia_dis_machine_type,
                                        ia_dis_machine_mode);
        switch (ia_dis_setenv_err)
        {
            case IA_DECODER_NO_ERROR:
                break;
            case IA_DECODER_INVALID_MACHINE_TYPE:
                return(IA_DIS_INVALID_MACHINE_TYPE);

            case IA_DECODER_INVALID_MACHINE_MODE:
                return(IA_DIS_INVALID_MACHINE_MODE);

            default:
                return(IA_DIS_INTERNAL_ERROR);
        }
    }

	if (do_assoc)
	{
        /* Associate mnemonics */
          for (i=0; (i<(sizeof(TOOL_X86CT_INST_INFO)/sizeof(TOOL_X86CT_INFO)));
               i++)
          {
              if (TOOL_X86CT_INST_INFO[i].inst_id ==  X86_ALIAS ||
				  TOOL_X86CT_INST_INFO[i].inst_id == IA_DECODER_INST_NONE)
				  continue;
              if ((err=ia_decoder_associate_one(ia_dis_id,
                         TOOL_X86CT_INST_INFO[i].inst_id,
                         &(TOOL_X86CT_INST_INFO[i]))) != IA_DECODER_NO_ERROR)
                  {
                      return(IA_DIS_INTERNAL_ERROR);
                  }
          }
	}
    return(IA_DIS_NO_ERROR);
}

/*****************************************************************************/
/***   ia_dis_fill_string function                                         ***/
/*****************************************************************************/

static void fill_string(char fill_character, char *str, int num, int at_least)
{
    int i, len;

    if ((int)strlen(str)+at_least > num)
    {
        len = strlen(str);
        for(i = len; i<len+at_least; i++)
          str[i] = fill_character;
        str[i]='\0';
        return;
    }

    for(i=strlen(str); i<num; i++)
      str[i] = fill_character;
    str[i]='\0';
}



/*****************************************************************************/
/***   string_U64                                                          ***/
/*****************************************************************************/

static void string_U64(U64 num, char *str)
{
    switch(ia_dis_radix)
    {
      case IA_DIS_RADIX_BINARY:
        strcpy(str,"0b");
        IEL_U64tostr(&num, str+2, IEL_BIN, 134);
        break;
      case IA_DIS_RADIX_OCTAL:
        strcpy(str,"0");
        IEL_U64tostr(&num, str+1, IEL_OCT, 134);
        break;
      case IA_DIS_RADIX_DECIMAL:
        IEL_S64tostr((S64*)&num, str, IEL_DEC, 134);
        break;
      case IA_DIS_RADIX_HEX:
	  case IA_DIS_RADIX_HEX_FULL:
        {
            strcpy(str,"0x");
            IEL_U64tostr(&num, str+2, IEL_HEX, 134); 
        }
        break;
    }
}





/*****************************************************************************/
/***   string_S64                                                          ***/
/***   convert S64 to string,  as signed                                   ***/
/*****************************************************************************/

static void string_S64(U64 num, char *str)
{
    switch(ia_dis_radix)
    {
      case IA_DIS_RADIX_BINARY:
        strcpy(str,"0b");
        IEL_U64tostr(&num, str+2, IEL_BIN, 134);
        break;
      case IA_DIS_RADIX_OCTAL:
        strcpy(str,"0");
        IEL_U64tostr(&num, str+1, IEL_OCT, 134);
        break;
      case IA_DIS_RADIX_DECIMAL:
        IEL_S64tostr((S64*)&num, str, IEL_SDEC, 134);
        break;
      case IA_DIS_RADIX_HEX:
        if (IEL_ISNEG(num))
        {
            IEL_S64tostr((S64*)&num, str, IEL_SDEC, 134);
        } 
		else
        {
            strcpy(str,"0x");
            IEL_U64tostr(&num, str+2, IEL_HEX, 134);
        }
		break;
	  case IA_DIS_RADIX_HEX_FULL:
		if (IEL_ISNEG(num))
        {
			sprintf(str, "0x%x", IEL_GETDW0(num));
		}
  		else
        {
            strcpy(str,"0x");
            IEL_U64tostr(&num, str+2, IEL_HEX, 134);
        }
        break;
    }
}
        

/*****************************************************************************/
/***   ia_dis_reg function                                                 ***/
/*****************************************************************************/

static void ia_dis_reg(char *str, IA_Decoder_Reg_Name name, IA_Decoder_Reg_Type type)
{
	strcpy(str, ia_dis_regs[name].reg_str);
}


/*****************************************************************************/
/***   ia_dis_addr function                                                ***/
/*****************************************************************************/

static void ia_dis_addr(U64 addr, char *str, Operand_Type *optype)
{
    static char tempstr[128];
    static char offstr[136];
    int  leng = 128;
    U64  offset;
    IA_Dis_Err err;

    err = (*ia_dis_client_gen_sym)(addr ,(unsigned int)(-1), tempstr, &leng, &offset);

    switch (err)
    {
      case IA_DIS_NO_ERROR:
        PUT_FIELD(IA_DIS_FIELD_ADDR_SYM);
        if (!IEL_ISZERO(offset))
        {
            PUT_FIELD(IA_DIS_FIELD_ADDR_PLUS);
            PUT_FIELD(IA_DIS_FIELD_ADDR_OFFSET);
            strcat(tempstr,"+0x");
            IEL_U64tostr(&offset, offstr, 16, 128);
            strcat(tempstr,offstr);
        }
        strcpy(str, tempstr);
        break;
      default:
        PUT_FIELD(IA_DIS_FIELD_ADDR_HEX);
        IEL_U64tostr(&offset, offstr, 16, 128);
        strcpy (str, "0x");
        fill_string('0', str, ADDR_SIZE-strlen(offstr)+strlen(str), 0);
        strcat(str, offstr);
        break;
    }

    *optype = DIS_ADDR;
}


/*****************************************************************************/
/***   ia_dis_disp function                                                ***/
/*****************************************************************************/

static void ia_dis_disp(U64 addr, char *str, int signed_imm, int is_rel,
						U64 addr_err, int operand_offset)
{
    static char tempstr[128];
    static char offstr[136];
    int  leng = 128;
    U64  offset;
    U32  instlen;
    IA_Dis_Err err;

    IEL_CONVERT1(instlen, siz);
    IEL_ADDS(addr, addr, instlen);
    err = (*ia_dis_client_gen_sym)(addr ,IEL_GETDW0(addr)+operand_offset,
									tempstr, &leng, &offset);

    

    switch (err)
    {
      case IA_DIS_NO_ERROR:
        PUT_FIELD(IA_DIS_FIELD_DISP_SYM);
        if (!IEL_ISZERO(offset))
        {
            if ((signed_imm) && (IEL_ISNEG(offset)))
            {
                string_S64(offset, offstr);
            } else
            {
                string_U64(offset, offstr);
                strcat(tempstr,"+");
                PUT_FIELD(IA_DIS_FIELD_DISP_PLUS);
                PUT_FIELD(IA_DIS_FIELD_DISP_OFFSET);
            }
            strcat(tempstr,offstr);
        }
        strcpy(str, tempstr);
        break;
      default:
        if (is_rel)     /* In case of relative */
        {
            if (!IEL_ISNEG(addr_err))
            {
                strcpy(str,".+");
                PUT_FIELD(IA_DIS_FIELD_DISP_SYM);
                PUT_FIELD(IA_DIS_FIELD_DISP_PLUS);
                PUT_FIELD(IA_DIS_FIELD_DISP_OFFSET);
            }
			else
            {
                strcpy(str,".");
                PUT_FIELD(IA_DIS_FIELD_DISP_OFFSET);
            }
			string_S64(addr_err, offstr);
			strcat(str,offstr);
        }
		else
        {
            strcpy(str,"");
            PUT_FIELD(IA_DIS_FIELD_DISP_VALUE);
        
			if (signed_imm)
			{
				string_S64(addr_err, offstr);
			} else
			{
				string_U64(addr_err, offstr);
			}
			strcat(str, offstr);
		}
        break;
    }
}

/*****************************************************************************/
/***   ia_dis_mnem  function                                               ***/
/*****************************************************************************/

static void ia_dis_mnem(IA_Decoder_Inst_Id inst, void *info, char *str,
						Operand_Type *optype, IA_Decoder_Info *dinfo)
{
	fill_string(' ', str, DIS_TAB1, 1);


	if (dinfo->prefix_info.n_lock_pref)
	{
		strcat(str, "lock ");
		PUT_FIELD(IA_DIS_FIELD_PREFIX);
	}

	if (dinfo->prefix_info.n_rep_pref)
	{
		if (dinfo->prefix_info.repeat_type == IA_DECODER_REPE)
		{
			strcat(str, "repe ");
		} else
		{
			strcat(str, "repne ");
		}
		PUT_FIELD(IA_DIS_FIELD_PREFIX);
	}

	strcat(str, ((TOOL_X86CT_INFO *)info)->Mnem);
	PUT_FIELD(IA_DIS_FIELD_MNEM);

	fill_string(' ', str, DIS_TAB2, 1);
	*optype = DIS_MNEM;
}

/*****************************************************************************/
/***   ia_dis_operand  function                                            ***/
/*****************************************************************************/

static void ia_dis_operand(IA_Decoder_Operand_Info  *opr, char *str,
						   Operand_Type *optype, const U64 *addr, int inst_size)
{
    static char tempstr[136];
    static char temp1[128];
    static char number[136];
    static char regstr[128], base[10], index[10];
    U64  sum, final_offset, off64;
	U32  the_size;
    int mem_format;
    unsigned long scale;
	int is_const = 0;
	
    if ((opr->type!=IA_DECODER_NO_OPER) && (*optype == DIS_OPER))
      PUT_FIELD(IA_DIS_FIELD_OPER_COMMA);

    switch(opr->type)
    {
      case IA_DECODER_NO_OPER:
        return;
      case IA_DECODER_PORT_IN_DX:
      case IA_DECODER_REGISTER:
        PUT_FIELD(IA_DIS_FIELD_OPER);
        ia_dis_reg(regstr, opr->reg_info.name, opr->reg_info.type);
		sprintf(tempstr, "%s", regstr);
        break;
      case IA_DECODER_PORT:
	  	PUT_FIELD(IA_DIS_FIELD_OPER);
		sprintf(tempstr,"$%ld", opr->port_number);
		break;
	  case IA_DECODER_CONST:
		is_const = 1;
		/* fall though */
      case IA_DECODER_IMMEDIATE:
        if ((signed long) opr->imm_info.value<0)
        {
            IEL_CONVERT2(opr->imm_info.val64,
						 (unsigned long)opr->imm_info.value, (unsigned long)(-1));
        } 
		else
        {
            IEL_CONVERT1(opr->imm_info.val64, opr->imm_info.value);
        }
        PUT_FIELD(IA_DIS_FIELD_OPER);
        if (opr->imm_info.signed_imm)
        {
            string_S64(opr->imm_info.val64, temp1);
        } else
        {
            string_U64(opr->imm_info.val64, temp1);
        }
		if (!is_const)
        {
            sprintf(tempstr,"$%s", temp1);
        } else
        {
            strcpy(tempstr, temp1);
        }
        break;
        
      case IA_DECODER_MEMORY:
        mem_format = 0;
        if (opr->mem_info.mem_base.valid)
        {
            ia_dis_reg(base, opr->mem_info.mem_base.name,
                    opr->mem_info.mem_base.type);
            mem_format |= DIS_MEM_BASE;
        };
        if (opr->mem_info.mem_index.valid)
        {
            ia_dis_reg(index, opr->mem_info.mem_index.name,
                    opr->mem_info.mem_index.type);
            mem_format |= DIS_MEM_INDEX;
        };
        if (opr->mem_info.mem_scale != 0)
        {
            mem_format |= (DIS_MEM_SCALE);
            scale = opr->mem_info.mem_scale;
        }

        strcpy(tempstr,"");
        if (prefix)
        {
            ia_dis_reg(regstr, opr->mem_info.mem_seg.name,
                    opr->mem_info.mem_seg.type);
            sprintf(tempstr, "%s:", regstr);
            PUT_FIELD(IA_DIS_FIELD_OPER_SEGOVR);
            PUT_FIELD(IA_DIS_FIELD_OPER_COLON);
        }


		if ((signed)opr->mem_info.mem_offset < 0)
		{
			IEL_CONVERT2(off64, opr->mem_info.mem_offset, (unsigned)(-1));
		}
		else
		{
			IEL_CONVERT2(off64, opr->mem_info.mem_offset, 0);
		}

        if (mem_format == 0)
		{
			string_S64(off64, tempstr);
		}
        else
		{
			/* don't print 0  0(%ebx) -> (%ebx) */
			if (opr->mem_info.mem_offset)
			{
				string_S64(off64, tempstr);
			}
			
			PUT_FIELD(IA_DIS_FIELD_OPER_OFS);
		}
        switch(mem_format)
        {
          case 0:
            break;
          case DIS_MEM_INDEX:
            sprintf(tempstr, "%s(%s)", tempstr, index);
            PUT_FIELD(IA_DIS_FIELD_OPER_LPARENT);
            PUT_FIELD(IA_DIS_FIELD_OPER_INDEX);
            PUT_FIELD(IA_DIS_FIELD_OPER_RPARENT);
            break;
          case DIS_MEM_BASE:
            sprintf(tempstr, "%s(%s)", tempstr, base);
            PUT_FIELD(IA_DIS_FIELD_OPER_LPARENT);
            PUT_FIELD(IA_DIS_FIELD_OPER_BASE);
            PUT_FIELD(IA_DIS_FIELD_OPER_RPARENT);
            break;
          case (DIS_MEM_INDEX | DIS_MEM_BASE):
            sprintf(tempstr, "%s(%s, %s)", tempstr,base, index);
            PUT_FIELD(IA_DIS_FIELD_OPER_LPARENT);
            PUT_FIELD(IA_DIS_FIELD_OPER_BASE);
            PUT_FIELD(IA_DIS_FIELD_OPER_COMMA);
            PUT_FIELD(IA_DIS_FIELD_OPER_INDEX);
            PUT_FIELD(IA_DIS_FIELD_OPER_RPARENT);
            break;
          case (DIS_MEM_BASE | DIS_MEM_INDEX | DIS_MEM_SCALE):
            sprintf(tempstr, "%s(%s, %s, %ld)", tempstr,base, index, scale);
            PUT_FIELD(IA_DIS_FIELD_OPER_LPARENT);
            PUT_FIELD(IA_DIS_FIELD_OPER_BASE);
            PUT_FIELD(IA_DIS_FIELD_OPER_COMMA);
            PUT_FIELD(IA_DIS_FIELD_OPER_INDEX);
            PUT_FIELD(IA_DIS_FIELD_OPER_COMMA);
            PUT_FIELD(IA_DIS_FIELD_OPER_SCALE);                
            PUT_FIELD(IA_DIS_FIELD_OPER_RPARENT);
            break;
          case (DIS_MEM_INDEX | DIS_MEM_SCALE):
            sprintf(tempstr, "%s(%s, %ld)", tempstr, index, scale);
            PUT_FIELD(IA_DIS_FIELD_OPER_LPARENT);
            PUT_FIELD(IA_DIS_FIELD_OPER_INDEX);
            PUT_FIELD(IA_DIS_FIELD_OPER_COMMA);
            PUT_FIELD(IA_DIS_FIELD_OPER_SCALE);                
            PUT_FIELD(IA_DIS_FIELD_OPER_RPARENT);
            break;
          default:
            sprintf(tempstr," Invalid_mem");
            break;
        }
        break;
        
      case IA_DECODER_IP_RELATIVE:
        if ((signed long) opr->ip_relative_offset < 0)
        {
            IEL_CONVERT2(opr->imm_info.val64,
						 (unsigned long)opr->ip_relative_offset,
						 (unsigned long)( -1));
        } else
        {
            IEL_CONVERT2(opr->imm_info.val64, opr->ip_relative_offset, 0);
        }
        IEL_ADDS(sum, opr->imm_info.val64, *addr);
		IEL_SUBS(final_offset, sum, *addr);
		IEL_CONVERT1(the_size,inst_size);
		IEL_ADDS(final_offset,the_size,final_offset);
        ia_dis_disp(sum, tempstr, 1, 1, final_offset, siz - opr->imm_info.size/8);
        break;
        
      case IA_DECODER_SEG_OFFSET:
        sprintf(tempstr,"$0x%x, $0x%x", opr->seg_offset_info.segment_number,
                opr->seg_offset_info.offset);
        PUT_FIELD(IA_DIS_FIELD_OPER);
        PUT_FIELD(IA_DIS_FIELD_COMMA);
        PUT_FIELD(IA_DIS_FIELD_OPER);
        break;
    }

    switch(*optype)
    {
      case DIS_EMPTY:
      case DIS_ADDR:
      case DIS_PREG:
      case DIS_MNEM:
        strcat(str," ");
        strcat(str, tempstr);
        break;
      case DIS_OPER:
		if(ia_dis_spaces == 1)
		{
			strcat(str,", ");
			strcat(str, tempstr);
		} else
		{
			strcat(str,",");
			strcat(str, tempstr);
		}
        break;
    }       
    *optype = DIS_OPER;
}



/*****************************************************************************/
/***   ia_dis_field function                                               ***/
/*****************************************************************************/
static void ia_dis_field(IA_Dis_Fields fields, _TCHAR *str)
{
    int last_field = 0;
    _TCHAR *ptr;
    int place;


    ptr = str;

    for(;;)
    {
        fields[last_field].first = ptr;
        fields[last_field].type = ia_dis_fields[last_field].type;
        place =  _tcscspn(ptr,_T("+:<>,()= "));
        if (*ptr == '\0')
        {
            fields[last_field].first = NULL;
            fields[last_field].type = IA_DIS_FIELD_NONE;
            if (last_field != ia_dis_last_field)
            {
            }
            return;
        }
        if (!place) place=1;
        fields[last_field++].length = place;
        
        ptr+=place;
        
        switch(*(ptr))
        {
          case '!': *ptr=':';
          			break;
          case '/':
          case '=':
          case ',':
          case '(':
          case ')': 
          case '>':
          case '<':
          case '+':
          case ':':
            fields[last_field].first = ptr;
            fields[last_field].type = ia_dis_fields[last_field].type;
            fields[last_field++].length = 1;
            ptr+=1;
            break;
        }

		/* Handle special case of %st[x] */
		if ((*(ptr-1) == ']') && (ptr >= str+6) && (!_tcscmp(ptr-6, _T("%st[")))) 
		{
			*(ptr-1) = ')';
			*(ptr-3) = '(';
		}
		if ((*(ptr-2) == ']') && (ptr >= str+7) && (!_tcscmp(ptr-7, _T("%st["))))
		{
			*(ptr-2) = ')';
			*(ptr-4) = '(';
		}
        for(;(*ptr)==' '; ptr++);
    }
}


/*****************************************************************************/
/***   x86 ia_dis_operand_list function                                    ***/
/*****************************************************************************/

static void ia_dis_ia_operand_list (IA_Decoder_Info *info,
									char *ascii,
									Operand_Type *last_exist,
									const U64 *address,
									int imp_op)
{
	int op_num=1;

    /* If relative add * before operand */
    
    if ((IA_DECODER_JMP(info)) &&
        (info->src1.type != IA_DECODER_IP_RELATIVE) &&
        (info->src1.type != IA_DECODER_SEG_OFFSET) &&
        (info->src1.type != IA_DECODER_IMMEDIATE) &&
        (info->src1.type != IA_DECODER_NO_OPER))
    {
        strcat(ascii," *");
        PUT_FIELD(IA_DIS_FIELD_INDIRECT);
    }

	
      
    if (IA_DECODER_BIT_OPRNDS_PRINT_RVRS & info->flags)
    {
		if (info->src2.type != IA_DECODER_NO_OPER)
		  op_num++;

		if (op_num!=imp_op)
		  ia_dis_operand(&info->src2, ascii, last_exist, address, info->size);

		if (info->src1.type != IA_DECODER_NO_OPER)
		  op_num++;
		if (op_num!=imp_op)
		  ia_dis_operand(&info->src1, ascii, last_exist, address, info->size);

    }
    else
    {
		if (info->src1.type != IA_DECODER_NO_OPER)
		  op_num++;
		if (op_num!=imp_op)
		{
			
			ia_dis_operand(&info->src1, ascii, last_exist, address, info->size);

		}
		if (info->src2.type != IA_DECODER_NO_OPER)
		  op_num++;
		if (op_num!=imp_op)
		  ia_dis_operand(&info->src2, ascii, last_exist, address, info->size);
    }

	if (info->dst1.type != IA_DECODER_NO_OPER)
	  op_num++;
	if (op_num!=imp_op)
	  ia_dis_operand(&info->dst1, ascii, last_exist, address, info->size);
}


#ifndef NO_MASM
#include "dismasm.c"
#endif


/*****************************************************************************/
/***   ia_dis_inst function                                                ***/
/*****************************************************************************/

IA_Dis_Err     ia_dis_inst(const U64 *                   address,
						   const IA_Decoder_Machine_Mode mach_mode,
						   const unsigned char *         bin_inst_buf,
						   const unsigned int            bin_inst_buf_length,
						   unsigned int *                actual_inst_length,
						   _TCHAR *                      ascii_inst_buf,
						   unsigned int *                ascii_inst_buf_length,
						   IA_Dis_Fields *               ascii_inst_fields)
{
    static char  buffer[1000];
    char         *ascii = buffer;
    IA_Decoder_Info info;
    IA_Decoder_Err  ia_decoder_error;
    Operand_Type last_exist;

    
    if ( ia_dis_id == -1 )      /*** ia_decoder never opened ***/
    {
        IA_Dis_Err     err;

        err = ia_dis_setup(ia_dis_machine_type, ia_dis_machine_mode, IA_DIS_ALIAS_ALL_REGS,
                        IA_DIS_RADIX_HEX, IA_DIS_STYLE_USL, NULL);

        if ( err != IA_DIS_NO_ERROR)         
        {
            return(err);
        }
    }

    IA_DIS_SET_MACHINE_MODE(mach_mode);

	if(ia_dis_style==IA_DIS_STYLE_MASM)
    {   
#ifdef NO_MASM
		return(IA_DIS_INTERNAL_ERROR);
#endif
 		return(masm_dis_inst(address, mach_mode, bin_inst_buf, bin_inst_buf_length,
							 actual_inst_length, ascii_inst_buf, 
							 ascii_inst_buf_length, ascii_inst_fields));
	
	}
    ia_dis_last_field = 0;
    
    /* check for NULL pointers error*/
    
    if ((address == NULL) ||
        (bin_inst_buf == NULL) ||
        (ascii_inst_buf == NULL) ||
        (ascii_inst_buf_length == NULL))
    {
        return(IA_DIS_NULL_PTR);
    };

    /* check for unaligned instruction error*/

	/*
	c?m
	the following if() statement  is bogus. (some int) % 1 is always false...
	I believe they meant something like what is in the #else clause.
	But I don't understand what they are trying to achieve as instructions
	don't have to be aligned on x86. So let's leave the code as is for now,
	knowing that the if will never turn true.
	*/

#if 1
    if (((*address).dw0_32) % 1)
#else
    if (((*address).dw0_64) & 1)
#endif
    {
        return(IA_DIS_UNALIGNED_INST);
    }

    ia_decoder_error = ia_decoder_decode(ia_dis_id,  (unsigned char*)bin_inst_buf,
                                   (int)bin_inst_buf_length, &info);

    switch(ia_decoder_error)
    {
      case IA_DECODER_NO_ERROR:
         break;
      case IA_DECODER_LOCK_ERR:
        return(IA_DIS_INVALID_OPCODE);
        break;
      case IA_DECODER_TOO_SHORT_ERR:
        return(IA_DIS_SHORT_BIN_INST_BUF);
      case IA_DECODER_RESERVED_OPCODE:
      case IA_DECODER_INVALID_PRM_OPCODE:
        return(IA_DIS_INVALID_OPCODE);
      default: return(IA_DIS_INTERNAL_ERROR);
    }

    instid = info.inst;
        
    /* Order: <preg> mnem Src1, ... Src5, Dst1, ... Dst5, <newisa>*/

    /* disassembly of instruction can begin */

    last_exist = DIS_EMPTY;


	/*
	c?m
	don't call ia_dis_addr. calling it causes the instruction
	address to appear twice. Oone comes from ia_dis_addr, the other
	from the calling code (debug_server or kernel_debugger), where
	symbols are being resolved. When we do all symbol resolution in
	the disassembler by providing ia_dis_client_gen_sym, then we can
	call ia_dis_addr again.
	*/
	
#if 0
	ia_dis_addr(*address, ascii, &last_exist);
    PUT_FIELD(IA_DIS_FIELD_ADDR_COLON);
    strcat(ascii, ": ");
#else
	ascii[0] = '\0';
#endif
	tabstop = strlen(ascii);
	if (tabstop<14)
	{
	  tabstop=14;
	}

    /* Nop special case */

    if ((*bin_inst_buf == 0x90) && (info.size == 1))
    {
        PUT_FIELD(IA_DIS_FIELD_MNEM);
        fill_string(' ', ascii, DIS_TAB1, 1);
        strcat(ascii, "nop");
    }
	else
    {
    	int op_num, imp_op, op_val, modx=0;
        prefix = (info.prefix_info.n_seg_pref != 0);
        siz = info.size;

        ia_dis_mnem(info.inst, info.client_info, ascii, &last_exist, &info);

		/* Find AI operands */
		op_num = 1;
		imp_op = 0;
		
		if ((op_val=((TOOL_X86CT_INFO *)(info.client_info))->opr3) > 0)
		{
			op_num++;
			if (op_val==2)
			  imp_op = op_num;
			
		}
		if ((op_val=((TOOL_X86CT_INFO *)(info.client_info))->opr2) > 0)
		{
			op_num++;
			if (op_val==2)
			  imp_op = op_num;
			if (op_val==4)
			  modx = 1;
				
		}
			
		if ((op_val=((TOOL_X86CT_INFO *)(info.client_info))->opr1) > 0)
		{
			op_num++;
			if (op_val==2)
			  imp_op = op_num;
			if (op_val==4)
			  modx = 1;
			
		}

		/* In case of out %dx --> out (%dx) */
		if (modx)
		{
			PUT_FIELD(IA_DIS_FIELD_OPER_LPARENT);
			strcat(ascii, "(");
		}
				
        ia_dis_ia_operand_list (&info, ascii, &last_exist, address, imp_op);

		if (modx)
		{
			PUT_FIELD(IA_DIS_FIELD_OPER_RPARENT);
			strcat(ascii, ")");
		}
    }

    if (actual_inst_length != NULL)
      *actual_inst_length = info.size;
        
    if ((strlen(ascii)*sizeof(_TCHAR)+1) > *ascii_inst_buf_length)
    {
        *ascii_inst_buf_length = strlen(ascii)+1;
        return(IA_DIS_SHORT_ASCII_INST_BUF);
    }

    _tcscpy(ascii_inst_buf, (_TCHAR *)(ascii));
    if (ascii_inst_fields != NULL)
    {
        ia_dis_field(*ascii_inst_fields, ascii_inst_buf);
    }
    return(IA_DIS_NO_ERROR);
}   


IA_Dis_Err default_function (U64          address,
							 unsigned int internal_disp_off,
							 char *       symbuf,
							 int *        sym_buf_length,
							 U64 *        offset) 
{
    if ((symbuf == NULL) || (offset == NULL))
    {
        return(IA_DIS_NULL_PTR);
    }

    *offset = address;
    return(IA_DIS_NO_SYMBOL);
}

/*****************************************************************************/

/* Auxliary functions */


const _TCHAR* ia_dis_ver_str()
{
	return(ver_string);
}

IA_Dis_Err ia_dis_ver(long *major, long *minor)
{

	if ((major==NULL) ||
		(minor==NULL))
	{
		return(IA_DIS_NULL_PTR);
	}
		  
	*major = API_MAJOR;
	*minor = API_MINOR;
	return(IA_DIS_NO_ERROR);
}
