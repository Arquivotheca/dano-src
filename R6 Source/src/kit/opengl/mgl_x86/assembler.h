#ifndef __ASSEMBLER_H__
#define __ASSEMBLER_H__


#define cc_JA 0x87
#define cc_JAE 0x83
#define cc_JB 0x82
#define cc_JBE 0x86
#define cc_JC 0x82
#define cc_JE 0x84
#define cc_JZ 0x84
#define cc_JG 0x8F
#define cc_JGE 0x8D
#define cc_JL 0x8C
#define cc_JLE 0x8E
#define cc_JNA 0x86
#define cc_JNAE 0x82
#define cc_JNB 0x83
#define cc_JNBE 0x87
#define cc_JNC 0x83
#define cc_JNE 0x85
#define cc_JNG 0x8E
#define cc_JNGE 0x8C
#define cc_JNL 0x8D
#define cc_JNLE 0x8F
#define cc_JNO 0x81
#define cc_JNP 0x8B
#define cc_JNS 0x89
#define cc_JNZ 0x85
#define cc_JO 0x80
#define cc_JP 0x8A
#define cc_JPE 0x8A
#define cc_JPO 0x8B
#define cc_JS 0x88
#define cc_JZ 0x84

extern void * asm_Jcc( void * pc, void * target, GLuint condition );
extern void * asm_Call( void * pc, void * );
extern void * asm_Jmp( void * pc, void * target );
extern void * asm_CopyBlock( void * pc, void *start, void *end );
extern void * asm_Align( void * pc );

#define asm_BLOCK(a) asm_CopyBlock(pc,&a,&a##_end)



#endif
