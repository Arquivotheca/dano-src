�
�;S   �+         $�     *��:  ,��:  S   �+                �}l/   �                                                                                                                                J                      RTSC     errno.h SMIM	  BEOS:TYPE   text/x-source-code        �                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    �
�;S   �+         $�     -��:  /��:  S   �+                �a�/   �                                                                                                                                �                      RTSC     fcntl.h SMIM	  BEOS:TYPE   text/x-source-code        �                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    �
�;S   �+         $�     0��:  2��:  S   �+                �~�/   �                                                                                                                                �                      RTSC     ioctls.h SMIM	  BEOS:TYPE   text/x-source-code        �                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   �
�;S   �+         $�     3��:  5��:  S   �+                �D�/   �	                                                                                          $                                      �                       RTSC     libc-lock.h SMIM	  BEOS:TYPE   text/x-source-code        �                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                �
�;S   �+         $�     6��:  8��:  S   �+                �+Q/   �                                                                                                                                                      RTSC     local_lim.h SMIM	  BEOS:TYPE   text/x-source-code        �                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                �
�;S   �+         $�     9��:  ;��:  S   �+                 ��/   �                                                                                                                                D                      RTSC     posix_opt.h SMIM	  BEOS:TYPE   text/x-source-code        �                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                �
�;S   �+         $�     <��:  >��:  S   �+                ��/   �                                                                                                                                	                      RTSC     sigaction.h SMIM	  BEOS:TYPE   text/x-source-code        �                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                �
�;S   �+         $�     ?��:  A��:  S   �+                ��/   �                                                                                                                                7                      RTSC     sigcontext.h SMIM	  BEOS:TYPE   text/x-source-code        �                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               �
�;S   �+         $�     B��:  D��:  S   �+                P�v/   �                                                                                                                                
                      RTSC     signum.h SMIM	  BEOS:TYPE   text/x-source-code        �                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   �
�;S   �+         $�     E��:  G��:  S   �+                ��/   �                                                                                                                                �
                      RTSC     socket.h SMIM	  BEOS:TYPE   text/x-source-code        �                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   �
�;S   �+         $�     H��:  J��:  S   �+                `D/   �                                                                                                                                H                      RTSC     sockunion.h SMIM	  BEOS:TYPE   text/x-source-code        �                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                �
�;S   �+         $�     K��:  M��:  S   �+                ���/   �                                                                                                                                �                      RTSC     stat.h SMIM	  BEOS:TYPE   text/x-source-code        �                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     �
�;S   �+         $�     N��:  P��:  S   �+                �($/   �                                                                                                                                �                      RTSC     stdio-lock.h SMIM	  BEOS:TYPE   text/x-source-code        �                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               //--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#ifndef MPRINTER_H
#define MPRINTER_H


#include <string.h>

#include <SupportDefs.h>
#include <DataIO.h>

#include "MPrinterTypes.h"
#include "DriverTypes.h"
#include "MDefinePrinter.h"

class BEpson;

// -----------------------------------------------------
// class name: MPrinter
// purpose: Access to the printer at a command level
// -----------------------------------------------------
class MPrinter
{
public:
			MPrinter(BEpson& epson);
virtual		~MPrinter(void);
			BDataIO *GetPort(void)	{ return pPrinterPort->DataIO(); }

			// Initial Settings
			status_t	Init				(void);
			status_t	Desinit				(void);
			status_t	SelectGraphicMode	(void);
			status_t	SetUnit				(uint8 unit);
			status_t	SetPageLength		(uint16 height);
			status_t	SetPageFormat		(uint16 top, uint16 bot);
			status_t	SetPaperDimension	(uint32 length, uint32 width);

			// Printing method control
			status_t	UniDirectionnalMode	(uint8 on_off);
			status_t	MicroweaveMode		(uint8 on_off);
			status_t	SetDotSize			(uint8 mode);
			
			// Raster commands
			status_t	SetVertPosition		(uint32 NbLines);
			status_t	SelectColor			(t_printer_color RasterColor);
			status_t	SetHorPosition		(uint16 NbDots);
			status_t	Move1440			(uint16 NbDots);
			status_t	PrintRasterData		(char *pBuffer, uint16 NbDots, uint16 NbLines = 0);
			status_t	FormFeed			(void);
			status_t	LineFeed			(void);
			status_t	OutBuffers			(void);
			
			// The compression routine
			int			Compress			(char *inrow, char **pOutput, uint32 len);
		
private:
			status_t	InitJob				(void);
			status_t	InitPrinting		(void);
	inline	ssize_t		Write				(void *buffer, const size_t s);
	inline	status_t	SendCmd				(uint8 *pCmd);


	BEpson&			fEpson;
	BTransportIO	*pPrinterPort;
	bool			fHighResolution;
	bool			fSixColors;
	bool			fRasterImageMode;
	bool			f1440Dpi;
	int				fNbSlices;
	uint8			fDotSize;
	uint8			fNbBitPix;
	uint8			fColorOfRaster;
	char			*pTempRLE;
	long			sizeRLE;
};

ssize_t MPrinter::Write(void *buffer, size_t s)
{
	const ssize_t WriteLen = GetPort()->Write(buffer, s);
	return WriteLen;
}

status_t MPrinter::SendCmd(uint8 *pCmd)
{
	const ssize_t WriteLen = Write(pCmd+1, pCmd[0]);
	if (WriteLen<pCmd[0])
		return B_ERROR;
	return B_OK;
}




class MPrinterUtils
{
public:
			MPrinterUtils(BTransportIO *stream, MDefinePrinter *printerDev, tPrinterDef *printerDef, bool enforceStatusRequest = true);
	virtual ~MPrinterUtils();
	
	// Utilities REMOTE1 commands
	status_t	EnterRemote();
	status_t	ExitRemote();
	status_t	CleanNozzles();
	status_t	CheckNozzles();
	status_t	CheckAlign();
	status_t	UserSelectAlign(int pattern);
	status_t	ModifyAlign(int pattern, int value);
	status_t	SaveSettings();
	status_t 	GetStatus(BMessage& status_message);
	status_t	ConvertToStatus(const BMessage& msg, printer_status_t *status);

private:
	inline BDataIO *GetPort(void)		{	return Transport()->DataIO(); }
	inline MDefinePrinter *PrinterDev()	{	return fPrinterDev; }
	inline tPrinterDef *PrinterDef()	{	return fPrinterDef; }
	inline BTransportIO *Transport()	{	return fTransport; }

	inline ssize_t Write(void *buffer, const size_t s)
	{
		const ssize_t WriteLen = GetPort()->Write(buffer, s);
		return WriteLen;
	}

	inline status_t SendCmd(uint8 *pCmd)
	{
		const ssize_t WriteLen = Write(pCmd+1, pCmd[0]);
		if (WriteLen < pCmd[0])
			return B_ERROR;
		return B_OK;
	}

	MDefinePrinter *fPrinterDev;
	tPrinterDef *fPrinterDef;
	BTransportIO *fTransport;

	bool fUsbEnabled;
	bool fStatusReportEnabled;
};

#endif
                                                                                                                                                                                                                                                                                                                                                                                             //*******************************************************************************************
// Project: Driver EPSON Stylus
// File: MPrinterTypes.h
//
// Purpose: Contient les defines dependant de l'imrpimante
//
// Author: M. AGOPIAN
// Last Modifications:
// ___________________________________________________________________________________________
// Author		| Date		| Purpose
// ____________	|__________	|_________________________________________________________________
// M.AG.		| 01/11/97	| Debut du projet
// ____________	|__________	|_________________________________________________________________
//********************************************************************************************

#ifndef MPRINTER_TYPES_H
#define MPRINTER_TYPES_H


// Defined classes
class MPrinter;

// Size of a normal dot, used as unit for the tables (inch)
#define DOT_SIZE_UNIT				(1.0/360.0)

// t_printer_color: couleur du raster
typedef enum
{
	M_BLACK_COLOR			=	0,
	M_MAGENTA_COLOR			=	1,
	M_CYAN_COLOR			=	2,
	M_YELLOW_COLOR			=	4,
	M_LIGHT_MAGENTA_COLOR	=	0x101,
	M_LIGHT_CYAN_COLOR		=	0x102
}t_printer_color;

// Definit le numero de version minimum des fichiers d'imprimantes
// VERSION_MIN: pas d'importance (changement de la valeur des parametres de l'imprimante)
// VERSION_MID: celui de l'imprimante doit etre == a celui-ci
// VERSION_MAJ: celui de l'imprimante doit etre == a celui-ci

#define	VERSION_MAJ		5
#define VERSION_MID		8
#define VERSION_MIN		0

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          #ifndef TCOLOR4_H
#define TCOLOR4_H

#include <SupportDefs.h>

class t_fcolor_cmyk
{
public:
	t_fcolor_cmyk() { }
	t_fcolor_cmyk(int32 f)	: cyan(f), magenta(f), yellow(f), black(f) { }

	static void InitMMX(void) { }
	static void ExitMMX(void) { }

	void error(const t_fcolor_cmyk& error, const int32 r, const int fact)
	{
		cyan	= (error.cyan + ((r*error.cyan)>>14)) >> fact;
		magenta	= (error.magenta + ((r*error.magenta)>>14)) >> fact;
		yellow	= (error.yellow + ((r*error.yellow)>>14)) >> fact;
		black	= (error.black + ((r*error.black)>>14)) >> fact;
	}

	void error(const t_fcolor_cmyk& error, const int fact)
	{
		cyan	= error.cyan >> fact;
		magenta	= error.magenta >> fact;
		yellow	= error.yellow >> fact;
		black	= error.black >> fact;
	}

	void prev(const t_fcolor_cmyk& error, const t_fcolor_cmyk& next)
	{
		cyan		= error.cyan	+ next.cyan;
		magenta		= error.magenta	+ next.magenta;
		yellow		= error.yellow	+ next.yellow;
		black		= error.black	+ next.black;
	}
	
	t_fcolor_cmyk& operator += (const t_fcolor_cmyk& c)
	{
		cyan		+= c.cyan;
		magenta		+= c.magenta;
		yellow		+= c.yellow;
		black		+= c.black;
		return *this;
	}

	t_fcolor_cmyk& operator -= (const t_fcolor_cmyk& c)
	{
		cyan		-= c.cyan;
		magenta		-= c.magenta;
		yellow		-= c.yellow;
		black		-= c.black;
		return *this;
	}

	void dither(	const t_fcolor_cmyk& prev_error,
					uint16 *pixel,
					int16 *f)
	{
		cyan		= prev_error.cyan		+ f[0];
		magenta		= prev_error.magenta	+ f[1];
		yellow		= prev_error.yellow		+ f[2];
		black		= prev_error.black		+ f[3];
		do_dither(pixel);
	}
	
	void dither(	const t_fcolor_cmyk& prev_error,
					uint16 *pixel)
	{
		*this = prev_error;
		do_dither(pixel);
	}

protected:

	inline void do_dither(uint16 * pixel)
	{
		if (cyan		>= 0x1FFF)	{ cyan		-= 0x3FFF;	pixel[0] = 1; } else { pixel[0] = 0; }
		if (magenta		>= 0x1FFF)	{ magenta	-= 0x3FFF;	pixel[1] = 1; } else { pixel[1] = 0; }
		if (yellow		>= 0x1FFF)	{ yellow	-= 0x3FFF;	pixel[2] = 1; } else { pixel[2] = 0; }
		if (black		>= 0x1FFF)	{ black		-= 0x3FFF;	pixel[3] = 1; } else { pixel[3] = 0; }
	}

	int32	cyan;
	int32	magenta;
	int32	yellow;
	int32	black;
};


#endif

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                            #ifndef TCOLOR4_2_H
#define TCOLOR4_2_H

#include <SupportDefs.h>
#include <Tcolor4.h>

class t_fcolor_cmyk_2 : public t_fcolor_cmyk
{
public:
	t_fcolor_cmyk_2() : t_fcolor_cmyk()			{ }
	t_fcolor_cmyk_2(int32 f) : t_fcolor_cmyk(f)	{ }

	void dither(	const t_fcolor_cmyk_2& prev_error,
					uint16 *pixel,
					int16 *f)
	{
		cyan		= prev_error.cyan		+ f[0];
		magenta		= prev_error.magenta	+ f[1];
		yellow		= prev_error.yellow		+ f[2];
		black		= prev_error.black		+ f[3];
		do_dither(pixel);
	}
	
	void dither(	const t_fcolor_cmyk_2& prev_error,
					uint16 *pixel)
	{
		*this = prev_error;
		do_dither(pixel);
	}
	
protected:

	inline void do_dither(uint16 *pixel)
	{
		if (cyan >= 0x0800)
		{
			if (cyan < 0x1800)			{	pixel[0] = 1;	cyan -= 0x1000; }
			else if (cyan < 0x3000)		{	pixel[0] = 2;	cyan -= 0x2000; }
			else						{	pixel[0] = 3;	cyan -= 0x4000; }
		} else pixel[0] = 0;

		if (magenta >= 0x0800)
		{
			if (magenta < 0x1800)		{	pixel[1] = 1;	magenta -= 0x1000; }
			else if (magenta < 0x3000)	{	pixel[1] = 2;	magenta -= 0x2000; }
			else						{	pixel[1] = 3;	magenta -= 0x4000; }
		} else pixel[1] = 0;

		if (yellow >= 0x0800)
		{
			if (yellow < 0x1800)		{	pixel[2] = 1;	yellow -= 0x1000; }
			else if (yellow < 0x3000)	{	pixel[2] = 2;	yellow -= 0x2000; }
			else						{	pixel[2] = 3;	yellow -= 0x4000; }
		} else pixel[2] = 0;

		if (black >= 0x0800)
		{
			if (black < 0x1800)			{	pixel[3] = 1;	black -= 0x1000; }
			else if (black < 0x3000)	{	pixel[3] = 2;	black -= 0x2000; }
			else						{	pixel[3] = 3;	black -= 0x4000; }
		} else pixel[3] = 0;
	}
};


#endif

                                                                                                                                                                                                                                                                                                                                                                                                                                                                               #ifdef __INTEL__

#ifndef TCOLOR4_2_MMX_H
#define TCOLOR4_2_MMX_H

#include <SupportDefs.h>
#include <Tcolor4_mmx.h>

class t_fcolor_cmyk_2_mmx : public t_fcolor_cmyk_mmx
{
public:
	t_fcolor_cmyk_2_mmx() : t_fcolor_cmyk_mmx()			{ }
	t_fcolor_cmyk_2_mmx(int32 f) : t_fcolor_cmyk_mmx(f)	{ }

	void dither(	const t_fcolor_cmyk_2_mmx& prev_error,
					uint16 *pixel,
					int16 *f)
	{
		asm volatile
		(
			"movq		 (%1), %%mm0 \n\t"
			"paddsw		 (%2), %%mm0 \n\t"
			"movq		%%mm0,  (%0) \n\t"
			:
			: "r"	((int16*)mmx_wrapper),				// %0
			  "r"	((int16*)f),						// %1
			  "r"	((int16*)prev_error.mmx_wrapper)	// %2
		);
		do_dither(pixel);
	}
	
	void dither(	const t_fcolor_cmyk_2_mmx& prev_error,
					uint16 *pixel)
	{
		asm volatile
		(
			"movq		 (%1), %%mm0 \n\t"
			"movq		%%mm0,  (%0) \n\t"
			:
			: "r"	((int16*)mmx_wrapper),				// %0
			  "r"	((int16*)prev_error.mmx_wrapper)	// %1
		);
		do_dither(pixel);
	}
	
protected:

	inline void do_dither(uint16 *pixel)
	{
		static const int16 data[6][4] = {
			{ 0x07ff, 0x07ff, 0x07ff, 0x07ff },			// 0
			{ 0x1800, 0x1800, 0x1800, 0x1800 },			// 8
			{ 0x3000, 0x3000, 0x3000, 0x3000 },			// 16
			{ 1,1,1,1 },								// 24
			{ -0x1000, -0x1000, -0x1000, -0x1000 },		// 32
			{ -0x2000, -0x2000, -0x2000, -0x2000 } };	// 40
		
		asm volatile
		(
			"movq	(%0), %%mm0 \n\t"				// Load source (Already in mm0?)
			"movq	%%mm0, %%mm6 \n\t"				// Copy to output
			"pxor	%%mm7, %%mm7 \n\t"				// Clear pixel out
			"movq	24(%2), %%mm2 \n\t"				// Preload pxiel 1s
			
			"movq	%%mm0, %%mm1 \n\t"				// dup source
			"pcmpgtw (%2), %%mm1 \n\t"				// create test mask
			"movq	%%mm2, %%mm3 \n\t"
			"movq	32(%2), %%mm4 \n\t"
			"pand	%%mm1, %%mm3 \n\t"
			"pand	%%mm1, %%mm4 \n\t"
			"paddsw %%mm3, %%mm7 \n\t"
			"paddsw %%mm4, %%mm6 \n\t"

			"movq	%%mm0, %%mm1 \n\t"				// dup source
			"pcmpgtw 8(%2), %%mm1 \n\t"				// create test mask
			"movq	%%mm2, %%mm3 \n\t"
			"movq	32(%2), %%mm4 \n\t"
			"pand	%%mm1, %%mm3 \n\t"
			"pand	%%mm1, %%mm4 \n\t"
			"paddsw %%mm3, %%mm7 \n\t"
			"paddsw %%mm4, %%mm6 \n\t"

			"movq	%%mm0, %%mm1 \n\t"				// dup source
			"pcmpgtw 16(%2), %%mm1 \n\t"			// create test mask
			"movq	%%mm2, %%mm3 \n\t"
			"movq	40(%2), %%mm4 \n\t"
			"pand	%%mm1, %%mm3 \n\t"
			"pand	%%mm1, %%mm4 \n\t"
			"paddsw %%mm3, %%mm7 \n\t"
			"paddsw %%mm4, %%mm6 \n\t"
			
			"movq	%%mm7, (%1) \n\t"
			"movq	%%mm6, (%0) \n\t"
			:
			: "r"	((int16*)mmx_wrapper),				// %0
			  "r"	((int16*)pixel),					// %1
			  "r"	((int16*)&data[0][0])				// %2
		);
	}
};


#endif

#endif

                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             #ifdef __INTEL__

#ifndef TCOLOR4_MMX_H
#define TCOLOR4_MMX_H

#include <SupportDefs.h>

class t_fcolor_cmyk_mmx
{
public:
	t_fcolor_cmyk_mmx()
	{
	}

	t_fcolor_cmyk_mmx(int32 v)
	{
		const int32 v32 = ((v<<16)|v);
		((int32 *)mmx_wrapper)[0] = v32;
		((int32 *)mmx_wrapper)[1] = v32;
	}

	static void InitMMX(void)
	{
		const uint32 mmx6[] ALLIGN(8) = {0x1FFF1FFF, 0x1FFF1FFF};
		const uint32 mmx7[] ALLIGN(8) = {0x3FFF3FFF, 0x3FFF3FFF};
		asm
		(
			"movq	 (%0), %%mm6	\n\t"
			"movq	 (%1), %%mm7	\n\t"
			:
			: "r" ((uint64 *)mmx6),	// %0
			  "r" ((uint64 *)mmx7)	// %1
		);
	}

	static void ExitMMX(void)
	{
		asm("emms\n\t");
	}

	void error(const t_fcolor_cmyk_mmx& error, const int32 r, const int fact)
	{
		asm volatile
		(
			"shll	$2, %3 \n\t"
			"movd	%3, %%mm0 \n\t"
			"movd	%3, %%mm4 \n\t"
			"psllq	$32, %%mm0 \n\t"
			"por	%%mm0, %%mm4 \n\t"
			"movq	%%mm4, %%mm0 \n\t"
			"psllq	$16, %%mm4 \n\t"
			"por	%%mm0, %%mm4 \n\t"		// mm4 = r,r,r,r  <<2
			"movq	 (%1), %%mm0	\n\t"
			"pmulhw	%%mm0, %%mm4	\n\t"			
			"movd	 (%2), %%mm5	\n\t"
			"paddsw	%%mm4, %%mm0	\n\t"
			"psraw	%%mm5, %%mm0	\n\t"
			"movq	%%mm0, (%0)		\n\t"
			:
			: "r"	((int16*)mmx_wrapper),			// %0
			  "r"	((int16*)error.mmx_wrapper),	// %1
			  "r"	(&fact),						// %2
			  "r"	(r)								// %3
		);
	}

	void error(const t_fcolor_cmyk_mmx& error, const int fact)
	{
		asm volatile
		(
			"movd	 (%2), %%mm5	\n\t"
			"movq	 (%1), %%mm0	\n\t"
			"psraw	%%mm5, %%mm0	\n\t"
			"movq	%%mm0, (%0)		\n\t"
			:
			: "r"	((int16*)mmx_wrapper),			// %0
			  "r"	((int16*)error.mmx_wrapper),	// %1
			  "r"	(&fact)							// %2
		);
	}

	void prev(const t_fcolor_cmyk_mmx& error, const t_fcolor_cmyk_mmx& next)
	{
		// *this = error + next
		asm volatile
		(
			"movq		(%2), %%mm0 \n\t"
			"paddsw		(%1), %%mm0 \n\t"
			"movq		%%mm0, (%0) \n\t"
			:
			: "r"	((int16*)mmx_wrapper),			// %0
			  "r"	((int16*)error.mmx_wrapper),	// %1
			  "r"	((int16*)next.mmx_wrapper)		// %2
		);
	}
	
	t_fcolor_cmyk_mmx& operator += (const t_fcolor_cmyk_mmx& c)
	{
		// *this += c
		asm volatile
		(
			"movq		(%1), %%mm0 \n\t"
			"paddsw		(%0), %%mm0 \n\t"
			"movq		%%mm0, (%0) \n\t"
			:
			: "r"	((int16*)mmx_wrapper),			// %0
			  "r"	((int16*)c.mmx_wrapper)			// %1
		);
		return *this;
	}

	t_fcolor_cmyk_mmx& operator -= (const t_fcolor_cmyk_mmx& c)
	{
		// *this -= c
		asm volatile
		(
			"movq		(%0), %%mm0 \n\t"
			"psubsw		(%1), %%mm0 \n\t"
			"movq		%%mm0, (%0) \n\t"
			:
			: "r"	((int16*)mmx_wrapper),			// %0
			  "r"	((int16*)c.mmx_wrapper)			// %1
		);
		return *this;
	}

	void dither(	const t_fcolor_cmyk_mmx& prev_error,
					uint16 *pixel_data,
					int16 *f)
	{
		// codage d'une couleur : s001 1111 1111 1111
		// maximum d'erreur     : s111 1111 1111 1111
		asm volatile
		(
			"movq		 (%1), %%mm0 \n\t"
			"paddsw		 (%2), %%mm0 \n\t"
			"movq		%%mm0, %%mm1 \n\t"
			"pcmpgtw	%%mm6, %%mm1 \n\t"
			"pand		%%mm7, %%mm1 \n\t"
			"psubsw		%%mm1, %%mm0 \n\t"
			"movq		%%mm1,  (%3) \n\t"
			"movq		%%mm0,  (%0) \n\t"
			:
			: "r"	((int16*)mmx_wrapper),				// %0
			  "r"	((int16*)f),						// %1
			  "r"	((int16*)prev_error.mmx_wrapper),	// %2
			  "r"	((int16*)pixel_data)				// %3
		);
	}
	
	void dither(	const t_fcolor_cmyk_mmx& prev_error,
					uint16 *pixel_data)
	{
		asm volatile
		(
			"movq		 (%1), %%mm0 \n\t"
			"movq		%%mm0, %%mm1 \n\t"
			"pcmpgtw	%%mm6, %%mm1 \n\t"
			"pand		%%mm7, %%mm1 \n\t"
			"psubsw		%%mm1, %%mm0 \n\t"
			"movq		%%mm1,  (%2) \n\t"
			"movq		%%mm0,  (%0) \n\t"
			:
			: "r"	((int16*)mmx_wrapper),				// %0
			  "r"	((int16*)prev_error.mmx_wrapper),	// %1
			  "r"	((int16*)pixel_data)				// %2
		);
	}

protected:
	int16	mmx_wrapper[4];
};


#endif

#endif

                                                                                                                                                                                                                                                                                                                                                                                                                           #ifndef TCOLOR6_H
#define TCOLOR6_H

#include <SupportDefs.h>

class t_fcolor_cmyklclm
{
public:
	t_fcolor_cmyklclm()
	{
	}

	t_fcolor_cmyklclm(int32 f)
			: cyan(f), magenta(f), yellow(f), black(f), light_cyan(f), light_mag(f)
	{
	}

	static void InitMMX(void) { }
	static void ExitMMX(void) { }
	
	void error(const t_fcolor_cmyklclm& error, const int32 r, const int fact)
	{
		cyan	= (error.cyan + ((r*error.cyan)>>14)) >> fact;
		magenta	= (error.magenta + ((r*error.magenta)>>14)) >> fact;
		yellow	= (error.yellow + ((r*error.yellow)>>14)) >> fact;
		black	= (error.black + ((r*error.black)>>14)) >> fact;
		light_cyan	= (error.light_cyan + ((r*error.light_cyan)>>14)) >> fact;
		light_mag	= (error.light_mag + ((r*error.light_mag)>>14)) >> fact;
	}

	void error(const t_fcolor_cmyklclm& error, const int fact)
	{
		cyan	= error.cyan >> fact;
		magenta	= error.magenta >> fact;
		yellow	= error.yellow >> fact;
		black	= error.black >> fact;
		light_cyan	= error.light_cyan >> fact;
		light_mag	= error.light_mag >> fact;
	}

	void prev(const t_fcolor_cmyklclm& error, const t_fcolor_cmyklclm& next)
	{
		cyan		= error.cyan		+ next.cyan;
		magenta		= error.magenta		+ next.magenta;
		yellow		= error.yellow		+ next.yellow;
		black		= error.black		+ next.black;
		light_cyan	= error.light_cyan	+ next.light_cyan;
		light_mag	= error.light_mag	+ next.light_mag;
	}
	
	t_fcolor_cmyklclm& operator += (const t_fcolor_cmyklclm& c)
	{
		cyan		+= c.cyan;
		magenta		+= c.magenta;
		yellow		+= c.yellow;
		black		+= c.black;
		light_cyan	+= c.light_cyan;
		light_mag	+= c.light_mag;
		return *this;
	}

	t_fcolor_cmyklclm& operator -= (const t_fcolor_cmyklclm& c)
	{
		cyan		-= c.cyan;
		magenta		-= c.magenta;
		yellow		-= c.yellow;
		black		-= c.black;
		light_cyan	-= c.light_cyan;
		light_mag	-= c.light_mag;
		return *this;
	}

	void dither(	const t_fcolor_cmyklclm& prev_error,
					uint16 *pixel,
					int16 *f)
	{
		cyan		= prev_error.cyan		+ f[0];
		magenta		= prev_error.magenta	+ f[1];
		yellow		= prev_error.yellow		+ f[2];
		black		= prev_error.black		+ f[3];
		light_cyan	= prev_error.light_cyan + f[4];
		light_mag	= prev_error.light_mag	+ f[5];
		do_dither(pixel);
	}
	
	void dither(	const t_fcolor_cmyklclm& prev_error,
					uint16 *pixel)
	{
		*this = prev_error;
		do_dither(pixel);
	}

protected:
	inline void do_dither(uint16 *pixel)
	{
		if (cyan		>= 0x1FFF)	{ cyan		-= 0x3FFF;	pixel[0] = 1; } else { pixel[0] = 0; }
		if (magenta		>= 0x1FFF)	{ magenta	-= 0x3FFF;	pixel[1] = 1; } else { pixel[1] = 0; }
		if (yellow		>= 0x1FFF)	{ yellow	-= 0x3FFF;	pixel[2] = 1; } else { pixel[2] = 0; }
		if (black		>= 0x1FFF)	{ black		-= 0x3FFF;	pixel[3] = 1; } else { pixel[3] = 0; }
		if (light_cyan	>= 0x1FFF)	{ light_cyan-= 0x3FFF;	pixel[4] = 1; } else { pixel[4] = 0; }
		if (light_mag	>= 0x1FFF)	{ light_mag	-= 0x3FFF;	pixel[5] = 1; } else { pixel[5] = 0; }
	}

	int32	cyan;
	int32	magenta;
	int32	yellow;
	int32	black;
	int32	light_cyan;
	int32	light_mag;
};

#endif
                                                                                              #ifndef TCOLOR6_2_H
#define TCOLOR6_2_H

#include <SupportDefs.h>
#include "Tcolor6.h"

class t_fcolor_cmyklclm_2 : public t_fcolor_cmyklclm
{
public:
	t_fcolor_cmyklclm_2() : t_fcolor_cmyklclm()
	{
	}

	t_fcolor_cmyklclm_2(int32 f) : t_fcolor_cmyklclm(f)
	{
	}

	void dither(	const t_fcolor_cmyklclm_2& prev_error,
					uint16 *pixel,
					int16 *f)
	{
		cyan		= prev_error.cyan		+ f[0];
		magenta		= prev_error.magenta	+ f[1];
		yellow		= prev_error.yellow		+ f[2];
		black		= prev_error.black		+ f[3];
		light_cyan	= prev_error.light_cyan + f[4];
		light_mag	= prev_error.light_mag	+ f[5];	
		do_dither(pixel);
	}
	
	void dither(	const t_fcolor_cmyklclm_2& prev_error,
					uint16 *pixel)
	{
		*this = prev_error;
		do_dither(pixel);
	}

protected:

	inline void do_dither(uint16 *pixel)
	{
		if (cyan >= 0x0800)
		{
			if (cyan < 0x1800)			{	pixel[0] = 1;	cyan -= 0x1000; }
			else if (cyan < 0x3000)		{	pixel[0] = 2;	cyan -= 0x2000; }
			else						{	pixel[0] = 3;	cyan -= 0x4000; }
		} else pixel[0] = 0;

		if (magenta >= 0x0800)
		{
			if (magenta < 0x1800)		{	pixel[1] = 1;	magenta -= 0x1000; }
			else if (magenta < 0x3000)	{	pixel[1] = 2;	magenta -= 0x2000; }
			else						{	pixel[1] = 3;	magenta -= 0x4000; }
		} else pixel[1] = 0;

		if (yellow >= 0x0800)
		{
			if (yellow < 0x1800)		{	pixel[2] = 1;	yellow -= 0x1000; }
			else if (yellow < 0x3000)	{	pixel[2] = 2;	yellow -= 0x2000; }
			else						{	pixel[2] = 3;	yellow -= 0x4000; }
		} else pixel[2] = 0;

		if (black >= 0x0800)
		{
			if (black < 0x1800)			{	pixel[3] = 1;	black -= 0x1000; }
			else if (black < 0x3000)	{	pixel[3] = 2;	black -= 0x2000; }
			else						{	pixel[3] = 3;	black -= 0x4000; }
		} else pixel[3] = 0;

		if (light_cyan >= 0x0800)
		{
			if (light_cyan < 0x1800)		{	pixel[4] = 1;	light_cyan -= 0x1000; }
			else if (light_cyan < 0x3000)	{	pixel[4] = 2;	light_cyan -= 0x2000; }
			else							{	pixel[4] = 3;	light_cyan -= 0x4000; }
		} else pixel[4] = 0;

		if (light_mag >= 0x0800)
		{
			if (light_mag < 0x1800)			{	pixel[5] = 1;	light_mag -= 0x1000; }
			else if (light_mag < 0x3000)	{	pixel[5] = 2;	light_mag -= 0x2000; }
			else							{	pixel[5] = 3;	light_mag -= 0x4000; }
		} else pixel[5] = 0;
	}
};

#endif
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          