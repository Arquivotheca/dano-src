#ifndef SISGLOBALS_H
#define SISGLOBALS_H

#include <kernel/OS.h>
#include <device/graphic_driver.h>
#include <drivers/KernelExport.h>
#include <drivers/PCI.h>
#include <drivers/ISA.h>
#include <Debug.h>
#include <Drivers.h>
#include <Accelerant.h>
#include <GraphicsCard.h>
#include <video_overlay.h>
#include <SupportDefs.h>
#include <stdio.h>
#include <string.h>
#include <drivers/genpool_module.h>

#include "SiS/bena4.h"
#include "SiS/sis5598defs.h"
#include "SiS/sis6326defs.h"
#include "SiS/sis620defs.h"
#include "SiS/sis_accelerant.h"

#define DEBUG 1
#define SIS_VERBOSE
//#define SIS_MEGA_VERBOSE


// SIS CHIPSET

#define MEMORY_MARGIN (256*1024)

///////////////////
// DEBUG SETTINGS
///////////////////

// vvddprintf : mega verbose mode
// vddprintf : verbose mode
// ddprintf  : debug mode

#ifdef SIS_MEGA_VERBOSE
#define vvddprintf ddprintf
#define SIS_VERBOSE
#else
#define vvddprintf (void)
#endif

#ifdef SIS_VERBOSE
#define vddprintf ddprintf
#else
#define vddprintf (void)
#endif

#if DEBUG > 0

#ifdef COMPILING_ACCELERANT
#define DPRINTF_ON _kset_dprintf_enabled_(TRUE) 
#define ddprintf(a) _kdprintf_ a
#else
#define DPRINTF_ON  set_dprintf_enabled(TRUE)
#define ddprintf(a) dprintf a
#endif

#else

#define DPRINTF_ON
#define ddprintf(a)

#endif

///////////////////////////
// INPUT/OUTPUT FUNCTIONS
///////////////////////////

#define inb(a)      *((vuchar *)(ci->ci_RegBase+(a)))
#define outb(a,b)   *((vuchar *)(ci->ci_RegBase+(a)))=(b)
#define inw(a)      *((vushort*)(ci->ci_RegBase+(a)))
#define outw(a,b)   *((vushort*)(ci->ci_RegBase+(a)))=(b)
#define inl(a)      *((vulong *)(ci->ci_RegBase+(a)))
#define outl(a,b)   *((vulong *)(ci->ci_RegBase+(a)))=(b)

#endif
