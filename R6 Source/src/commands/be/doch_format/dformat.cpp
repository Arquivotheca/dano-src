/*
 * $Log:   V:/Flite/archives/OSAK/util/dformat/DFORMAT.C_V  $
 * 
 *    Rev 1.8   24 Apr 2000 16:02:16   dimitrys
 * Add Special SPL signature writing for Firmware Ver1.23
 *
 *    Rev 1.7   23 Feb 2000 17:52:24   dimitrys
 * Switch to bigger unit if noOfUnits >= 12K in order to
 *   keep noOfUnits smaller than 12K (192M DOC Size)
 *
 *    Rev 1.6   02 Feb 2000 13:13:20   dimitrys
 * Add 'persentUse' % of BootUnits to TransferUnits
 *
 *    Rev 1.5   27 Jan 2000 17:12:00   dimitrys
 * Convert *realBootBlocks into boot Blocks within real
 *   Units
 */
/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-2000			*/
/*									*/
/************************************************************************/
#include <OS.h>
#include <StorageKit.h>
#include <fs_info.h>
 
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "dformat.h"
/*-------------------------------------------------------------------*/
#define VERSION_NAME TEXT("Version 1.0(ES)")
#define COPYRIGHT    TEXT("Copyright (C) M-Systems, 1992-1999, Be Inc., 2000")
/*-------------------------------------------------------------------*/
#if (OS_APP != OS_BEOS)
extern unsigned long window;
Anand *getAnandRec(unsigned drive);
NFDC21Vars *getNFDC21Vars(unsigned drive);
#endif

#ifndef SIGN_CHECK
unsigned char defaultSignature[] = { 'B','I','P','O' };
#endif /* ! SIGN_CHECK */

#if (OS_APP == OS_BEOS)

#include "doch_ioctl.h"


int		doch_dev;	
int		verbosity = 1;


static void
dump_phys_info(const PhysicalInfo* phys_info)
{
	if(verbosity >= 1) 
		printf("flash type %u, unit size %ld, media size %ld, chip size %ld, interleaving %d\n",
			phys_info->type, phys_info->unitSize, phys_info->mediaSize, phys_info->chipSize, phys_info->interleaving);
}

static status_t
open_doch_device(unsigned device_num)
{
	char name[sizeof("/dev/") + sizeof(DOCH_DEVICE_ROOT_NAME) + 3 + sizeof("raw")];
	
	if(device_num>999)
		device_num = 999;
	
	sprintf((char*)name, "/dev/%s%u/raw", DOCH_DEVICE_ROOT_NAME, device_num);
	if((doch_dev = open(name, O_RDWR)) < 0)
	{
		fprintf(stderr, "Can't open %s: %s\n", name, strerror(errno));
		return errno;
	}
	if(verbosity >=1 )
		printf("Opened %s\n", name);
	return B_OK;
}




inline void toUNAL(unsigned char FAR0 *unal, unsigned short n)
{
  unal[1] = (unsigned char)(n >> 8);
  unal[0] = (unsigned char)n;
}


static const NFDC21Vars*
getNFDC21Vars(unsigned unused_drive)
{
	static NFDC21Vars	vars;
	
	if(ioctl(doch_dev, DOCH_copyNFDC21Vars, &vars, sizeof(NFDC21Vars)) != 0)
	{
		fprintf(stderr, "Ioctl error %d for DOCH_copyNFDC21Vars: %s\n", errno, strerror(errno));
		exit(1);
	}

	return &vars;
}



static const Anand*
getAnandRec(unsigned unused_drive)
{
	static Anand	anand_rec;
	
	if(ioctl(doch_dev, DOCH_copyAnandRec, &anand_rec, sizeof(Anand)) != 0)
	{
		fprintf(stderr, "Ioctl error %d for DOCH_copyAnandRec: %s\n", errno, strerror(errno));
		exit(1);
	}

	return &anand_rec;
}


static status_t
copyANANDPhysUnits(ANANDPhysUnit* phys_units, unsigned unused_drive, size_t buffer_size)
{
	if(ioctl(doch_dev, DOCH_copyANANDPhysUnits, phys_units, buffer_size) != 0)
	{
		fprintf(stderr, "Ioctl error %d for DOCH_copyANANDPhysUnits: %s\n", errno, strerror(errno));
		exit(1);
	}
	return B_OK;
}


/*----------------------------------------------------------------------*/
/*		           b d C a l l   				                                  */
/*									                                                    */
/* Common entry-point to all file-system functions. Macros are          */
/* to call individual function, which are separately described below.	  */
/*                                                                      */
/* Parameters:                                                          */
/*	function	: file-system function code (listed below)	              */
/*	ioreq		: IOreq structure				                                    */
/*                                                                      */
/* Returns:                                                             */
/*	FLStatus	: 0 on success, otherwise failed                          */
/*----------------------------------------------------------------------*/

FLStatus bdCall(FLFunctionNo functionNo, IOreq FAR2 *ioreq)
{
	DOCH_bdCall_buffer	call_buf;
	int					error_verbosity;
	
	/* make buffer validation in the driver less complicated: for ioregs without pointers the driver
	has to validate only the ioctl buffer */
	call_buf.functionNo = functionNo;
	memcpy(&call_buf.ioreq, ioreq, sizeof(IOreq));
	
	if(ioctl(doch_dev, DOCH_bdCall, &call_buf, sizeof(DOCH_bdCall_buffer)) != 0)
	{
		switch(functionNo)
		{
		case FL_ABS_MOUNT:
		case FL_DISMOUNT_VOLUME:
			error_verbosity = 3;
			break;
			
		default:
			error_verbosity = 0;
		}
		if(verbosity >= error_verbosity)
			fprintf(stderr, "Ioctl error %d for DOCH_bdCall_buffer(%d): %s\n", errno, functionNo, strerror(errno));

		return (FLStatus)errno;
	}
	return (FLStatus)B_OK;
}

#endif



/*-------------------------------------------------------------------*/
#if (OS_APP == OS_WINCE)
DOC_Parameters curDocParams;
/*---------------------------------------------------------*/
void getDocParameters(void FAR2 *docParams)
{ /* Return DOC_Parameters to caller. */
  DOC_Parameters *docParamPtr = (DOC_Parameters FAR2 *)docParams;
  docParamPtr->docAccess  = curDocParams.docAccess;
  docParamPtr->docShift   = curDocParams.docShift;
  docParamPtr->docAddress = curDocParams.docAddress;
}
/*---------------------------------------------------------*/
int getArgFromStr(FLCHAR *aa[], FLCHAR *str)
{ /* Put into 'aa' pointers to arguments from 'str' string. */
  char sfl;
  int  i,j,len,sw;

  for(j=i=0,len=STRLEN(str),sfl=FALSE;( i < len );i++) {
    if( !isspace(str[i]) ) {            /* word              */
      if( !sfl ) {                      /* start of the word */
	sfl = TRUE;
	sw = i;
      }
    }
    else {                              /* space       */
      if( sfl ) {                       /* end of word */
	str[i] = TEXT('\0');
	aa[j++] = &(str[sw]);
	sfl = FALSE;
      }
    }
  }
  if( sfl ) aa[j++] = &(str[sw]);
  aa[j] = NULL;
  return(j);
}
#endif /* OS_WINCE */
/*-------------------------------------------------------------------*/
unsigned short getNFTLUnitSizeBits(PhysicalInfo FAR2 *phinfo)
{
  long size;
  unsigned short nftlUnitSizeBits, mediaNoOfUnits;

  for(nftlUnitSizeBits=0,size=1L;( size < phinfo->unitSize );
      nftlUnitSizeBits++, size <<= 1);

  mediaNoOfUnits = (unsigned short)(phinfo->mediaSize >> nftlUnitSizeBits);

  /* Adjust unit size so header unit fits in one unit */
  while(mediaNoOfUnits * sizeof(ANANDPhysUnit) + SECTOR_SIZE > (1UL << nftlUnitSizeBits)) {
    nftlUnitSizeBits++;
    mediaNoOfUnits >>= 1;
  }
  /* Bound number of units to find room in 64 Kbytes Segment */
  if( (mediaNoOfUnits >= MAX_UNIT_NUM) && (nftlUnitSizeBits < MAX_UNIT_SIZE_BITS) ) {
    nftlUnitSizeBits++;
    mediaNoOfUnits >>= 1;
  }
  return(nftlUnitSizeBits);
}
/*-------------------------------------------------------------------*/
void initRandomNum(void)
{
#if (OS_APP == OS_DOS) || (OS_APP == OS_QNX) || (OS_APP == OS_BEOS)
  srand((unsigned int)clock());            /* Init random sequence */
#endif /* OS_DOS || OS_QNX */
}
/*-------------------------------------------------------------------*/
int getRandomNum(void)
{
#if (OS_APP == OS_DOS) || (OS_APP == OS_QNX) || (OS_APP == OS_BEOS)
  return( rand() );
#endif /* OS_DOS || OS_QNX */
#if (OS_APP == OS_WINCE)
  return( (int)Random() );
#endif /* OS_WINCE */
}
/*-------------------------------------------------------------------*/
FILE_HANDLE fileOpen( FLCHAR *fileName, unsigned fileAttr )
{
#if (OS_APP == OS_DOS) || (OS_APP == OS_QNX) || (OS_APP == OS_BEOS)
  return( open(fileName, fileAttr) );
#endif /* OS_DOS */
#if (OS_APP == OS_WINCE)
  return( CreateFile(fileName, fileAttr, 0, NULL, OPEN_EXISTING,
		     FILE_ATTRIBUTE_NORMAL, NULL) );
#endif /* OS_WINCE */
}
/*-----------------------------------------------*/
int fileClose( FILE_HANDLE fileHandle )
{
#if (OS_APP == OS_DOS) || (OS_APP == OS_QNX) || (OS_APP == OS_BEOS)
  return( close(fileHandle) );
#endif /* OS_DOS */
#if (OS_APP == OS_WINCE)
  return( (int)CloseHandle(fileHandle) );
#endif /* OS_WINCE */
}
/*-----------------------------------------------*/
int fileRead( FILE_HANDLE fileHandle, void *buf, unsigned blockSize )
{
#if (OS_APP == OS_DOS) || (OS_APP == OS_QNX) || (OS_APP == OS_BEOS)
  return( read(fileHandle, buf, blockSize) );
#endif /* OS_DOS */
#if (OS_APP == OS_WINCE)
  DWORD readNow;
  ReadFile(fileHandle,(LPVOID)buf,(DWORD)blockSize,(LPDWORD)&readNow,NULL);
  return((int)readNow);
#endif /* OS_WINCE */
}
/*-----------------------------------------------*/
long fileLseek( FILE_HANDLE fileHandle, long offset, int moveMethod )
{
#if (OS_APP == OS_DOS) || (OS_APP == OS_QNX) || (OS_APP == OS_BEOS)
  return( lseek(fileHandle, offset, moveMethod) );
#endif /* OS_DOS */
#if (OS_APP == OS_WINCE)
  DWORD curPtr;
  curPtr = SetFilePointer(fileHandle,offset,NULL,(DWORD)moveMethod);
  return((long)curPtr);
#endif /* OS_WINCE */
}
/*-----------------------------------------------*/
long fileLength( FILE_HANDLE fileHandle )
{
#if (OS_APP == OS_DOS) || (OS_APP == OS_QNX)
  return( filelength(fileHandle) );
#endif /* OS_DOS */
#if (OS_APP == OS_WINCE)
  DWORD fileSize;
  return( GetFileSize( fileHandle, (LPDWORD)&fileSize) );
#endif /* OS_WINCE */
#if (OS_APP == OS_BEOS)
	struct stat	fst;
	fstat(fileHandle, &fst);
	return (fstat(fileHandle, &fst) == 0) ? fst.st_size : 0;
#endif
}
/*-------------------------------------------------------------------*/
#ifdef WRITE_EXB_IMAGE
/*-------------------------------------------------------------------
// Purpose:  Write boot image (collection of BIOS extention-style code
//           modules) to boot area taking care of bad blocks. Write SPL
//           in way compatible with IPL.
// NOTE   :  This method is for DOC 2000 and Millennium only.
//-------------------------------------------------------------------*/
FLStatus writeExbDriverImage( FILE_HANDLE fileExb, unsigned short FAR2 *realBootBlocks,
	      unsigned char GlobalExbFlags, unsigned short flags, int signOffset,
             const Anand FAR2 *anandPtr, ANANDPhysUnit FAR2 *physicalUnits )
{
  unsigned short unitSizeBits      = anandPtr->unitSizeBits;
  unsigned short erasableBlockBits = anandPtr->erasableBlockSizeBits;
  unsigned short blockInUnitBits   = unitSizeBits - erasableBlockBits;
  /* units in boot partition including bad units */
  unsigned short bootUnits  = anandPtr->bootUnits;
  long erasableBlockSize   = 1L << erasableBlockBits;
  unsigned char splFactor = (flags & BIG_PAGE) && !(flags & MDOC_ASIC) ? SPL_FACTOR : SPL_FACTOR - 1;
  long splHeaderAddr = (flags & MDOC_ASIC) ? IPL_SIZE : 0L;
  long bootFileSize;
  /* mCnt - number of code modules in bootimage file */
  unsigned short i, len, iUnit, mCnt, splSizeInFlash, datapiece, byteCnt, heapSize;
  WorkPlanEntry  work[MAX_CODE_MODULES];
  unsigned char buf[BLOCK], anandMark[2];
  CardAddress addr, startAddr;
  BIOSHeader  hdr;
  IOreq ioreq;

  /* Init special mark for Internal EEprom Mode for Millennium */
  anandMark[0] = anandMark[1] = 0x55;

  /* make sure that SPL resides in "good" region of boot area */
  splSizeInFlash = SPL_SIZE * splFactor;

  for(addr=0;( addr < splSizeInFlash ); addr += erasableBlockSize)
    if( IS_BAD(physicalUnits[(unsigned short)(addr >> unitSizeBits)]) )
      return(flGeneralFailure);         /* bad blocks in SPL area */

  /* set SPL entry in workplan */
  if( flags & MDOC_ASIC ) {
    work[0].fileOff   = 0L;
    work[0].len       = SPL_SIZE;
  }
  else {
    work[0].fileOff   = IPL_SIZE;
    work[0].len       = SPL_SIZE - IPL_SIZE;
  }
  work[0].flashAddr = (CardAddress)0;

  /* find out sizes of code modules in bootimage file */
  bootFileSize = fileLength(fileExb);

  for(mCnt=1;( (work[mCnt-1].fileOff+work[mCnt-1].len) < bootFileSize );mCnt++) {
    work[mCnt].fileOff = work[mCnt-1].fileOff + work[mCnt-1].len;
    fileLseek(fileExb, work[mCnt].fileOff, FILE_BEGIN);
    fileRead(fileExb, &hdr, sizeof(hdr));
    work[mCnt].len = (unsigned short)hdr.lenMod512 * BLOCK;
    if( ((hdr.signature[0] != 0x55) || (hdr.signature[1] != 0xAA)) ||
	(work[mCnt].fileOff + work[mCnt].len > bootFileSize) )
      return(flBadLength);                /* bad code module header */
  }
  /* last code module is just 8K pad so ignore it */
  mCnt -= 1;

  /* try to accomodate all code modules in "good" regions of boot area */
  startAddr = splSizeInFlash;
  iUnit = (unsigned short)(startAddr >> unitSizeBits);
  for(i=1;( i < mCnt );i++) {
    /* find nearest "good" region */
    for(;( iUnit < bootUnits );iUnit++) {
      if( !IS_BAD(physicalUnits[iUnit]) ) /* good unit encountered */
        break;
      startAddr = (CardAddress)(iUnit + 1) << unitSizeBits;
    }
    if( iUnit == bootUnits )
      return(flVolumeTooSmall);           /* out of boot area */

    work[i].flashAddr = startAddr;

    for(len = 0, addr=startAddr;( len < work[i].len );len += BLOCK) {
      /* find real length of the module */
      iUnit = addr >> unitSizeBits;
      if( IS_BAD(physicalUnits[iUnit]) )  /* bad unit encountered */
	addr = (CardAddress)(iUnit + 1) << unitSizeBits;
      addr += BLOCK;
      if( iUnit == bootUnits )
        return(flVolumeTooSmall);         /* out of boot area */
    }
    startAddr = addr;                     /* next region */
  } /* all the code modules have been accomodated */
  /* set real number of blocks for image */
  *realBootBlocks = 1 + ((((SPL_FACTOR - splFactor) * SPL_SIZE) + startAddr - 1) >> erasableBlockBits);
  /* Convert real Boot Blocks into boot Blocks within real Units */
  *realBootBlocks = (1 + ((*realBootBlocks-1) >> blockInUnitBits)) << blockInUnitBits;

  /* erase entire boot area */
  for(i=0;( i < (*realBootBlocks) ); i++)
    if( !IS_BAD(physicalUnits[i >> blockInUnitBits]) ) {  /* skip bad units */
      ioreq.irHandle = 0;
      ioreq.irUnitNo = i;
      ioreq.irUnitCount = 1;
      checkStatus( flPhysicalErase(&ioreq) );
    }

  /* write SPL to flash in a fashion compatible with IPL */
  tffsset (buf, 0xff, sizeof(buf) );

  for(i=0;( i < mCnt ); i++) {
    fileLseek(fileExb, work[i].fileOff, FILE_BEGIN);

    addr = work[i].flashAddr;
    datapiece = (i == 0 ? BLOCK/splFactor : BLOCK);
    for(byteCnt=0;( byteCnt < work[i].len ); byteCnt += datapiece, addr += BLOCK) {
      iUnit = addr >> unitSizeBits;
      while( IS_BAD(physicalUnits[iUnit]) ) {         /* skip bad units */
        iUnit++;
        addr = (CardAddress)iUnit << unitSizeBits;
      }

      fileRead(fileExb, buf, datapiece);

      if( ((addr == 0L) || (addr == BLOCK)) && (flags & MDOC_ASIC)) {
        if( (addr == BLOCK) && (signOffset != SIGN_OFFSET) ) {
          /* Change byte #406 to non-0xFF value to force Internal EEprom Mode */
          ioreq.irHandle = 0;
          ioreq.irFlags = EXTRA;
          ioreq.irAddress = addr + 6;
          ioreq.irByteCount = 2;
          ioreq.irData = (void FAR1 *)anandMark;
          checkStatus( flPhysicalWrite(&ioreq) );
        }
      }

      if( addr == splHeaderAddr ) {                   /* SPL header */
	/* Change splHeader.biosHdr.lenMod512 to cover the whole boot area */
	SplHeader FAR2 *p = (SplHeader FAR2 *)buf;

	unsigned char tmp     = p->biosHdr.lenMod512;
	p->biosHdr.lenMod512  = ((unsigned long)(*realBootBlocks) << erasableBlockBits) / BLOCK;
	p->chksumFix         -= (p->biosHdr.lenMod512 - tmp);

	/* generate random run-time ID and write it into splHeader. */
	initRandomNum();                              /* Init random sequence */
        p->runtimeID[0] = (unsigned char)getRandomNum();
        p->runtimeID[1] = (unsigned char)getRandomNum();
        p->chksumFix -= (unsigned char)(p->runtimeID[0]);
        p->chksumFix -= (unsigned char)(p->runtimeID[1]);

	/* calculate TFFS heap size and write it into splHeader. */
        heapSize = 3 * anandPtr->noOfUnits + 2 * KBYTE;
        toUNAL2(p->tffsHeapSize, heapSize);
        p->chksumFix -= (unsigned char)(heapSize);
        p->chksumFix -= (unsigned char)(heapSize >> 8);
      }
      else
	if( (i == 1) && (byteCnt == 0) ) {
	  /* put QUIET into SS header */
	  SSHeader FAR2 *ssp = (SSHeader FAR2 *)buf;

	  ssp->exbFlags  |= GlobalExbFlags;
	  ssp->chksumFix -= GlobalExbFlags;
	}
	else
	  if( (i == 2) && (byteCnt == 0) ) {
	    /* put "install as first drive" & QUIET mark into TFFS header */
	    TffsHeader FAR2 *p = (TffsHeader FAR2 *)buf;

	    p->exbFlags  |= GlobalExbFlags;
	    p->chksumFix -= GlobalExbFlags;
	  }
      ioreq.irHandle = 0;
      if( signOffset == SIGN_OFFSET ) /* if singOffset == 8, use EDC */
	ioreq.irFlags = EDC;
      else ioreq.irFlags = 0;
      ioreq.irAddress = addr;
      ioreq.irByteCount = BLOCK;
      ioreq.irData = (void FAR1 *)buf;
      checkStatus( flPhysicalWrite(&ioreq) );

      if( (addr % erasableBlockSize) == 0 ) { /* write SPL signature */
        ioreq.irHandle = 0;
        ioreq.irFlags = EXTRA;
        ioreq.irAddress = addr + signOffset;
        ioreq.irByteCount = SIGNATURE_NAME;
        ioreq.irData = (void FAR1 *)SIGN_SPL;
        checkStatus( flPhysicalWrite(&ioreq) );
      }
    }
  }
  iUnit = 1 + ((addr - 1) >> erasableBlockBits);
  for(;( iUnit < (*realBootBlocks) );iUnit++) {
    if( !IS_BAD(physicalUnits[iUnit >> blockInUnitBits]) ) { /* skip bad units */
      addr = (CardAddress)iUnit << erasableBlockBits;
      ioreq.irHandle = 0;                 /* write signature */
      ioreq.irFlags = EXTRA;
      ioreq.irAddress = addr + signOffset;
      ioreq.irByteCount = SIGNATURE_NAME;
      ioreq.irData = (void FAR1 *)SIGN_SPL;
      checkStatus( flPhysicalWrite(&ioreq) );
    }
  }
  return(flOK);
}
#endif /* WRITE_EXB_IMAGE */
/*-----------------------------------------------------------------------
// Purpose:  Write boot loader image (collection of binary data) to boot
//           area taking care of bad blocks. Marking is made in next way:
//           ZZZZXXXX, where ZZZZ - 4 letters (default is BIPO), and
//           XXXX - is serial unit number (from 0000 to FFFF). Last unit
//           that contains data is FFFF, and if boot loader area contains
//           more units, all other units will have FFFF number in signature.
// NOTE   :  This method is for DOC 2000 only.
// Parameters:
//           file        - open file descriptor of the image
//           startBlock     - start to write from this block of partition
//           areaLen        - length of the writing chunk
//           sigh           - pointer to signature
//           anandPtr       - pointer to Anand algorithm structure
//           physicalUnits  - pointer to bad block table array
// Returns:
//           flNoSpaceInVolume - not enough space to write image
//           flWriteFault      - error writing to media
//           flOK              - success
//-----------------------------------------------------------------------*/
FLStatus writeBootAreaFile( FILE_HANDLE file, unsigned short startBlock,
              unsigned long areaLen, unsigned char FAR2 *sign, int signOffset,
	      const Anand FAR2 *anandPtr, ANANDPhysUnit FAR2 *physicalUnits )
{
  unsigned short unitSizeBits      = anandPtr->unitSizeBits;
  unsigned short erasableBlockBits = anandPtr->erasableBlockSizeBits;
  unsigned short blockInUnitBits   = unitSizeBits - erasableBlockBits;
  unsigned short blockInUnit       = 1 << blockInUnitBits;
  /* amount of blocks to write */
  long writeBlocks = 1 + ((areaLen - 1) >> erasableBlockBits);
  /* blocks in boot partition including bad blocks */
  long bootBlocks  = anandPtr->bootUnits << blockInUnitBits;
  long erasableBlockSize   = 1L << erasableBlockBits;
  char stopFlag    = FALSE;
  CardAddress addr, i0, j;
  IOreq ioreq;
  int  readFlag;
  unsigned short iBlock, freeBlocks, wBlock;
  unsigned char buf[BLOCK], signature[SIGNATURE_LEN+1];

  /* check for space in boot area partition */
  freeBlocks = 0;
  for(iBlock=startBlock;( iBlock < bootBlocks ); iBlock += blockInUnit)
    if( !IS_BAD(physicalUnits[iBlock >> blockInUnitBits]) ) /* skip bad units */
      freeBlocks += blockInUnit;

  if( freeBlocks < writeBlocks )
    return(flNoSpaceInVolume);

  if( FILE_OPEN_ERROR(file) )                         /* mark only */
    stopFlag = TRUE;

  tffscpy(signature, sign, SIGNATURE_NAME);

  for(wBlock=0,iBlock=startBlock;( wBlock < writeBlocks );iBlock++) {
    if( !IS_BAD(physicalUnits[iBlock >> blockInUnitBits]) ) { /* skip bad units */

      ioreq.irHandle = 0;                   /* erase block */
      ioreq.irUnitNo = iBlock;
      ioreq.irUnitCount = 1;
      checkStatus( flPhysicalErase(&ioreq) );

      if( !stopFlag )                  /* write block from file */
        for(i0=0;( i0 < erasableBlockSize );i0+=BLOCK) {
          readFlag = fileRead(file, buf, BLOCK);
	  if( readFlag > 0 ) {
            ioreq.irHandle = 0;             /* write buffer */
            if( signOffset == SIGN_OFFSET ) /* if singOffset == 8, use EDC */
              ioreq.irFlags = EDC;
            else ioreq.irFlags = 0;
	    ioreq.irAddress = ((CardAddress)iBlock << erasableBlockBits) + i0;
            ioreq.irByteCount = BLOCK;      /* maybe readFlag, but no EDC */
	    ioreq.irData = (void FAR1 *)buf;
	    checkStatus( flPhysicalWrite(&ioreq) );
	  }
	  else {
	    stopFlag = TRUE;
	    if( i0 == 0 )                   /* change previous signature */
	      tffscpy(signature+SIGNATURE_NAME,"FFFF",SIGNATURE_NUM);
	    break;
	  }
	}
      if( wBlock != 0 ) {                   /* skip first time */
	ioreq.irHandle = 0;                 /* write signature */
	ioreq.irFlags = EXTRA;
        ioreq.irAddress = addr + signOffset;
	ioreq.irByteCount = SIGNATURE_LEN;
	ioreq.irData = (void FAR1 *)signature;
	checkStatus( flPhysicalWrite(&ioreq) );
      }
      if( wBlock == (writeBlocks - 1) )     /* last block */
	stopFlag = TRUE;
      if( stopFlag )                        /* create current signature */
	tffscpy(signature+SIGNATURE_NAME,"FFFF",SIGNATURE_NUM);
      else {
        for(i0=wBlock,j=SIGNATURE_LEN;( j > SIGNATURE_NAME );j--) {
	  signature[j-1] = (unsigned char)((i0 % 10) + '0');
	  i0 /= 10;
	}
      }
      addr = (CardAddress)iBlock << erasableBlockBits;
      wBlock++;                             /* another block written */
    }
  }
  /* write last signature */
  ioreq.irHandle = 0;                       /* write signature */
  ioreq.irFlags = EXTRA;
  ioreq.irAddress = addr + signOffset;
  ioreq.irByteCount = SIGNATURE_LEN;
  ioreq.irData = (void FAR1 *)signature;
  checkStatus( flPhysicalWrite(&ioreq) );
  return(flOK);
}

#if (OS_APP != OS_BEOS)
/*-------------------------------------------------------------------*/
void USAGE(void)
{
  PRINTF(TEXT("Usage: dformat [-s:{ bootimage | ! }] [-l:length] [-w:window] [-n:signature]\n"));
  PRINTF(TEXT("         -s:bootimage - write bootimage file to DiskOnChip\n"));
  PRINTF(TEXT("         -s:!         - mark bootimage area\n"));
  PRINTF(TEXT("         -l:length    - bootimage area length\n"));
  PRINTF(TEXT("         -w:window    - use window hex address explicitely\n"));
  PRINTF(TEXT("         -n:signature - signature for bootimage area (default - BIPO)\n"));
  PRINTF(TEXT("         -o:offset    - offset of signature for bootimage area (default - 8)\n"));
  PRINTF(TEXT("         -h | ?       - print help screen\n"));
#ifdef WRITE_EXB_IMAGE
  PRINTF(TEXT("         -e:exbimage  - write exbimage file to DiskOnChip (DOS Driver)\n"));
  PRINTF(TEXT("         -f           - make DiskOnChip the first disk in the system\n"));
#endif /* WRITE_EXB_IMAGE */
#if (OS_APP == OS_WINCE)
  PRINTF(TEXT("         -a:access    - width of the data access to the DiskOnChip\n"));
  PRINTF(TEXT("         -p:shift     - physical address shift before virtual converting (default - 8)\n"));
#endif /* OS_WINCE */
}
void Print_Ver(void)
{
  PRINTF(TEXT("DiskOnChip 2000 Formatter for %s, %s\n"),OS_NAME,VERSION_NAME);
  PRINTF(TEXT("%s\n\n"),COPYRIGHT);
}
/*-------------------------------------------------------------------*/
#if (OS_APP == OS_DOS)
int cdecl main( int argc, char **argv )
#endif /* OS_DOS */
#if (OS_APP == OS_QNX)
int main( int argc, char **argv )
#endif /* OS_QNX */
#if (OS_APP == OS_WINCE)
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
		    LPTSTR lpCmdLine, int nCmdShow )
#endif /* OS_WINCE */
{
  IOreq ioreq;
  FLStatus status;
  PhysicalInfo phinfo;
  FILE_HANDLE fileBoot;
  int  i, j;
  unsigned short startBDKBlock = 0, nftlUnitSizeBits, mediaNoOfUnits, bootUnits;
  int  signOffset = SIGN_OFFSET, sflag = LEAVE_BOOT;
  FormatParams formatparams = STD_FORMAT_PARAMS;
  long bootFileSize = -1L;              /* leave previous boot partition */
  long tmpSize, exbFileSize = 0L;
  unsigned char sign[SIGNATURE_LEN];
  FLCHAR *err;
  Anand FAR2 *anandPtr;
  ANANDPhysUnit FAR2 *physicalUnits;
#ifdef WRITE_EXB_IMAGE
  unsigned short flags;
  unsigned char exbFlags = 0;
  FILE_HANDLE fileExb;
  NFDC21Vars FAR2 *nfdcVars;
#endif /* WRITE_EXB_IMAGE */

#if (OS_APP == OS_DOS)
  fileBoot = -1;
#endif /* OS_DOS */

#if (OS_APP == OS_WINCE)
  int  argc;
  FLCHAR *argv[20];
  int  docAccessType, physicalShift;
  /* Convert lpCmdLine -> argc, argv */
  argc = getArgFromStr(argv,lpCmdLine);
  fileBoot = NULL;
  docAccessType = DOC_ACCESS_TYPE;
  physicalShift = DOC_PHYSICAL_SHIFT;
#endif /* OS_WINCE */

  tffsset(sign,0xFF,SIGNATURE_LEN);     /* set 0xFF signature */
#ifndef SIGN_CHECK
  tffscpy(sign,defaultSignature,SIGNATURE_NAME);
#endif /* ! SIGN_CHECK */

  Print_Ver();

  for(i=ARG_START;( i < argc );i++) {           /* Command Line Processing */
    PRINTF(TEXT("%s\n"),argv[i]);
    if( *argv[i] == TEXT('-') )
      switch( toupper(*(argv[i]+1)) ) {
	case 'S':                        /* BDK BootImage file */
           if( *(argv[i]+3) == TEXT('!') ) {
             sflag = ERASE_BOOT;
             bootFileSize = 0L;
           }
	   else {
	     sflag = WRITE_BOOT;
             fileBoot = fileOpen(argv[i]+3,O_RDONLY | O_BINARY);
             if( FILE_OPEN_ERROR(fileBoot) ) {
	       PRINTF(TEXT("Error open file %s\n"),argv[i]+3);
	       return(flFileNotFound);
	     }
             tmpSize = fileLength(fileBoot);
             PRINTF(TEXT("File Size = %ld\n"),tmpSize);
             if( tmpSize == 0L ) {
	       PRINTF(TEXT("Error open file %s\n"),argv[i]+3);
	       return(flFileNotFound);
	     }
             if( bootFileSize < tmpSize )
               bootFileSize = tmpSize;
           }
	   break;

	case 'W':                        /* DiskOnChip Window flag */
	   window = STRTOL(argv[i]+3,&err,0);
	   PRINTF(TEXT("Window = %lx\n"),window);
	   break;

	case 'L':                        /* BDK partition Length flag */
	   sflag = WRITE_BOOT;
           tmpSize = STRTOL(argv[i]+3,&err,0);
           if( bootFileSize < tmpSize )
             bootFileSize = tmpSize;
           PRINTF(TEXT("Boot Size = %ld\n"),tmpSize);
           break;

        case 'N':                       /* BDK Signature flag */
           for(j=0;( j < SIGNATURE_NAME );j++)
	     sign[j] = (unsigned char)(*(argv[i]+3+j));
	   break;

        case 'O':                       /* BDK Signature Offset flag */
           signOffset = (int)STRTOL(argv[i]+3,&err,0);
           if( signOffset != SIGN_OFFSET )
             signOffset = 0;
           PRINTF(TEXT("Sign Offset = %d\n"),signOffset);
           break;

#ifdef WRITE_EXB_IMAGE
        case 'E':                       /* Write EXB driver image */
           if( *(argv[i]+3) == TEXT('!') ) {
             exbFileSize = 0L;
           }
           else {
	     sflag = WRITE_BOOT;
             fileExb = fileOpen(argv[i]+3,O_RDONLY | O_BINARY);
             if( FILE_OPEN_ERROR(fileExb) ) {
	       PRINTF(TEXT("Error open file %s\n"),argv[i]+3);
	       return(flFileNotFound);
	     }
             tmpSize = fileLength(fileExb);
             PRINTF(TEXT("File Size = %ld\n"),tmpSize);
             exbFileSize = tmpSize;
           }
           break;

        case 'F':                       /* Set 'Install_First' flag */
           exbFlags |= INSTALL_FIRST;
	   break;
#endif /* WRITE_EXB_IMAGE */

#if (OS_APP == OS_WINCE)
        case 'A':                       /* DiskOnChip data access flag */
           docAccessType = (int)STRTOL(argv[i]+3,&err,0);
           PRINTF(TEXT("Doc Access Type = %d\n"),docAccessType);
           break;

        case 'P':                       /* Physical address shift flag */
           physicalShift = (int)STRTOL(argv[i]+3,&err,0);
           if( physicalShift != DOC_PHYSICAL_SHIFT )
             physicalShift = 0;
           PRINTF(TEXT("Physical Address Shift = %d\n"),physicalShift);
           break;
#endif /* OS_WINCE */

	case 'H':
	case '?':
	   USAGE();
	   return(flOK);

	default:
	   PRINTF(TEXT("Illegal option %s\n"),argv[i]);
	   USAGE();
	   return(flUnknownCmd);
      }
    else {
      PRINTF(TEXT("Illegal option %s\n"),argv[i]);
      USAGE();
      return(flUnknownCmd);
    }
  }

#ifdef SIGN_CHECK
  if( sflag == WRITE_BOOT )                        /* BDK signature check */
    for(i=0;( i < SIGNATURE_NAME );i++)
      if( sign[i] == 0xFF ) {
	PRINTF(TEXT("Illegal signature: %s\n"),sign);
	return(flBadLength);
      }
#endif /* SIGN_CHECK */

#if (OS_APP == OS_WINCE)
  curDocParams.docAccess  = docAccessType;
  curDocParams.docShift   = physicalShift;
  curDocParams.docAddress = window;
#endif /* OS_WINCE */

  /* Low level checking */
  ioreq.irHandle = 0;
  ioreq.irData = &phinfo;
  if( (status = flGetPhysicalInfo(&ioreq)) != flOK ) {
    PRINTF(TEXT("DiskOnChip not found\n"));
    return(status);
  }

#ifdef FORMAT
  ioreq.irHandle = 0;
#if (OS_APP == OS_QNX)
  ioreq.irFlags = TL_FORMAT_ONLY;
#else
  ioreq.irFlags = TL_FORMAT;
#endif /* OS_QNX */
  /* maybe problem for 32 Kb units */
  nftlUnitSizeBits = getNFTLUnitSizeBits((PhysicalInfo FAR2 *)&phinfo);
  if( exbFileSize > 0 )
    exbFileSize = ((long)(1 + ((exbFileSize - 1) >> nftlUnitSizeBits))) << nftlUnitSizeBits;
  if( bootFileSize > 0 )
    bootFileSize = ((long)(1 + ((bootFileSize - 1) >> nftlUnitSizeBits))) << nftlUnitSizeBits;
  formatparams.bootImageLen = bootFileSize + exbFileSize;

  /* use boot area correction for big boot area */
  if( formatparams.bootImageLen > 0 )
    bootUnits = (unsigned short)(formatparams.bootImageLen >> nftlUnitSizeBits);
  else bootUnits = 0;
  mediaNoOfUnits = (unsigned short)(phinfo.mediaSize >> nftlUnitSizeBits);

  if( bootUnits >= mediaNoOfUnits ) {
    PRINTF(TEXT("Medium too small for given boot area!\n"));
    flExit();
    return(flVolumeTooSmall);
  }
  /* add 'percentUse'% of bootUnits to transfer units */
  formatparams.percentUse -= (unsigned)(((long)(100 - formatparams.percentUse)
                             * (bootUnits)) / (mediaNoOfUnits - bootUnits));
  ioreq.irData = &formatparams;
  PRINTF(TEXT("Formatting..."));
  status = flFormatVolume(&ioreq);
  if( status != flOK ) {
    PRINTF(TEXT("\nFormatting failed!\n"));
    flExit();
    return(status);
  }
#else /* FORMAT */
  ioreq.irHandle = 0;
  PRINTF(TEXT("Mounting..."));
  status = flAbsMountVolume(&ioreq);

  if( status != flOK ) {
    PRINTF(TEXT("\nMounting failed!\n"));
    flExit();
    return(status);
  }
#endif /* ! FORMAT */

  PRINTF(TEXT("Ok\n"));
/* ioreq.irHandle = 0;
  status = flSectorsInVolume(&ioreq);
  PRINTF(TEXT("Sectors in Volume = %ld\n"),ioreq.irLength); */

  anandPtr = getAnandRec(0);
  PRINTF(TEXT("Num of units = %u\n"),anandPtr->noOfUnits);
  /* get bad blocks table */
#ifdef MALLOC
  physicalUnits = (ANANDPhysUnit FAR2 *)MALLOC(anandPtr->noOfUnits * sizeof(ANANDPhysUnit));
  if( physicalUnits == NULL )
    return(flNotEnoughMemory);
  tffscpy( physicalUnits, anandPtr->physicalUnits,
	   anandPtr->noOfUnits * sizeof(ANANDPhysUnit));
#else
  physicalUnits = anandPtr->physicalUnits;
#endif
  /* disMount before low_level operations */
  ioreq.irHandle = 0;
  flDismountVolume(&ioreq);            /* deletes bad block table */

  if( sflag == WRITE_BOOT ) {
#ifdef WRITE_EXB_IMAGE
    if( exbFileSize > 0L ) {
      PRINTF(TEXT("Write Exb Image\n"));
      nfdcVars = getNFDC21Vars(0);
      /* flash.flags for BIG_PAGE, FULL_PAGE, var.flags for MDOC_ASIC */
      flags = anandPtr->flash.flags | nfdcVars->flags;
      if( (status = writeExbDriverImage( fileExb, (unsigned short FAR2 *)&startBDKBlock,
                      exbFlags, flags, signOffset, anandPtr, physicalUnits)) != flOK ) {
        PRINTF(TEXT("Exb Image Writing Fault\n"));
      }
      PRINTF(TEXT("Exb Image Size in Blocks = %d\n"),startBDKBlock);
    }
#endif /* WRITE_EXB_IMAGE */
    if( (status == flOK) && (bootFileSize > 0L) ) {
      PRINTF(TEXT("Write Boot Image\n"));
      if( (status = writeBootAreaFile( fileBoot, startBDKBlock, bootFileSize,
                      sign, signOffset, anandPtr, physicalUnits )) != flOK ) {
        PRINTF(TEXT("Boot Image Writing Fault\n"));
      }
    }
  }
#ifdef WRITE_EXB_IMAGE
  if( !FILE_OPEN_ERROR(fileExb) )
    fileClose(fileExb);
#endif /* WRITE_EXB_IMAGE */
  if( !FILE_OPEN_ERROR(fileBoot) )
    fileClose(fileBoot);
#ifdef MALLOC
  FREE(physicalUnits);
#endif
  flExit();
  if( status == flOK )
    PRINTF(TEXT("Ok\n"));
  return(status);
}




#else

void Print_Ver(void)
{
  PRINTF(TEXT("DiskOnChip 2000 Formatter for %s, %s\n"),OS_NAME,VERSION_NAME);
  PRINTF(TEXT("%s\n\n"),COPYRIGHT);
}


static void
usage(void)
{
	printf("Usage: doch_format [-e] [-f] [-b:{ exbimage | ! }] [-l:length]\n");
	printf("         -e           - erase everything on DiskOnChip\n");
	printf("         -b:exbimage  - write exbimage/BIOS file to DiskOnChip\n");
	printf("         -l:length    - bootimage/BIOS area length\n");
	printf("         -f           - make DiskOnChip the first disk in the system\n");
	printf("         -y           - do not pause for confirmation before formatting\n");
	printf("         -v:verbosity - message vebosity level from 0 to 10 (default is 1)\n");
	printf("         -h | ?       - print help screen\n");
	printf("\n");
	printf("Examples:\n");
	printf("          doch_format -y -b:DOC123P.EXB\n");
	printf("          doch_format\n");
}


static bool
get_confirmation(void)
{
	int	r;
	printf("Do you want to format/erase DiskOnChip (y/N)? ");
	r = getchar();
	printf("\n");
	return (tolower(r) == 'y');
}


static status_t
unmount_doch(void)
{
	status_t	err;
	IOreq		ioreq;
	
	err = flDismountVolume(&ioreq);
	if(err && (verbosity >= 3))
		fprintf(stderr, "Error %ld unmounting volume\n", err);
		
	return err;
}


static status_t
mount_doch(void)
{
	status_t	err;
	IOreq		ioreq;
	
	err = flAbsMountVolume(&ioreq);
	if(err && (verbosity >= 3))
		fprintf(stderr, "Error %ld mounting volume\n", err);
		
	return err;
}



/* assumes that DOCH is UNmounted */
static status_t 
erase_doch(unsigned unit_num)
{
	IOreq		ioreq;
	status_t	err;
	unsigned	i;
	int			p, p_old;
	
	if(verbosity >= 1)
		printf("Erasing...");
	for(p_old=0,i=0; i<unit_num ; )
	{
		ioreq.irUnitNo = i;
		ioreq.irUnitCount = 1;
		err = flPhysicalErase(&ioreq);
		if(err != B_OK)
		{
			fprintf(stderr, "Error %ld erasing unit #%u\n", err, i);
			return err;
		}
		++i;
		p = i*100; p /= unit_num;
		if(p != p_old)
		{
			if(verbosity >= 2)
				printf("\rErased   %3d%%", p);
			p_old = p;
		}
	}
	if(verbosity >= 1)
		printf(" Ok\n");
	return B_OK;
}



/* assumes that DOCH is UNmounted */
/* arg == -1 means do not erase/use default */
static status_t
format_doch(long int rounded_boot_image_len, int percent_use, int spare_units_num)
{
	IOreq			ioreq;
	FormatParams	format_params = STD_FORMAT_PARAMS;
	status_t		err;
	
	format_params.bootImageLen = rounded_boot_image_len;
	if(percent_use != -1)
		format_params.percentUse = percent_use;
	if(spare_units_num != -1)
		format_params.noOfSpareUnits = spare_units_num;
		
	ioreq.irData = &format_params;
	ioreq.irFlags = TL_FORMAT | TL_FORMAT_ONLY;
	
	if(verbosity >= 1)
		printf("Formatting...");
	err = flFormatVolume(&ioreq);
	if(err != B_OK)
		fprintf(stderr, "Error %ld\n", err);
	else
		if(verbosity >= 1)
			printf("Ok\n");
	
	return err;
}



/* assumes that DOCH is mounted */
static status_t
get_doch_internal_data(const Anand** anandPtr, ANANDPhysUnit** physicalUnits)
{
	*anandPtr = getAnandRec(0);
	
	if(verbosity >= 1)
		printf("Num of units = %u\n", (*anandPtr)->noOfUnits);
	// get bad blocks table
	*physicalUnits = (ANANDPhysUnit* ) malloc((*anandPtr)->noOfUnits * sizeof(ANANDPhysUnit));
	if( *physicalUnits == NULL )
		return ENOMEM;
	
	copyANANDPhysUnits(*physicalUnits, 0, (*anandPtr)->noOfUnits * sizeof(ANANDPhysUnit));
	return B_OK;
}



static void
free_doch_internal_data(const Anand* anandPtr, ANANDPhysUnit* physicalUnits)
{
	free(physicalUnits);
}


/* doesn't assume DOCH's mount state, returns UNmounted DOCH */
static status_t
write_doch_bios(const char* bios_file_name, bool is_first_drive)
{
	int				bios_fh;
	status_t		err;
	unsigned short	flags;
	unsigned short	startBDKBlock;

	const Anand*	anandPtr;
	ANANDPhysUnit*	physicalUnits;

	
	bios_fh = open(bios_file_name, O_RDONLY | O_BINARY);
	if(bios_fh == -1)
	{
		fprintf(stderr, "Can't open file %s: %s\n", bios_file_name, strerror(errno));
		return errno;
	}

	mount_doch();	
	err = get_doch_internal_data(&anandPtr, &physicalUnits);
	if(err)
		return err;
	
	//disMount before low_level operations
	err = unmount_doch();
	if(err)
	{
		fprintf(stderr, "Error %ld unmounting volume\n", err);
		goto end;
	}
		
	
	/* flash.flags for BIG_PAGE, FULL_PAGE, var.flags for MDOC_ASIC */
	flags = anandPtr->flash.flags | getNFDC21Vars(0)->flags;	/* FIXME multidisk: arg for getNFDC21Vars() */
	
	if( (err = writeExbDriverImage( bios_fh, (unsigned short FAR2 *)&startBDKBlock,
					is_first_drive ? INSTALL_FIRST : 0, flags, SIGN_OFFSET, anandPtr, physicalUnits)) != flOK )
	{
		fprintf(stderr, "Error %ld writing DiskOnChip BIOS\n", err);
		goto end;
	}
	if(verbosity >= 1)
		printf("BIOS Image Size in Blocks = %d\n", startBDKBlock);

end:
	free_doch_internal_data(anandPtr, physicalUnits);
	return err;
}


/* doesn't assume DOCH's mount state, returns UNmounted DOCH */
static status_t
get_physical_info(PhysicalInfo* phys_info)
{
	IOreq		ioreq;
	status_t	err;	
	
	unmount_doch();
	
	ioreq.irData = phys_info;
	err = flGetPhysicalInfo(&ioreq);
	if(err)
		fprintf(stderr, "Error %ld getting DiskOnChip physical info\n", err);
	
	return err;	
}


void flExit(void)
{
	mount_doch();
	close(doch_dev);
}



static status_t 
unmount_all_file_systems_on_doch(int device_num)
{
	status_t	err = B_OK;
	BVolumeRoster vr;
	BVolume v;

	char device_name[sizeof("/dev/") + sizeof(DOCH_DEVICE_ROOT_NAME) + 16];
		
	if(device_num>999)
		device_num = 999;
	
	sprintf(device_name, "/dev/%s%u/", DOCH_DEVICE_ROOT_NAME, device_num);
	
	while(vr.GetNextVolume(&v) == B_OK)
	{
		fs_info 	fsi;

		BDirectory*	rd = new BDirectory;
		v.GetRootDirectory(rd);
		BPath		mount_dir(rd, NULL);
		delete rd;

		fs_stat_dev(v.Device(), &fsi);

		if(verbosity >= 3)
			printf("Volume %s, root dir %s, device name %s\n", fsi.volume_name, mount_dir.Path(), fsi.device_name );

		if(strncmp(device_name, fsi.device_name, strlen(device_name)) == 0)
		{
			if(verbosity >= 1)
				printf("Unmounting %s from %s\n", fsi.device_name, mount_dir.Path());
				
			err = unmount(mount_dir.Path());
			if(err)
			{
				fprintf(stderr, "Can't unmout %s from %s: %s\n", fsi.device_name, mount_dir.Path(), strerror(errno));
				break;
			}
		}		
	
	}
	return err;
}


int main(int argc, char* argv[])
{
	status_t		err = B_OK;
	PhysicalInfo	phys_info;
	unsigned short	nftlUnitSizeBits;
	int				i;

	int 			percent_use = -1; /* default */
	int				spare_units_num = -1; /* default */
	const char*		bios_file_name = NULL;
	long			bios_size = -1; /* leave old BIOS */
	bool			do_erase = FALSE;
	bool			is_first_drive = FALSE;
	bool			ask_confirmation = TRUE;
	int				device_num = 0;

	setbuf(stdout, NULL);

	Print_Ver();

	for(i=1;  i < argc; i++)
	{
		if( *argv[i] == '-')
		{
			switch( toupper(*(argv[i]+1)))
			{
			case 'L':                        // Length flag
				bios_size = atoi(argv[i]+3);
				break;
	
			case 'B':                       // Write EXB driver image
				if( *(argv[i]+3) == '!')
					bios_size = 0L;
				else
					bios_file_name = argv[i]+3;
				break;
	
			case 'F':                       // Set 'Install_First' flag
				is_first_drive = TRUE;
				break;

			case 'E':                       
				do_erase = TRUE;
				break;

			case 'V':                       
				verbosity = atoi(argv[i]+3);
				break;

			case 'Y':                       
				ask_confirmation = FALSE;
				break;
	
			case 'H':
			case '?':
				usage();
				goto err0;
	
			default:
				printf("Illegal option %s\n", argv[i]);
				usage();
				err = flUnknownCmd;
				goto err0;
			}
		}
		else
		{
			printf("Illegal option %s\n", argv[i]);
			usage();
			err = flUnknownCmd;
			goto err0;
		}
	}

	if((err=open_doch_device(device_num)))
		goto err0;

	if(ask_confirmation)	
		if(!get_confirmation())
			goto err1;

	if((err = unmount_all_file_systems_on_doch(device_num)) != B_OK)
		goto err1;

	if((err=get_physical_info(&phys_info)))
	{
		fprintf(stderr, "Can't find DiskOnChip: error %ld\n", err);
		goto err1;
	}

	if(verbosity >= 1)
		dump_phys_info(&phys_info);

	if(do_erase)
		erase_doch(phys_info.mediaSize/phys_info.unitSize);

	/* Find the rounded size of the DOCH BIOS */
	nftlUnitSizeBits = getNFTLUnitSizeBits(&phys_info);
	if(bios_file_name != NULL)
	{
		struct stat	fst;
	
		if(stat(bios_file_name, &fst) != 0)
		{
			fprintf(stderr, "Can't get file length of %s: %s\n", bios_file_name, strerror(errno));
			err = errno;
			goto err1;
		}
		bios_size = MAX(fst.st_size, bios_size);
	}
	if(bios_size > 0)
		bios_size = ((long)(1 + ((bios_size - 1) >> nftlUnitSizeBits))) << nftlUnitSizeBits;

	if(bios_size >= phys_info.mediaSize)
	{
		fprintf(stderr, "BIOS area is too big (%ld bytes) for the DiskOnChip (%ld bytes)\n", bios_size, phys_info.mediaSize);
		err = flVolumeTooSmall;
		goto err1;
	}
	

	if((err = format_doch(bios_size, percent_use, spare_units_num)) != B_OK)
	{
		fprintf(stderr, "Error %ld formatting DiskOnChip\n", err);
		goto err1;
	}
	
	if(bios_file_name != NULL)
	{
		if((err = write_doch_bios(bios_file_name, is_first_drive)) != B_OK)
		{
			fprintf(stderr, "Error %ld writing DiskOnChip BIOS\n", err);
			goto err1;
		}
	}

err1:
	flExit();
err0:
	return err;
}
#endif