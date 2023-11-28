#include <OS.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SupportDefs.h>

#include "syscall_gen.h"

/* FIXME for now it's copy from mps.h */
typedef enum
{
	B_INTR_DELIVERY_NONE,

	B_INTR_DELIVERY_FIXED,
	B_INTR_DELIVERY_LOWEST_PRIORITY,
	B_INTR_DELIVERY_NMI,
	B_INTR_DELIVERY_EXT_INTR,
	B_INTR_DELIVERY_OTHER,
	B_INTR_DELIVERY_RESERVED
}	interrupt_delivery_mode_t;

const char* const interrupt_delivery_mode_names[] =
{
	"none           ",

	"fixed          ",
	"lowest priority",
	"NMI            ",
	"buggy SMP BIOS: external (8259 PIC)",
	"other          ",
	"reserved       "
};


#define	B_INTR_DM_USE_PHYSICAL		0x1

typedef struct
{
	long						irq;
	uint32						cpu_affinity;
	interrupt_delivery_mode_t	delivery_mode;
	uint32						flags;
}	interrupt_mode_t;

/*
struct { status_t err; const char* name; } err_names[] =
{
	{ B_NO_MEMORY, "B_NO_MEMORY" }, 
	{ B_IO_ERROR,	"B_IO_ERROR" },				
	{ B_PERMISSION_DENIED, "B_PERMISSION_DENIED" },		
	{ B_BAD_INDEX, "B_BAD_INDEX" },				
	{ B_BAD_TYPE, "B_BAD_TYPE" },					
	{ B_BAD_VALUE, "B_BAD_VALUE" },				
	{ B_MISMATCHED_VALUES, "B_MISMATCHED_VALUES" },		
	{ B_NAME_NOT_FOUND,	 "B_NAME_NOT_FOUND" },		
	{ B_NAME_IN_USE,	 "B_NAME_IN_USE" },		
	{ B_TIMED_OUT,		 "B_TIMED_OUT" },	
    { B_INTERRUPTED,     "B_INTERRUPTED" },     
	{ B_WOULD_BLOCK,     "B_WOULD_BLOCK" },
    { B_CANCELED,        "B_CANCELED" }, 
	{ B_NO_INIT,		 "B_NO_INIT" },	
	{ B_BUSY,			 "B_BUSY" },		
	{ B_NOT_ALLOWED,	 "B_NOT_ALLOWED" },			
	{ B_OK,				"unknown" }
};


static const char*
my_strerror(status_t err)
{
	int	i;
	
	for(i=0; err_names[i].err != B_OK; i++ )
		if(err == err_names[i].err)
			break;
	return  err_names[i].name;
}
*/

static status_t
get_interrupt_mode(interrupt_mode_t* intr_mode)
{
	return _kgeneric_syscall_(GSC_GET_INTERRUPT_MODE, intr_mode, sizeof(interrupt_mode_t));
}


static status_t
set_interrupt_mode(const interrupt_mode_t* intr_mode)
{
	return _kgeneric_syscall_(GSC_SET_INTERRUPT_MODE, (void*)intr_mode, sizeof(interrupt_mode_t));
}

static bool
get_check_focus_mode(void)
{
	return _kgeneric_syscall_(GSC_GET_CHECK_FOCUS_MODE, NULL, 0);
}


static bool
set_check_focus_mode(bool new_check_focus)
{
	bool new_check_focus_tmp = new_check_focus;
	return _kgeneric_syscall_(GSC_SET_CHECK_FOCUS_MODE, &new_check_focus_tmp, sizeof(new_check_focus_tmp));
}


static void 
print_check_focus_mode(void)
{
	status_t	fm = get_check_focus_mode();
	
	printf("focus processor checking is ");
	switch(fm)
	{
		case FALSE:
			printf("disabled\n");
			break;
			
		case TRUE:
			printf("enabled\n");
			break;
		
		default:
			printf("undetermined. error = %d, %s\n", fm, strerror(fm));
	}
}


static void 
print_interrupt_mode(const interrupt_mode_t* intr_mode)
{
	int cpu;
	
	printf("interrupt %02d: %s, %s, CPU(s): ", intr_mode->irq, interrupt_delivery_mode_names[intr_mode->delivery_mode],
		(intr_mode->flags & B_INTR_DM_USE_PHYSICAL) ? "physical" : "logical ");
	for(cpu=0; cpu<32; cpu++)
		if(intr_mode->cpu_affinity & (1<<cpu))
			printf("%d ", cpu);
	printf("\n");
}


static void
print_all_interrupts_mode(void)
{
	interrupt_mode_t	intr_mode;
	
	for(intr_mode.irq=0; get_interrupt_mode(&intr_mode)== B_OK;  intr_mode.irq++)
	{
		if(intr_mode.delivery_mode != B_INTR_DELIVERY_NONE)
			print_interrupt_mode(&intr_mode);
	}
}

static bool
compare_interrupt_modes(const interrupt_mode_t*	im1, const interrupt_mode_t* im2)
{
	return ((im1->cpu_affinity	== im2->cpu_affinity) &&
			(im1->delivery_mode	== im1->delivery_mode) &&
			(im1->flags			== im1->flags));
}

static status_t 
set_all_interrupts_mode(interrupt_mode_t*	new_intr_mode)
{
	interrupt_mode_t	intr_mode;
	status_t			rv;
	status_t			ret = B_OK;

	for(new_intr_mode->irq=intr_mode.irq=0; get_interrupt_mode(&intr_mode)== B_OK;  new_intr_mode->irq = ++intr_mode.irq)
	{
		switch(intr_mode.delivery_mode)
		{
		case B_INTR_DELIVERY_FIXED:
		case B_INTR_DELIVERY_LOWEST_PRIORITY:
			if(!compare_interrupt_modes(new_intr_mode, &intr_mode))
			{
				rv = set_interrupt_mode(new_intr_mode);
				if((rv != B_OK) || (get_interrupt_mode(&intr_mode) != B_OK) || (!compare_interrupt_modes(new_intr_mode, &intr_mode)) )
				{
					fprintf(stderr, "Can't set interrupt mode for interrupt %d, error = %x, %s\n", intr_mode.irq, rv, strerror(rv));
					ret = B_ERROR;		
				}
			}
			break;
		}
	}
	return ret;
}

static void
print_usage(void)
{
	printf(
	"intraff [-help] [-list] [-one | -all] [-focus | -nofocus] [irq# cpu_affinity_bitmap]\n"
	"   -list                     - list mode for all interrupts\n"
	"   -one                      - distribute all interrupts to the one cpu\n"
	"   -all                      - distribute all interrupts to the all cpus\n"
	"   -focus                    - enable checking of a focus cpu\n"
	"   -nofocus                  - disable checking of a focus cpu\n"
	"   -irq# cpu_affinity_bitmap - disribute the irq# interrupt only to cpus in cpu_affinity_bitmap\n\n"
	"Examples:\n"
	"   intraff -list -all -nofocus -list\n"
	"   intraff -one\n"
	"   intraff -one  14 3  15 3\n\n"
	"-all -nofocus combination is the best for a general BeOS system\n"
	"-one is a compatability mode\n"
	);
}



int main(int argc, char* argv[])
{
	interrupt_mode_t	intr_mode_one	= {0, 1, B_INTR_DELIVERY_FIXED, B_INTR_DM_USE_PHYSICAL};
	interrupt_mode_t	intr_mode_all	= {0, 42, B_INTR_DELIVERY_LOWEST_PRIORITY, 0};
	interrupt_mode_t	intr_mode		= {0, 42, B_INTR_DELIVERY_LOWEST_PRIORITY, 0};
	
	system_info			sysinfo;
	int					i;
	long				irq;
	const char*			opt;
	
	if(argc < 2)
	{
		print_usage();
		return 1;
	}
	
	get_system_info(&sysinfo);
	intr_mode_all.cpu_affinity = (1<<sysinfo.cpu_count)-1; 
	
	for(i=1; i<argc; i++)
	{
		opt = argv[i]; 

		if(!strcmp(opt, "-help"))
		{
			print_usage();
		} else
		if(!strcmp(opt, "-list"))
		{
			print_check_focus_mode();
			print_all_interrupts_mode();
		} else
		if(!strcmp(opt, "-one"))
		{
			set_all_interrupts_mode(&intr_mode_one);
		} else
		if(!strcmp(opt, "-all"))
		{
			set_all_interrupts_mode(&intr_mode_all);
		} else
		if(!strcmp(opt, "-focus"))
		{
			set_check_focus_mode(TRUE);
		} else
		if(!strcmp(opt, "-nofocus"))
		{
			set_check_focus_mode(FALSE);
		} else
		if((irq = atoi(opt)) != 0)
		{
			intr_mode.cpu_affinity = atoi(argv[i+1]);
			intr_mode.irq = irq;
			set_interrupt_mode(&intr_mode);
			i++; 
		}
		else
		{
			fprintf(stderr, "Bad option: %s\n", opt);
		}
	}
	
	return 0;
}
