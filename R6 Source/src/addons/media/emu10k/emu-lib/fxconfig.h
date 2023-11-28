/* @doc OSAPI */
/******************************************************************
*
*   Copyright (c) E-mu Systems, Inc. 1997.  All Rights Reserved
*
*******************************************************************
*
* @module fxconfig.h |
*
* This file contains general definitions which can
* be modified by the user to fit their platform.
*
* @comm Host resource and OS-specific definitions are
* expected to be changed depending on the requirements
* of the product.
*
*******************************************************************
*/

#ifndef _FXCONFIG_H
#define _FXCONFIG_H

#include "datatype.h"
#include "hal8210.h"
#include "cfg8210.h"
#include "os8210.h"
#include "ip8210.h"
#include "dbg8210.h"

BEGINEMUCTYPE

/******************************************************************
*
* Host Resource Definitions
* These definitions can be overridden by the user
*
* Note:  If the FX_DYNAMIC flag (below) is cleared, the values of
*        of the following constants will have a substantial effect
*        on the amount of memory that is allocated to the managers
*        up front.  These formulae show the static allocation 
*        requirements of the managers (as they relate to the FXMAX
*        constants):
*
*        Manager					Static bytes:
*        -------					-------------
*        Resource					7464 * CHIPS +
*									  20 * CHIPS*PARTS +
*									 120 * CHIPS*PGMS 
*        Program					TBD
*        Parameter					TBD
*        Patch						TBD
******************************************************************
*/
#define FXMAX_CHIPS				2		/* Per machine */
#define FXMAX_PARTITIONS		32		/* Per chip */
#define FXMAX_PROGRAMS			256		/* Per chip */
#define FXMAX_CALLBACKS			256*3	/* Per chip */
#define FXMAX_INTERPOLATORS		8		/* Per chip */
#define FXMAX_STUBS				4		/* Per program (on average) */

/* Preallocated map structures */
#define MAX_INODES			FXMAX_CHIPS*FXMAX_PROGRAMS*6 
								  /* (max pgms + max holes) *
								   * ( 1 program space +
								   *   1 TRAM space    +
								   *   1 XTRAM or table space ) 
								   */

/* Validate FXIDs before use? */
#define VALIDATE_PGMID			TRUE
#define VALIDATE_PARTITIONID	TRUE
#define VALIDATE_CHIPID			TRUE

#define XMASK 0xfff

/* End host resource definitions */

/* EMU8010-specific definitions -- possibly to be moved to CHIPINFO
 * structure in fxresman.c.
 */

/* Opcodes */
#define FXINSTR_INTERP		0xE
#define FXINSTR_SKIP		0xF

/* Constants */
#define ZERO_CONSTANT		0x040
#define UNCONDITIONAL_SKIP	0x04F
#define RAMP_ONE			0x7fffffff	/* positive full scale */

/* GPR-mapped registers */
#define CONDITION_CODE_REG	0x057
#define INPUTPORT0			0x000	
#define OUTPUTPORT0			0x020

/* TRAM/XTRAM */
#define FXXTRAMADDRMIN		0x380
#define FXXTRAMADDRMAX		0x39f
#define FXTRAMADDRMIN		0x300
#define FXTRAMADDRMAX		0x37f

/* Ramper constant */
#define RAMP_CONSTANT		0x55

/* These constants specify what is to be done for adjusting values
 * if a TRAM/XTRAM buffer is being used as a general purpose register.
 * I.e. When we read a value ulValue from address addr:
 *  if( addr > LAST_GENERALGPR ) ulValue <<= TRAMADJSHIFT;
 */
#define LAST_GENERALGPR		0x1ff
#define TRAMADJSHIFT		12

/* Typedefs */
typedef struct _fxChipHandleTag {
	HALID halid;
	IPID ipid;
} fxChipHandle;

/* End of EMU8010-specific definitions */

/*****************************************************************
* 
* @contents1 FX8010 Operating System Integration Manual |
*
* This document contains information about the functions and macros
* used to bind the managers to the operating system and FX8010
* hardware.
*
* @head3 Redefinable Macros | 
* These topics cover the macros that must be redefined in order to
* integrate the managers onto a host platform.
*
* @subindex FX8010 Hardware Integration
* @subindex Mutual Exclusion Semaphore Macros
* @subindex OS-Specific Macros
*
******************************************************************
*/

/*****************************************************************
* 
* @contents2 OS-Specific Macros |
*
* These macros can be overridden by the operating system integration
* programmer.
*
* The following macros are used by the various managers to link
* to the operating system on the host platform.  These macros
* should be redefined by the user for their OS.
*
* These macros are:
*
* <f OS_MALLOC><nl><f OS_FREE><nl><f OS_TRAPTIMER><nl><f OS_CALLBACK>
* <nl><f OS_FUNC2HANDLE>
*
******************************************************************
*/

/* The FX_DYNAMIC flag indicates whether or not the system has
 * the ability or the desire to dynamically allocate memory for 
 * the managers.  If this flag is zero, the system will allocate
 * a maximum static buffer at startup and never use more than that.
 * If the value is nonzero, it will use the OS_MALLOC and OS_FREE
 * macros for dynamic allocation.
 */
#define FX_DYNAMIC              0

/* The FXPSEUDO_DYNAMIC flag indicates that the system can dynamically
 * allocate memory at startup only.  Buffers will be allocated during
 * chip discovery and initialization, but will remain intact (never
 * freed) until powered down, or deallocated by the OS.  
 * FXPSEUDO_DYNAMIC is mutually exclusive to FX_DYNAMIC.  If both are
 * defined, FX_DYNAMIC is assumed.
 */
#define FXPSEUDO_DYNAMIC		0


/* @func void * | OS_MALLOC |
 * 
 * Like ANSI <f malloc()>, returns pointer to allocated block of size <p s>,
 * or NULL on error.
 *
 * @parm size_t | s | Specifies size of block, in bytes, to allocate.
 *
 * @rdesc This macro returns a pointer to the block.
 */
#define OS_MALLOC(s)		malloc(s)

/* @func void | OS_FREE |
 *
 * Like ANSI <f free()>, returns block <p p> previously allocated by
 * <f OS_MALLOC()> to the heap.
 * 
 * @parm void *	| p | Specifies block to free.
 */
#define OS_FREE(p)			free(p)


/* @contents2 Mutual Exclusion Semaphore Macros |
 * The following macros are for handling mutual exclusion semaphores.
 * It is the responsibility of the operating system to manage the
 * the handles and to insure that the process that requested the 
 * semaphore is the one to release it.
 *
 * All semaphores are assumed to have some intelligence.  The OS must
 * follow these rules:
 *
 * >> If a semaphore is free, and is requested by a process, it should
 *    be given to that process with a count of zero.
 * >> Each time THAT SAME PROCESS requests the semaphore, the count
 *    should be incremented by one
 * >> Each time THAT SAME PROCESS releases the semaphore, the count
 *    should be decremented by one. Upon a release on a count of zero,
 *    the semaphore should be freed for other processes
 *
 * The default implementation presented assumes a single-tasked
 * system.
 *
 * The macros that must be defined in FXCONFIG.H are as follows:
 *
 * <f OS_CREATEMUTEX><nl><f OS_REQUESTMUTEX><nl><f OS_RELEASEMUTEX>
 * <nl><f OS_WAITMUTEX><nl><f OS_POLLMUTEX><nl><f OS_DELETEMUTEX>
 *
 */

	/* @func BOOL | OS_CREATEMUTEX |
	 *
	 * Creates a unique mutex semaphore and writes its value to <p *pulHandle>.
	 *
	 * @rdesc Returns TRUE on successful creation, and FALSE otherwise.
	 *
	 * @parm ULONG * | pulHandle | Specifies handle buffer for output.
	 *
	 */
	#define OS_CREATEMUTEX(pulHandle)   TRUE

	/* @func BOOL | OS_REQUESTMUTEX |
	 *
	 * Attempts an instantaneous request to obtain the specified mutex
	 * semaphore.
	 *
	 * @parm ULONG | ulHandle | Specifies mutex semaphore.
	 *
     * @rdesc If it is available, the OS gives the semaphore to the
	 * process and returns TRUE.  If it is unavailable, FALSE is returned.
	 * This macro does not wait for it to be available.
	 */
	#define OS_REQUESTMUTEX(ulHandle)   TRUE

	/* @func void | OS_RELEASEMUTEX |
	 * 
	 * Releases a semaphore that was granted to a process.
	 *
	 * @parm ULONG | ulHandle | Specifies mutex semaphore.
	 */
	#define OS_RELEASEMUTEX(ulHandle)   {}

	/* @func void | OS_WAITMUTEX |
	 *
	 * This macro will wait INDEFINITELY for a semaphore to be released
	 * by another process.  Once the semaphore is available, it will be
	 * given to the calling process and execution will continue.
	 *
	 * @parm ULONG | ulHandle | Specifies mutex semaphore.
	 */
	#define OS_WAITMUTEX(ulHandle)      while(0){}

	/* @func void | OS_DELETEMUTEX |
	 * 
	 * The semaphore is no longer needed, and this macro will deallocate
	 * it.
	 *
	 * @parm ULONG | ulHandle | Specifies mutex semaphore.
	 */
	#define OS_DELETEMUTEX(ulHandle)    {}

	/* @func BOOL | OS_POLLMUTEX |
	 * 
	 * This macro is the same as <f OS_REQUESTMUTEX>, except that the
	 * semaphore is not given to the calling process, if it is available.
	 * This is simply to test the status of a semaphore.
	 *
	 * @parm ULONG | ulHandle | Specifies mutex semaphore.
	 *
     * @rdesc If it is available, this macro returns TRUE.  If it is 
	 * unavailable, FALSE is returned.
	 */
	#define OS_POLLMUTEX(ulHandle)      TRUE

/* @func void | OS_TRAPTIMER |
 * 
 * This macro will trap a timer source signal and install function
 * <p fHandler> to be called upon receipt.  It is the responsibility of
 * the lower level code to handle all hardware bit manipulation before
 * or after the handler returns.  This macro will also return a
 * a fraction telling the callback manager what the granularity of the
 * timer is in relation to approximately one sample period.  For
 * example, if the sample rate is 48kHz, and the timer interrupts the
 * manager every millisecond (1kHz), then the ratio of samples to clock
 * is 48/1. Therefore <p *pnNumerator> = 48, and <p *pnDenominator> = 1.
 *
 *
 * @parm (void *)()	| fHandler		| Specifies timer handler function.
 * @parm int *		| pnNumerator	| Specifies buffer for timer numerator output.
 * @parm int *		| pnDenomintor	| Specifies buffer for timer denominator output.
 *
 */
#define OS_TRAPTIMER(fHandler,pnNumerator,pnDenominator)      \
					{ *pnNumerator = 1; *pnDenominator = 1; }

/* @func ULONG	| OS_FUNC2HANDLE |
 * 
 * This macro translates a callback address to a 32-bit handler.  The
 * macro <f OS_CALLBACK> will be used to convert the handle back.  The
 * default implementation presented assumes a single flat address
 * space shared by all processes with no protection.
 *
 * @parm (void *)()(ULONG,ULONG,ULONG,ULONG) | fFunction |
 *  Specifies function to virtualize.
 *
 * @rdesc This macro returns an opaque handle for the function.
 */
#define OS_FUNC2HANDLE(fFunction)   (ULONG)fFunction

/* @func void | OS_CALLBACK |
 * 
 * This macro will perform a callback to a registered function using
 * handle <p ulHandle>, which was returned by <f OS_FUNC2HANDLE>.  A four
 * ULONG parameter list is also provided.  The default implementation
 * presented assumes a single flat address space shared by all
 * processes with no protection.
 *
 * @parm ULONG | ulHandle		| Specifies handle returned by <f OS_FUNC2HANDLE>.
 * @parm ULONG | callID			| Specifies callback ID.
 * @parm ULONG | ulEvent		| Specifies event type.
 * @parm ULONG | ulParam		| Specifies user-defined parameter.
 * @parm ULONG | ulSystemParam	| Specifies system-defined parameter.
 *
 */
//#define OS_CALLBACK(ulHandle,callID,ulEvent,ulParam,ulSystemParam)    \
//         (*(void (*)())ulHandle)(callID,ulEvent,ulParam,ulSystemParam)

#define OS_CALLBACK(ulHandle,callID,ulEvent,ulParam,ulSystemParam)    \
        OSFxCallBack(ulHandle,callID,ulEvent,ulParam,ulSystemParam)    

/*****************************************************************
* 
* @contents2 FX8010 Hardware Interface |
*
* These macros can be overridden by the operating system integration
* programmer.
*
* The following macros are used by the parameter manager to link
* the host platform to the DSP.  These macros should be redefined by the 
* user for their platform.
*
* These macros are:
*
* <f OS_TRAPDSPINTERRUPT><nl><f OS_READGPR><nl><f OS_WRITEGPR>
* <nl><f OS_READINSTRUCTION><nl><f OS_WRITEINSTRUCTION>
* <nl><f OS_READFLAGS><nl><f OS_WRITEFLAGS>
*
******************************************************************
*/
 
/* @func void | OS_TRAPDSPINTERRUPT |
 *
 * This macro will trap the DSP interrupt signal for the specified
 * chip and install function <p fHandler> to be called upon receipt.  It
 * is the responsibility of the lower level code to handle any hardware
 * bit manipulation before or after the handler returns.  The chip handle
 * of the chip that triggered the interrupt will be passed to the handler
 * function.
 * 
 * @parm ULONG			| ulChipHandle	| Specifies opaque hardware handle.
 * @parm (void *)(ULONG)| fHandler		| Specifies handler function.
 */
BEGINEMUCTYPE
BOOL fxIsrCallback(IPHANDLE, enIPType, DWORD, unIPReply*);
BOOL fxCallback(IPHANDLE, enIPType, DWORD, unIPReply*);
void SetDSPfHandler(ULONG);
ENDEMUCTYPE

#define OS_TRAPDSPINTERRUPT(ulChipHandle,fxHandler) {}
#if 0
#define OS_TRAPDSPINTERRUPT(ulChipHandle,fxHandler)   \
{  \
	IPHANDLE ipHandle; \
   \
    stIPInfo ipInfo;  \
	ipInfo.type = IP_FX8010; \
    ipInfo.interruptParameter = 0; \
	ipInfo.userParameter = ulChipHandle; \
    ipInfo.fISRHandler = fxIsrCallback;  /* call function fHandler with ulChipHandel as a param */ \
                                         /* this calls eric's callback always return false */  \
    ipInfo.fHandler = fxCallback;        /* call a dummy function that returns true */         \
    /* get an ip id */    \
	\
	SetDSPfHandler((ULONG)fxHandler); \
    if (ipRegisterCallback(((fxChipHandle *)ulChipHandle)->ipid, &ipInfo, &ipHandle) != SUCCESS) \
        ASSERT(0);          \
} 
#endif

 

/* @func ULONG | OS_READGPR |
 * 
 * This macro returns the value of the physical GPR address of the
 * specified chip.  This is a raw value, no fixing is performed.
 * 
 * @parm ULONG	| ulChipHandle	| Specifies hardware handle.
 * @parm ADDR	| usPhysicalAddr| Specifies 10-bit physical GPR address.
 *
 * @rdesc This macro returns the value of specified GPR.
 */
#define OS_READGPR(ulCH,usP) \
         ((usP<0x200) ? LSEPtrRead(((fxChipHandle *)ulCH)->halid,((ULONG)(usP))<<16) : \
		 (usP<0x300) ? (LSEPtrRead(((fxChipHandle *)ulCH)->halid,((ULONG)(usP))<<16)) : \
  	     (LSEPtrRead(((fxChipHandle *)ulCH)->halid,((ULONG)(usP))<<16)&0xfffff))

/* @func void | OS_WRITEGPR |
 *
 * This macro writes the value <p ulValue> to the physical GPR address of
 * the specified chip.  <p ulValue> is a raw value.
 * 
 * @parm ULONG	| ulChipHandle	| Specifies hardware handles.
 * @parm ADDR	| usPhysicalAddr| Specifies 10-bit physical GPR address.
 * @parm ULONG	| ulValue		| Specifies value to write.
 */
#define OS_WRITEGPR(ulCH,usP,ulV) \
		if(usP<0x200) LSEPtrWrite(((fxChipHandle *)ulCH)->halid,((ULONG)(usP)<<16),ulV); \
		else if(usP<0x300) LSEPtrWrite(((fxChipHandle *)ulCH)->halid,((ULONG)(usP)<<16),ulV&0xfffff); \
		else LSEPtrWrite(((fxChipHandle *)ulCH)->halid,((ULONG)(usP)<<16), \
				(LSEPtrRead(((fxChipHandle *)ulCH)->halid,((ULONG)(usP)<<16))&0x00f00000)|((ulV)&0xfffff));

/* @func void | OS_READINSTRUCTION |
 *
 * This macro reads the value of the physical instruction address into
 * <p pulUCL> and <p pulUCH>.
 *
 * @ex <p ulUCL> and <p ulUCH> are of the format: |
 *
 * ulUCL:
 *
 * // 31                                               16
 * // +-------------------------------------+----------+
 * // | 0  0  0  0  0  0  0  0   0  0  0  0 |  X[9:6]  |
 * // +-------------------------------------+----------+
 * // 15                                               0
 * // +------+----------+------------------------------+
 * // |    X[5:0]       |           Y[9:0]             |
 * // +-----------------+------------------------------+
 *
 * ulUCH:
 *
 * // 31                                               16
 * // +------------------------+------------+----------+
 * // | 0  0  0  0  0  0  0  0 |Opcode[3:0] |  R[9:6]  |
 * // +------------------------+------------+----------+
 * // 15                                               0
 * // +------+----------+------------------------------+
 * // |    R[5:0]       |           A[9:0]             |
 * // +-----------------+------------------------------+
 *
 * 
 * @parm ULONG	| ulChipHandle	| Specifies hardware handles.
 * @parm ADDR	| usPhysicalAddr| Specifies physical instruction address.
 * @parm ULONG *| pulUCL		| Specifies a buffer for the low dword of the instruction.
 * @parm ULONG *| pulUCH		| Specifies a buffer for the high dword of the instruction.
 */
#define OS_READINSTRUCTION(ulCH,usP,pulUCL,pulUCH) \
        *(pulUCL) = LSEPtrRead(((fxChipHandle *)ulCH)->halid,0x04000000+((ULONG)(usP)<<17)); \
        *(pulUCH) = LSEPtrRead(((fxChipHandle *)ulCH)->halid,0x04010000+((ULONG)(usP)<<17));

 /* @func void | OS_WRITEINSTRUCTION |
 *
 * This macro writes the instruction contained in <p ulUCL> and <p ulUCH>
 * to the physical instruction address <p usPhysicalAddr>.
 * The instruction is the same format as in <f OS_READINSTRUCTION>.
 * 
 * @parm ULONG	| ulChipHandle	| Specifies hardware handles.
 * @parm ADDR	| usPhysicalAddr| Specifies physical instruction address.
 * @parm ULONG	| ulUCL			| Specifies the low dword of the instruction.
 * @parm ULONG	| ulUCH			| Specifies the high dword of the instruction.
 */

#define OS_WRITEINSTRUCTION(ulCH,usP,ulUCL,ulUCH) \
		LSEPtrWrite( (((fxChipHandle *)ulCH)->halid),0x04000000+((ULONG)(usP)<<17),ulUCL); \
		LSEPtrWrite( (((fxChipHandle *)ulCH)->halid),0x04010000+((ULONG)(usP)<<17),ulUCH);

 /* @func BYTE | OS_READFLAGS |
 *
 * This macro will return the TRAM addressing flags for the specified
 * TRAM/XTRAM engine buffer.  <p usTRAMbuf> contains the physical
 * TRAM/XTRAM address buffer address.
 *
 * @ex  The returned flag byte is in the following format: |
 *
 * +------------------------+
 * |   M[3:0]    0  0  0  A |
 * +------------------------+
 *
 * @rdesc The flags are returned where M is the PHYSICAL enumeration of the access mode for the
 * specified chip, and A is the align bit.
 * 
 * @parm ULONG	| ulChipHandle	| Specifies hardware handle.
 * @parm USHORT	| usTRAMbuf		| Specifies X/TRAM address buffer address.
 */
#define OS_READFLAGS(ulCH,usT) \
		(BYTE)((LSEPtrRead(((fxChipHandle *)ulCH)->halid,(((ULONG)usT))<<16)>>16)&0xb0) + \
		(BYTE)((LSEPtrRead(((fxChipHandle *)ulCH)->halid,(((ULONG)usT))<<16)>>22)&1)

/* @func void | OS_WRITEFLAGS |
 *
 * This macro will set the TRAM addressing flags for the specified 
 * TRAM/XTRAM engine buffer.  The buffer is enumerated as described in
 * <f OS_READFLAGS>.  The flag byte argument is the same as described in
 * <f OS_READFLAGS> and contains the PHYSICAL enumeration of the mode.
 * 
 * @parm ULONG	| ulChipHandle	| Specifies hardware handle.
 * @parm USHORT	| usTRAMbuf		| Specifies enumerated X/TRAM buffer.
 * @parm BYTE	| bPhysicalFlags| Specifies flags to write.
 */
#define OS_WRITEFLAGS(ulCH,usT,bF) \
		LSEPtrWrite(((fxChipHandle *)ulCH)->halid, ((ULONG)(usT))<<16, \
			(((ULONG)((bF)&1))<<22) + \
			(((ULONG)((bF)&0xb0))<<16) + \
			(LSEPtrRead(((fxChipHandle *)ulCH)->halid,((ULONG)(usT))<<16)&0xfffffL) );

/* @func void | OS_WRITETABLE |
 *
 * This macro will write a ULONG value to a physical table addres.
 * 
 * @parm ULONG	| ulChipHandle	| Specifies hardware handle.
 * @parm ADDR	| usPhysAddr	| Specifies physical table address.
 * @parm ULONG	| ulValue		| Specifies value to write.
 */
#define OS_WRITETABLE( ulChipHandle, usPhysAddr, ulValue ) /* NOP for EMU10K1 */

/* @func void | OS_READTABLE |
 *
 * This macro will read a ULONG value from a physical table addres.
 * 
 * @parm ULONG	| ulChipHandle	| Specifies hardware handle.
 * @parm ADDR	| usPhysAddr	| Specifies physical table address.
 */
#define OS_READTABLE( ulChipHandle, usPhysAddr ) 0 /* NOP for EMU10K1 */

/* @func void | OS_WRITEBASEADDRREG |
 *
 * This function sets the XTRAM base address register.  Note, address is assumed
 * to be on a page boundary (i.e. lowest 12 bits are zero).
 *
 * @parm ULONG	| ulChipHandle	| Specifies hardware handle.
 * @parm ULONG	| ulBaseAddr	| Physical address for XTRAM base register.
 */
#define OS_WRITEBASEADDRREG(ulC,ulBA) LSEPtrWrite(((fxChipHandle *)ulC)->halid,0x00410000,ulBA);

/* @func void | OS_WRITEXTRAMSIZE |
 *
 * This function sets the XTRAM size register.
 *
 * @parm ULONG	| ulChipHandle	| Specifies hardware handle.
 * @parm ULONG	| ulSize		| Size, 0=16Kb,1=32Kb,2=64Kb,3=128Kb,4=256Kb,5=512Kb,6=1Mb,7=2Mb
 */
#define OS_WRITEXTRAMSIZE(ulC,ulSize) LSEPtrWrite(((fxChipHandle *)ulC)->halid,0x00440000,ulSize&7);

#define OS_STOPZEROCOUNTER(ulC)	\
		LSEPtrWrite(((fxChipHandle *)ulC)->halid, 0x00520000, \
			(LSEPtrRead( ((fxChipHandle *)ulC)->halid, 0x00520000 )|0x80000000LU) );

#define OS_STARTZEROCOUNTER(ulC)	\
		LSEPtrWrite(((fxChipHandle *)ulC)->halid, 0x00520000, \
			(LSEPtrRead( ((fxChipHandle *)ulC)->halid, 0x00520000 )&0x7fffffffLU) );

#define OS_STOPSINGLESTEP(ulC)	\
		LSEPtrWrite(((fxChipHandle *)ulC)->halid, 0x00520000, \
			LSEPtrRead( ((fxChipHandle *)ulC)->halid, 0x00520000 ) & ~(0x8000L) );

#define OS_STARTSINGLESTEP(ulC)	\
		LSEPtrWrite(((fxChipHandle *)ulC)->halid, 0x00520000, \
			LSEPtrRead( ((fxChipHandle *)ulC)->halid, 0x00520000 ) | 0x8000L );

#define OS_STARTXTRAM(ulC) \
		L8010SERegWrite( ((fxChipHandle *)ulC)->halid, HC, L8010SERegRead( ((fxChipHandle *)ulC)->halid, HC ) & (~HC_LT) ); 

#define OS_STOPXTRAM(ulC) \
		L8010SERegWrite( ((fxChipHandle *)ulC)->halid, HC, L8010SERegRead( ((fxChipHandle *)ulC)->halid, HC ) | HC_LT ); 

/* ALIGN BIT CALCULATION MACROS */
#define CALCULATETRAMWRITEALIGN(i,b)	(((i)>=((b)*3))?1:0)
#define CALCULATEXTRAMWRITEALIGN(i,b)	(((((int)(i)-128))>=((b)*4))?1:0)
#define CALCULATETRAMREADALIGN(i,b)		(((((int)(i)-  1))<=((b)*3))?1:0)
#define CALCULATEXTRAMREADALIGN(i,b)	(((((int)(i)-127))<=((b)*4))?1:0)

/* End OS-specific functions */

ENDEMUCTYPE

#endif
