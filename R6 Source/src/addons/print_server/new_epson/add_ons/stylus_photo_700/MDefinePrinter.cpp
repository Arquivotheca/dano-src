#include <stdio.h>
#include <string.h>

#include <DataIO.h>
#include "MDefinePrinter.h"
#include "MColorProcess.h"

const char *gName = "EPSON Stylus Photo 700";
const tPrinterInfo gPrinterInfo =
{
	gName	,					// Name
	ID_STYLUS_PHOTO_700,		// ID
	5,							// DefaultPage
	32,							// Nb Black Nozzles
	32,							// Nb Color Nozzles
	90,							// DPI Black Nozzles
	90,							// DPI Color Nozzles
	0,
	0,
	64*1024,					// Internal Input buffer size
	4,							// Epson Remote level
	B_SOFTWARE_CONTROL,			// Flow control
	SYNCHRONOUS_SERIAL_MODE,	// Serial Mode
	0							// Size of the structure
};

const tPage gPage[] = {
		{ "User Defined",			42, 2976, 42, 42, 4800, 198, M_PLAIN_FORMAT },
		{ "Legal",					42, 2976, 42, 42, 4800, 198, M_PLAIN_FORMAT },
		{ "Letter",					42, 2976, 42, 42, 3720, 198, M_PLAIN_FORMAT },
		{ "Executive",				42, 2526, 42, 42, 3540, 198, M_PLAIN_FORMAT },
		{ "Half-Letter",			42, 1896, 42, 42, 2820, 198, M_PLAIN_FORMAT },
		{ "A4",						42, 2892, 42, 42, 3969, 198, M_PLAIN_FORMAT },
		{ "A5",						42, 2020, 42, 42, 2736, 198, M_PLAIN_FORMAT },
		{ "A6",						42, 1404, 42, 42, 1864, 198, M_PLAIN_FORMAT },
		{ "B5",						42, 2495, 42, 42, 3402, 198, M_PLAIN_FORMAT },
		{ "#10 Envelope",		   397, 3336, 42, 42, 1245, 198, M_ENVELOPE_FROMAT },
		{ "DL Envelope",			99, 3034, 42, 42, 1319, 198, M_ENVELOPE_FROMAT },
		{ "C6 Envelope",			42, 2212, 42, 42, 1375, 198, M_ENVELOPE_FROMAT },
		{ "Postcard",				42, 1333, 42, 42, 1857, 198, M_POSTCARD_FROMAT },
		{ "Two-way Postcard",		42, 2750, 42, 42, 1857, 198, M_POSTCARD_FROMAT },
		{ "Index card 5x8",			42, 1716, 42, 42, 2640, 198, M_INDEX_CARD_FROMAT },
		{ "Index card 8x10",		42, 2796, 42, 42, 3360, 198, M_INDEX_CARD_FROMAT },
		{ "Photo Paper 4x6",		42, 1525, 42, 42, 2245, 198, M_PLAIN_FORMAT },
		{ "Panoramic Photo Paper",	42, 2892, 42, 120, 8100, 198, M_PLAIN_FORMAT }
				};

const tPage gPageLimits[2] = {
		{ "_min",					42, 1418, 42, 42,  1418, 198, M_UNKNOWN_FORMAT },
		{ "_max",					42, 2976, 42, 42, 15600, 198, M_UNKNOWN_FORMAT }
						};

const tPrinterRes gRes[] = {
		{ "Draft (180 Dpi)",		180, 180 },
		{ "Normal (360 Dpi)",		360, 360 },
		{ "High (360 Dpi)",			720, 360 },
		{ "Super (720 Dpi)",		720, 720 },
		{ "Photo (1440 Dpi)",		1440, 720 }	};

const tPrinterPaper gPaper[] = {
		{ "Plain paper" 						},
		{ "360 dpi Ink jet paper"				},
		{ "Photo Quality Ink jet paper"			},
		{ "Transparency"						},
		{ "Photo Paper"							},
		{ "Photo Quality Glossy film"			}	};

const uint8 paperFeedValue[sizeof(gPage)/sizeof(tPage)][sizeof(gPaper)/sizeof(tPrinterPaper)] = {
	{ 1, 1, 1, 3, 0, 3 },	// userdef
	{ 1, 1, 1, 3, 0, 3 },	// legal
	{ 1, 1, 1, 3, 0, 3 },	// letter
	{ 1, 1, 1, 3, 0, 3 },	// executive
	{ 1, 1, 1, 3, 0, 3 },	// half letter
	{ 1, 1, 1, 3, 0, 3 },	// a4
	{ 1, 1, 1, 3, 0, 3 },	// a5
	{ 1, 1, 1, 3, 0, 3 },	// a6
	{ 1, 1, 1, 3, 0, 3 },	// B5
	{ 2, 2, 2, 3, 2, 3 },	// #10 envelope
	{ 2, 2, 2, 3, 2, 3 },	// DL
	{ 2, 2, 2, 3, 2, 3 },	// C6
	{ 2, 2, 2, 3, 2, 3 },	// postcard
	{ 2, 2, 2, 3, 2, 3 },	// 2 ways postcard
	{ 2, 2, 2, 3, 2, 3 },	// index 5x8
	{ 2, 2, 2, 3, 2, 3 },	// index 8x10
	{ 1, 1, 1, 3, 0, 3 },	// photo paper
	{ 1, 1, 1, 3, 0, 3 },	// panoramique
};

const tColorMode gColorMode[] = {	{"Color",			6 },
									{"Black & White",	1 }		};

const tWeaveMode gWeaveMode[] = {	{"MicroWeave OFF", 	0 },
									{"MicroWeave ON",	1 },
									{"MicroWeave Super",0 }		};

const tSpeedMode gSpeedMode[] = {	{"Speed Bidir.",	0 },
									{"Speed Normal",	1 }		};

const tMicroDot gMicroDot[] =	{	{"Micro dot",		1 },
									{"Single dot",		2 },
									{"Double dot",		3 },
									{"Quad dot",		4 }		};

// resolution, paper -> icone(3 max, niveau de qualite), microdot, microweave, speed (low=1)
const int32 gRes2Paper_default = 0;
const tRes2Paper gRes2Paper[] =	{
									{1, 0, 0,	2, 0, 0 },	// 360x360 normal : double - ok
									{2, 0, 1,	1, 2, 0 },	// 720x360 normal : single - ok
									{3, 0, 2,	0, 2, 1 },	// 720x720 normal : micro  - ok
									{2, 1, 1,	1, 2, 1 },	// 720x360 couche : single - ok
									{3, 2, 2,	1, 2, 1 },	// 720x720 couche : single - ok
									{1, 3, 1,	2, 2, 1 },	// 360x360 transp : Double - ok
									{3, 4, 2,	1, 2, 1 },  // 720x720 photo  : single - ok
									{3, 5, 2,	0, 2, 1 }	// 720x720 film   : micro  - ok(?)
								};

const int gNbPages		= sizeof(gPage)/sizeof(tPage);
const int gNbRes		= sizeof(gRes)/sizeof(tPrinterRes);
const int gNbPaper		= sizeof(gPaper)/sizeof(tPrinterPaper);
const int gNbColorMode	= sizeof(gColorMode)/sizeof(tColorMode);
const int gNbWeaveMode	= sizeof(gWeaveMode)/sizeof(tWeaveMode);
const int gNbSpeedMode	= sizeof(gSpeedMode)/sizeof(tSpeedMode);
const int gNbMicroDot	= sizeof(gMicroDot)/sizeof(tMicroDot);
const int gNbAllowed	= sizeof(gRes2Paper)/sizeof(tRes2Paper);


// ---------------------------------------------------------------

class MPrinterDriver : public MDefinePrinter
{
public:
		MPrinterDriver();
	virtual	status_t	ioctl			(uint32 cmd, void *arg1);
	virtual MColorProcess* instantiate_MColorProcess(tPrinterDef *pDriver);
};


MDefinePrinter *instantiate_MDefinePrinter()
{
	return new MPrinterDriver();
}


MPrinterDriver::MPrinterDriver()
{
	fPrinterInfo = gPrinterInfo;
	fAttributes = EP_SUPPORTS_USB | EP_IS_PHOTO;

	fPage = gPage;
	fPageLimits = gPageLimits;
	fRes = gRes;
	fPaper = gPaper;
	fColorMode = gColorMode;
	fWeaveMode = gWeaveMode;
	fSpeedMode = gSpeedMode;
	fMicroDot = gMicroDot;
	fRes2Paper = gRes2Paper;

	fNbPages = gNbPages;
	fNbRes = gNbRes;
	fNbPaper = gNbPaper;
	fNbColorMode = gNbColorMode;
	fNbWeaveMode = gNbWeaveMode;
	fNbSpeedMode = gNbSpeedMode;
	fNbMicroDot = gNbMicroDot;
	fNbAllowed = gNbAllowed;
	fRes2Paper_default = gRes2Paper_default;
}

MColorProcess *MPrinterDriver::instantiate_MColorProcess(tPrinterDef *printer_def)
{
	return new MColorProcess(printer_def);
}

status_t MPrinterDriver::ioctl(uint32 cmd, void *arg1)
{
	switch (cmd)
	{
		case IOCTL_GET_PAGE_TYPE:
		{
			int32 index = *(int32 *)arg1;
			if ((index < 0) || (index >= gNbPages))
				return B_ERROR;
			*(uint32 *)arg1 = gPage[index].type;
			return B_OK;
		}
		case IOCTL_PAPER_FEED_SEQUENCE:
		{
			tPaperFeedSequence& paperFeed = *((tPaperFeedSequence *)arg1);
			uint8 cmd0[7] = {'S','N',3,0, 0,0,0};
			cmd0[6] = paperFeedValue[paperFeed.page][paperFeed.paper];
			if (paperFeed.port->Write(cmd0, 7) != 7)
				return B_ERROR;
			return B_OK;
		}
	}
	return MDefinePrinter::ioctl(cmd, arg1);
}
