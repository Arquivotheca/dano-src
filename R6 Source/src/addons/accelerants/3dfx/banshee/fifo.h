/* :ts=8 bk=0
 *
 * fifo.h: Fifo management routines.
 *
 * $Id:$
 *
 * Andrew Kimpton					1999.11.03
 *
 * Copyright 1998 Be Incorporated.  Confidential unpublished work.
 * All Rights Reserved.
 */

/* Chipfield ids that glide uses. */
#define kChipFieldShift (8UL + 3UL)
typedef enum {
  eChipBroadcast    = 0x00UL,
  eChipFBI          = 0x01UL,
  eChipTMU0         = 0x02UL,
  eChipTMU1         = 0x04UL,
  eChipTMU2         = 0x08UL,
  eChipAltBroadcast = 0x0FUL,
} FifoChipField;

#define BROADCAST_ID eChipBroadcast

#define FifoMakeRoom(p, n) FifoAllocateSlots(p, (n)+1)
#define FifoWriteLong(p, a, v) WRITE_FIFO(p, a, v)
#define WRITE_FIFO(_n, loc, _val) \
  do { \
    *(ci->CmdTransportInfo[_n].fifoPtr)++ = _val; \
  } while(0) 

/*
** Send a packet header type 2 to the cmdfifo
*/
#define SET_PKT2_HEADER(_mask) \
    WRITE_FIFO (1, 0, ((_mask) << SSTCP_PKT2_MASK_SHIFT) | SSTCP_PKT2)

// P6FENCE
uint32 p6FenceVar;
#define P6FENCE __asm__ __volatile__ ("xchg %%eax, p6FenceVar": : :"%eax" );
