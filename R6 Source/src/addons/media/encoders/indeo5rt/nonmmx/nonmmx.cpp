#include <OS.h>

bool check_mmx_match()
{
	cpuid_info ci;
	get_cpuid(&ci, 1, 0);
	return (ci.eax_1.features & (1<<23)) ? false : true;
}
