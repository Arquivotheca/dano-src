//--------------------------------------------------------------------
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

class BEpsonCommon;

// -----------------------------------------------------
// class name: MPrinter
// purpose: Access to the printer at a command level
// -----------------------------------------------------
class MPrinter
{
public:
			MPrinter(BEpsonCommon& epson);
virtual		~MPrinter(void);
			BDataIO *GetPort(void)	{ return pPrinterPort->DataIO(); }

			// Initial Settings
			status_t	SmallInit			(void);
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


	BEpsonCommon&		fEpson;
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
			return (status_t)((WriteLen<0) ? WriteLen : B_ERROR);
		return B_OK;
	}

	MDefinePrinter *fPrinterDev;
	tPrinterDef *fPrinterDef;
	BTransportIO *fTransport;

	bool fUsbEnabled;
	bool fStatusReportEnabled;
};

#endif
