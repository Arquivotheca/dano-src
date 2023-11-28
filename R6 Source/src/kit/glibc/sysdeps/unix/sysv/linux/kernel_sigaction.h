/* This is the sigaction structure from the Linux 2.1.20 kernel.  */

#define HAVE_SA_RESTORER

struct old_kernel_sigaction {
	__sighandler_t k_sa_handler;
	unsigned long sa_mask;
	unsigned long sa_flags;
	void (*sa_restorer) __P ((void));
};

/* This is the sigaction structure from the Linux 2.1.68 kernel.  */

struct kernel_sigaction {
	__sighandler_t k_sa_handler;
	unsigned long sa_flags;
	void (*sa_restorer) __P ((void));
	sigset_t sa_mask;
};
