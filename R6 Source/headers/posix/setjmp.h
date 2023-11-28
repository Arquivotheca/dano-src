#ifndef _SETJMP_H_
#define _SETJMP_H_

#include <be_setup.h>

#if __POWERPC__

typedef long *jmp_buf[70];
typedef long *sigjmp_buf[(sizeof(jmp_buf)/sizeof(int)) + (32*4)];

__extern_c_start

int		__setjmp(jmp_buf);
int  sigsetjmp(sigjmp_buf jmp, int savemask);

__extern_c_end

# define setjmp(jmp_buf)	__setjmp(jmp_buf)

#elif	__GNUC__

#include <signal.h>

typedef int __jmp_buf[6];
typedef struct __jmp_buf_tag	/* C++ doesn't like tagless structs.  */
  {
    /* NOTE: The machine-dependent definitions of `__sigsetjmp'
       assume that a `jmp_buf' begins with a `__jmp_buf'.
       Do not move this member or add others before it.  */
    __jmp_buf __jmpbuf;		/* Calling environment.  */
    int __mask_was_saved;	/* Saved the signal mask?  */
    sigset_t __saved_mask;	/* Saved signal mask.  */
  } jmp_buf[1];

__extern_c_start

int __sigsetjmp(jmp_buf env, int savemask);

__extern_c_end

typedef jmp_buf sigjmp_buf;

# define setjmp(env)	__sigsetjmp ((env), 0)
# define sigsetjmp(env, savemask)	__sigsetjmp ((env), (savemask))

#endif

__extern_c_start

void	longjmp(jmp_buf,int);

void siglongjmp(sigjmp_buf jmp, int val);

__extern_c_end


#endif /* _SETJMP_H_ */
