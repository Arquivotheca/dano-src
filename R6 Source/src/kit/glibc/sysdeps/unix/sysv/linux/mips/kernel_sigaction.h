/* This is the sigaction structure from the Linux 2.1.24 kernel.  */

#include <sgidefs.h>

#define HAVE_SA_RESTORER

struct kernel_sigaction {
	unsigned int	sa_flags;
	__sighandler_t	sa_handler;
	unsigned long	sa_mask;
	unsigned int    __pad0[3]; /* reserved, keep size constant */

	/* Abi says here follows reserved int[2] */
	void		(*sa_restorer)(void);
#if (_MIPS_ISA == _MIPS_ISA_MIPS1) || (_MIPS_ISA == _MIPS_ISA_MIPS2)
	/*
	 * For 32 bit code we have to pad struct sigaction to get
	 * constant size for the ABI
	 */
	int		pad1[1]; /* reserved */
#endif
};
