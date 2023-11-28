/* ++++++++++
	mac_map.h
	Copyright (C) 1996 Be Inc.  All Rights Reserved.

	Structures that define the first block and the partition
	table of a MacOS file system.
 ++++++++++ */

#ifndef _MAC_MAP_H
#define _MAC_MAP_H

#define	sbSIGWord	0x4552
#define	pMapSIG		0x504D

struct Block0 {
	uint16	sbSig;			/*unique value for SCSI block 0*/
	uint16	sbBlkSize;		/*block size of device*/
	uint32	sbBlkCount;		/*number of blocks on device*/
	uint16	sbDevType;		/*device type*/
	uint16	sbDevId;		/*device id*/
	uint32	sbData;			/*not used*/
	uint16	sbDrvrCount;	/*driver descriptor count*/
	uchar	ddBlock[4];		/*1st driver's starting block*/
	uint16	ddSize;			/*size of 1st driver (512-byte blks)*/
	uint16	ddType;			/*system type (1 for Mac+)*/
	uint16	ddPad[243];		/*ARRAY[0..242] OF INTEGER; not used*/
}; typedef struct Block0 Block0;

struct Partition {
	uint16	pmSig;			/*unique value for map entry blk*/
	uint16	pmSigPad;		/*currently unused*/
	uint32	pmMapBlkCnt;	/*# of blks in partition map*/
	uint32	pmPyPartStart;	/*physical start blk of partition*/
	uint32	pmPartBlkCnt;	/*# of blks in this partition*/
	uchar	pmPartName[32];	/*ASCII partition name*/
	uchar	pmParType[32];	/*ASCII partition type*/
	uint32	pmLgDataStart;	/*log. # of partition's 1st data blk*/
	uint32	pmDataCnt;		/*# of blks in partition's data area*/
	uint32	pmPartStatus;	/*bit field for partition status*/
	uint32	pmLgBootStart;	/*log. blk of partition's boot code*/
	uint32	pmBootSize;		/*number of bytes in boot code*/
	uint32	pmBootAddr;		/*memory load address of boot code*/
	uint32	pmBootAddr2;	/*currently unused*/
	uint32	pmBootEntry;	/*entry point of boot code*/
	uint32	pmBootEntry2;	/*currently unused*/
	uint32	pmBootCksum;	/*checksum of boot code*/
	uchar	pmProcessor[16];/*ASCII for the processor type*/
	uint16	pmPad[188];		/*512 bytes long currently unused*/
}; typedef struct Partition Partition;

#endif /* _MAC_MAP_H */
