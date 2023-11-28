/*
 * $Log:   V:/Flite/archives/oldFLite/DFORMAT.H_V  $
   
      Rev 1.5   May 17 2000 12:06:34   vadimk
   remove // comments

      Rev 1.4   24 Apr 2000 15:24:20   dimitrys
   Add Special SPL header for Firmware Ver1.23
 */
/************************************************************************/
/*                                                                      */
/*		FAT-FTL Lite Software Development Kit			*/
/*		Copyright (C) M-Systems Ltd. 1995-2000			*/
/*									*/
/************************************************************************/

#ifndef _DFORMAT_H_
#define _DFORMAT_H_
#ifdef __cplusplus
extern "C" {
#endif
/*-------------------------------------------------------------------*/
/*-- DOS --- WINCE --- QNX --- BeOS ---*/
#define OS_DOS      0
#define OS_WINCE    1
#define OS_QNX      2
#define OS_BEOS     3
#define OS_APP      OS_BEOS
/*-------------------------------------------------------------------*/
#if (OS_APP == OS_DOS)
#include <fcntl.h>
#include <io.h>
#include <time.h>
/*-------------------------------------*/
typedef int       FILE_HANDLE;
typedef char      FLCHAR;
#define TEXT(t)   t
#define OS_NAME   TEXT("DOS")
#define STRTOL    strtol
#define STRLEN    strlen
#define PRINTF    printf
#define FILE_OPEN_ERROR(h) (h == -1)
#define ARG_START 1
#define FILE_BEGIN    SEEK_SET
#define FILE_CURRENT  SEEK_CUR
#define FILE_END      SEEK_END
#endif /* DOS */
/*-------------------------------------------------------------------*/
#if (OS_APP == OS_WINCE)
#include <windows.h>
/*-------------------------------------*/
typedef HANDLE    FILE_HANDLE;
typedef wchar_t   FLCHAR;
#define OS_NAME   TEXT("WINCE")
#define STRTOL    wcstol
#define O_RDONLY  GENERIC_READ
#define O_BINARY  0
#define isspace(c) ((c) == TEXT(' '))
#define STRLEN    wcslen
#define PRINTF    wprintf
#define FILE_OPEN_ERROR(h) (h == NULL)
#define ARG_START 0
#define DOC_ACCESS_TYPE     8
#define DOC_PHYSICAL_SHIFT  8
typedef struct {
  int  docAccess;
  int  docShift;
  unsigned long docAddress;
} DOC_Parameters;
#endif /* WINCE */
/*-------------------------------------------------------------------*/
#if (OS_APP == OS_QNX)
#include <sys/seginfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
/*-------------------------------------*/
typedef int       FILE_HANDLE;
typedef char      FLCHAR;
#define TEXT(t)   t
#define OS_NAME   TEXT("QNX")
#define STRTOL    strtol
#define STRLEN    strlen
#define PRINTF    printf
#define FILE_OPEN_ERROR(h) (h == -1)
#define ARG_START 1
#define O_BINARY  0
#define FILE_BEGIN    SEEK_SET
#define FILE_CURRENT  SEEK_CUR
#define FILE_END      SEEK_END
#endif /* QNX */
/*-------------------------------------------------------------------*/
#if (OS_APP == OS_BEOS)
#include <OS.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
/*-------------------------------------*/
typedef int       FILE_HANDLE;
typedef char      FLCHAR;
#define TEXT(t)   t
#define OS_NAME   TEXT("BeOS")
#define STRTOL    strtol
#define STRLEN    strlen
#define PRINTF    printf
#define FILE_OPEN_ERROR(h) (h == -1)
#define ARG_START 1
#define FILE_BEGIN    SEEK_SET
#define FILE_CURRENT  SEEK_CUR
#define FILE_END      SEEK_END
#endif /* BEOS */
/*-------------------------------------------------------------------*/
#include "fatlite.h"
#include "nftllite.h"
#include "diskonc.h"
/*-------------------------------------------------------------------*/
#define FORMAT
#define WRITE_EXB_IMAGE
/*#define SIGN_CHECK*/
/*-------------------------------------------------------------------*/
#define SPL_FACTOR     2
#define SPL_SIZE       0x2000
#define BLOCK          512
#define KBYTE          1024
#define IPL_SIZE       1024
#define MAX_CODE_MODULES 6
#define LEAVE_BOOT     0
#define WRITE_BOOT     1
#define ERASE_BOOT     2
#define SIGNATURE_NAME 4
#define SIGNATURE_NUM  4
#define SIGNATURE_LEN  (SIGNATURE_NAME + SIGNATURE_NUM)
/*-----------------------------------------------*/
#define SIGN_OFFSET    8
/*-----------------------------------------------*/
#if !(OS_APP == OS_BEOS)
#define MIN(a,b)   ((a) < (b) ? (a) : (b))
#define MAX(a,b)   ((a) > (b) ? (a) : (b))
#endif
/*---------------------------------------------------------*/
#ifdef WRITE_EXB_IMAGE
/* EXB Flag definitions */
#define	INSTALL_FIRST	1
#define	EXB_IN_ROM	2
#define	QUIET		4
#define FLOPPY		0x10
#define INT15_DISABLE   8
/*-------------------------------------*/
#define SIGN_SPL       "Дима"
/* ------------------------------------- */
typedef struct
{
  long           fileOff;   /* offset of BIOS extention in boot image file  */
  unsigned short len;       /* it's length in unsigned chars */
  unsigned long  flashAddr; /* address in flash */
} WorkPlanEntry;
/* ------------------------------------- */
typedef struct
{
  unsigned char  signature[2]; /* BIOS extention signature (0xAA55) */
  unsigned char  lenMod512; /* length in unsigned chars modulo 512 */
} BIOSHeader;
/*-------------------------------------*/
typedef struct
{
  unsigned char  jmpOpcode[2];
  BIOSHeader biosHdr;
      /* Note: At run-time biosHdr.lenMod512 contains size of entire DOC 2000
      boot area modulo 512 as set by DFORMAT  */
  Unaligned      runtimeID;        /* filled in by DFORMAT  */
  Unaligned      tffsHeapSize;     /* filled in by DFORMAT  */
  unsigned char  chksumFix;        /* changed by DFORMAT */
  unsigned char  version;
  unsigned char  subversion;
  char  copyright[35];    /* "SPL_for_DOC_2000 (c)1996 M-systems", 0 */
} SplHeader;
/*------------------------------------- */
typedef struct /* data structure representing TFFS header */
{
  BIOSHeader     biosHdr;
  unsigned char  jmpOpcode[3];
  char           tffsSignature[4];  /* "TFFS" */
  Unaligned      runtimeID;         /* passed by SPL............... */
  Unaligned      windowBase;        /* ..............and saved here */
  unsigned char  firstDiskNumber;   /* filled in............  */
  unsigned char  lastDiskNumber;    /* ..........at run-time */
  unsigned char  exbFlags;          /* filled in by writeExbDriverImage() */
  Unaligned      heapLen;           /* not used for now */
  Unaligned      versionNo;         /* filled in at run-time */
  unsigned char  chksumFix;         /* changed by writeExbDriverImage() */
} TffsHeader;
/* ------------------------------------- */
typedef struct
{
  BIOSHeader     biosHdr;
  unsigned char  jmpOpcode[3];
  char           tffsSignature[4];  /* "TFFS" */
  unsigned char  exbFlags;          /* filled in by writeExbDriverImage() */
  unsigned char  chksumFix;         /* changed by writeExbDriverImage() */
  unsigned char  heapLen;           /* not used for now */
  Unaligned      windowBase;        /* filled in at run-time */
} SSHeader;

#endif /* WRITE_EXB_IMAGE */
#ifdef __cplusplus
}
#endif
#endif /* _DFORMAT_H_ */