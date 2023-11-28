#ifndef _DISASM_P_H
#define _DISASM_P_H

#if __INTEL__

#define B_DISASM_FLAG_DEBUG_SERVER_PRIVATE  0x80000000

struct disasm_cookie {
	int (*read_word)(void *thr, void *addr, long *val);
	void *tmr;
	void *thr;
	void *got_address;
};

#endif /* __INTEL__ */

#endif /* _DISASM_P_H */
