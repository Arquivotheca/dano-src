#ifndef _SYSCALL_GEN_H
#define _SYSCALL_GEN_H

#ifdef __cplusplus
extern "C" {
#endif

status_t _kgeneric_syscall_(uint32 opcode, void* buf, size_t buf_size);

/* opcodes and bufer definitions for _kgeneric_syscall_() */

#define GSC_DEST_MASK						0xF0000000
#define GSC_PROC							0x00000000
#define GSC_CPU								0x10000000
#define GSC_ARCH							0x20000000

#define GSC_FLAGS_MASK						0x0F000000
#define GSC_PROT_WRITE						0x01000000

#define	GSC_OPCODE(opcode_, dest_, flags_)	( (dest_) | (flags_) | (opcode_) )

#define GSC_CHECK_BUF_SIZE(buf_type)	do { if(buf_size < sizeof(buf_type)) { ret = EFAULT; goto end; } } while(0)


/* calls to proc */
#define	GSC_OPCODE_PROC(opcode_, flags_)	GSC_OPCODE(opcode_, GSC_PROC, flags_)
#define GSC_TEST							GSC_OPCODE(1, GSC_PROC, GSC_PROT_WRITE)
	
/* mapped to set_kdebug_framebuffer() as defined in bootscreen.h */
#define GSC_SET_KDEBUG_FRAMEBUFFER			GSC_OPCODE(2, GSC_PROC, 0)

/* calls to cpu */
#define	GSC_OPCODE_CPU(opcode_, flags_)		GSC_OPCODE(opcode_, GSC_CPU, flags_)
#ifdef __INTEL__
#define	GSC_GET_CPUID						GSC_OPCODE_CPU(1, GSC_PROT_WRITE)
typedef struct 
{
	cpuid_info*	info;
	uint32		eax_register;
	uint32		cpu;
} gsc_cpuid_t;

#define	GSC_DISABLE_CPUS_SERIAL_NUMBER		GSC_OPCODE_CPU(2, 0)
#endif /* ifdef __INTEL__ */


/* calls to arch */
#define	GSC_OPCODE_ARCH(opcode_, flags_)		GSC_OPCODE(opcode_, GSC_ARCH, flags_)

#ifdef __INTEL__


#define	GSC_GET_INTERRUPT_MODE							GSC_OPCODE_ARCH(1, GSC_PROT_WRITE)
#define	GSC_SET_INTERRUPT_MODE							GSC_OPCODE_ARCH(2, 0)
/* format of the buffer for GSC_GET/SET_INTERRUPT_MODE is in mps.h */

#define	GSC_GET_CHECK_FOCUS_MODE						GSC_OPCODE_ARCH(3, 0)
/* no buffer, the return value is the result */

#define	GSC_SET_CHECK_FOCUS_MODE						GSC_OPCODE_ARCH(4, 0)
/*buffer is bool new_check_focus_mode */

#define	GSC_STEAL_VESA_MEMORY							GSC_OPCODE_ARCH(5, 0)

#endif /* ifdef __INTEL__ */


#ifdef __cplusplus
}
#endif

#endif
