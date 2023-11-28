#ifndef __BEOS_H__
#define __BEOS_H__

#include <stdio.h>

#define	BASED_SELF
typedef char 			TCHAR,*PTCHAR;
#define far
#define huge
typedef int DWORD;
typedef long LONG;
#define FAR
#define far
#define _far
#define NEAR
#define HUGE
#define FOURCC unsigned long

typedef LONG LPARAM;
typedef int BOOL;
typedef short WORD;
typedef LONG LRESULT;
typedef char WCHAR;
#define DECLARE_HANDLE(x) typedef WORD x
typedef void * LPVOID;
typedef DWORD *LPDWORD;
typedef unsigned int UINT;
typedef void * HPALETTE;
typedef void * HWND;
typedef void * HDC;
typedef void * HLOCAL;
typedef void * HMODULE;
typedef unsigned char BYTE;
typedef BYTE *LPBYTE;
typedef BYTE * PBYTE;
typedef UINT WPARAM;
typedef char * LPTSTR;

#define _MAX_PATH          260 
#define CALLBACK
#define PASCAL
#define __based(X)
#define GMEM_MOVEABLE       0x0002
#define GMEM_ZEROINIT       0x0040
#define FALSE 0
#ifndef TRUE
#define TRUE (!FALSE)
#endif
#define TEXT(a) a
#define LPTR 0x000001 //                (LMEM_FIXED | LMEM_ZEROINIT)
#define GHND 0x000002 //              (GMEM_MOVEABLE | GMEM_ZEROINIT)
#define LocalAlloc(X,Y) malloc(Y)
#define LocalFree(X) free(X)
#define GlobalAllocPtr(X,Y) malloc(Y)
#define GlobalFreePtr(X) free(X)
#define BI_RGB        0L
#define BI_BITFIELDS    3L
#ifndef min
#define min(X,Y) (((X)<(Y))?(X):(Y))
#endif
#ifndef max
#define max(X,Y) (((X)>(Y))?(X):(Y))
#endif
#define __inline inline
#define CopyMemory(X,Y,Z) memcpy(X,Y,Z)
#define MoveMemory(X,Y,Z) memmove(X,Y,Z)
#define _fmemcpy memcpy
typedef struct
{
	int x;
} *LPPALETTEENTRY;
typedef struct {
        DWORD      biSize;
        LONG       biWidth;
        LONG       biHeight;
        WORD       biPlanes;
        WORD       biBitCount;
        DWORD      biCompression;
        DWORD      biSizeImage;
        LONG       biXPelsPerMeter;
        LONG       biYPelsPerMeter;
        DWORD      biClrUsed;
        DWORD      biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER, *PBITMAPINFOHEADER;
typedef struct {
        BYTE    rgbBlue;
        BYTE    rgbGreen;
        BYTE    rgbRed;
        BYTE    rgbReserved;
} RGBQUAD, *LPRGBQUAD;
typedef struct
{
	int x;
} BITMAP;
typedef void * HBITMAP;
#include "compddk.h"

long MulDiv(long,long,long);

unsigned long _lrotr(unsigned long, int);
#endif
