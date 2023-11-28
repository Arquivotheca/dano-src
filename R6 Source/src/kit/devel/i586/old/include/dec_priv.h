
#define IA_DECODER_MAX_CLIENTS     20

#define DEFAULT_MACHINE_TYPE    IA_DECODER_CPU_P6MM
#define DEFAULT_MACHINE_MODE    IA_DECODER_MODE_PROTECTED_32


typedef struct
{
    int                      is_used;
    IA_Decoder_Machine_Type  machine_type;
    IA_Decoder_Machine_Mode  machine_mode;
    void **                  info_ptr;     /*** after dynamic allocation,     ***/
                                           /*** info_ptr points to an array   ***/
                                           /*** of pointers. The i'th pointer ***/
                                           /*** in the array is a ptr to the  ***/
                                           /*** client info.                  ***/
} Client_Entry;

Client_Entry   ia_clients_table[IA_DECODER_MAX_CLIENTS];



typedef struct
{
	const IA_Inst_Imp_Info_t * stat_info_p_16;
	const IA_Inst_Imp_Info_t * stat_info_p_32;
}IA_Inst_Imp_Info_p;
	
extern const IA_Inst_Imp_Info_p ia_inst_info[];

#define IMPLIED_MEMORY_OPERAND(di)  (IA_DECODER_IMP_MEM_READ(di) ||    \
                                     IA_DECODER_IMP_MEM_WRITE(di))

#define OPERANDS_POS        24
#define OPERANDS_MASK       0xf
#define OPERANDS(di)        (((di->flags) >> OPERANDS_POS) & OPERANDS_MASK)

#define SET_NONE(opr)     opr.type           = IA_DECODER_NO_OPER;

typedef enum
{
    OPERANDS_NONE,
    OPERANDS_SRC1,
    OPERANDS_DEST,
    OPERANDS_SRC1nDEST,
    OPERANDS_SRC1_SRC2,
    OPERANDS_DEST_SRC1,
    OPERANDS_SRC1nDEST_SRC2,
    OPERANDS_DEST_SRC1_SRC2,
    OPERANDS_SRC1nDEST_SRC2_SRC3,
    OPERANDS_SRC1nDST1_SRC2nDST2
} Operands;

#define X86_USER_FLAGS      (IA_DECODER_BIT_8086           |               \
                            IA_DECODER_BIT_V86             |               \
                            IA_DECODER_BIT_P5              |               \
                            IA_DECODER_BIT_P6              |               \
                            IA_DECODER_BIT_P7              |               \
                            IA_DECODER_BIT_PRIVILEGE       |               \
                            IA_DECODER_BIT_LOCK            |               \
                            IA_DECODER_BIT_OPER_ERR        |               \
                            IA_DECODER_BIT_IMPLIED_OPR     |               \
                            IA_DECODER_BIT_TYPE            |               \
                            IA_DECODER_BIT_W_NEED_PREFIX   |               \
                            IA_DECODER_BIT_L_NEED_PREFIX   |               \
                            IA_DECODER_BIT_STOP_TRANS      |               \
                            IA_DECODER_BIT_STRING_OP       |               \
                            IA_DECODER_BIT_READ            |               \
                            IA_DECODER_BIT_WRITE           |               \
                            IA_DECODER_BIT_IMP_MEM_READ    |               \
                            IA_DECODER_BIT_IMP_MEM_WRITE   |               \
                            IA_DECODER_BIT_P5MM            |               \
                            IA_DECODER_BIT_P6MM			|				\
							IA_DECODER_BIT_IAS_VALID		|				\
							IA_DECODER_BIT_OPRNDS_ORDER	|				\
							IA_DECODER_BIT_OPRNDS_PRINT_RVRS)

/*****************************************************************************/




