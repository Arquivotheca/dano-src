#include <stdio.h>
#include <OS.h>

#include "syscall_gen.h"

#if __INTEL__

static void
disable_cpus_serial_number(int cpu_count)
{
	bool		old_sne[B_MAX_CPU_COUNT];
	cpuid_info	info;
	int			cpu_num;

	for(cpu_num=0; cpu_num <cpu_count; ++cpu_num)
	{
		get_cpuid(&info, 1, cpu_num);
		old_sne[cpu_num] = (0 != (info.eax_1.features & (1<<18)));
	}
	
	_kgeneric_syscall_(GSC_DISABLE_CPUS_SERIAL_NUMBER, NULL, 0);

	for(cpu_num=0; cpu_num <cpu_count; ++cpu_num)
	{
		get_cpuid(&info, 1, cpu_num);
		printf("CPU #%d serial number:  old state: %s,  new state: %s\n", cpu_num, 
			old_sne[cpu_num] ? "enabled" : "disabled", (info.eax_1.features & (1<<18)) ? "STILL ENABLED!" : "disabled" );
	}
}


static void
dump_cache_info(uint32 info, const char* name, uint s0, uint s1, uint s2, uint s3)
{
	uint32	ls, lpt, ass, s;
	
	ls = info & ((1<<s0)-1);
	info >>=s0;
	
	lpt = info & ((1<<s1)-1);
	info >>=s1;

	ass = info & ((1<<s2)-1);
	info >>=s2;

	s = info & ((1<<s3)-1);
	info >>=s3;
	
	printf("\t%s %u kbytes, ", name, s);
	if(ass == ((1<<s2)-1) )
		printf("fully associative, ");
	else
		printf("%u-way set associative, ", ass);
	printf("%u lines/tag, %u bytes/line\n", lpt, ls);	
}

static void
dump_TLB_info(uint16 info, const char* name)
{
	const uint8* const ptr = (const uint8*)&info;
	
	printf("\t%s %u entries, ", name, ptr[0]);
	if(ptr[1] == 0xFF )
		printf("fully associative\n");
	else
		printf("%u-way set associative\n", ptr[1]);
}


static void
dump_cpuid_info(int cpu_count)
{
	int	cpu_num;
	
	for(cpu_num=0; cpu_num <cpu_count; ++cpu_num)
	{
		cpuid_info	info;
		uint32		max_eax;
		uint32		cpu_sig;
		char		cpu_hw_name[64];
		uint		i;
		bool		is_intel_cpu;
		bool		is_cyrix_cpu;

		//if(get_cpuid(NULL, 0, cpu_num) != B_OK)
		if(get_cpuid(&info, 0, cpu_num) != B_OK)
			continue;
		max_eax = info.eax_0.max_eax;
		printf("CPU #%d: %.12s\n", cpu_num, info.eax_0.vendorid);
		is_intel_cpu = !strncmp(info.eax_0.vendorid, "GenuineIntel", 12);
		is_cyrix_cpu = !strncmp(info.eax_0.vendorid, "CyrixInstead", 12);
		
		if(get_cpuid(&info, 1, cpu_num) != B_OK)
			continue;
		printf("\ttype %u, family %u, model %u, stepping %u, features 0x%08x\n",
			info.eax_1.type, info.eax_1.family, info.eax_1.model, info.eax_1.stepping, info.eax_1.features);
		cpu_sig = info.regs.eax; 
			
		if((max_eax < 2) || is_cyrix_cpu)	/* Cyrix's cache cpuid documentation is poor. */
			goto extended_cpuid;
		if(get_cpuid(&info, 2, cpu_num) == B_OK)
		{
			uint8 call_num = info.eax_2.call_num;
			
			while(call_num--)	/* FIXME somehow lock CPUID with eax == 2 ??? */
			for(i=0; i<15; i++)
			{
				char	unknown_desc_buf[64];
				char*	cache_desc;
				
				if(!(info.eax_2.cache_descriptors[2 + ((i+1) & 0xFC)] & 0x80))
				{
					switch(info.eax_2.cache_descriptors[i])
					{
					case 0x00: /* Null descriptor */
						continue;
					case 0x01: cache_desc = "Instruction TLB: 4k-byte pages, 4-way set associative, 32 entries"; break;
					case 0x02: cache_desc = "Instruction TLB: 4M-byte pages, fully associative, 2 entries"; break;
					case 0x03: cache_desc = "Data TLB: 4k-Byte Pages, 4-way set associative, 64 entries"; break;
					case 0x04: cache_desc = "Data TLB: 4M-Byte Pages, 4-way set associative, 8 entries"; break;
					case 0x06: cache_desc = "L1I cache: 8 kbytes, 4-way set associative, 32 bytes/line"; break;
					case 0x08: cache_desc = "L1I cache: 16 kbytes, 4-way set associative, 32 bytes/line"; break;
					case 0x0A: cache_desc = "L1D cache: 8 kbytes, 2-way set associative, 32 bytes/line"; break;
					case 0x0C: cache_desc = "L1D cache: 16 kbytes, 2-way or 4-way set associative, 32 bytes/line"; break;
					case 0x40: cache_desc = "No L2 Cache"; break;
					case 0x41: cache_desc = "L2 cache: 128 kbytes, 4-way set associative, 32 bytes/line"; break;
					case 0x42: cache_desc = "L2 cache: 256 kbytes, 4-way set associative, 32 bytes/line"; break;
					case 0x43: cache_desc = "L2 cache: 512 kbytes, 4-way set associative, 32 bytes/line"; break;
					case 0x44: cache_desc = "L2 cache: 1 Mbyte, 4-way set associative, 32 bytes/line"; break;
					case 0x45: cache_desc = "L2 cache: 2 Mbyte, 4-way set associative, 32 bytes/line"; break;
					case 0x82: cache_desc = "L2 cache: 256 kbytes, 8-way set associative, 32 bytes/line"; break;
					default:   cache_desc = unknown_desc_buf; sprintf(unknown_desc_buf, "Unknown cache descriptor 0x%02x", info.eax_2.cache_descriptors[i]);
					}
					printf("\t%s\n", cache_desc);
				}
			}
		}

		if(max_eax < 3)
			goto extended_cpuid;
		if(get_cpuid(&info, 3, cpu_num) == B_OK)
			printf("\tSerial number: %04X-%04X-%04X-%04X-%04X-%04X\n",
				cpu_sig>>16, cpu_sig & 0xFFFF,
				info.eax_3.serial_number_high>>16, info.eax_3.serial_number_high & 0xFFFF,
				info.eax_3.serial_number_low>>16, info.eax_3.serial_number_low & 0xFFFF);
		
extended_cpuid:
		if(is_intel_cpu)
			continue;
		if(get_cpuid(&info, 0x80000000, cpu_num) != B_OK)
			continue;
		max_eax = info.regs.eax;
		
		if(max_eax < 0x80000001)
			continue;
		if(get_cpuid(&info, 0x80000001, cpu_num) != B_OK)
			continue;
		printf("\tExtended information:\n");
		printf("\ttype %u, family %u, model %u, stepping %u, features 0x%08x\n",
			info.eax_1.type, info.eax_1.family, info.eax_1.model, info.eax_1.stepping, info.eax_1.features);
			
		if(max_eax < 0x80000004)
			continue;
		memset(cpu_hw_name, 0, sizeof(cpu_hw_name));
		for(i=0; i<3; i++)
		{
			uint32 tmp;
			if(get_cpuid(&info, 0x80000002+i, cpu_num) != B_OK)
				continue;
			tmp = info.regs.ecx;
			info.regs.ecx = info.regs.edx;
			info.regs.edx = tmp;
			memcpy(cpu_hw_name + 16*i, &info, 16); 
		}
		printf("\tName: %s\n", cpu_hw_name);
		
		if(is_cyrix_cpu)	/* Cyrix's cache cpuid documentation is poor. */
			continue;
	
		if(max_eax < 0x80000005)
			continue;
		if(get_cpuid(&info, 0x80000005, cpu_num) != B_OK)
			continue;
		dump_TLB_info(info.regs.ebx & 0xFFFF, "instruction TLB:");
		dump_TLB_info(info.regs.ebx >>16,     "data TLB       :");
		dump_cache_info(info.regs.edx, "L1I cache:", 8,8,8,8);
		dump_cache_info(info.regs.ecx, "L1D cache:", 8,8,8,8);

		if(max_eax < 0x80000006)
			continue;
		if(get_cpuid(&info, 0x80000006, cpu_num) != B_OK)
			continue;
		dump_cache_info(info.regs.ecx, "L2  cache:", 8,4,4,16);
	}
	printf("\n");
}
#endif


void
dump_cpu(system_info *sysinfo)
{
	char *name;
	
	switch(sysinfo->cpu_type) {
#if __POWERPC__
	case B_CPU_PPC_601:
		name = "PowerPC 601";
		break;
	case B_CPU_PPC_603:
		name = "PowerPC 603";
		break;
	case B_CPU_PPC_603e:
		name = "PowerPC 603e";
		break;
	case B_CPU_PPC_750:
		name = "PowerPC 750";
		break;
	case B_CPU_PPC_604:
		name = "PowerPC 604";
		break;
	case B_CPU_PPC_604e:
		name = "PowerPC 604e";
		break;
#endif
#if __INTEL__
	case B_CPU_X86:
		name = "unknown x86";
		break;
	case B_CPU_INTEL_PENTIUM:
	case B_CPU_INTEL_PENTIUM75:
		name = "Pentium";
		break;
	case B_CPU_INTEL_PENTIUM_486_OVERDRIVE:
	case B_CPU_INTEL_PENTIUM75_486_OVERDRIVE:
		name = "Pentium 486 Overdrive";
		break;
	case B_CPU_INTEL_PENTIUM_MMX:
	case B_CPU_INTEL_PENTIUM_MMX_MODEL_8:
		name = "Pentium MMX";
		break;
	case B_CPU_INTEL_PENTIUM_PRO:
		name = "Pentium Pro";
		break;
	case B_CPU_INTEL_PENTIUM_II_MODEL_3:
		name = "Pentium II model 3";
		break;
	case B_CPU_INTEL_PENTIUM_II_MODEL_5:
		name = "Pentium II model 5";
		break;
	case B_CPU_INTEL_CELERON:
		name = "Celeron";
		break;
	case B_CPU_INTEL_PENTIUM_III:
	case B_CPU_INTEL_PENTIUM_III_MODEL_8:
		name = "Pentium III";
		break;
	case B_CPU_AMD_K5_MODEL0:
	case B_CPU_AMD_K5_MODEL1:
	case B_CPU_AMD_K5_MODEL2:
	case B_CPU_AMD_K5_MODEL3:
		name = "K5";
		break;
	case B_CPU_AMD_K6_MODEL6:
	case B_CPU_AMD_K6_MODEL7:
		name = "K6";
		break;
	case B_CPU_AMD_K6_2:
		name = "K6-2";
		break;
	case B_CPU_AMD_K6_III:
		name = "K6-III";
		break;
	case B_CPU_AMD_ATHLON_MODEL1:
		name = "Athlon";
		break;
	case B_CPU_CYRIX_GXm:
		name = "GXm";
		break;
	case B_CPU_CYRIX_6x86MX:
		name = "6x86MX";
		break;
	case B_CPU_IDT_WINCHIP_C6:
		name = "WinChip C6";
		break;
	case B_CPU_IDT_WINCHIP_2:
		name = "WinChip 2";
		break;
	case B_CPU_RISE_mP6:
		name = "mP6";
		break;
#endif
	default:
		name = "(unknown)";
		break;
	}
	printf("%d %s revision %.4x running at %dMHz ",
		sysinfo->cpu_count, name, sysinfo->cpu_revision,
		(int) (sysinfo->cpu_clock_speed / 1000000));

    printf("(ID: 0x%.8x 0x%.8x)\n", sysinfo->id[0], sysinfo->id[1]);

#if __INTEL__
	dump_cpuid_info(sysinfo->cpu_count);
#endif

}



void
dump_platform(system_info *sysinfo)
{
	char *name;
	
	switch(sysinfo->platform_type) {
	case B_BEBOX_PLATFORM:
		name = "BeBox";
		break;
	case B_MAC_PLATFORM:
		name = "Macintosh";
		break;
	case B_AT_CLONE_PLATFORM:
		name = "IntelArchitecture";
		break;
	default:
		name = "unknown";
		break;
	}
	
	printf("%s\n", name);
}



void
dump_mem(system_info *sysinfo)
{
	printf("%8d bytes free      (used/max %8d / %8d)\n",
		   sysinfo->max_pages * B_PAGE_SIZE - sysinfo->used_pages * B_PAGE_SIZE,
		   sysinfo->used_pages * B_PAGE_SIZE, sysinfo->max_pages * B_PAGE_SIZE);
}


void
dump_sem(system_info *sysinfo)
{
	printf("%8d semaphores free (used/max %8d / %8d)\n",
		   sysinfo->max_sems - sysinfo->used_sems,
		   sysinfo->used_sems,    sysinfo->max_sems);
}

void
dump_ports(system_info *sysinfo)
{
	printf("%8d ports free      (used/max %8d / %8d)\n",
		sysinfo->max_ports - sysinfo->used_ports,   
		sysinfo->used_ports, sysinfo->max_ports);
}
		
void
dump_thread(system_info *sysinfo)
{
	printf("%8d threads free    (used/max %8d / %8d)\n",
		   sysinfo->max_threads - sysinfo->used_threads, 
		   sysinfo->used_threads, sysinfo->max_threads);
}

void
dump_team(system_info *sysinfo)
{
	printf("%8d teams free      (used/max %8d / %8d)\n",
		   sysinfo->max_teams - sysinfo->used_teams,   
		   sysinfo->used_teams, sysinfo->max_teams);
}


void
dump_kinfo(system_info *sysinfo)
{
	printf("Kernel name: %s built on: %s %s version 0x%Lx\n",
		   sysinfo->kernel_name, sysinfo->kernel_build_date,
		   sysinfo->kernel_build_time, sysinfo->kernel_version);
}

void
dump_system_info(system_info *sysinfo)
{
	dump_kinfo(sysinfo);
	dump_cpu(sysinfo);
	dump_mem(sysinfo);
	dump_sem(sysinfo);
	dump_ports(sysinfo);
	dump_thread(sysinfo);
	dump_team(sysinfo);
}


void
do_help(char *pname)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "\t%s [-id|-cpu|-mem|-semaphore|-ports|-threads|-teams"
			"|-platform|-disable_cpu_sn]\n", pname);
}

int
main(int argc, char **argv)
{
	int i;
	system_info	sysinfo;

    if (is_computer_on() == 0) {
        printf("The computer is not on! No info available\n");
		exit(-1);
    }

	if (get_system_info(&sysinfo) < B_NO_ERROR) {
		printf("Error getting system information!\n");
		return 1;
	}

	if (argc < 2) {
		dump_system_info(&sysinfo);
		return 0;
	}

	for(i=1; i < argc; i++) {
		if (strncmp(argv[i], "-id", strlen(argv[i])) == 0) {
			printf("0x%.8x 0x%.8x\n", sysinfo.id[0], sysinfo.id[1]);
		} else if (strncmp(argv[i], "-cpu", strlen(argv[i])) == 0) {
			dump_cpu(&sysinfo);
		} else if (strncmp(argv[i], "-mem", strlen(argv[i])) == 0) {
			dump_mem(&sysinfo);
		} else if (strncmp(argv[i], "-semaphores", strlen(argv[i])) == 0) {
			dump_sem(&sysinfo);
		} else if (strncmp(argv[i], "-ports", strlen(argv[i])) == 0) {
			dump_ports(&sysinfo);
		} else if (strncmp(argv[i], "-threads", strlen(argv[i])) == 0) {
			dump_thread(&sysinfo);
		} else if (strncmp(argv[i], "-teams", strlen(argv[i])) == 0) {
			dump_team(&sysinfo);
		} else if (strncmp(argv[i], "-kinfo", strlen(argv[i])) == 0) {
			dump_kinfo(&sysinfo);
		} else if (strncmp(argv[i], "-platform", strlen(argv[i])) == 0) {
			dump_platform(&sysinfo);
#if __INTEL__
		} else if (strncmp(argv[i], "-disable_cpu_sn", strlen(argv[i])) == 0) {
			disable_cpus_serial_number(sysinfo.cpu_count);
#endif
		} else if (argv[i]) {
			do_help(argv[0]);
			return 0;
		}
	}
	

	return 0;
}
