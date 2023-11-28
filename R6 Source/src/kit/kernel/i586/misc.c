#include <unistd.h>
#include <signal.h>
#include <OS.h>
#include <image.h>
#include <PCI.h>
#include <nvram.h>
#include <priv_syscalls.h>
#include <Debug.h>

#include "fsc.h"

int32	*cv_factor_ptr;
int64	*system_time_base_ptr;
int64	*system_real_time_base_ptr;

uint32	__supported_fsc_types = 0;



void
debugger(const char *str)
{
	_debugger(str);
}


void
_eieio(void)
{
}


void _init_fast_syscall(void)
{
	system_info	sysinfo;
	cpuid_info	info;
	
	get_system_info(&sysinfo);
	
	switch(sysinfo.cpu_type & B_CPU_X86_VENDOR_MASK)
	{
	case B_CPU_INTEL_X86:
		if(	(get_cpuid(&info, 1, 0) == B_OK) &&	(info.eax_1.features & (1<<11)) )
		{
			/* from Intel's docs */
			if((info.eax_1.family == 6) && (info.eax_1.model < 3))
			{
				break; /* PPro does not support SYSENTER/SYSEXIT */ 
			}
			else
			{
				__supported_fsc_types = FSC_INTEL;
			}
		}	
		break;
	
	case B_CPU_AMD_X86:
		if(	(get_cpuid(&info, 0x80000001, 0) == B_OK) && (info.regs.edx & (1<<11)) )
		{
			__supported_fsc_types = FSC_AMD;
		}	
		break;	
	}
}

