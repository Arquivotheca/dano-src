/******************************************************************************
 * Private definitions for RAGE128 CPUTIL.C                                   *
 *                                                                            *
 * Copyright (c) 1999 ATI Technologies Inc.  All rights reserved.             *
 *****************************************************************************/
#ifndef _CPUTIL_H
#define _CPUTIL_H

// Note that the ring buffer size must be power-of-2, min size 2 DWORDs
#define RING_SIZE       0x00010000   // 256k ring buffer (64k DWORDs)
#define RING_SIZE_LOG2  15           // log2 (RING_SIZE) - 1

#define VERTEX_BUF_SIZE (1L << 20)   // 1MB vertex buffer
#define INDIRECT_BUF_SIZE (1L << 20) // 1MB indirect buffer

#define CP_CSQ_CNTL__CSQ_MODE__PRIDIS_INDDIS    0x00000000
#define CP_CSQ_CNTL__CSQ_MODE__PRIPIO_INDDIS    0x10000000
#define CP_CSQ_CNTL__CSQ_MODE__PRIBM_INDDIS     0x20000000
#define CP_CSQ_CNTL__CSQ_MODE__PRIPIO_INDBM     0x30000000
#define CP_CSQ_CNTL__CSQ_MODE__PRIBM_INDBM      0x40000000
#define CP_CSQ_CNTL__CSQ_MODE__PRIPIO_INDPIO    0xF0000000

#define CP_ME_CNTL__ME_MODE__FREE_RUNNING       0x40000000

#if 1
#define CCE_WATERMARK_L 16
#define CCE_WATERMARK_M 8
#define CCE_WATERMARK_N 8
#define CCE_WATERMARK_K 128
#else
#define CCE_WATERMARK_L 2
#define CCE_WATERMARK_M 1
#define CCE_WATERMARK_N 1
#define CCE_WATERMARK_K 1
#endif

#pragma pack(1)

typedef struct tagRBINFO {
    volatile uint32 *ReadIndexPtr;           // Current Read pointer index
    uint32 ReadPtrPhysical;                  // Physical address of read pointer
    uint32 WriteIndex;                       // Current write pointer index
    uint32 *LinearPtr;                       // Virtual address of ring buffer
	uint32 Offset;							// NEW
    uint32 Size;                             // Size of ring buffer in DWORDs
} RBINFO;

typedef struct tagBUFINFO {
    uint32 *LinearPtr;                  // Virtual address of buffer
    uint32 Offset;                      // Offset of buffer relative to
                                       //   base of memory
    uint32 Size;                        // Size of buffer in DWORDs
} BUFINFO;

typedef struct tagCPModeTable
{
    uint32 CSQmode;
    int8 primaryCSQ;
    int8 primaryBM;
    int8 indirectCSQ;
    int8 indirectBM;
} CPModeTable;

#endif // _CPUTIL_H


