/*****************************************************************************/
/***   string_masm_U64                                                     ***/
/*****************************************************************************/

static void string_masm_U64(U64 num, char *str)
{
    switch(ia_dis_radix)
    {
      case IA_DIS_RADIX_BINARY:
        strcpy(str,"0");
        IEL_U64tostr(&num, str+1, IEL_BIN, 134);
		strcat(str,"b");
        break;
      case IA_DIS_RADIX_OCTAL:
        strcpy(str,"0");
        IEL_U64tostr(&num, str+1, IEL_OCT, 134);
		strcat(str,"o");
        break;
      case IA_DIS_RADIX_DECIMAL:
        IEL_S64tostr((S64*)&num, str, IEL_DEC, 134);
        break;
      case IA_DIS_RADIX_HEX:
	  case IA_DIS_RADIX_HEX_FULL:
        {
            strcpy(str,"0");
            IEL_U64tostr(&num, str+1, IEL_HEX, 134);
			strcat(str,"h");
        }
        break;
	  default:
		break;
    }
}


/*****************************************************************************/
/***   string_masm_S64                                                     ***/
/***   convert U64 to string,  as signed                                   ***/
/*****************************************************************************/

static void string_masm_S64(U64 num, char *str)
{
    switch(ia_dis_radix)
    {
      case IA_DIS_RADIX_BINARY:
        strcpy(str,"0");
        IEL_U64tostr(&num, str+1, IEL_BIN, 134);
		strcat(str,"b");
        break;
      case IA_DIS_RADIX_OCTAL:
        strcpy(str,"0");
        IEL_U64tostr(&num, str+1, IEL_OCT, 134);
		strcat(str,"o");
        break;
      case IA_DIS_RADIX_DECIMAL:
        IEL_S64tostr((S64*)&num, str, IEL_SDEC, 134);
        break;
      case IA_DIS_RADIX_HEX:
        if (IEL_ISNEG(num))
        {
            IEL_S64tostr((S64*)&num, str, IEL_SDEC, 134);
        } else
        {
            strcpy(str,"0");
            IEL_U64tostr(&num, str+1, IEL_HEX, 134);
			strcat(str,"h");
        }
        break;
	  case IA_DIS_RADIX_HEX_FULL:
        if (IEL_ISNEG(num))
        {
            sprintf(str, "0%xh", IEL_GETDW0(num));
        } 
		else
        {
            strcpy(str,"0");
            IEL_U64tostr(&num, str+1, IEL_HEX, 134);
			strcat(str,"h");
        }
        break;

    }
}

/*****************************************************************************/
/***   string_masm_S64_sign                                                     ***/
/***   convert U64 to string,  as signed                                   ***/
/*****************************************************************************/

static void string_masm_S64_sign(U64 num, char *str)
{
    switch(ia_dis_radix)
    {
      case IA_DIS_RADIX_BINARY:
        strcpy(str,"+0");
        IEL_U64tostr(&num, str+2, IEL_BIN, 134);
		strcat(str,"b");
        break;
      case IA_DIS_RADIX_OCTAL:
        strcpy(str,"+0");
        IEL_U64tostr(&num, str+2, IEL_OCT, 134);
		strcat(str,"o");
        break;
      case IA_DIS_RADIX_DECIMAL:
		strcpy(str,"+");
        IEL_S64tostr((S64*)&num, str+1, IEL_SDEC, 134);
        break;
      case IA_DIS_RADIX_HEX:
        if (IEL_ISNEG(num))
        {
            IEL_S64tostr((S64*)&num, str, IEL_SDEC, 134);
        } else
        {
            strcpy(str,"+0");
            IEL_U64tostr(&num, str+2, IEL_HEX, 134);
			strcat(str,"h");
        }
        break;
	  case IA_DIS_RADIX_HEX_FULL:
        if (IEL_ISNEG(num))
        {
            sprintf(str, "+0%xh", IEL_GETDW0(num));
        } 
		else
        {
            strcpy(str,"+0");
            IEL_U64tostr(&num, str+2, IEL_HEX, 134);
			strcat(str,"h");
        }
        break;
    }
}


/*****************************************************************************/
/***   ia_dis_masm_addr function                                           ***/
/*****************************************************************************/

static void ia_dis_masm_addr(U64 addr, char *str, Operand_Type *optype)
{
    static char tempstr[128];
    static char offstr[136];
    int  leng = 128;
    U64  offset;
    IA_Dis_Err err;

    err = (*ia_dis_client_gen_sym)(addr, (unsigned int)-1,tempstr, &leng, &offset);

    switch (err)
    {
      case IA_DIS_NO_ERROR:
        PUT_FIELD(IA_DIS_FIELD_ADDR_SYM);
        if (!IEL_ISZERO(offset))
        {
            PUT_FIELD(IA_DIS_FIELD_ADDR_PLUS);
            PUT_FIELD(IA_DIS_FIELD_ADDR_OFFSET);
            strcat(tempstr,"+0");
            IEL_U64tostr(&offset, offstr, 16, 128);
            strcat(tempstr,offstr);
			strcat(tempstr,"h");
        }
        strcpy(str, tempstr);
        break;
      default:
        PUT_FIELD(IA_DIS_FIELD_ADDR_HEX);
        IEL_U64tostr(&offset, offstr, 16, 128);
        strcpy (str, "0");
        fill_string('0', str, ADDR_SIZE-strlen(offstr)+strlen(str), 0);
        strcat(str, offstr);
		strcat(str, "h");
        break;
    }

    *optype = DIS_ADDR;
}


static void ia_dis_masm_disp(U64 label_addr/*for callback*/, char *str, int signed_imm, int is_rel,
							 U64 addr_err, int operand_offset)
{
    static char tempstr[128];
    static char offstr[136];
    int  leng = 128;
    U64  offset;
    U32  instlen;
    IA_Dis_Err err;
	U64 segm_off;

	IEL_ASSIGNU(segm_off, label_addr);
	IEL_CONVERT1(instlen, siz);
	IEL_ADDS(label_addr, label_addr, instlen);
	IEL_ADDS(addr_err, addr_err, instlen);
    err = (*ia_dis_client_gen_sym)(label_addr ,IEL_GETDW0(label_addr)+operand_offset,
									tempstr, &leng, &offset);
    

    switch (err)
    {
      case IA_DIS_NO_ERROR:
        PUT_FIELD(IA_DIS_FIELD_DISP_SYM);
        if (!IEL_ISZERO(offset))
        {
            if (signed_imm)
            {
                string_masm_S64_sign(offset, offstr);
            } 
			else
            {
                string_masm_U64(offset, offstr);
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
			strcpy(str,"$");
			string_masm_S64_sign(addr_err, offstr);
			strcat(str, offstr);
            PUT_FIELD(IA_DIS_FIELD_DISP_SYM);
            PUT_FIELD(IA_DIS_FIELD_DISP_PLUS);
            PUT_FIELD(IA_DIS_FIELD_DISP_OFFSET);
        } 
		else
        {
			unsigned short segm = (unsigned short)IEL_GETDW1(segm_off);
			unsigned int   off  = IEL_GETDW0(segm_off);
			U64 segm64, off64;

			/* segment selector:offset */
			IEL_CONVERT2(segm64, segm, 0);
			string_masm_U64(segm64,offstr);
			strcpy(str, offstr);
			strcat(str, ":");
			IEL_CONVERT2(off64, off, 0);
			string_masm_U64(off64,offstr);
			strcat(str, offstr);

            PUT_FIELD(IA_DIS_FIELD_DISP_VALUE);
		}
        break;
    }
}

static void ia_dis_masm_operand(IA_Decoder_Operand_Info  *opr, char *str, int is_MMX,
								Operand_Type *optype, const U64 *addr, int imm_op_size)
{
    static char tempstr[136];
    static char sym_name[136];
    static char off_str[136];
	static char addend_str[136];
    static char regstr[128], base[10], index[10];
    static U64  sum, final_offset;
    static int mem_format, scale;
	U64 off64;
	int leng;
	int internal_disp_offset;

    if ((opr->type!=IA_DECODER_NO_OPER) && (*optype == DIS_OPER))
      PUT_FIELD(IA_DIS_FIELD_OPER_COMMA);


    switch(opr->type)
    {
      case IA_DECODER_NO_OPER:
        return;
      case IA_DECODER_PORT_IN_DX:
      case IA_DECODER_REGISTER:
        PUT_FIELD(IA_DIS_FIELD_OPER);
		sprintf(tempstr, ia_dis_regs[opr->reg_info.name].reg_masm_str);
        break;

      case IA_DECODER_PORT:
	  	PUT_FIELD(IA_DIS_FIELD_OPER);
		sprintf(tempstr,"%ld",opr->port_number);
		break;
      case IA_DECODER_CONST:
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
            string_masm_S64(opr->imm_info.val64, tempstr);
        } 
		else
        {
            string_masm_U64(opr->imm_info.val64, tempstr);
        }
		break;

      case IA_DECODER_MEMORY:
		strcpy(tempstr,"");
		switch(opr->mem_info.size)
		{
		  case 1:
			sprintf(tempstr,"BYTE PTR ");
			break;
		  case 2:
			sprintf(tempstr,"WORD PTR ");
			break;
		  case 4:
			sprintf(tempstr,"DWORD PTR ");
			break;
		  case 6:
			sprintf(tempstr,"FWORD PTR ");
			break;
		  case 8:
			  if (is_MMX)
			  {
				sprintf(tempstr,"MMWORD PTR ");
			  }
			  else
			  {
				sprintf(tempstr,"QWORD PTR ");
			  }
			break;
		  case 10:
			sprintf(tempstr,"TBYTE PTR ");
			break;
		  case 16:
			sprintf(tempstr,"XMMWORD PTR ");
			break;
		  default:
			break;
		}
		PUT_FIELD(IA_DIS_FIELD_OPER_SIZE);
		PUT_FIELD(IA_DIS_FIELD_PTR_DIRECT);
				  
		internal_disp_offset = siz - imm_op_size/8 - opr->mem_info.mem_off.size/8; 
        mem_format = 0;
        if (opr->mem_info.mem_base.valid)
        {
            sprintf(base, "%s", ia_dis_regs[opr->mem_info.mem_base.name].reg_masm_str);
            mem_format |= DIS_MEM_BASE;
        }
        if (opr->mem_info.mem_index.valid)
        {
			sprintf(index, "%s", ia_dis_regs[opr->mem_info.mem_index.name].reg_masm_str);

            mem_format |= DIS_MEM_INDEX;
        }
        if (opr->mem_info.mem_scale != 0)
        {
            mem_format |= (DIS_MEM_SCALE);
            scale = (int)opr->mem_info.mem_scale;
        }


        if (prefix)
        {
			sprintf(regstr, "%s", ia_dis_regs[opr->mem_info.mem_seg.name].reg_masm_str);
            sprintf(tempstr, "%s%s:", tempstr, regstr);
            PUT_FIELD(IA_DIS_FIELD_OPER_SEGOVR);
            PUT_FIELD(IA_DIS_FIELD_OPER_COLON);
			prefix = 0;
        }

        switch(mem_format)
        {
          case 0:
            break;
          case DIS_MEM_INDEX:
            sprintf(tempstr, "%s[%s", tempstr, index);
            PUT_FIELD(IA_DIS_FIELD_OPER_S_LPARENT);
            PUT_FIELD(IA_DIS_FIELD_OPER_INDEX);
            break;
          case DIS_MEM_BASE:
            sprintf(tempstr, "%s[%s", tempstr, base);
            PUT_FIELD(IA_DIS_FIELD_OPER_S_LPARENT);
            PUT_FIELD(IA_DIS_FIELD_OPER_BASE);
            break;
          case (DIS_MEM_INDEX | DIS_MEM_BASE):
            sprintf(tempstr, "%s[%s+%s", tempstr,base, index);
            PUT_FIELD(IA_DIS_FIELD_OPER_S_LPARENT);
            PUT_FIELD(IA_DIS_FIELD_OPER_BASE);
            PUT_FIELD(IA_DIS_FIELD_OPER_PLUS);
            PUT_FIELD(IA_DIS_FIELD_OPER_INDEX);
            break;
          case (DIS_MEM_BASE | DIS_MEM_INDEX | DIS_MEM_SCALE):
    		if (scale >1)
	            sprintf(tempstr, "%s[%s+%s*%d", tempstr,base, index, scale);
            else
                sprintf(tempstr, "%s[%s+%s", tempstr,base, index);
            PUT_FIELD(IA_DIS_FIELD_OPER_S_LPARENT);
            PUT_FIELD(IA_DIS_FIELD_OPER_BASE);
            PUT_FIELD(IA_DIS_FIELD_OPER_PLUS);
            PUT_FIELD(IA_DIS_FIELD_OPER_INDEX);
            PUT_FIELD(IA_DIS_FIELD_OPER_MUL);
            PUT_FIELD(IA_DIS_FIELD_OPER_SCALE);                
            break;
          case (DIS_MEM_INDEX | DIS_MEM_SCALE):
            sprintf(tempstr, "%s[%s*%d", tempstr, index, scale);
            PUT_FIELD(IA_DIS_FIELD_OPER_S_LPARENT);
            PUT_FIELD(IA_DIS_FIELD_OPER_INDEX);
            PUT_FIELD(IA_DIS_FIELD_OPER_MUL);
            PUT_FIELD(IA_DIS_FIELD_OPER_SCALE);                
            break;
          default:
            sprintf(tempstr," Invalid_mem");
            break;
        } /* switch mem_format */
		if (mem_format == 0)
		{
			/* check for symbolic info first */
			U64 addend64;
			IEL_ZERO(addend64);

			leng = sizeof(sym_name);
			IEL_CONVERT2(off64, opr->mem_info.mem_offset,0);

			if ((*ia_dis_client_gen_sym)(off64, IEL_GETDW0(*addr)+internal_disp_offset,
										 sym_name, &leng, &addend64) == IA_DIS_NO_ERROR)
			{
				if (IEL_ISZERO(addend64))
				{
					sprintf(tempstr,"%s[%s]", tempstr, sym_name);
					PUT_FIELD(IA_DIS_FIELD_OPER_S_LPARENT);
					PUT_FIELD(IA_DIS_FIELD_DISP_SYM);
					PUT_FIELD(IA_DIS_FIELD_OPER_S_RPARENT);
				}
				else /* print addend */
				{
					string_masm_S64_sign(addend64,addend_str);
					sprintf(tempstr, "%s[%s%s]", tempstr, sym_name, addend_str);
						
					PUT_FIELD(IA_DIS_FIELD_OPER_S_LPARENT);
					PUT_FIELD(IA_DIS_FIELD_DISP_SYM);
					PUT_FIELD(IA_DIS_FIELD_OPER_PLUS);
					PUT_FIELD(IA_DIS_FIELD_OPER_OFS);
					PUT_FIELD(IA_DIS_FIELD_OPER_S_RPARENT);
				}
			}
			else  /* no symbol info	*/
			{
				if ((signed long)opr->mem_info.mem_offset <0)
				{ 
					IEL_CONVERT2(off64,opr->mem_info.mem_offset, (unsigned)(-1));
				}
				else
				{
					IEL_CONVERT2(off64,opr->mem_info.mem_offset, 0);
				}
 				/* print constant  */
				string_masm_S64(off64, off_str);
				sprintf(tempstr,"%s[%s]",tempstr,off_str); 

				PUT_FIELD(IA_DIS_FIELD_ADDR_SYM);
				PUT_FIELD(IA_DIS_FIELD_OPER_S_LPARENT);
				PUT_FIELD(IA_DIS_FIELD_OPER_OFS);
				PUT_FIELD(IA_DIS_FIELD_OPER_S_RPARENT);
			}
		} 
		else /* mem_format != 0 */
		{
			U64 addend64;
			IEL_ZERO(addend64);
			leng = sizeof(sym_name);
			IEL_CONVERT2(off64, opr->mem_info.mem_offset,0);

			/* check if the offset is a mapped symbol */
			if ((*ia_dis_client_gen_sym)(off64 ,IEL_GETDW0(*addr)+internal_disp_offset, 
				sym_name, &leng, &addend64) == IA_DIS_NO_ERROR)
			{
				if (IEL_ISZERO(addend64))
				{
					sprintf(tempstr,"%s + %s]", tempstr, sym_name);
					PUT_FIELD(IA_DIS_FIELD_OPER_PLUS);
					PUT_FIELD(IA_DIS_FIELD_DISP_SYM);
				}
				else /* print addend */
				{
					string_masm_S64_sign(addend64,addend_str);
    			    sprintf(tempstr,"%s+%s%s]", tempstr,sym_name,addend_str);
					PUT_FIELD(IA_DIS_FIELD_OPER_S_LPARENT);
					PUT_FIELD(IA_DIS_FIELD_DISP_SYM);
					PUT_FIELD(IA_DIS_FIELD_OPER_PLUS);
					PUT_FIELD(IA_DIS_FIELD_OPER_OFS);
					PUT_FIELD(IA_DIS_FIELD_OPER_S_RPARENT);
				}
			}
			else 
				/* no symbol found */
			{
				if (opr->mem_info.mem_offset == 0)
				{
					strcat(tempstr, "]");
					PUT_FIELD(IA_DIS_FIELD_OPER_S_RPARENT);
					break;
				}
				if ((signed long)opr->mem_info.mem_offset <0)
				{ 
					IEL_CONVERT2(off64,opr->mem_info.mem_offset, (unsigned)(-1));
				}
				else
				{
					IEL_CONVERT2(off64,opr->mem_info.mem_offset, 0);
				}
				string_masm_S64_sign(off64, off_str);
				sprintf(tempstr,"%s%s]",tempstr, off_str);
				PUT_FIELD(IA_DIS_FIELD_OPER_PLUS);
				PUT_FIELD(IA_DIS_FIELD_OPER_OFS);
				PUT_FIELD(IA_DIS_FIELD_OPER_S_RPARENT);

			} /* print symbol or constant */
		} /* endif mem_format */
	    break;
    
      case IA_DECODER_IP_RELATIVE:
		if (opr->ip_relative_offset == 0)
        {
            strcpy(tempstr,"+0");
			PUT_FIELD(IA_DIS_FIELD_OPER_PLUS);
			PUT_FIELD(IA_DIS_FIELD_OPER_OFS);
        }
        else	
        {
			if ((signed) opr->ip_relative_offset < 0)
			{
				IEL_CONVERT2(off64, opr->ip_relative_offset, (unsigned long)-1);
			} 
			else
			{
				IEL_CONVERT2(off64, opr->ip_relative_offset, 0);
			}
			if (opr->imm_info.size == 8)
			{
				strcat(tempstr," short");
				PUT_FIELD(IA_DIS_FIELD_PTR_DIRECT);
			} 
			IEL_ADDS(sum, off64, *addr);
			ia_dis_masm_disp(sum, tempstr, 1, 1, off64, siz - opr->imm_info.size/8);
		}
        break;

      case IA_DECODER_SEG_OFFSET:
		  IEL_ZERO(final_offset);
		IEL_CONVERT2(sum,opr->seg_offset_info.offset, opr->seg_offset_info.segment_number); 
        ia_dis_masm_disp(sum, tempstr, 1, 0, final_offset, (siz - opr->imm_info.size/8));
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
        strcat(str,", ");
        strcat(str, tempstr);
        break;
    }       
    *optype = DIS_OPER;
}

/*****************************************************************************/
/***   ia_dis_masm_field function                                          ***/
/*****************************************************************************/
static void ia_dis_masm_field(IA_Dis_Fields fields, _TCHAR *str)
{
    int last_field = 0;
    _TCHAR *ptr;
    int place;


    ptr = str;

    for(;;)
    {
        fields[last_field].first = ptr;
        fields[last_field].type = ia_dis_fields[last_field].type;
        place =  _tcscspn(ptr,_T("+:<>,[]=* "));
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
          case '/':
          case '=':
          case ',':
          case ']':
          case '[': 
          case '+':
          case ':':
            fields[last_field].first = ptr;
            fields[last_field].type = ia_dis_fields[last_field].type;
            fields[last_field++].length = 1;
            ptr+=1;
            break;
        }

        for(;(*ptr)==' '; ptr++);
    }
}


/*****************************************************************************/
/***   x86 MASM ia_dis_operand_list function                               ***/
/*****************************************************************************/

static void ia_dis_masm_operand_list (IA_Decoder_Info *info,
									  char *ascii,
									  Operand_Type *last_exist,
									  const U64 *address,
									  int imp_op)
{
	int op_num=1;
	int imm_oper_size = 0;

	/* get the size of immediate operand */
	if (info->src1.type == IA_DECODER_IMMEDIATE)
	{
		imm_oper_size += info->src1.imm_info.size;
	}
	if (info->src2.type == IA_DECODER_IMMEDIATE)
	{
		imm_oper_size += info->src2.imm_info.size;
	}
	
	if (info->dst1.type != IA_DECODER_NO_OPER)
	  op_num++;
	if (op_num!=imp_op)
	  ia_dis_masm_operand(&info->dst1, ascii, IA_DECODER_MM(info), last_exist,
							address, imm_oper_size);
	if (info->src1.type != IA_DECODER_NO_OPER)
	  op_num++;
	if (op_num!=imp_op)
		ia_dis_masm_operand(&info->src1, ascii, IA_DECODER_MM(info), last_exist,
							address, imm_oper_size);	
	ia_dis_masm_operand(&info->src2, ascii, IA_DECODER_MM(info), last_exist, 
						address, imm_oper_size);
}


/*****************************************************************************/
/***   ia_dis_mnem  function                                               ***/
/*****************************************************************************/

static void ia_dis_masm_mnem(IA_Decoder_Inst_Id inst, void *info, char *str,
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

	strcat(str, ((TOOL_X86CT_INFO *)info)->Mnem1);
	PUT_FIELD(IA_DIS_FIELD_MNEM);

	fill_string(' ', str, DIS_TAB2, 1);
	*optype = DIS_MNEM;
}


/*****************************************************************************/
/***   masm_dis_inst function                                              ***/
/*****************************************************************************/

IA_Dis_Err     masm_dis_inst(const U64 *                address,
							 const IA_Decoder_Machine_Mode mode,
							 const unsigned char *      bin_inst_buf,
							 const unsigned int         bin_inst_buf_length,
							 unsigned int *             actual_inst_length,
							 _TCHAR *                   ascii_inst_buf,
							 unsigned int *             ascii_inst_buf_length,
							 IA_Dis_Fields *               ascii_inst_fields)
{
    static char     buffer[1000];
    char            *ascii = buffer;
    IA_Decoder_Info info;
    IA_Decoder_Err  ia_decoder_error;
    Operand_Type    last_exist;

    


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

    
    if (((*address).dw0_32) % 1)
    {
        return(IA_DIS_UNALIGNED_INST);
    }

    ia_decoder_error = ia_decoder_decode(ia_dis_id, (unsigned char *) bin_inst_buf,
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
    
    last_exist = DIS_EMPTY;

	ia_dis_masm_addr(*address, ascii, &last_exist);
    PUT_FIELD(IA_DIS_FIELD_ADDR_COLON);
    strcat(ascii, ": ");
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
        
        ia_dis_masm_mnem(info.inst, info.client_info, ascii, &last_exist, &info);



		op_num = 1;
		imp_op = 0;
		if ((op_val=((TOOL_X86CT_INFO *)(info.client_info))->opr1) > 0)
		{
			op_num++;
			/* do not print implicit operand dx */
			if ((op_val==2) && (IA_DECODER_IMPLIED_OPER(&info)))	
			  imp_op = op_num;
			if (op_val==4)
			  modx = 1;
			
		}

		if ((op_val=((TOOL_X86CT_INFO *)(info.client_info))->opr2) > 0)
		{
			op_num++;
/*	second operand is implicit in USL style only 
		if (op_val==2)
			  imp_op = op_num;					*/
			if (op_val==4)
			  modx = 1;
				
		}

				
		ia_dis_masm_operand_list (&info, ascii, &last_exist, address, imp_op);
			
		if (prefix)
		{
			PUT_FIELD(IA_DIS_FIELD_REMARK);
			sprintf(ascii, "%s        ; %s:",ascii, 
				ia_dis_regs[info.prefix_info.segment_register].reg_masm_str );
		} 

		if (actual_inst_length != NULL)
		  *actual_inst_length = info.size;
			
		if ((strlen(ascii)+1) > *ascii_inst_buf_length)
		{
			*ascii_inst_buf_length = strlen(ascii)+1;
			return(IA_DIS_SHORT_ASCII_INST_BUF);
		};
		_tcscpy(ascii_inst_buf, (_TCHAR *)ascii);
		if (ascii_inst_fields != NULL)
		{
			ia_dis_masm_field(*ascii_inst_fields, ascii_inst_buf);
		}
		return(IA_DIS_NO_ERROR);
	}

    if (actual_inst_length != NULL)
      *actual_inst_length = info.size;
        
    if ((strlen(ascii)+1) > *ascii_inst_buf_length)
    {
        *ascii_inst_buf_length = strlen(ascii)+1;
        return(IA_DIS_SHORT_ASCII_INST_BUF);
    };
    _tcscpy(ascii_inst_buf, (_TCHAR *)ascii);
    if (ascii_inst_fields != NULL)
    {
        ia_dis_field(*ascii_inst_fields, ascii_inst_buf);
    }
    return(IA_DIS_NO_ERROR);
}   
