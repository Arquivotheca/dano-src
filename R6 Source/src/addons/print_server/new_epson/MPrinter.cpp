//--------------------------------------------------------------------
//
//	Written by: Mathias Agopian
//	
//	Copyright 2000 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>

#include <BeBuild.h>
#include <SupportDefs.h>
#include <Errors.h>
#include <Message.h>

#include "Driver.h"
#include "MPrinter.h"

// Printer Commands in the format:
// Length of the command, ...

// ESC/P2 commands
uint8 cmdInit[]					=	{4,	ESC,'@', ESC,'@'};
uint8 cmdDesinit[]				=	{4,	ESC,'@', ESC,'@'};
uint8 cmdControlPaper[]			=	{3,	ESC, 0x19, '1'};
uint8 cmdSelectGraphicMode[]	=	{6,	ESC,'(','G',1,0,1};
uint8 cmdSetUnit[]				=	{6,	ESC,'(','U',1,0,10};
uint8 cmdSetPageLength[]		= 	{7,	ESC,'(','C',0,2,0,0};
uint8 cmdSetPageFormat[]		=	{9,	ESC,'(','c',4,0,0,0,0,0};
uint8 cmdMicroweaveMode[]		=	{6,	ESC,'(','i',1,0,0};
uint8 cmdSetDotSize[]			=	{7,	ESC,'(','e',2,0,0,0};
uint8 cmdSetVertPosition[]		=	{7,	ESC,'(','v',2,0,1,0};
uint8 cmdExSelectColor[]		=	{7,	ESC,'(','r',2,0,0,0};
uint8 cmdSetRelHor1440Pos[]		=	{9, ESC,'(','\\',4,0,160,5,0,0};
uint8 cmdSetPrintingSpeed[]		=	{6, ESC,'(','s',1,0,0};
uint8 cmdPrintRasterData[]		=	{8,	ESC,'.',1,10,10,1,0,0};
uint8 cmdUniDirectionnalMode[]	=	{3,	ESC,'U',0};
uint8 cmdSelectColor[]			=	{3,	ESC,'r',0};
uint8 cmdSetHorPosition[]		=	{4,	ESC,'$',0,0};
uint8 cmdFormFeed[]				=	{1,	FF};
uint8 cmdLineFeed[]				=	{1,	CR};
uint8 cmdLineFeedAlpha[]		=	{1,	LF};
uint8 cmdOutBuffers[]			=	{2, ESC, 06};

uint8 cmdTransferRasterImage[]		=	{9, ESC, 'i', 0, 1, 1, 0,0, 0,0};
uint8 cmdSetResolutionOfRaster[]	=	{9, ESC, '(','D', 4,0, 0x40, 0x38, 0, 0};
uint8 cmdSetUnitExtended[]			=	{10,ESC, '(','U', 5,0, 1,1,1, 0xA0, 0x05};
uint8 cmdSetVertPositionExtended[]	=	{9,	ESC, '(','v',4,0, 0,0,0,0};
uint8 cmdSetHorPositionExtended[]	=	{9,	ESC, '(','$',4,0, 0,0,0,0};
uint8 cmdSetPaperDimensionExtended[]=	{13,ESC, '(','S',8,0, 0,0,0,0, 0,0,0,0};

// EPSON Remote1 commands
uint8 cmdEnterRemote[]			=	{13, ESC, '(','R',8,0,0,'R','E','M','O','T','E','1'};
uint8 cmdExitRemote[]			=	{4, ESC, 0,0,0 };
uint8 cmdHandleStatus[]			=	{6, 'S','T',2,0, 0,1};
uint8 cmdPaperFeed[]			=	{7, 'S','N',3,0, 0,0,0};
uint8 cmdCheckNozzles[]			=	{6, 'N','C',2,0, 0,0};
uint8 cmdCleanNozzles[]			=	{6, 'C','H',2,0, 0,0};
uint8 cmdVI[]					=	{6, 'V','I',2,0, 0,0};
uint8 cmdDotCheck[]				=	{6, 'D','C',2,0, 0,1};
uint8 cmdDotAlignment[]			=	{8, 'D','A',4,0, 0,1,0,1};
uint8 cmdDotTest[]				=	{7, 'D','T',3,0, 0,1,0};
uint8 cmdSaveSettings[]			=	{4, 'S','V',0,0};

// New commands for the 870
uint8 cmdRollPaper[]			=	{10,'E','X',6,0, 0,0,0,0,5,0};	// Turn roll paper mode on/off last byte is the parameter (00=off, 01=on)
uint8 cmdInkRemainingCounter[]	=	{6, 'I','R',2,0, 0,0};			// last byte is the parameter (00=stop, 01=start, 02=automatic counter stop, 03=automatic counter start, 04-FF=reserved)
uint8 cmdHorPosition[]			= 	{7, 'F','P',3,0, 0,0,0};		// last 2 bytes are parameters (00 00=standard, other values for other settings)

// Specials Commands
uint8 cmdEnableUSB[]			=	{27,	0, 0, 0, ESC, 1,
											'@','E','J','L',' ','1','2','8','4','.','4',0x0A,
											'@','E','J','L',' ',' ',' ',' ',' ',0x0A};

#define M_IS_PHOTO(_x)		(	((_x)==ID_STYLUS_PHOTO_750) ||		\
								((_x)==ID_STYLUS_PHOTO_PM770C) ||	\
								((_x)==ID_STYLUS_PHOTO) ||			\
								((_x)==ID_STYLUS_PHOTO_700) ||		\
								((_x)==ID_STYLUS_PHOTO_870) ||		\
								((_x)==ID_STYLUS_PHOTO_780) ||		\
								((_x)==ID_STYLUS_PHOTO_790) ||		\
								((_x)==ID_STYLUS_PHOTO_1270) ||		\
								((_x)==ID_STYLUS_PHOTO_2000P) ||	\
								((_x)==ID_STYLUS_PHOTO_EX)			\
							) 


#define M_IS_USB(_x)		(	((_x)==ID_STYLUS_PHOTO_750) ||		\
								((_x)==ID_STYLUS_PHOTO_PM770C) || 	\
								((_x)==ID_STYLUS_PHOTO_870) ||		\
								((_x)==ID_STYLUS_PHOTO_780) ||		\
								((_x)==ID_STYLUS_PHOTO_790) ||		\
								((_x)==ID_STYLUS_PHOTO_1270) ||		\
								((_x)==ID_STYLUS_PHOTO_2000P) ||	\
								((_x)==ID_STYLUS_670) || 			\
								((_x)==ID_STYLUS_760) || 			\
								((_x)==ID_STYLUS_777) || 			\
								((_x)==ID_STYLUS_880) || 			\
								((_x)==ID_STYLUS_1160) || 			\
								((_x)==ID_STYLUS_740) ||			\
								((_x)==ID_STYLUS_900) ||			\
								((_x)==ID_STYLUS_980)				\
							)

#define M_IS_MULTIDOT(_x)	(	((_x)==ID_STYLUS_PHOTO_750) || 		\
								((_x)==ID_STYLUS_PHOTO_PM770C) || 	\
								((_x)==ID_STYLUS_PHOTO_870) ||		\
								((_x)==ID_STYLUS_PHOTO_780) ||		\
								((_x)==ID_STYLUS_PHOTO_790) ||		\
								((_x)==ID_STYLUS_PHOTO_1270) ||		\
								((_x)==ID_STYLUS_PHOTO_2000P) ||	\
								((_x)==ID_STYLUS_740) || 			\
								((_x)==ID_STYLUS_670) || 			\
								((_x)==ID_STYLUS_760) || 			\
								((_x)==ID_STYLUS_777) || 			\
								((_x)==ID_STYLUS_880) || 			\
								((_x)==ID_STYLUS_1160) || 			\
								((_x)==ID_STYLUS_900) ||			\
								((_x)==ID_STYLUS_980)				\
							)

#define M_IS_SMART_IC(_x)	(	((_x)==ID_STYLUS_777) || 			\
								((_x)==ID_STYLUS_PHOTO_870) ||		\
								((_x)==ID_STYLUS_PHOTO_2000P) ||	\
								((_x)==ID_STYLUS_PHOTO_1270)		\
							)


// -----------------------------------------------------
// class name: MPrinter
// purpose: Access to the printer at a command level
// -----------------------------------------------------
MPrinter::MPrinter(BEpsonCommon& epson)
			:	fEpson(epson),
				pPrinterPort(fEpson.Transport()),
				fHighResolution(false),
				fSixColors(fEpson.PrinterDef().color_mode == 6),
				fRasterImageMode(false),
				f1440Dpi(false),
				fNbSlices(1),
				fDotSize(fEpson.PrinterDef().microdot),
				fNbBitPix(1),
				pTempRLE(NULL),
				sizeRLE(2 * ((fEpson.PrinterDef().printer_width + 7)/8 + 3) & 0xFFFFFFFC)
{
	if (fEpson.PrinterDev()->IsEpsonModule())
	{
		// In this case we don't initialize what's not needed
	}
	else
	{
		if (fEpson.PrinterDef().soft_microweave == 2)
		{ // Super Microweave mode, we do the microweaving in software
			sizeRLE *= fEpson.PrinterDef().printer_info.gNbColorNozzle;
			fHighResolution = true;
		}
	
		fRasterImageMode = M_IS_MULTIDOT(fEpson.PrinterDef().printer_info.gID);	// New raster mode supported
		if (fRasterImageMode == true)
		{ // adjust the RLE buffer to the pixel bit-size
			fNbBitPix = ((fDotSize >> 4) & 0x01)+1;
			sizeRLE *= fNbBitPix;
		}
	
		// 1440 horizontal is not supported by the printer
		f1440Dpi = (fEpson.Settings().DeviceXdpi() == 1440);
	
		// Find the number of slices needed
		if (fHighResolution == true)
		{
			fNbSlices = fEpson.PrinterDef().hslices;
			if (fNbSlices == 0)
			{ // This means one pass
				fNbSlices = 1;
				if (f1440Dpi == true)
					fNbSlices = 2;
			}
		}
	
		sizeRLE /= fNbSlices;
		pTempRLE = (char *)malloc(sizeRLE);
	}
}

MPrinter::~MPrinter(void)
{
	free(pTempRLE);
}

status_t MPrinter::SmallInit(void)
{
	// Enable USB
	if (M_IS_USB(fEpson.PrinterDef().printer_info.gID))
	{
		if (SendCmd(cmdEnableUSB) != B_OK)
			return B_ERROR;
		pPrinterPort->Sync();
		snooze(250000);
	}
	
	// Init the printer
	if (SendCmd(cmdInit) != B_OK)
		return B_ERROR;

	// Epson Remote Supported ?
	uint32 remote_level = 0; // By default, no remote
	fEpson.PrinterDev()->ioctl(IOCTL_GET_EPSON_REMOTE_LEVEL, &remote_level);
	if (remote_level > 0)
	{
		if (SendCmd(cmdEnterRemote) != B_OK)
			return B_ERROR;

		tPaperFeedSequence paperFeed(GetPort(), fEpson.PrinterDef().res, fEpson.PrinterDef().paperformat, fEpson.PrinterDef().paper);
		if (fEpson.PrinterDev()->ioctl(IOCTL_PAPER_FEED_SEQUENCE, &paperFeed) != B_OK)
			return B_ERROR;

		// Handle status reply
		if (SendCmd(cmdHandleStatus) != B_OK)
			return B_ERROR;

		if (remote_level >= 5)
		{
			if (SendCmd(cmdRollPaper) != B_OK)	
				return B_ERROR;

			if (SendCmd(cmdHorPosition) != B_OK)	
				return B_ERROR;
		}

		if (M_IS_SMART_IC(fEpson.PrinterDef().printer_info.gID))
		{ // If smart ink counter, start the counter in automatic mode
			cmdInkRemainingCounter[6] = 3;
			if (SendCmd(cmdInkRemainingCounter) != B_OK)	
				return B_ERROR;
		}

		// Exit remote mode
		if (SendCmd(cmdExitRemote) != B_OK)	
			return B_ERROR;
	}
	return B_OK;
}

status_t MPrinter::Init(void)
{
	if (pTempRLE == NULL)
		return B_NO_MEMORY;	
	
	uint32 x_dpi = fEpson.Settings().DeviceXdpi();
	uint32 y_dpi = fEpson.Settings().DeviceYdpi();

	if (fRasterImageMode == false)
	{	// Set the "print raster"'s parameters
		cmdPrintRasterData[3] = 1;	// Compression ON
		cmdPrintRasterData[4] = (uint8)(3600 / y_dpi);		// resolution Y
		cmdPrintRasterData[5] = (uint8)(3600 / x_dpi);		// resolution X
		cmdPrintRasterData[6] = 1;								// One line at a time
		
		// 1440 horizontal is not supported by the printer
		if (f1440Dpi)
			cmdPrintRasterData[5] = (uint8)(3600 / 720);		// use XRES = 720 instead of 1440
	
		if (fHighResolution == true)
		{ // Software microweave, adapt the Y resolution
			cmdPrintRasterData[4] = (uint8)(3600 / fEpson.PrinterDef().printer_info.gColorNozzleDPI);
			cmdPrintRasterData[6] = fEpson.PrinterDef().printer_info.gNbColorNozzle;
		}
	}
	else
	{ // New RASTER mode (Multidot printers)
		cmdTransferRasterImage[3] = 0;			// color
		cmdTransferRasterImage[4] = 1;			// compression ON
		cmdTransferRasterImage[5] = fNbBitPix;	// bit length
		cmdTransferRasterImage[8] = 1;			// One line at a time
		cmdTransferRasterImage[9] = 0;
		cmdSetResolutionOfRaster[8] = (uint8)(14400 / y_dpi);				// resolution Y
		cmdSetResolutionOfRaster[9] = (uint8)(14400 / (x_dpi/fNbSlices));	// resolution X

		if (fHighResolution == true)
		{ // Software microweave, adapt the Y resolution
			cmdSetResolutionOfRaster[8] = (uint8)(14400 / fEpson.PrinterDef().printer_info.gColorNozzleDPI);	// resolution Y
			cmdTransferRasterImage[8] = fEpson.PrinterDef().printer_info.gNbColorNozzle;
		}
	}

	// Basic initialisation (USB, Remote)
	status_t result = SmallInit();

	// Init page graphic mode, units and page size
	if (result == B_OK)
		result = InitJob();

	// Init printing method : speed, microweave, etc...
	if (result == B_OK)
		result = InitPrinting();

	return result;
}

status_t MPrinter::InitJob(void)
{
	if (pTempRLE == NULL)
		return B_NO_MEMORY;

	const uint16 top	= fEpson.PrinterDef().printable_top;
	const uint16 bottom	= fEpson.PrinterDef().printable_bottom;
	const uint16 height	= fEpson.PrinterDef().printer_height;
	const uint16 width	= fEpson.PrinterDef().printer_width;
	const uint32 x_dpi	= fEpson.Settings().DeviceXdpi();
	const uint32 y_dpi	= fEpson.Settings().DeviceYdpi();

	status_t result = SelectGraphicMode();
	if (result == B_OK)
	{
		// Set units to the real value
		if (fRasterImageMode == false)
		{	
			result = SetUnit(3600 / y_dpi);
		}
		else
		{
			if (x_dpi > 1440)
			{ // For compatibility, we switch to 1/2880 only when needed
				cmdSetUnitExtended[6] = 2880 / y_dpi;
				cmdSetUnitExtended[7] = 2880 / y_dpi;
				cmdSetUnitExtended[8] = 2880 / x_dpi;
				cmdSetUnitExtended[9] = 0x40;
				cmdSetUnitExtended[10] = 0x0B;
			}
			else
			{
				cmdSetUnitExtended[6] = 1440 / y_dpi;
				cmdSetUnitExtended[7] = 1440 / y_dpi;
				cmdSetUnitExtended[8] = 1440 / x_dpi;
				cmdSetUnitExtended[9] = 0xA0;
				cmdSetUnitExtended[10] = 0x05;
			}
			result = SendCmd(cmdSetUnitExtended);
		}

		if ((result == B_OK) && (
				(fEpson.PrinterDef().printer_info.gID == ID_STYLUS_600) ||
				(fEpson.PrinterDef().printer_info.gID == ID_STYLUS_800) ||
				(fEpson.PrinterDef().printer_info.gID == ID_STYLUS_850) ||
				(fEpson.PrinterDef().printer_info.gID == ID_STYLUS_1520)||
				(fEpson.PrinterDef().printer_info.gID == ID_STYLUS_3000)))
		{ // Les stylus de la serie "4" necessitent cette commande (980017 [level II])
			cmdSetPrintingSpeed[6] = (x_dpi == 1440) ? 2 : 0;
			result = SendCmd(cmdSetPrintingSpeed);
		}	
	}

	// Set the page size
	if (result == B_OK)
		result = SetPageFormat(top, bottom);

	 // Set the paper dimension, this will handle the 'expanded' case
	if ((result == B_OK) && (fRasterImageMode == true))
	{
		// fEpson.PrinterDef().printer_width is given in pixels (ie: xdpi), we must provide
		// thepage size in the ydpi resolution.
		// top and bottom, on the other hand are already given in ydpi, thus no modfication is needed.
		result = SetPaperDimension(height, (width * y_dpi) / x_dpi);
	}

	return result;
}



status_t MPrinter::InitPrinting(void)
{
	status_t result = UniDirectionnalMode((uint8)fEpson.PrinterDef().speed_mode);
	if (result == B_OK)
		result = MicroweaveMode((uint8)fEpson.PrinterDef().weave_mode);

	if ((result == B_OK) && (M_IS_PHOTO(fEpson.PrinterDef().printer_info.gID)))
		result = SendCmd(cmdControlPaper);

	if (result == B_OK)
		result = SetDotSize(fDotSize);
	
	if ((result == B_OK) && (fRasterImageMode == true))
		result = SendCmd(cmdSetResolutionOfRaster);

	return result;
}




status_t MPrinter::Desinit(void)
{
	if (SendCmd(cmdDesinit) != B_OK)	
		return B_ERROR;

	// Epson Remote Supported ?
	uint32 remote_level = 0; // By default, no remote
	fEpson.PrinterDev()->ioctl(IOCTL_GET_EPSON_REMOTE_LEVEL, &remote_level);

	if (remote_level >= 4)
	{
		// The Stylus 800 compatible printers need a Paper feed desinit sequence
		if (SendCmd(cmdEnterRemote) != B_OK)	
			return B_ERROR;
	
		if (M_IS_SMART_IC(fEpson.PrinterDef().printer_info.gID))
		{ // If smart ink counter, stop the counter in automatic mode
			cmdInkRemainingCounter[6] = 2;
			if (SendCmd(cmdInkRemainingCounter) != B_OK)	
				return B_ERROR;
		}

		cmdPaperFeed[7] = 0;
		if (SendCmd(cmdPaperFeed) != B_OK)	
			return B_ERROR;
	
		if (SendCmd(cmdExitRemote) != B_OK)	
			return B_ERROR;
	}
	
	return B_OK;
}





status_t MPrinter::SelectGraphicMode(void)
{
	return SendCmd(cmdSelectGraphicMode);
}

status_t MPrinter::SetUnit(uint8 unit)
{
	cmdSetUnit[6] = unit;
	return SendCmd(cmdSetUnit);
}

status_t MPrinter::SetPageLength(uint16 height)
{
	cmdSetPageLength[6] = height & 0xFF;
	cmdSetPageLength[7] = height >> 8;
	return SendCmd(cmdSetPageLength);
}

status_t MPrinter::SetPageFormat(uint16 top, uint16 bot)
{
	cmdSetPageFormat[6] = top & 0xFF;
	cmdSetPageFormat[7] = top >> 8;
	cmdSetPageFormat[8] = bot & 0xFF;
	cmdSetPageFormat[9] = bot >> 8;
	return SendCmd(cmdSetPageFormat);
}

status_t MPrinter::SetPaperDimension(uint32 length, uint32 width)
{
	cmdSetPaperDimensionExtended[6] = length & 0xFF;
	cmdSetPaperDimensionExtended[7] = (length >> 8) & 0xFF;
	cmdSetPaperDimensionExtended[8] = (length >> 16) & 0xFF;
	cmdSetPaperDimensionExtended[9] = length >> 24;

	cmdSetPaperDimensionExtended[10] = width & 0xFF;
	cmdSetPaperDimensionExtended[11] = (width >> 8) & 0xFF;
	cmdSetPaperDimensionExtended[12] = (width >> 16) & 0xFF;
	cmdSetPaperDimensionExtended[13] = width >> 24;

	return SendCmd(cmdSetPaperDimensionExtended);
}

status_t MPrinter::UniDirectionnalMode(uint8 mode)
{
	cmdUniDirectionnalMode[3] = mode;
	return SendCmd(cmdUniDirectionnalMode);
}

status_t MPrinter::MicroweaveMode(uint8 mode)
{
	cmdMicroweaveMode[6] = mode;
	return SendCmd(cmdMicroweaveMode);
}

status_t MPrinter::SetDotSize(uint8 mode)
{
	cmdSetDotSize[7] = mode;
	return SendCmd(cmdSetDotSize);
}

status_t MPrinter::SetVertPosition(uint32 NbLines)
{
	if (fRasterImageMode == false)
	{
		cmdSetVertPosition[6] = NbLines & 0xFF;
		cmdSetVertPosition[7] = NbLines >> 8;
		return SendCmd(cmdSetVertPosition);
	}
	else
	{	
		cmdSetVertPositionExtended[6] = NbLines & 0xFF;
		cmdSetVertPositionExtended[7] = (NbLines >> 8) & 0xFF;
		cmdSetVertPositionExtended[8] = (NbLines >> 16) & 0xFF;
		cmdSetVertPositionExtended[9] = NbLines >> 24;
		return SendCmd(cmdSetVertPositionExtended);
	}
}

status_t MPrinter::SelectColor(t_printer_color RasterColor)
{
	if (fRasterImageMode == true)
	{	
		fColorOfRaster = (RasterColor & 0x0F) | ((RasterColor >> 4) & 0xF0);
		return B_OK;
	}

	uint8 *cmd;
	if (fSixColors)
	{
		cmdExSelectColor[6] = (RasterColor >> 8) & 0xFF;
		cmdExSelectColor[7] = (RasterColor & 0xFF);
		cmd = cmdExSelectColor;
	}
	else
	{
		cmdSelectColor[3] = RasterColor;
		cmd = cmdSelectColor;
	}
	return SendCmd(cmd);
}

status_t MPrinter::SetHorPosition(uint16 NbDots)
{
	if (NbDots == 0)
		return LineFeed();	// positionner la tete en 0	
	
	if (fRasterImageMode == false)
	{
		if (f1440Dpi)
		{ // 1440 dpi est supporte par une commande speciale et uniquement relative
			LineFeed();
			return Move1440(NbDots);
		}
		else
		{
			cmdSetHorPosition[3] = NbDots & 0xFF;
			cmdSetHorPosition[4] = NbDots >> 8;
			return SendCmd(cmdSetHorPosition);
		}
	}
	else
	{	
		cmdSetHorPositionExtended[6] = NbDots & 0xFF;
		cmdSetHorPositionExtended[7] = (NbDots >> 8) & 0xFF;
		cmdSetHorPositionExtended[8] = (NbDots >> 16) & 0xFF;
		cmdSetHorPositionExtended[9] = NbDots >> 24;
		return SendCmd(cmdSetHorPositionExtended);
	}
}

status_t MPrinter::Move1440(uint16 NbDots)
{
	cmdSetRelHor1440Pos[8] = NbDots & 0xFF;
	cmdSetRelHor1440Pos[9] = NbDots >> 8;
	return SendCmd(cmdSetRelHor1440Pos);
}

status_t MPrinter::PrintRasterData(char *pBuffer, uint16 NbDots, uint16 NbLines)
{
	ssize_t WriteLen;
	size_t len;

	if (fRasterImageMode == true)
	{
		const int compression = cmdTransferRasterImage[4];
		const uint16 horizontalByteCount = ((NbDots * cmdTransferRasterImage[5]) + 7)/8;

		cmdTransferRasterImage[3] = fColorOfRaster;		// color
		cmdTransferRasterImage[8] = NbLines;
		cmdTransferRasterImage[9] = 0;
		cmdTransferRasterImage[6] = horizontalByteCount & 0xFF;
		cmdTransferRasterImage[7] = horizontalByteCount >> 8;

		char *pOut = pBuffer;
		const int in_len = len = horizontalByteCount * NbLines;
		if (compression == 1)							// Compression RLE ON?
		{
			pOut = NULL;
			len = Compress(pBuffer, &pOut, in_len);		// RLE compression	

			// Attention: On ne peut switcher le mode de compression *QUE*
			// Si l'on remplit soit-meme toutes les buses (super wicroweave)
			if (fHighResolution == true)
			{
				if (len >= in_len)
				{
					// La compression RLE n'est pas efficace
					// Ne pas compresser!
					len = in_len;
					pOut = pBuffer;
					cmdTransferRasterImage[4] = 0;
				}
			}
		}

		if (SendCmd(cmdTransferRasterImage) != B_OK)
			return B_ERROR;
		WriteLen = Write(pOut, len);
	
		// Remettre la compression comme on l'a trouvé
		cmdTransferRasterImage[4] = compression;	
	}
	else
	{
		const int compression = cmdPrintRasterData[3];
		cmdPrintRasterData[6] = NbLines;
		cmdPrintRasterData[7] = NbDots & 0xFF;
		cmdPrintRasterData[8] = NbDots >> 8;
	
		char *pOut = pBuffer;
		const int in_len = len = (((NbDots + 7)/8) * cmdPrintRasterData[6]);
		if (compression == 1)							// Compression RLE ON?
		{
			pOut = NULL;
			len = Compress(pBuffer, &pOut, in_len);		// RLE compression
	
			// Attention: On ne peut switcher le mode de compression *QUE*
			// Si l'on remplit soit-meme toutes les buses (super wicroweave)
			if (fHighResolution == true)
			{
				if (len >= in_len)
				{
					// La compression RLE n'est pas efficace
					// Ne pas compresser!
					len = in_len;
					pOut = pBuffer;
					cmdPrintRasterData[3] = 0;
				}
			}
		}
	
		if (SendCmd(cmdPrintRasterData) != B_OK)
			return B_ERROR;			
		WriteLen = Write(pOut, len);
	
		// Remettre la compression comme on l'a trouvé
		cmdPrintRasterData[3] = compression;	
	}

	return ((WriteLen<len) ? (B_ERROR) : (B_OK));
}

status_t MPrinter::FormFeed(void)
{
	return SendCmd(cmdFormFeed);
}

status_t MPrinter::LineFeed(void)
{
	return SendCmd(cmdLineFeed);
}

status_t MPrinter::OutBuffers(void)
{
	return SendCmd(cmdOutBuffers);
}

int	MPrinter::Compress(char *pInrow, char **pOutput, uint32 len)
{
	*pOutput = pTempRLE;
	size_t out_len = fEpson.Compress((void **)pOutput, (size_t)sizeRLE, (const void *)pInrow, (size_t)len, BPrinterRasterAddOn::B_RLE_128);
	#if DEBUG
	if (outlen < 0) { // Should not happen
		debugger("RLE Buffer overflow!\n");
	}
	#endif
	return (int)out_len;
}

// ----------------------------------------------------------------
// #pragma mark -


MPrinterUtils::MPrinterUtils(	BTransportIO *stream,
								MDefinePrinter *printerDev,
								tPrinterDef *printerDef,
								bool enforceStatusRequest)
	:	fPrinterDev(printerDev),
		fPrinterDef(printerDef),
		fTransport(stream),
		fUsbEnabled(false),
		fStatusReportEnabled(enforceStatusRequest == false)
{
}

MPrinterUtils::~MPrinterUtils()
{
}

status_t MPrinterUtils::EnterRemote()
{
	status_t result;
	if ((fUsbEnabled==false) && (M_IS_USB(PrinterDef()->printer_info.gID)))	// Enable USB
	{
		if ((result = SendCmd(cmdEnableUSB)) != B_OK)
			return result;
		Transport()->Sync();	// Make sure the data is gone
		snooze(1000000);	// Some printers need to wait a little here
		fUsbEnabled = true;
	}

	((result = SendCmd(cmdInit)) == B_OK) &&
	((result = SendCmd(cmdEnterRemote)) == B_OK) &&
	(fStatusReportEnabled == false) && ((result = SendCmd(cmdHandleStatus)) == B_OK);	
	if (result == B_OK)
		fStatusReportEnabled = true;
	
	return result;
}

status_t MPrinterUtils::ExitRemote()
{
	status_t result;
	((result = SendCmd(cmdExitRemote)) == B_OK) &&
	((result = SendCmd(cmdDesinit)) == B_OK);
	if (result == B_OK)
		Transport()->Sync();
	return result;
}

status_t MPrinterUtils::GetStatus(BMessage& status_message)
{
	char status[128];
	ssize_t read = GetPort()->Read(status, 127);
	if (read < 0)
		return (status_t)read;
		
	if (read == 0)
	{ // No status from the printer
		status_message.MakeEmpty();
		if (fStatusReportEnabled == false)
		{ // We never asked for status reply. Do it!
			if (EnterRemote() != B_OK)
			{ // try to switch to READ/WRITE mode
				Transport()->SetAccessMode(O_RDWR);
				EnterRemote();
				ExitRemote();
				Transport()->SetAccessMode(O_RDONLY);
			}
			else
			{
				ExitRemote();
			}
		}
		return B_OK;
	}

	fStatusReportEnabled = true;
	status[read] = 0;	
	status_t result;	
	status_message.AddString("epst:status", status);
	if ((result = PrinterDev()->ioctl(IOCTL_PARSE_STATUS, &status_message)) != B_OK)
		return result;

	return B_OK;
}


status_t MPrinterUtils::CleanNozzles()
{
	status_t result;
	((result = Transport()->SetAccessMode(O_RDWR)) == B_OK)	&&
	((result = EnterRemote()) == B_OK)						&&
	((result = SendCmd(cmdCleanNozzles)) == B_OK)			&&
	((result = ExitRemote()) == B_OK);
	Transport()->SetAccessMode(O_RDONLY);
	return result;
}

status_t MPrinterUtils::CheckNozzles(void)
{
	status_t result;
	((result = Transport()->SetAccessMode(O_RDWR)) == B_OK)	&&
	((result = EnterRemote()) == B_OK)					&&
	((result = SendCmd(cmdCheckNozzles)) == B_OK)		&&
	((result = SendCmd(cmdExitRemote)) == B_OK)			&&
	((result = SendCmd(cmdLineFeed)) == B_OK)			&&
	((result = SendCmd(cmdLineFeedAlpha)) == B_OK)		&&
	((result = SendCmd(cmdLineFeed)) == B_OK)			&&
	((result = SendCmd(cmdLineFeedAlpha)) == B_OK)		&&
	((result = SendCmd(cmdEnterRemote)) == B_OK)		&&
	((result = SendCmd(cmdVI)) == B_OK)					&&
	((result = SendCmd(cmdExitRemote)) == B_OK)			&&
	((result = SendCmd(cmdFormFeed)) == B_OK);
	if (result == B_OK)
		Transport()->Sync();
	Transport()->SetAccessMode(O_RDONLY);
	return result;
}

status_t MPrinterUtils::CheckAlign(void)
{
	status_t result = EnterRemote();
	if (result != B_OK)
		return result;
	
	uint8 cmd[256];
	strcpy((char *)(cmd+1), "                                    ** A **\n\r\n\r");
	cmd[0] = strlen((char *)(cmd+1))-1;

	((result = SendCmd(cmdExitRemote)) == B_OK)		&&
	((result = SendCmd(cmd)) == B_OK)				&&
	((result = SendCmd(cmdEnterRemote)) == B_OK);

	if (result != B_OK)
		return result;
		
	for (int i=0 ; i<4 ; i++) {
		cmdDotCheck[6] = i;
		if (SendCmd(cmdDotCheck) != B_OK)
			return B_ERROR;
	}
	
	((result = SendCmd(cmdExitRemote)) == B_OK)		&&
	((result = SendCmd(cmdFormFeed)) == B_OK)		&&
	((result = SendCmd(cmdDesinit)) == B_OK);
	if (result == B_OK)
		Transport()->Sync();
	return result;
}


status_t MPrinterUtils::UserSelectAlign(int pattern_mask)
{
	status_t result = EnterRemote();
	if (result != B_OK)
		return result;

	uint8 cmd[256];
	strcpy((char *)(cmd+1), "                                    ** B **\n\r\n\r");
	cmd[0] = strlen((char *)(cmd+1))-1;
	if (SendCmd(cmd) != B_OK)
		return B_ERROR;

	((result = SendCmd(cmdExitRemote)) == B_OK)		&&
	((result = SendCmd(cmd)) == B_OK)				&&
	((result = SendCmd(cmdEnterRemote)) == B_OK);

	if (result != B_OK)
		return result;

	for (int i=0 ; i<4 ; i++) {
		if (pattern_mask & 0x1) {
			cmdDotTest[6] = i;
			if (SendCmd(cmdDotTest) != B_OK)
				return B_ERROR;
		}
		pattern_mask >>= 1;
	}

	((result = SendCmd(cmdExitRemote)) == B_OK)		&&
	((result = SendCmd(cmdFormFeed)) == B_OK)		&&
	((result = SendCmd(cmdDesinit)) == B_OK);
	if (result == B_OK)
		Transport()->Sync();
	return result;
}

status_t MPrinterUtils::ModifyAlign(int pattern, int value)
{
	cmdDotAlignment[6] = pattern;
	cmdDotAlignment[8] = value;
	status_t result;
	((result = SendCmd(cmdExitRemote)) == B_OK)		&&
	((result = SendCmd(cmdDotAlignment)) == B_OK)	&&
	((result = SendCmd(cmdEnterRemote)) == B_OK);
	return result;
}

status_t MPrinterUtils::SaveSettings(void)
{
	status_t result;
	((result = SendCmd(cmdExitRemote)) == B_OK)		&&
	((result = SendCmd(cmdSaveSettings)) == B_OK)	&&
	((result = SendCmd(cmdEnterRemote)) == B_OK);
	return result;
}


status_t MPrinterUtils::ConvertToStatus(const BMessage& msg, printer_status_t *status)
{
	int32 st, er, k,c,m,y,lc,lm, t;
	if (msg.FindInt32("epst:stat", &st) != B_OK)			st=-1;
	if (msg.FindInt32("epst:error", &er) != B_OK)			er=-1;
	if (msg.FindInt32("epst:black", &k) != B_OK)			k=-1;
	if (msg.FindInt32("epst:cyan", &c) != B_OK)				c=-1;
	if (msg.FindInt32("epst:magenta", &m) != B_OK)			m=-1;
	if (msg.FindInt32("epst:yellow", &y) != B_OK)			y=-1;
	if (msg.FindInt32("epst:light_cyan",	&lc) != B_OK)	lc=-1;
	if (msg.FindInt32("epst:light_magenta", &lm) != B_OK)	lm=-1;
	if (msg.FindInt32("epst:time_sec", &t) != B_OK)	t=-1;

	status->status = printer_status_t::B_UNKNOWN;
	if (st == 0)
	{ // Status code = Error
		status->status = printer_status_t::B_ERROR;
		switch (er)
		{ // Findout which error
			case 4: status->status = printer_status_t::B_PAPER_JAM;	break;
			case 5:	status->status = printer_status_t::B_NO_INK;	break;
			case 6:	status->status = printer_status_t::B_NO_PAPER;	break;
		}
	}
	else if (st == 2)	status->status = printer_status_t::B_PRINTING;	// Buffer Full
	else if (st == 3)	status->status = printer_status_t::B_PRINTING;	// Printing
	else if (st == 4)	status->status = printer_status_t::B_ONLINE;	// Idle
	else if (st == 7)	status->status = printer_status_t::B_CLEANING;	// Cleaning
	
	status->time_sec = t;
	status->CMYKcmyk[0] = (uint8)(c & 0xFF);
	status->CMYKcmyk[1] = (uint8)(m & 0xFF);
	status->CMYKcmyk[2] = (uint8)(y & 0xFF);
	status->CMYKcmyk[3] = (uint8)(k & 0xFF);
	status->CMYKcmyk[4] = (uint8)(lc & 0xFF);
	status->CMYKcmyk[5] = (uint8)(lm & 0xFF);
	status->CMYKcmyk[6] = 0xFF;
	status->CMYKcmyk[7] = 0xFF;

	return B_OK;
}

