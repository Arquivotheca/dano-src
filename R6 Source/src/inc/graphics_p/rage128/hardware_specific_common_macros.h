//////////////////////////////////////////////////////////////////////////////
// Hardware-Specific Macros
//
// NAME : hardware_specific_common_macros.h
// DESC : 
//    This file contains the definitions of macros used by the generic
// skeleton that are hardware-specific.
//
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// Macros ////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#include "definesR128.h"
#include "registersR128.h"

//////////////////////////////////////////////////////////////////////////////
// General macros.


// The name of the graphics driver. This should be an ordinary string of the
// form "<foo>", as it will be used in "<foo>""<bar>" style string constructs
// to produce longer names for semaphores, memory areas, etc.
// sprintf would be overkill (and less robust) for this.
#define DRIVER_NAME "r128 graphics driver"


// PCI device vendor ID
#define ATI_VENDOR 0x1002


// define the name of the accelerant that we
// should look for when getting accelerant signiture
//#define ACCELERANT_NAME "matrox.accelerant"        
#define ACCELERANT_NAME "rage128.accelerant"     



/////////////////////////////////////////////////////////////////////////////
// Register read and write macros used by my code.                         //
/////////////////////////////////////////////////////////////////////////////
//#define REG_DEBUG

//	ddprintf(("WRITE_REG( %08x[%08x], %08x )) \n",X, (int32)(&((volatile uint32 *) (si->card.regs))[X/4]),Y));                  
//	ddprintf(("READ_REG( %08x[%08x] )) = %08x  \n",X, 							
//	(int32)(&((volatile uint32 *) (si->card.regs))[X/4]),((volatile uint32 *) (si->card.regs))[X/4] ));

#ifdef REG_DEBUG
    #define DEBUG_PRINT_WRITE(X,Y) ddprintf(("WRITE_REG( %08x[%s], %08x )) \n",X, find_mnemonic (X),Y))
	#define DEBUG_PRINT_READ(X,Y)  ddprintf(("READ_REG( %08x[%s] )) = %08x  \n",X,find_mnemonic ( X),((volatile uint32 *) (si->card.regs))[X/4] )); 
#else
    #define DEBUG_PRINT_WRITE(X,Y)
	#define DEBUG_PRINT_READ(X,Y)
#endif	

char * find_mnemonic ( int address );

//#define WAIT4ME snooze(750);
#define WAIT4ME

#ifdef REG_DEBUG
  #define WRITE_REG(X,Y);								  \
	WAIT4ME                                               \
	DEBUG_PRINT_WRITE(X,Y) ;                              \
	((volatile uint32 *) (si->card.regs))[X/4] = (Y);	
	
  //org-todd	((volatile uint32 *) (regs))[X] = (Y);	
  #define READ_REG(X,Y);		                         \
	WAIT4ME                                              \
	DEBUG_PRINT_WRITE(X,Y);                              \
	Y = ((volatile uint32 *) (si->card.regs))[X/4];

#else
  #define WRITE_REG(X,Y);								 \
	((volatile uint32 *) (si->card.regs))[X/4] = (Y);	
	
//org-todd	((volatile uint32 *) (regs))[X] = (Y);	
  #define READ_REG(X,Y);								\
	Y = ((volatile uint32 *) (si->card.regs))[X/4];
#endif	
//	Y = ((volatile uint32 *) (regs))[X];

//////////////////////////////////////////////////////////////////////////////
// This Is The End Of The File ///////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
