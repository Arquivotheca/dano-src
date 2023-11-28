//--------------------------------------------------------------------
//	Epson Printing System header
//	Written by: Mathias Agopian
//	
//	Copyright 2001 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef EPSON_PRINTING_SYSTEM_H
#define EPSON_PRINTING_SYSTEM_H

extern short sSEInit(void *pvParam);
extern short sSEOut(void *pvParam);
extern short sSEEnd(void *pvParam);


namespace SEPrivate
{

inline uint32 WIDTHBYTES(uint32 bits) {
	return (((bits) + 31) / 32 * 4);
}

typedef	short	(* LPSPOOLFUNC)(void *hParam, char *pBuf, long cbBuf);

typedef struct tagPOINT {
	long x;
	long y;
} POINT; 

typedef struct tagSE_INIT_PARAM {
	short sVersion;
	void **ppvGlobal;
	POINT SrcPaperSize;
	POINT SrcPrintArea;
	POINT SrcMargin;
	short sPaperSizeID;
	short sPrintingMode;
	void *pvPrintOutFunc;
	void *pvFuncParam;
} SE_INIT_PARAM;

typedef struct tagSE_OUT_PARAM {
	short sVersion;
	void *pvGlobal;
	long lWidthBytes;
	long lHeight;
	void *pvBits;
} SE_OUT_PARAM;

typedef struct tagSE_END_PARAM {
	short sVersion;
	void *pvGlobal;
	int bFlush;
} SE_END_PARAM;

} using namespace SEPrivate;

#endif

