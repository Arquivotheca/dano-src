// stubs so that we can compile in IAD with memory checking
// turned on, and run in EXP with it doing the checking.
#include <OS.h>
void chkr_check_addr(const void *address, size_t len, unsigned char right)
{
	(void)address;
	(void)len;
	(void)right;
#if 0
	debugger("Get out of here!  We don't support purify debugging!");
#endif
}

void chkr_check_str(const char *ptr, unsigned char right)
{
	(void)ptr;
	(void)right;
#if 0
	debugger("Get out of here!  We don't support purify debugging!");
#endif
}

void chkr_set_right(const void *address, size_t len, unsigned char right)
{
	(void)address;
	(void)len;
	(void)right;
#if 0
	debugger("Get out of here!  We don't support purify debugging!");
#endif
}

void chkr_copy_bitmap (void* dest, void* src, size_t orig_len)
{
	(void)dest;
	(void)src;
	(void)orig_len;
#if 0
	debugger("Get out of here!  We don't support purify debugging!");
#endif
}

void chkr_check_exec(const void *ptr)
{
	(void)ptr;
#if 0
	debugger("Get out of here!  We don't support purify debugging!");
#endif
}
