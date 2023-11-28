/* :ts=8 bk=0
 *
 * fifo.c: Fifo Allocation and management routines.
 *
 * $Id:$
 *
 * Andrew Kimpton					1999.11.03
 *
 * Copyright 1998 Be Incorporated.  Confidential unpublished work.
 * All Rights Reserved.
 */
#include <BeBuild.h>
#include <support/ByteOrder.h>

#include <add-ons/graphics/Accelerant.h>

#include <graphics_p/3dfx/banshee/banshee.h>
#include <graphics_p/3dfx/banshee/banshee_regs.h>
#include <graphics_p/3dfx/common/bena4.h>
#include <graphics_p/3dfx/common/debug.h>

#include "protos.h"

#include "fifo.h"

extern thdfx_card_info  *ci;

#define FIFO_END_ADJUST  (sizeof(uint32) << 3)


status_t InitFifo(int32 fifo_number)
{
	// Set up FIFO location and control registers on the card
	
	if (fifo_number == 1)
	{
		_V3_WriteReg_NC( ci, V3_CMD_BASE_ADDR_1, (ci->CmdTransportInfo[1].fifoOffset) >> 12 );
		_V3_WriteReg_NC( ci, V3_CMD_BUMP_1, 0 );
		_V3_WriteReg_NC( ci, V3_CMD_RD_PTR_L_1, ci->CmdTransportInfo[1].fifoOffset );
		_V3_WriteReg_NC( ci, V3_CMD_RD_PTR_H_1, 0 );
		_V3_WriteReg_NC( ci, V3_CMD_A_MIN_1, ci->CmdTransportInfo[1].fifoOffset - 4 );
		_V3_WriteReg_NC( ci, V3_CMD_A_MAX_1, ci->CmdTransportInfo[1].fifoOffset - 4 );
		_V3_WriteReg_NC( ci, V3_CMD_FIFO_DEPTH_1, 0 );
		_V3_WriteReg_NC( ci, V3_CMD_FIFO_HOLE_CNT_1, 0 );
	}
	else if (fifo_number == 0)
	{
		_V3_WriteReg_NC( ci, V3_CMD_BASE_ADDR_0, (ci->CmdTransportInfo[0].fifoOffset) >> 12 );
		_V3_WriteReg_NC( ci, V3_CMD_BUMP_0, 0 );
		_V3_WriteReg_NC( ci, V3_CMD_RD_PTR_L_0, ci->CmdTransportInfo[0].fifoOffset );
		_V3_WriteReg_NC( ci, V3_CMD_RD_PTR_H_0, 0 );
		_V3_WriteReg_NC( ci, V3_CMD_A_MIN_0, ci->CmdTransportInfo[0].fifoOffset - 4 );
		_V3_WriteReg_NC( ci, V3_CMD_A_MAX_0, ci->CmdTransportInfo[0].fifoOffset - 4 );
		_V3_WriteReg_NC( ci, V3_CMD_FIFO_DEPTH_0, 0 );
		_V3_WriteReg_NC( ci, V3_CMD_FIFO_HOLE_CNT_0, 0 );
	}
	
	if (ci->device_id == DEVICEID_BANSHEE)
	{
		_V3_WriteReg_NC( ci, V3_CMD_FIFO_THRESH, (0x9 << 5) | 0x2 );
	}
	else
	{
		_V3_WriteReg_NC( ci, V3_CMD_FIFO_THRESH, (0xf << 5) | 0x8 );
	}

	if (fifo_number == 1)
	{
		_V3_WriteReg_NC( ci, V3_CMD_BASE_SIZE_1, (((ci->CmdTransportInfo[1].fifoSize >> 12) - 1) | SST_EN_CMDFIFO));
	}
	else
	{
		_V3_WriteReg_NC( ci, V3_CMD_BASE_SIZE_0, (((ci->CmdTransportInfo[0].fifoSize >> 12) - 1) | SST_EN_CMDFIFO));
	}

	// Store our fifo information so that we're set to write to it

	/* Compute Virtual FIFO address extents */
	ci->CmdTransportInfo[fifo_number].fifoStart = (uint32 *) ci->BaseAddr1 + ( ci->CmdTransportInfo[fifo_number].fifoOffset >> 2 );
	ci->CmdTransportInfo[fifo_number].fifoEnd   = ci->CmdTransportInfo[fifo_number].fifoStart + ( ci->CmdTransportInfo[fifo_number].fifoSize >> 2 );

	/* Adjust room values. 
	** RoomToEnd needs enough room for the jmp packet since we never
	** allow the hw to auto-wrap. RoomToRead needs to be adjusted so that
	** we never acutally write onto the read ptr.
	**
	** fifoRoom is generally the min of roomToEnd and roomToRead, but we
	** 'know' here that roomToRead < roomToEnd.   
	*/
	ci->CmdTransportInfo[fifo_number].roomToEnd = ci->CmdTransportInfo[fifo_number].fifoSize - FIFO_END_ADJUST;
	ci->CmdTransportInfo[fifo_number].fifoRoom  = ci->CmdTransportInfo[fifo_number].roomToReadPtr = ci->CmdTransportInfo[fifo_number].roomToEnd - sizeof( uint32 );
	
	/* Set initial fifo state. hw read and sw write pointers at
	 * start of the fifo.
	 */
	ci->CmdTransportInfo[fifo_number].fifoPtr  = ci->CmdTransportInfo[fifo_number].fifoStart;
	ci->CmdTransportInfo[fifo_number].fifoRead = GetFifoHwReadPtr(fifo_number);
	
	if ( (void*)ci->CmdTransportInfo[fifo_number].fifoPtr != (void*)ci->CmdTransportInfo[fifo_number].fifoRead )
	{
		dprintf(("Initial fifo state is incorrect\n" ));
		return B_ERROR;
	}
	
	/* The hw is now in a usable state from the fifo macros.
	 * 
	 * NB: See the comment in fxglide.h for the difference between
	 * these flags.
	 */
	return B_OK;
}

void FifoMakeSpace(uint32 fifo_number, uint32 slots)
{
  uint32 slots_available;

//dprintf(("FifoMakeSpace: ENTER, slots = %d\n", slots));
  /*
  ** Check to see if we have to wrap to get enough space.
  */
  if (slots > (ci->CmdTransportInfo[fifo_number].fifoEnd) - (ci->CmdTransportInfo[fifo_number].fifoPtr))
	{
//dprintf(("FifoMakeSpace: wrapping FIFO, fifoEnd = %p, fifoPtr = %p\n", ci->CmdTransportInfo[fifo_number].fifoEnd, ci->CmdTransportInfo[fifo_number].fifoPtr));
    /*
    ** Make sure that the read pointer is not ahead of us in the
    ** the command fifo before wrapping.
    ** This insures two things:
    **   1) There is room at fifo_ptr for the JMP packet.
    **   2) There are slots available at the beginning of the fifo up to the read_ptr.
    **
    ** Since we are wrapping, insure that the read pointer is not at the
    ** beginning of the fifo to prevent the ambiguous situation described
    ** below.
    */
    do {
      ci->CmdTransportInfo[fifo_number].fifoRead = GetFifoHwReadPtr(fifo_number);
    }
    while (ci->CmdTransportInfo[fifo_number].fifoRead > ci->CmdTransportInfo[fifo_number].fifoPtr || 
	   				ci->CmdTransportInfo[fifo_number].fifoRead == ci->CmdTransportInfo[fifo_number].fifoStart);
//dprintf(("FifoMakeSpace: fifoRead = %p, fifoPtr = %p, fifoStart = %p\n", ci->CmdTransportInfo[fifo_number].fifoRead, ci->CmdTransportInfo[fifo_number].fifoPtr, ci->CmdTransportInfo[fifo_number].fifoStart));
    /*
    ** Put a jump command in command fifo to wrap to the beginning.
    */
    *(ci->CmdTransportInfo[fifo_number].fifoPtr) = (ci->CmdTransportInfo[fifo_number].fifoOffset >> 2) << SSTCP_PKT0_ADDR_SHIFT |
      		SSTCP_PKT0_JMP_LOCAL;
    P6FENCE;

    /*
    ** Reset the fifo_ptr to the beginning of the command fifo.
    */
    ci->CmdTransportInfo[fifo_number].fifoPtr = ci->CmdTransportInfo[fifo_number].fifoStart;
  }

  /*
  ** Wait for enough slots to satisfy the request.
  */
  do {
    ci->CmdTransportInfo[fifo_number].fifoRead = GetFifoHwReadPtr(fifo_number);

    /*
    ** If the HW read_ptr is ahead the SW fifo_ptr, we don't allocate the
    ** fifo slot immediately behind the HW read_ptr.  This is to prevent
    ** the following ambiguous situation...
    **
    ** If (HW read_ptr == SW fifo_ptr) is it because the HW read_ptr has
    ** caught up to the SW fifo_ptr and the fifo is completely empty
    ** OR is it because the SW fifo_ptr has caught up to the HW read_ptr
    ** and the fifo is completely full?
    */
    if ((uint32*)ci->CmdTransportInfo[fifo_number].fifoRead > ci->CmdTransportInfo[fifo_number].fifoPtr)
      slots_available = (uint32)(ci->CmdTransportInfo[fifo_number].fifoRead) - (uint32)(ci->CmdTransportInfo[fifo_number].fifoPtr) - 1;
    else
      slots_available = ci->CmdTransportInfo[fifo_number].fifoEnd - ci->CmdTransportInfo[fifo_number].fifoPtr;
//dprintf(("FifoMakeSpace: slots_available = %d, fifoRead = %p, fifoPtr = %p, fifoEnd = %p\n",
//		slots_available, ci->CmdTransportInfo[fifo_number].fifoRead, ci->CmdTransportInfo[fifo_number].fifoPtr, ci->CmdTransportInfo[fifo_number].fifoEnd));
	} while (slots > slots_available);

//dprintf(("FifoMakeSpace - EXIT, ci->CmdTransportInfo[%d].fifoRoom = 0x%x\n", fifo_number, ci->CmdTransportInfo[fifo_number].fifoRoom));
  ci->CmdTransportInfo[fifo_number].fifoRoom = slots_available-slots;
}

// Make room in the fifo for 'slots' entries - a slot is 32 bits wide

void 
FifoAllocateSlots(int fifo_number, int slots) {

//  if (pTDFX->fifoEnd-pTDFX->fifoPtr<pTDFX->fifoSlots)
//  	dprintf(("FifoAllocateSlots(%d) - FIFO overrun\n", slots);

  ci->CmdTransportInfo[fifo_number].fifoRoom -= slots * sizeof(uint32);
  if (ci->CmdTransportInfo[fifo_number].fifoRoom < 0)
		FifoMakeSpace(fifo_number, slots * sizeof(uint32));
}


uint32 GetFifoHwReadPtr (int32 fifo_number)
{
	uint32 rVal = 0;
	uint32 status, readPtrL1, readPtrL2;

	do
	{
		if (fifo_number == 1)
			readPtrL1 = _V3_ReadReg( ci, V3_CMD_RD_PTR_L_1 );
		else
			readPtrL1 = _V3_ReadReg( ci, V3_CMD_RD_PTR_L_0 );
			
		status = _V3_ReadReg( ci, V3_3D_STATUS );
		
		if (fifo_number == 1)
			readPtrL2 = _V3_ReadReg( ci, V3_CMD_RD_PTR_L_1 );
		else
			readPtrL1 = _V3_ReadReg( ci, V3_CMD_RD_PTR_L_0 );
	} while (readPtrL1 != readPtrL2);

	rVal = (((uint32) ci->CmdTransportInfo[fifo_number].fifoStart) +
			  readPtrL2 - (uint32) ci->CmdTransportInfo[fifo_number].fifoOffset);

	return rVal;
}										  /* GetFifoHwReadPtr */
