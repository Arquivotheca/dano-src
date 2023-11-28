#include <OS.h>
#include <KernelExport.h>

#include <graphics_p/video_overlay.h>

#include <graphics_p/radeon/r6mer.h>
#include <graphics_p/radeon/CardState.h>
#include <graphics_p/radeon/radeon_ioctls.h>

#include <add-ons/graphics/Accelerant.h>

#include "proto.h"

/****************************************************************************
 * Radeon  Chapter 5 Sample Code                                              *
 *                                                                            *
 * CCEUTIL.C                                                                  *
 * This files contains the routines used to setup and maintain the            *
 * CCE subsystem and ring buffer.                                             *
 *                                                                            *
 * Copyright (c) 2000 ATI Technologies Inc.  All rights reserved.             *
 ******************************************************************************/
#if 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include "regdef.h"
#include "regmask.h"
#include "regshift.h"
#include "main.h"
#include "defines.h"
#include "agp.h"
#include "cce.h"
#include "cputil.h"
#include "r6mer.h"
#endif

//volatile uint8 readbuf[32 + sizeof (uint32)];

//static RBINFO RingBuf = { 0 };
//static BUFINFO VertexBuf = { 0 };
//static BUFINFO IndirectBuf = { 0 };

//static int CCEFifoSize;
//static uint8 CCEBMFlag, CCEAGPFlag, CCEVBFlag, CCEIBFlag;
//static uint32 bm_save_state;
//static uint32 CCERequestedMode;
//static GART_INFO *PCIGartInfo = NULL;
//static _AGP_INFO *AGP_Info = NULL;
//static uint32 read_ptr_offset = 0;

//static int CSQmodeIndex = 0;
//static int CSQPrimary = 0;
//static int CSQIndirect = 0;

//uint32 savebmchunk0val = 0;
//uint32 savebmcommand = 0;

//static FILE *ccefile;

// CCE mode table.  Entries correspond to CCE operating mode constants as
// defined in CCE.H

static CPModeTable CPmode[] = {
	{CP_CSQ_CNTL__CSQ_MODE__PRIDIS_INDDIS, FALSE, FALSE, FALSE, FALSE},
	{CP_CSQ_CNTL__CSQ_MODE__PRIPIO_INDDIS, TRUE, FALSE, FALSE, FALSE},
	{CP_CSQ_CNTL__CSQ_MODE__PRIBM_INDDIS, TRUE, TRUE, FALSE, FALSE},
	{CP_CSQ_CNTL__CSQ_MODE__PRIPIO_INDBM, TRUE, FALSE, TRUE, TRUE},
	{CP_CSQ_CNTL__CSQ_MODE__PRIBM_INDBM, TRUE, TRUE, TRUE, TRUE},
	{CP_CSQ_CNTL__CSQ_MODE__PRIPIO_INDPIO, TRUE, FALSE, TRUE, FALSE},
};
static void CPLoadMicroEngineRAMData (CardInfo *ci);
static int32 CSQPrimaryIdle (CardInfo *ci);
static int32 CSQIndirectIdle (CardInfo *ci);
static int32 CPSubmitPrimaryPIO (CardInfo *ci, uint32 *, uint32);
static int32 CPSubmitIndirectPIO (CardInfo *ci, uint32 *, uint32);
static int32 GetAvailPrimaryQueue (CardInfo *ci);
static int32 GetAvailIndirectQueue (CardInfo *ci);

static int CCESetupBusMaster (CardInfo *ci);

static int32 CCESubmitPacketsBM (CardInfo *ci, uint32 *, uint32);

static void CreateCCEBuffers (void);

extern uint32 GetPhysical (uint32);
//extern GART_INFO *SetupPCIGARTTable (uint32);


void Radeon_Finish(CardInfo *ci )
{
	ci->scratch[6] = 0;
	ci->scratch[6 +8] = 0;
	ci->scratch[6 +16] = 0;
	ci->scratch[6 +24] = 0;

	Radeon_Flush(ci);
	Radeon_WaitForIdle (ci);
	Radeon_FlushPixelCache (ci);
}


void Radeon_WriteRegFifo( CardInfo *ci, int32 reg, uint32 data )
{
	uint32 buf[2];
	buf[0] = (reg>>2) & 0x7fff;
	buf[1] = data;
	ci->CPSubmitPackets( ci, buf, 2 );
}

/******************************************************************************
 * Radeon_CPInit                                                               *
 *  Function: Initializes the CCE microcode engine                            *
 *    Inputs: none.                                                           *
 *   Outputs: CCE Status code.  See cce.h for details.                        *
 ******************************************************************************/
int32 Radeon_CPInit (CardInfo * ci, int32 index)
{
	uint32 cce_buf_size, temp;

	// Load RAM Data to MicroEngine
	CPLoadMicroEngineRAMData (ci);

	// Validate CCEmode and set up necessary parameters
	if ((index < CSQ_MODE_PRIPIO_INDDIS) || (index > CSQ_MODE_PRIPIO_INDPIO))
	{
		return (CCE_FAIL_INVALID_CCE_MODE);
	}									  // if

	// Perform a soft reset of the engine
	Radeon_ResetEngine (ci);
	ci->CCEBMFlag = CPmode[index].primaryBM;
	if (ci->CCEBMFlag)
	{
		ci->CPSubmitPackets = CCESubmitPacketsBM;
		if (CCESetupBusMaster (ci))
		{
			return (CCE_FAIL_BUS_MASTER_INIT);
		}								  // if
		cce_buf_size = RING_SIZE_LOG2;
	}
	else
	{
		ci->CPSubmitPackets = CPSubmitPrimaryPIO;
		ci->CPIndirectSubmitPackets = CPSubmitIndirectPIO;
		cce_buf_size = 0;
	}									  // if

	// Set the Rage 128 to requested CCE mode.
	ci->CCERequestedMode = CPmode[index].CSQmode;
	WRITE_REG_32 (CP_CSQ_CNTL, ci->CCERequestedMode);

	//Get queue size
	temp = READ_REG_32 (CP_CSQ_CNTL);
	ci->CSQPrimary = temp & CP_CSQ_CNTL__CSQ_CNT_PRIMARY_MASK;
	ci->CSQIndirect = (temp & CP_CSQ_CNTL__CSQ_CNT_INDIRECT_MASK)
		>> CP_CSQ_CNTL__CSQ_CNT_INDIRECT__SHIFT;

	// Set the CCE to free running mode
	temp = READ_REG_32 (CP_ME_CNTL);
	temp |= CP_ME_CNTL__ME_MODE__FREE_RUNNING;
	WRITE_REG_32 (CP_ME_CNTL, temp);

	ci->CSQmodeIndex = index;
	ci->Flush = Radeon_Flush;
	ci->Finish = Radeon_Finish;

	return (CCE_SUCCESS);
}										  // Radeon_CCEInit

void Radeon_CPInitClone (CardInfo * ci)
{
	if (ci->CCEBMFlag)
	{
		ci->CPSubmitPackets = CCESubmitPacketsBM;
	}
	else
	{
		ci->CPSubmitPackets = CPSubmitPrimaryPIO;
		ci->CPIndirectSubmitPackets = CPSubmitIndirectPIO;
	}									  // if
	
	ci->Flush = Radeon_Flush;
	ci->Finish = Radeon_Finish;
}

/******************************************************************************
 * Radeon_CCEEnd                                                                *
 *  Function: Shuts down the CCE microcode engine                             *
 *    Inputs: waitmode                                                        *
 *   Outputs: none.                                                           *
 ******************************************************************************/
void Radeon_CPEnd (CardInfo *ci, int32 waitmode)
{
	uint32 data;

	if (waitmode == CCE_END_WAIT)
	{
		// Wait for engine idle before ending.  It does not matter if the
		// engine fails to idle as we will reset it shortly.
		if (CPmode[ci->CSQmodeIndex].primaryCSQ)
		{
			CSQPrimaryIdle (ci);
		}
		if (CPmode[ci->CSQmodeIndex].indirectCSQ)
		{
			CSQIndirectIdle (ci);
		}
	}									  // if

	if (ci->CCEBMFlag)
	{
//		CCENextPage ();			  // only necessary for PCI GART. Since we're
		// shutting down who cares if not necessary when
		// using AGP. 

		// Signal CCE that we are done submitting bus-mastered packets
	}									  // if

	// Stop the CCE microengine by setting it to single-stepping mode
	WRITE_REG_32 (CP_ME_CNTL, 0x00000000);

	// Set the Rage 128 to standard PIO mode.
	WRITE_REG_32 (CP_CSQ_CNTL, CP_CSQ_CNTL__CSQ_MODE__PRIDIS_INDDIS);

	if (ci->CCEBMFlag)
	{
		WRITE_REG_32 (BUS_CNTL, ci->bm_save_state);

		if (ci->CCEAGPFlag)
		{
			// Shut down AGP
//			Radeon_EndAGP ();
		}
		else
		{
			// Disable PCI_GART.
			data = READ_REG_32 (AIC_CTRL);
			data &= ~AIC_CTRL__TRANSLATE_EN;
			WRITE_REG_32 (AIC_CTRL, data);
			//disable bus mastering
			data = READ_REG_32 (BUS_CNTL);
			data |= BUS_CNTL__BUS_MASTER_DIS;
			WRITE_REG_32 (BUS_CNTL, data);

			// Restore original BM_CHUNK_0_VAL value.

			// Free 'AGP space' memory.
//			DPMI_freememory (PCIGartInfo->mem_handle);

			// Free PCI GART page memory.
//			DPMI_freedosmem (PCIGartInfo->handle);

		}								  // if
	}									  // if

	// Perform a soft reset of the engine.

	Radeon_ResetEngine (ci);
	return;
}										  // Radeon_CCEEnd

#if 1
/******************************************************************************
 * CCESetupBusMaster                                                          *
 *  Function: This function sets up the ring buffer for bus mastering.        *
 *    Inputs: none.                                                           *
 *   Outputs: CCE Status code.  See cce.h for details.                        *
 ******************************************************************************/
static int CCESetupBusMaster (CardInfo *ci)
{
	uint32 temp, rdata;
	uint32 fb_top, agp_top, aic_addr;
	uint32 *ptemp;

dprintf (("Radeon_accel: CCESetupBusMaster \n"));
	// For the sake of simplicity, put the ring buffer at the start of AGP
	// space, factoring in alignment restrictions.
	ci->RingBuf.Offset = 0;

	// Align the offset to a 128-uint8 boundary.  Strictly speaking, since an
	// offset of zero was chosen, the following step is unnecessary, but it
	// is good form to perform this step in case the ring buffer needs to be
	// elsewhere.

	ci->RingBuf.Offset = align (ci->RingBuf.Offset, 128);
	ci->read_ptr_offset = align ((uint32) ci->readbuf, 32);
	ci->RingBuf.ReadIndexPtr = (uint32 *) ci->read_ptr_offset;
	
	snooze( 10000 ); //Radeon_Delay (1);
	
	ci->RingBuf.Size = RING_SIZE;
	ci->RingBuf.WriteIndex = 0;
	//enable bus master
	ci->bm_save_state = READ_REG_32 (BUS_CNTL);
	WRITE_REG_32 (BUS_CNTL, (ci->bm_save_state & ~BUS_CNTL__BUS_MASTER_DIS));

	// program the FB chip space.
//	temp = READ_REG_32 (CONFIG_APER_SIZE) - 1;
//	rdata = (temp & 0xFFFF0000);
//	WRITE_REG_32 (MC_FB_LOCATION, rdata);

	ci->CCEAGPFlag = 0;//Radeon_InitAGP (RING_BUFFER_SIZE);
	if (ci->CCEAGPFlag)
	{
		//GetAGPINFO (&AGP_Info);
		//ci->RingBuf.LinearPtr = (uint32 *) (AGP_Info->LogicalAddress + RingBuf.Offset);
		//temp = READ_REG_32 (AIC_CTRL);
		//temp &= ~AIC_CTRL__TRANSLATE_EN;
		//WRITE_REG_32 (AIC_CTRL, temp);
	}
	else
	{
		// No AGP available, use PCI GART mapping instead.
dprintf (("Radeon_accel: CCESetupBusMaster USE PCI\n"));
#if 0
		temp = READ_REG_32 (MC_FB_LOCATION);
		temp = (temp | 0x0000FFFF) + 1;
		rdata = temp >> 16;
		temp += 0x400000;
		rdata |= ((temp - 1) & 0xFFFF0000);
		WRITE_REG_32 (MC_AGP_LOCATION, rdata);
		WRITE_REG_32 (AGP_BASE, 0);

		if (!(PCIGartInfo = SetupPCIGARTTable (APERTURE_SIZE_8MB)))
		{
			// If even a PCI GART table is not available, then bus mastering
			// is not possible.
			return (CCE_FAIL_BUS_MASTER_INIT);
		}								  // if

		//setup the AIC_LO_ADDR and AIC_HI_ADDR values so that PCI accesses
		//to the ring buffer get translated
		fb_top = READ_REG_32 (MC_FB_LOCATION) | MC_FB_LOCATION__MC_FB_START_MASK;
		agp_top = READ_REG_32 (MC_AGP_LOCATION) | MC_AGP_LOCATION__MC_AGP_START_MASK;
		if (fb_top > agp_top)
		{
			aic_addr = fb_top + 1;
		}
		else if (fb_top < agp_top)
		{
			aic_addr = agp_top + 1;
		}
		else
		{
			// error
		}
		WRITE_REG_32 (AIC_LO_ADDR, (aic_addr & 0xFFFFF000));
		aic_addr += RING_SIZE * 4;
		WRITE_REG_32 (AIC_HI_ADDR, (aic_addr & 0xFFFFF000));

		// Write the GART page address.  Since this address is 4KB
		// aligned, bit 0 is cleared.  Hence, GART will be enabled.
		WRITE_REG_32 (AIC_PT_BASE, PCIGartInfo->paddress);

		temp = READ_REG_32 (AIC_CTRL);
		temp |= AIC_CTRL__TRANSLATE_EN;
		WRITE_REG_32 (AIC_CTRL, temp);
#endif

		ci->RingBuf.LinearPtr = ci->pciMemBase/*PCIGartInfo->pointer*/ +
			(ci->RingBuf.Offset / sizeof (uint32));

		temp = READ_REG_32 (AIC_TLB_ADDR);
		temp = READ_REG_32 (AIC_TLB_DATA);
	}									  // if

	// init the PM4 micro engine
	WRITE_REG_32 (CP_CSQ_CNTL, 0);
	WRITE_REG_32 (CP_ME_CNTL, CP_ME_CNTL__ME_MODE);
	WRITE_REG_32 (CP_RB_CNTL, 0);
	WRITE_REG_32 (CP_RB_BASE, 0);
	WRITE_REG_32 (CP_RB_RPTR, 0);
	WRITE_REG_32 (CP_RB_WPTR, 0);

	//the number of clocks that a write to CP_RB_WPTR register
	//will be delayed until actually taking effect.
	WRITE_REG_32 (CP_RB_WPTR_DELAY, 0);

	//
	// Store the size of the Ring Buffer
	//
	temp = READ_REG_32 (CP_RB_CNTL);
	temp &= ~(CP_RB_CNTL__RB_BUFSZ_MASK | CP_RB_CNTL__RB_BLKSZ_MASK | CP_RB_CNTL__MAX_FETCH_MASK);

	// Put the physical address of read pointer into PM4_BUFFER_DL_RPTR_ADDR
	temp = READ_REG_32 (AIC_LO_ADDR);
	WRITE_REG_32 (CP_RB_RPTR_ADDR, temp + ci->RingBuf.Size*4 );
	
	// Setup the scratch regs that we use to sync the engine
	WRITE_REG_32 (SCRATCH_ADDR, temp + ci->RingBuf.Size*4 +32 );
	ci->scratch = &((uint32 *)ci->pciMemBase)[ci->RingBuf.Size + (32/4)];
	WRITE_REG_32 (SCRATCH_UMSK, 0x3f );
	
	ci->RingBuf.ReadIndexPtr = (ci->RingBuf.LinearPtr + ci->RingBuf.Size);

	temp |= (0x00000000 | RING_SIZE_LOG2);	//jgt
	WRITE_REG_32 (CP_RB_CNTL, temp);	  //jgt

	WRITE_REG_32 (CP_RB_WPTR, ci->RingBuf.WriteIndex);
	WRITE_REG_32 (CP_RB_RPTR, ci->RingBuf.WriteIndex);

	temp = READ_REG_32 (AIC_LO_ADDR);
	WRITE_REG_32 (CP_RB_BASE, temp);

	return (CCE_SUCCESS);
}										  // CCESetupBusMaster
#endif

/******************************************************************************
 * CCESubmitPacketsPIO                                                        *
 *  Function: This function submits packets to the CCE FIFO.  It determines   *
 *            if it can accept the packet that has been submitted, then it    *
 *            writes the data to the FIFO even and odd registers.  If an odd  *
 *            number of packets is submitted, a dummy (Type-2) packet is sent *
 *            to ensure that an even number of register writes is performed.  *
 *    Inputs: uint32 *ClientBuf - pointer to the data to be submitted          *
 *            uint32 DataSize - size of the packet submitted                   *
 *   Outputs: CCE Status code.  See cce.h for details.                        *
 ******************************************************************************/
#if 1
#define CP_CSQ_APER_PRIMARY__BEGIN  0x1000
#define CP_CSQ_APER_PRIMARY__END    0x11FC
int32 CPSubmitPrimaryPIO (CardInfo *ci, uint32 * ClientBuf, uint32 DataSize)
{
	static uint16 count = CP_CSQ_APER_PRIMARY__BEGIN;
	int size;
	int i;

//dprintf(( "CPSubmitPrimaryPIO ci=%p  buf=%p  size=%ld \n", ci, ClientBuf, DataSize ));
	while (1)
	{
		size = GetAvailPrimaryQueue (ci);
//dprintf(( "size=%ld \n", size ));
		if (size == 0)
		{
			continue;
		}
//dprintf(( "1 \n" ));
		if (DataSize < size)
		{
			for (i = 0; i < DataSize; i++)
			{
//dprintf(( "2 \n" ));
				WRITE_REG_32 (count, *ClientBuf++);
				if (count == CP_CSQ_APER_PRIMARY__END)
					count = CP_CSQ_APER_PRIMARY__BEGIN;
				else
					count += 4;
			}
			break;
//dprintf(( "3 \n" ));
		}
		else
		{
//dprintf(( "4 \n" ));
			for (i = 0; i < size; i++)
			{
//dprintf(( "5 \n" ));
				WRITE_REG_32 (count, *ClientBuf++);
				if (count == CP_CSQ_APER_PRIMARY__END)
					count = CP_CSQ_APER_PRIMARY__BEGIN;
				else
					count += 4;
			}
			DataSize -= size;
//dprintf(( "6 \n" ));
		}								  // while
	}
//dprintf(( "7 \n" ));
	return (CCE_SUCCESS);
}

#define CP_CSQ_APER_INDIRECT__BEGIN  0x1300
#define CP_CSQ_APER_INDIRECT__END    0x13FC

int32 CPSubmitIndirectPIO (CardInfo *ci, uint32 * ClientBuf, uint32 DataSize)
{
	static uint16 count = CP_CSQ_APER_INDIRECT__BEGIN;
	int size;
	int i;

	CSQPrimaryIdle (ci);
	while (1)
	{
		size = GetAvailIndirectQueue (ci);
		if (size == 0)
		{
			continue;
		}
		if (DataSize < size)
		{
			WRITE_REG_32 (CP_IB_BUFSZ, DataSize);
			for (i = 0; i < DataSize; i++)
			{
				WRITE_REG_32 (count, *ClientBuf++);
				if (count == CP_CSQ_APER_INDIRECT__END)
					count = CP_CSQ_APER_INDIRECT__BEGIN;
				else
					count += 4;
			}
			break;
		}
		else
		{
			WRITE_REG_32 (CP_IB_BUFSZ, size);
			for (i = 0; i < size; i++)
			{
				WRITE_REG_32 (count, *ClientBuf++);
				if (count == CP_CSQ_APER_INDIRECT__END)
					count = CP_CSQ_APER_INDIRECT__BEGIN;
				else
					count += 4;
			}
			DataSize -= size;
		}								  // while
	}
	return (CCE_SUCCESS);
}
#else
int CCESubmitPacketsPIO (uint32 * ClientBuf, uint32 DataSize)
{

	while (DataSize > 0)
	{
		if (CPWaitForCSQ (1))
		{
			return (CCE_FAIL_TIMEOUT);
		}
		WRITE_REG_32 (CP_CSQ_APER_PRIMARY, *ClientBuf++);
		DataSize -= 1;
	}
	return (CCE_SUCCESS);
}

#endif

int32 GetAvailRingBufferQueue (CardInfo *ci)
{
	int rptr, wptr;
	int diff, size;

	size = ci->RingBuf.Size;
	rptr = *(ci->RingBuf.ReadIndexPtr);	//READ_REG_32(CP_RB_RPTR);
	wptr = ci->RingBuf.WriteIndex;
	diff = (int) wptr - rptr;
	if (diff < 0)
	{
		return ((size - (size + diff)) - 1);
	}
	else
	{
		return ((size - diff) - 1);
	}
	return 0;
}

#if 1
/******************************************************************************
 * CCESubmitPacketsBM                                                         *
 *  Function: This function is our ring buffer manager.  It determines if it  *
 *            can accept the packet that has been submitted, then it copies   *
 *            the data to the ring buffer.                                    *
 *    Inputs: uint32 *ClientBuf - pointer to the data to be submitted          *
 *            uint32 DataSize - size of the packet submitted                   *
 *   Outputs: CCE Status code.  See cce.h for details.                        *
 ******************************************************************************/
#if 1
static int32 CCESubmitPacketsBM (CardInfo *ci, uint32 * ClientBuf, uint32 DataSize)
{
	uint32 *tptr;
	int size;
	int i;

//dprintf ((" 1,%ld ",DataSize));
//dprintf (("Radeon_accel: CCESubmitPacketsBM 1 \n"));

	ci->scratch[6] = 0;
	ci->scratch[6 +8] = 0;
	ci->scratch[6 +16] = 0;
	ci->scratch[6 +24] = 0;

	while (1)
	{

		size = GetAvailRingBufferQueue (ci);
		if (size == 0)
		{
			continue;
		}
		if (DataSize < size)
		{
			tptr = ci->RingBuf.LinearPtr + ci->RingBuf.WriteIndex;
			for (i = 0; i < DataSize; i++)
			{
				ci->RingBuf.WriteIndex += 1;
				*tptr++ = *ClientBuf++;
				if (ci->RingBuf.WriteIndex >= ci->RingBuf.Size)
				{
					ci->RingBuf.WriteIndex = 0;
					tptr = ci->RingBuf.LinearPtr + ci->RingBuf.WriteIndex;
				}						  // if
			}
			break;
		}
		else
		{
			tptr = ci->RingBuf.LinearPtr + ci->RingBuf.WriteIndex;
			for (i = 0; i < size; i++)
			{
				ci->RingBuf.WriteIndex += 1;
				*tptr++ = *ClientBuf++;
				if (ci->RingBuf.WriteIndex >= ci->RingBuf.Size)
				{
					ci->RingBuf.WriteIndex = 0;
					tptr = ci->RingBuf.LinearPtr + ci->RingBuf.WriteIndex;
				}						  // if
			}
			DataSize -= size;
		}								  // while
	}
//dprintf (("Radeon_accel: CCESubmitPacketsBM 16 \n"));
	// Update pointer.
	WRITE_REG_32 (CP_RB_WPTR, ci->RingBuf.WriteIndex);
//dprintf (("Radeon_accel: CCESubmitPacketsBM 17 \n"));
//dprintf (("2"));
	return (CCE_SUCCESS);
}
#else
static int CCESubmitPacketsBM (uint32 * ClientBuf, uint32 DataSize)
{
	uint32 *tptr;
	uint16 starttick, endtick;
	uint32 hwdbg;
	uint32 test;

	// We shall arbitrarily fail if the incoming packet is bigger than our
	// ring buffer.  A better algorithm would break up the incoming packet
	// into small enough chunks to feed to the buffer.

	if (DataSize >= RingBuf.Size)
	{
		return (CCE_FAIL_BAD_PACKET);
	}									  // if

	starttick = *((uint16 *) (DOS_TICK_ADDRESS));
	endtick = starttick;

	tptr = RingBuf.LinearPtr + RingBuf.WriteIndex;
	while (DataSize > 0)
	{
		RingBuf.WriteIndex += 1;
		*tptr++ = *ClientBuf++;
		if (RingBuf.WriteIndex >= RingBuf.Size)
		{
			RingBuf.WriteIndex = 0;
			tptr = RingBuf.LinearPtr + RingBuf.WriteIndex;
		}								  // if
		test = *(RingBuf.ReadIndexPtr);	//READ_REG_32(CP_RB_RPTR);
		while (RingBuf.WriteIndex == test)
		{
			endtick = *((uint16 *) (DOS_TICK_ADDRESS));
			test = READ_REG_32 (CP_RB_RPTR);
			if (abs (endtick - starttick) > FIFO_TIMEOUT)
			{
				// Attempt to reset CCE.  Not quite working yet...
				RingBuf.WriteIndex = 0;
				Radeon_ResetEngine ();
				WRITE_REG_32 (CP_RB_WPTR, 0);
				WRITE_REG_32 (CP_RB_RPTR, 0);
				return (CCE_FAIL_TIMEOUT);
			}							  // if
		}								  // while
		DataSize -= 1;
	}									  // while

	// Update pointer.
	WRITE_REG_32 (CP_RB_WPTR, RingBuf.WriteIndex);

	return (CCE_SUCCESS);
}										  // CCESubmitPacketsBM
#endif

#endif

/******************************************************************************
 * Radeon_Flush                                                                 *
 *  Function: This function flushes pending writes to the CCE FIFO.           *
 *    Inputs: none.                                                           *
 *   Outputs: none.                                                           *
 ******************************************************************************/
void Radeon_Flush ( CardInfo *ci )
{
	// Need to fence the write combine regs.
	ci->scratch[6] = 0;
	ci->scratch[6 +8] = 0;
	ci->scratch[6 +16] = 0;
	ci->scratch[6 +24] = 0;

//dprintf (("Radeon_accel: Radeon_Flush 1 \n"));
	if (ci->CCEBMFlag)
		WRITE_REG_32 (CP_RB_WPTR, ci->RingBuf.WriteIndex);
//	CSQIndirectIdle( ci );
//dprintf (("Radeon_accel: Radeon_Flush 2 \n"));
}

/******************************************************************************
 * CPLoadMicroEngineRAMData                                                           *
 *  Function: This function loads the microcode into the CCE microengine.     *
 *    Inputs: none.                                                           *
 *   Outputs: CCE Status code.  See cce.h for details.                        *
 ******************************************************************************/
void CPLoadMicroEngineRAMData ( CardInfo *ci )
{
	int i;
	uint32 data;

	// Wait for engine idle before loading the microcode.
	data = READ_REG_32 (WAIT_UNTIL);
	Radeon_WaitForIdle (ci);

	// Set starting address for writing the microcode.
	WRITE_REG_32 (CP_ME_RAM_ADDR, 0);

	for (i = 0; i < 256; i += 1)
	{
		// The microcode write address will automatically increment after
		// every microcode data high/low pair.  Note that the high uint32
		// must be written first for address autoincrement to work correctly.
		WRITE_REG_32 (CP_ME_RAM_DATAH, aPM4_Microcode[i][1]);
		WRITE_REG_32 (CP_ME_RAM_DATAL, aPM4_Microcode[i][0]);
	}									  // for
}										  // CPLoadMicroEngineRAMData

int32 CSQPrimaryIdle (CardInfo *ci)
{
	int data;

dprintf (("Radeon_accel: CSQPrimaryIdle 1 \n"));
	if (CPmode[ci->CSQmodeIndex].primaryCSQ == TRUE)
	{
		while (1)
		{								  // Ensure Queue is empty before waiting for engine idle.
			data = GetAvailPrimaryQueue (ci);
			if (data >= (ci->CSQPrimary - 1))
				break;
			snooze( 500 );
		}
		while (1)
		{								  //check for CSQ busy 
			data = READ_REG_32 (CP_STAT);
			if (!(data & CP_STAT__CSQ_PRIMARY_BUSY))
				break;
			snooze( 500 );
		}
	}
	// flush the pixel cache
	Radeon_FlushPixelCache (ci);
	return (CCE_SUCCESS);
}										  // CSQPrimaryIdle

int32 CSQIndirectIdle (CardInfo *ci)
{
	int data;

	if (CPmode[ci->CSQmodeIndex].indirectCSQ == TRUE)
	{
		while (1)
		{								  // Ensure Queue is empty before waiting for engine idle.
			data = GetAvailIndirectQueue (ci);
			if (data >= (ci->CSQIndirect - 1))
				break;
			snooze( 500 );
		}
		while (1)
		{								  //check for CSQ busy and CP busy
			data = READ_REG_32 (CP_STAT);
			if (!(data & CP_STAT__CSQ_INDIRECT_BUSY))
				break;
			snooze( 500 );
		}
	}
	// flush the pixel cache
	Radeon_FlushPixelCache (ci);
	return (CCE_SUCCESS);
}										  // CSQIndirectIdle

/******************************************************************************
 * CCEWaitForFifo - wait n empty CCE FIFO entries.                            *
 *  Function: The FIFO contains up to 192 empty entries, depending on current *
 *            CCE configuration (see PM4_BUFFER_CNTL). The 'entries' value    *
 *            must be 1 to 192. This function implements the same timeout     *
 *            mechanism as the CCEWaitForIdle() function.                     *
 *    Inputs: entries - number of entries spaces to wait for. Max - 192       *
 *   Outputs: CCE Status code.  See cce.h for details.                        *
 ******************************************************************************/
int32 CCEWaitForFifo (CardInfo *ci, uint32 entries)
{
	bigtime_t starttick, endtick;
	uint32 temp;

dprintf (("Radeon_accel: CCEWaitForFifo 1 \n"));
	starttick = system_time();
	endtick = starttick;
	while (((temp = READ_REG_32 (CP_CSQ_CNTL)) & 0x000000FF) < entries)
	{
		endtick = system_time();
		if (abs (endtick - starttick) > 1000000 )
		{
//dprintf (("Radeon_accel: CCEWaitForFifo FAIL \n"));
			return (CCE_FAIL_TIMEOUT);
		}								  // if
		snooze( 500 );
	}									  // while
//dprintf (("Radeon_accel: CCEWaitForFifo OK \n"));
	return (CCE_SUCCESS);
}										  // CCEWaitForFifo

int32 GetAvailPrimaryQueue (CardInfo *ci)
{
	int rptr, wptr;
	int diff, size;

dprintf (("Radeon_accel: GetAvailPrimaryQueue 1 \n"));
	size = ci->CSQPrimary;
	rptr = (READ_REG_32 (CP_CSQ_STAT) & CP_CSQ_STAT__CSQ_RPTR_PRIMARY_MASK);
	wptr = ((READ_REG_32 (CP_CSQ_STAT) & CP_CSQ_STAT__CSQ_WPTR_PRIMARY_MASK)
			  >> CP_CSQ_STAT__CSQ_WPTR_PRIMARY__SHIFT);
	diff = (int) wptr - rptr;
//dprintf (("Radeon_accel: GetAvailPrimaryQueue 2 \n"));
	if (diff < 0)
	{
//dprintf (("Radeon_accel: GetAvailPrimaryQueue ret = %ld \n",((size - (size + diff)) - 1)));
		return ((size - (size + diff)) - 1);
	}
	else
	{
//dprintf (("Radeon_accel: GetAvailPrimaryQueue ret = %ld \n",((size - diff) - 1)));
		return ((size - diff) - 1);
	}
//dprintf (("Radeon_accel: GetAvailPrimaryQueue ret = 0 \n"));
	return 0;
}

int32 GetAvailIndirectQueue (CardInfo *ci)
{
	int rptr, wptr;
	int diff, size;

	size = ci->CSQIndirect;
	rptr = ((READ_REG_32 (CP_CSQ_STAT) & CP_CSQ_STAT__CSQ_RPTR_INDIRECT_MASK)
			  >> CP_CSQ_STAT__CSQ_RPTR_INDIRECT__SHIFT);
	wptr = ((READ_REG_32 (CP_CSQ_STAT) & CP_CSQ_STAT__CSQ_WPTR_INDIRECT_MASK)
			  >> CP_CSQ_STAT__CSQ_WPTR_INDIRECT__SHIFT);
	diff = (int) wptr - rptr;
	if (diff < 0)
	{
		return ((size - (size + diff)) - 1);
	}
	else
	{
		return ((size - diff) - 1);
	}
	return 0;
}

#if 0
/****************************************************************************
 * CreateBuffer - Create a logical buffer.                                  *
 *  Function: This function creates a logical description of a linear       *
 *            buffer as described by the offset and size parameters. It is  *
 *            used to create the vertex and indirect buffers in response to *
 *             CreateCCEBuffers function.                                   * 
 *    Inputs: BUFINFO* pbuf - pointer to the buffer's BUFINFO structure.    *
 *            uint32 offset - uint8 offset of the buffer from the base memory.*
 *            uint32 size - size of the buffer in uint8s.                     *
 *   Outputs: None.                                                         *
 ****************************************************************************/

static void CreateBuffer (BUFINFO * pbuf, uint32 offset, uint32 size)
{
	if (CCEBMFlag)
	{
		if (CCEAGPFlag)
		{
			pbuf->LinearPtr = (uint32 *) ((uint32) AGP_Info->uint8Pointer + offset);
		}
		else
		{
			pbuf->LinearPtr = (uint32 *) ((uint32) PCIGartInfo->pointer + offset);
		}
	}

	pbuf->Offset = offset;
	pbuf->Size = size;
}

/****************************************************************************
 * CreateCCEBuffers - Create Vertex and/or Indirect buffers.                *
 *  Function: This function places the indirect buffer and/or vertex after  *
 *            the ring buffer in AGP or PCI GART space. If both are created,*
 *            the vertex buffer is placed after the indirect buffer. The    *
 *            buffer offsets are set at the next 4k boundary following the  *
 *            previous buffer.                                              *
 *    Inputs: None.                                                         *
 *   Outputs: None.                                                         *
 ****************************************************************************/

static void CreateCCEBuffers (void)
{
	uint32 offset, lastoffset;
	uint32 size;

	// Zero structures

	memset (&VertexBuf, 0, sizeof (BUFINFO));
	memset (&IndirectBuf, 0, sizeof (BUFINFO));

	lastoffset = RingBuf.Offset + (RingBuf.Size << 2);

	// Create an indirect buffer if needed and place it at
	// next 4K offset after end of ring buffer.

	if (CCEIBFlag)
	{
		offset = align (lastoffset + 4096, 4096);
		size = INDIRECT_BUF_SIZE;

		CreateBuffer (&IndirectBuf, offset, size);

		lastoffset = offset + size;
	}

	// Create vertex buffer if needed and place it at next
	// 4K offset after ring or indirect buffer.

	if (CCEVBFlag)
	{
		offset = align (lastoffset + 4096, 4096);
		size = VERTEX_BUF_SIZE;

		CreateBuffer (&VertexBuf, offset, size);

		lastoffset = offset + size;
	}
}

/****************************************************************************
 * Radeon_GetVertexBufferPtr - return a pointer to the top of the vertex      *
 *            buffer.                                                       *
 *  Function: This function returns the VertexBuf.LinearPtr member.         *
 *    Inputs: None.                                                         *
 *   Outputs: uint32 pointer to the top of the vertex buffer.                *
 ****************************************************************************/

uint32 *Radeon_GetVertexBufferPtr (void)
{
	return (VertexBuf.LinearPtr);
}

/****************************************************************************
 * Radeon_GetVertexBufferOffset - return offset to the top of the vertex      *
 *            buffer.                                                       *
 *  Function: This function returns the VertexBuf.Offset member.            *
 *    Inputs: None.                                                         *
 *   Outputs: uint32 offset to the top of the vertex buffer.                 *
 ****************************************************************************/

uint32 Radeon_GetVertexBufferOffset (void)
{
	return (VertexBuf.Offset);
}

/****************************************************************************
 * Radeon_GetVertexBufferSize - return offset to the top of the vertex        *
 *            buffer.                                                       *
 *  Function: This function returns the VertexBuf.Size member.              *
 *    Inputs: None.                                                         *
 *   Outputs: uint32 size of the vertex buffer.                              *
 ****************************************************************************/

uint32 Radeon_GetVertexBufferSize (void)
{
	return (VertexBuf.Size);
}

/****************************************************************************
 * Radeon_GetIndirectBufferPtr - return a pointer to the top of the indirect  *
 *            buffer.                                                       *
 *  Function: This function returns the IndirectBuf.LinearPtr member.       *
 *    Inputs: None.                                                         *
 *   Outputs: uint32 pointer to the top of the indirect buffer.              *
 ****************************************************************************/

uint32 *Radeon_GetIndirectBufferPtr (void)
{
	return (IndirectBuf.LinearPtr);
}

/****************************************************************************
 * Radeon_GetIndirectBufferOffset - return offset to the top of the indirect  *
 *            buffer.                                                       *
 *  Function: This function returns the IndirectBuf.Offset member.          *
 *    Inputs: None.                                                         *
 *   Outputs: uint32 offset to the top of the indirect buffer.               *
 ****************************************************************************/

uint32 Radeon_GetIndirectBufferOffset (void)
{
	return (IndirectBuf.Offset);
}

/****************************************************************************
 * Radeon_GetIndirectBufferSize - return size of the indirect buffer.         *
 *  Function: This function returns the IndirectBuf.Size member.            *
 *    Inputs: None.                                                         *
 *   Outputs: uint32 size of the indirect buffer.                            *
 ****************************************************************************/

uint32 Radeon_GetIndirectBufferSize (void)
{
	return (IndirectBuf.Size);
}

/****************************************************************************
 * CCENextPage - Write 4K plus two uint32s of Type-2 packets (NOP) into      *
 *            ring buffer and submit.                                       *
 *  Function: This function is as a workaround for a PCI GART entry         *
 *            hardware bug in the Rage 128 ASIC.                            * 
 *    Inputs: None.                                                         *
 *   Outputs: None.                                                         *
 ****************************************************************************/

void CCENextPage (void)
{
	uint32 *tptr;
	uint32 i;

	tptr = RingBuf.LinearPtr + RingBuf.WriteIndex;

	for (i = 0; i < 1026; i++)
	{
		*tptr++ = CCE_PACKET2;

		RingBuf.WriteIndex += 1;

		if (RingBuf.WriteIndex >= RingBuf.Size)
		{
			RingBuf.WriteIndex = 0;
			tptr = RingBuf.LinearPtr + RingBuf.WriteIndex;
		}								  // if
	}									  // for

	// Update pointer.

}

//**********************************************************************
#define FORCE_ON_GUI_ENG  (SCLK_CNTL__FORCE_E2 | \
                           SCLK_CNTL__FORCE_RB | \
                           SCLK_CNTL__FORCE_RE | \
                           SCLK_CNTL__FORCE_SE | \
                           SCLK_CNTL__FORCE_CP)

#define RBBM_SOFT_RESET_GUI   (RBBM_SOFT_RESET__SOFT_RESET_E2 | \
                          RBBM_SOFT_RESET__SOFT_RESET_PP | \
                          RBBM_SOFT_RESET__SOFT_RESET_RB | \
                          RBBM_SOFT_RESET__SOFT_RESET_RE | \
                          RBBM_SOFT_RESET__SOFT_RESET_SE | \
                          RBBM_SOFT_RESET__SOFT_RESET_CP)

#endif
