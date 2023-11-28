#ifndef WINDOWS_H
#define WINDOWS_H

#include <OS.h>
#include <support/SupportDefs.h>
typedef void * PVOID;
#define PASCAL
typedef unsigned char BYTE;
typedef BYTE * PBYTE;
typedef unsigned short WORD;
typedef WORD * PWORD;
typedef struct
{
	unsigned short x;
	unsigned short y;
} POINT;
typedef uint32 DWORD;
typedef DWORD *PDWORD;

#endif
